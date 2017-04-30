#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#define BUFFSIZE 4096

void err_sys(const char* x);
void sprintfFileError(char* filename, const char* message);
void addNumbersFromFile(char * filename, std::vector<std::string> * v);
bool stringComparator(std::string &s1, std::string &s2);

int main(int argc, char ** argv) {
    int filesCount = argc - 1;
    char *outputFileName;

    if (filesCount < 1) {
        err_sys("Format: app inputFile1 [...inputFileN] outputFile");
    } else {
        outputFileName = argv[filesCount];
    }

    std::vector <std::string> *numbers = new std::vector<std::string>();

    for (int i = 1; i < filesCount; i++) {
        addNumbersFromFile(argv[i], numbers);
    }

    std::sort(numbers->begin(), numbers->end(), stringComparator);

    int outputFd;
    if ((outputFd = creat(outputFileName, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        sprintfFileError(outputFileName, "Can't create file %s for write.");
    }

    for (std::vector<std::string>::iterator it = numbers->begin(); it != numbers->end(); it++) {
        const char *buf = it->c_str();
        if (dprintf(outputFd, "%s \n", buf) < strlen(buf) + 2) {
            sprintfFileError(outputFileName, "Can't write to file %s.");
        }
    }


}

void addNumbersFromFile(char * filename, std::vector<std::string> * v) {
    int fd;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        sprintfFileError(filename, "Can't open file %s for read.");
    }

    int n;
    char currentSymbol;
    std::string *currentString = new std::string();
    bool state = false;

    while ((n = read(fd, &currentSymbol, sizeof(char))) > 0) {
        if ((currentSymbol >= '0' && currentSymbol <= '9')) {
            if (!state)
                state = true;
            currentString->push_back(currentSymbol);
        } else if (state && (currentSymbol < '0' || currentSymbol > '9')) {
            state = false;
            v->push_back(*currentString);
            currentString = new std::string();
        }
    }

}

bool stringComparator(std::string &s1, std::string &s2) {
    if (s1.length() < s2.length())
            return true;
        if (s2.length() < s1.length())
            return false;
        else
    return (s1 < s2);
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
