#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <wait.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#define BUFFSIZE 4096
#define TIMELIMIT 5

void err_sys(const char* x);
void sprintfFileError(char* filename, const char* message);
void addNumbersFromFile(char * filename, std::vector<int> * v);
void sortingProcedure(std::vector <int> *numbers);
bool string_to_int(std::string str, int &x);

int pid;
bool setSimulatedDelayInChild = false;

int main(int argc, char ** argv) {
    int filesCount = argc - 1;

    if (strcmp(argv[filesCount], "--broke-sort") == 0) {
        setSimulatedDelayInChild = true;
        filesCount--;
    }

    char *outputFileName;

    if (filesCount < 1) {
        err_sys("Format: app inputFile1 [...inputFileN] outputFile [--broke-sort]");
    } else {
        outputFileName = argv[filesCount];
    }

    std::vector <int> *numbers = new std::vector<int>();

    for (int i = 1; i < filesCount; i++) {
        addNumbersFromFile(argv[i], numbers);
    }

    sortingProcedure(numbers);

    int outputFd;
    if ((outputFd = creat(outputFileName, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        sprintfFileError(outputFileName, "Can't create file %s for write.");
    }

    for (std::vector<int>::iterator it = numbers->begin(); it != numbers->end(); it++) {
        dprintf(outputFd, "%d \n", (*it));
    }


}

void timeLimitExceed() {
    kill(pid, SIGKILL);
    err_sys("Sort is broken (time limit exceed)");
}

void sortingProcedure(std::vector <int> *numbers) {
    if ((pid = fork()) < 0) {
        err_sys("Can't fork the process");
    } else if (pid == 0) {
        // return to child process
        alarm(TIMELIMIT);
        if (setSimulatedDelayInChild) {
            sleep(5);
        }
        std::sort(numbers->begin(), numbers->end());
    } else {
        // return to parent process
        int status;
        wait(&status);
        if (WIFSIGNALED(status)) {
            if (WTERMSIG(status) == SIGALRM) {
                timeLimitExceed();
            }
        }
        exit(0);
    }
}

void addNumbersFromFile(char * filename, std::vector<int> * v) {
    int fd;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        sprintfFileError(filename, "Can't open file %s for read.");
    }

    int n;
    char currentSymbol;
    std::string currentString = std::string();
    bool state = false;

    while ((n = read(fd, &currentSymbol, sizeof(char))) > 0) {
        if ((currentSymbol >= '0' && currentSymbol <= '9')) {
            if (!state)
                state = true;
            currentString.push_back(currentSymbol);
        } else if (state && (currentSymbol < '0' || currentSymbol > '9')) {
            state = false;
            int value;
            if (!string_to_int(currentString, value)) {
                err_sys("Can't convert string to int\n");
            }
            v->push_back(value);
            currentString = std::string();
        }
    }

}

bool string_to_int(std::string str, int &x)
{
    std::istringstream ss(str);

    while (!ss.eof())
    {
       if (ss >> x)
           return true;

       ss.clear();
       ss.ignore();
    }
    return false; // There is no integer!
}

void sprintfFileError(char* filename, const char* message) {
    char * error_msg_buf = new char[BUFFSIZE];
    snprintf(error_msg_buf, BUFFSIZE, message, filename);
    err_sys(error_msg_buf);
}

void err_sys(const char* x)
{
    perror(x);
    exit(1);
}
