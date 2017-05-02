#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/param.h>

#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <iostream>

#define CONFIG_FILE "config"
#define BUFSIZE 4096
#define _MAX_INT_DIG 10
#define ATTEMPTS_MAX 50

namespace std
{
    inline string to_string(int _Val)
    {   // convert long long to string
        char _Buf[2 * _MAX_INT_DIG];
        snprintf(_Buf, _MAX_INT_DIG, "%d", _Val);
        return (string(_Buf));
    }
}

void process_config();
void execute_process(std::vector<std::string> &);
void execute_program(std::vector<std::string> &, std::string &);

void update_pid(std::string, pid_t);
void remove_pid_file(std::string &);

void reboot_handler(int sig);
void sigint_handler(int sig);

void err_sys(const char *);

std::ifstream infile(CONFIG_FILE);

std::set<pid_t> pid_set;

int main() {
    signal(SIGHUP, reboot_handler);
    unsigned int fd;
    struct rlimit flim;

    /*if (getppid() != 1){
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        if (fork() != 0)
            exit(1);
        setsid();
    }
    getrlimit(RLIMIT_NOFILE, &flim);
    for (fd=0; fd < flim.rlim_max; fd++)
        close(fd);*/

    chdir("/");
    openlog("TASK MANAGER", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "Started (pid: %d)", getpid());

    process_config();
    while(1) {
    }
}

void process_config() {
    std::string str,
                delimiter = " ";

    while (std::getline(infile, str)) {
        if (str.length() > 0) {
            std::vector<std::string> tokens;
            size_t last = 0, next = 0;
            while ((next = str.find(delimiter, last)) != std::string::npos) {
                std::string token = str.substr(last, next - last);
                last = next + 1;
                tokens.push_back(token);
            }
            tokens.push_back(str.substr(last, next - last));

            pid_t pid;
            switch(pid = fork()) {
            case -1:
                syslog(LOG_ERR, "Fork in proc %d failed", getppid());
                break;
            case 0:
                signal(SIGINT, sigint_handler);
                execute_process(tokens);
                syslog(LOG_INFO, "Process pid %d executed", getpid());
                pid_set.erase(getpid());
                exit(0);
                break;
            default:
                syslog(LOG_INFO, "Starting child process with pid %d (parent pid: %d)", pid, getpid());
                pid_set.insert(pid);
                break;
            }
        }
    }
}

void execute_process(std::vector<std::string> &configuration) {

    int configLength = configuration.size();

    bool isRespawn = configuration[configLength-1] == "respawn";
    std::string program = configuration[0];

    std::vector<std::string> arguments;
    arguments = std::vector<std::string>(configuration.begin()+1, configuration.end()-1);

    pid_t pid;

    while (1) {
        if ((pid = fork()) == -1) {
            syslog(LOG_ERR, "Fork in proc %d failed", getppid());
        } else if (pid == 0) {
            execute_program(arguments, program);
            exit(0);
        } else {
            syslog(LOG_INFO, "Starting child process with pid %d (parent pid: %d)", pid, getpid());
            pid_set.insert(pid);
            syslog(LOG_INFO, "Set pid %d in file /tmp/%s", pid, program.c_str());
            update_pid(program, pid);
            syslog(LOG_INFO, "Wait process pid %d executing", pid);
            waitpid(pid, NULL, 0);
            syslog(LOG_INFO, "Process pid %d executed", pid);
            pid_set.erase(pid);
            if (!isRespawn) {
                remove_pid_file(program);
                break;
            }
        }
        sleep(5);
    }
}

void execute_program(std::vector<std::string> &arguments, std::string &program_name) {
    int attempts_count, arg_count = 0;
    char * exec_args[1024];
    exec_args[arg_count++] = strdup(program_name.c_str());
    for (int i = 0; i < arguments.size(); i++) {
        exec_args[arg_count++] = strdup(arguments[i].c_str());
    }
    exec_args[arg_count++] = 0;


    while(1){
        if (attempts_count == ATTEMPTS_MAX){
            attempts_count = 0;
            syslog(LOG_WARNING, "So many execution attempts of %s. Try again after 60 min.", program_name.c_str());
            sleep(3600);
        }
        if (execvp(strdup(program_name.c_str()), exec_args) < 0){
            syslog(LOG_INFO, "Program %s execution failed. Attempt number: %d", program_name.c_str(), attempts_count);
            attempts_count++;
            continue;
        }
        return;
    }
}

void update_pid(std::string name, pid_t pid) {
    char path[BUFSIZE];
    sprintf(path, "/tmp/%s", name.c_str());
    int fd;

    if ((fd = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        syslog(LOG_ERR, "Can't update file %s", path);
    }

    std::string buf = std::to_string(pid);
    int buf_length = buf.length();
    write(fd, buf.c_str(), sizeof(char)*buf_length);
}

void remove_pid_file(std::string &program_name) {
    int ret;
    char path[BUFSIZE];
    sprintf(path, "/tmp/%s", program_name.c_str());

    if ((ret = remove(path)) == -1) {
        syslog(LOG_ERR, "Can't remove file %s", path);
    }
}

void reboot_handler(int sig) {
    syslog(LOG_INFO, "SIGHUP to %d received, send SIGINT to descendants", getpid());
    for (std::set<pid_t>::iterator it = pid_set.begin(); it != pid_set.end(); it++) {
        syslog(LOG_INFO, "Send SIGINT to %d", (int)(*it));
        kill((*it), SIGINT);
    }
    process_config();
}

void sigint_handler(int sig) {
    syslog(LOG_INFO, "SIGINT to %d received, send SIGKILL to descendants", getpid());
    for (std::set<pid_t>::iterator it = pid_set.begin(); it != pid_set.end(); it++) {
        syslog(LOG_INFO, "Send SIGKILL to %d", (int)(*it));
        kill((*it), SIGKILL);
    }
    kill(getpid(), SIGKILL);
}


void err_sys(const char *message) {
    perror(message);
    exit(1);
}


