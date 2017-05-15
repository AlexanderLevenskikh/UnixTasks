#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#define BUFFSIZE 1024

void err_sys(const char* x);

int main(int argc, char **argv) {

    int totalSize = 0;

    char *unzippedSparse;

    if (argc != 2) {
        err_sys("Usage: ./pr output-filename");
    }

    unzippedSparse = argv[1];

    int fd;
    if ((fd = creat(unzippedSparse, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        char *error_str;
        sprintf(error_str, "Can't create file %s for write", unzippedSparse);
        err_sys(error_str);
    }

    int number_of_bytes;
    char buffer[BUFFSIZE];

    while ((number_of_bytes = read(STDIN_FILENO, buffer, BUFFSIZE)) > 0) {
        totalSize += number_of_bytes;
        ftruncate(fd, totalSize);

        int counter = 0;
        char *pointer = buffer;
        int i;

        for (i = 0; i < number_of_bytes; i++) {

            if (*pointer == 0) {
                counter++;
            } else {
                if (counter > 0) {
                    lseek(fd, counter, SEEK_CUR);
                    counter = 0;
                }
                write(fd, pointer, 1);
            }

            pointer++;
        }

        if (counter > 0) {
            lseek(fd, counter, SEEK_CUR);
            counter = 0;
        }
    }

    if (number_of_bytes < 0)
        err_sys("Read error");

    close(fd);
    exit(0);
}

void err_sys(const char* x)
{
    perror(x);
    exit(1);
}
