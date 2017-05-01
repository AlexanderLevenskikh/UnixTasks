#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <iostream>

#define CONFIG_FILE "config"

void process_config();
void execute_process(std::vector<std::string> &);
void execute_once_fork(std::vector<std::string> &, std::string &);
void execute_program();
void update_pid(std::string name, pid_t pid);
void err_sys(const char *);

std::set<pid_t> pid_set;

int main() {
    process_config();
}

void process_config() {
    std::ifstream infile(CONFIG_FILE);
    std::string str,
                delimiter = " ";

    while (std::getline(infile, str)) {
        if (str.length() > 0) {
            std::vector<std::string> tokens;
            size_t last = 0, next = 0;
            while ((next = str.find(" ", last)) != std::string::npos) {
                std::string token = str.substr(last, next - last);
                last = next + 1;
                tokens.push_back(token);
            }
            tokens.push_back(str.substr(last, next - last));

            execute_process(tokens);
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
    switch (pid = fork()) {
        case -1:
            // logging
            err_sys("Can't fork process");
            break;
        case 0:
            if (isRespawn) {
                while (1) execute_once_fork(arguments, program);
            } else
                execute_once_fork(arguments, program);
            exit(0);

        default:
            pid_set.insert(pid);
            update_pid(program, pid);
            break;
    }
}

void execute_once_fork(std::vector<std::string> &arguments, std::string &program_name) {
    // child pid for execution
    pid_t pid;
    switch (pid = fork()) {
        case -1:
            // logging
            err_sys("Can't fork process");
            break;
        case 0:
            execute_program();
            exit(0);

        default:
            pid_set.insert(pid);
            update_pid(program_name, pid);
            break;
    }
}

void execute_program() {

}

void update_pid(std::string name, pid_t pid) {

}

void err_sys(const char *message) {
    perror(message);
    exit(1);
}
