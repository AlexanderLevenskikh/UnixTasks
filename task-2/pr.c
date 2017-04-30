#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#define BUFFSIZE 4096

void err_sys(const char* x);

int main(int argc, char **argv) {

    char *unzippedSparse, *unzipped;

    if (argc > 3 || argc < 2) {
        err_sys("./pr unzipped [unzippedSparse]");
    } else if (argc == 2) {
        unzipped = argv[1];
        unzippedSparse = "file_unzipped_sparse";
    } else if (argc == 3) {
        unzipped = argv[1];
        unzippedSparse = argv[2];
    }

    struct stat stat_struct;
    int unzippedFd = open(unzipped, O_RDONLY);

    fstat(unzippedFd, &stat_struct);

    int fd = creat(unzippedSparse, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        char *error_str;
        sprintf(error_str, "Can't create file %s for write", unzippedSparse);
        err_sys(error_str);
    }

    ftruncate(fd, stat_struct.st_size);

    int number_of_bytes;
    char buffer[BUFFSIZE];

    while ((number_of_bytes = read(unzippedFd, buffer, BUFFSIZE)) > 0) {
        int counter = 0;
        char *pointer = buffer;

        for (int i=0; i<number_of_bytes; i++) {

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
