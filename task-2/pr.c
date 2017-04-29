#include <stdio.h>
#include <unistd.h>

#define BUFFSIZE 4096

void err_sys(const char* x);

int main(int argc, char **argv) {

    char *filename;

    if (argc > 2) {
        err_sys("./pr [filename]");
    } else if (argc == 1) {
        filename = "file_unzipped_sparse";
    } else if (argc == 2) {
        filename = argv[1];
    }

    int number_of_bytes;
    char buffer[BUFFSIZE];

    while ((number_of_bytes = read(STDIN_FILENO, buffer, BUFFSIZE)) > 0) {

    }

    if (number_of_bytes < 0)
        err_sys("Read error");

    exit(0);
}

void err_sys(const char* x)
{
    perror(x);
    exit(1);
}
