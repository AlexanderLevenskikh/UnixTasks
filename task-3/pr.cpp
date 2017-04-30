#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

void err_sys(const char* x);
void addNumbersFromFile(char * filename, std::vector<std::string> * v);
bool stringComparator(std::string &s1, std::string &s2);

int main(int argc, char ** argv) {
    int filesCount = argc - 1;
    char *outputFileName;

    if (filesCount < 1) {
        err_sys("Format: app inputFile1 [...inputFileN] outputFile");
    } else if (filesCount == 1) {
        outputFileName = "result";
    } else {
        outputFileName = argv[filesCount];
    }

    std::vector <std::string> *numbers = new std::vector<std::string>();

    for (int i = 1; i <= filesCount; i++) {
        addNumbersFromFile(argv[i], numbers);
    }

    std::sort(numbers->begin(), numbers->end(), stringComparator);

    for (std::vector<std::string>::iterator it = numbers->begin(); it != numbers->end(); it++) {
        std::cout << *it << "\n";
    }


}

void addNumbersFromFile(char * filename, std::vector<std::string> * v) {
    int fd;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        char * error_msg;
        sprintf(error_msg, "Can't open file %s for read.", filename);
        err_sys(error_msg);
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

void err_sys(const char* x)
{
    perror(x);
    exit(1);
}
