#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#define MAX_LENGTH_FILENAME 255
#define MAX_USERNAME_LENGTH 100
#define BUFSIZE 4096

char *filename, *username, *password;
int file_part;
char lockfile[MAX_LENGTH_FILENAME];
const char* lck = ".lck.lck";

void change_password(int);
bool is_locked();
void lock_file(char *, char);
void lock_file_part(char *, char, int);
void unlock_file(char *);
void unlock_file_part(char *);

void read_file_part_and_change_password(char *, char *, char *, int);
void write_file_part(int);

int main(int argc, char ** argv) {
	if (argc != 5) {
		err_sys("Incorrect arguments number. Usage: ./pr filename username password part");
	}

	filename = argv[1];
	username = argv[2];
	password = argv[3];
	file_part = argv[4];

	int filename_length = (sizeof(filename) / sizeof(char));
	if (filename_length > MAX_LENGTH_FILENAME) {
		err_sys("Incorrect filename length.");
	}
	strcpy(lockfile, filename);
	strcpy(lockfile, ".lck");

	change_password(file_part);

	return 0;
}

void change_password(int file_part) {
	while (is_locked()) {
		sleep(5);
	}
    // lock the filename.lck file
    lock_file(lockfile, 'w');
    // lock the filename file part (from arguments)
    lock_file_part(filename, 'r', file_part);
    read_file_part_and_change_password(filename, username, password, file_part);
    // remove read lock from file part
    unlock_file_part(filename);
    // lock filename part on write
    lock_file_part(filename, 'w', file_part);
    write_file_part(file_part);
    unlock_file_part(filename);


}


int file_exists(char *filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}

void err_sys(const char *message) {
	perror(message);
	exit(1);
}
