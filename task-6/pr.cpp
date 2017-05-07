#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#define MAX_LENGTH_FILENAME 255
#define MAX_USERNAME_LENGTH 100
#define MAX_PASSWORD_LENGTH 50
#define MAX_USERNAMES_COUNT 4096
#define BUFSIZE 4096

char usernames[MAX_USERNAMES_COUNT][MAX_USERNAME_LENGTH];
char passwords[MAX_USERNAMES_COUNT][MAX_PASSWORD_LENGTH];

char *filename, *username, *password, *file_part_str;
int file_part;
char lockfile[MAX_LENGTH_FILENAME];
char locklockfile[MAX_LENGTH_FILENAME];

int number_of_strings = 0;

void change_password(int);
bool is_file_part_locked(char *, int, bool);
int file_exists(char *);
void lock_file(char *, char);
void lock_file_part(char *, char, int);
void unlock_file(char *);
void unlock_file_part(char *);

void read_file_part_and_change_password(char *, char *, char *);
void write_file_part(char *);

void err_sys(const char*);

int main(int argc, char ** argv) {
	if (argc != 5) {
		err_sys("Incorrect arguments number. Usage: ./pr filename username password part");
	}

	filename = argv[1];
	username = argv[2];
	password = argv[3];
	file_part_str = argv[4];

	char *end_pointer;
	file_part = strtol(file_part_str, &end_pointer, 10);


	int filename_length = (sizeof(filename) / sizeof(char));
	if (filename_length > MAX_LENGTH_FILENAME) {
		err_sys("Incorrect filename length.");
	}

    snprintf(lockfile, sizeof lockfile, "%s%s", filename, ".lck");
    snprintf(locklockfile, sizeof lockfile, "%s%s", filename, ".lck.lck");

	change_password(file_part);

	return 0;
}

void change_password(int file_part) {
	while (file_exists(locklockfile)) {
        printf("%s exists. Wait.\n", locklockfile);
        fflush(stdout);
        sleep(2);
	}

	while(is_file_part_locked(lockfile, file_part, true)) {
		sleep(1);
	}

    // lock the filename.lck file
    lock_file(locklockfile, 'w');
    // lock the filename file part (from arguments)
    lock_file_part(lockfile, 'r', file_part);
    unlock_file(locklockfile);

    read_file_part_and_change_password(filename, username, password);
    for (int i=0;i<number_of_strings;i++) {
        printf("%s %s\n", usernames[i], passwords[i]);
    }

    // remove read lock from file part
    lock_file(locklockfile, 'w');
    unlock_file_part(lockfile);
    unlock_file(locklockfile);

    while (file_exists(locklockfile)) {
        printf("%s exists. Wait.\n", locklockfile);
        fflush(stdout);
        sleep(2);
	}

	while(is_file_part_locked(lockfile, file_part, false)) {
        fflush(stdout);
		sleep(1);
	}

    // lock filename part on write
    lock_file(locklockfile, 'w');
    lock_file_part(lockfile, 'w', file_part);
    unlock_file(locklockfile);

    write_file_part(filename);

    lock_file(locklockfile, 'w');
    unlock_file_part(lockfile);
    unlock_file(locklockfile);


}

bool is_file_part_locked(char * lockfile, int file_part, bool want_locking_on_read) {
    FILE *fp;
    bool result = false;

    if (file_exists(lockfile)) {
        if (want_locking_on_read) {
            if ((fp = fopen(lockfile, "r")) == NULL) {
                err_sys("Can't open lockfile!");
            }
            int pid, locked_part;
            char lock_type;

            while (fscanf(fp, "%d %c %d\n", &pid, &lock_type, &locked_part) != EOF) {
                if (lock_type == 'w') {
                    printf("Part of file %s (1..%d) locked by pid %d on write. Can't lock on read.\n", filename, locked_part, pid);
                    result = true;
                    break;
                }
            }
            fclose(fp);
        } else {
            printf("File %s exists. Can't lock part on write", filename);
            result = true;
        }
    }

    return result;
}

void lock_file(char* lockfile, char operation) {
    if (file_exists(lockfile)) {
        err_sys("Error! Lockfile exists. Exit(1)");
    }

    FILE *f = fopen(lockfile, "w");
    if (f != NULL) {
        fprintf(f, "%d %c\n", getpid(), operation);
        fclose(f);
    } else {
        err_sys("Error in file creating");
    }
}

void unlock_file(char *lockfile) {
    if(remove(lockfile) != 0) {
        printf("Error: unable to delete the file %s", lockfile);
    }
}

void lock_file_part(char* lockfile, char operation, int file_part) {
    FILE *f = fopen(lockfile, "a");
    if (f != NULL) {
        fprintf(f, "%d %c %d\n", getpid(), operation, file_part);
        fclose(f);
    } else {
        err_sys("Error in file append");
    }
}

void unlock_file_part(char * lockfile) {
    int current_pid = getpid();

    FILE *fp, *tempfile;

    if ((tempfile = fopen("/tmp/tmp_lockfile", "w+")) == NULL) {
		err_sys("Can't open passwords file");
	}

	if ((fp = fopen(lockfile, "r")) == NULL) {
		err_sys("Can't open lockfile!");
	}

	int pid, locked_part;
	char lock_type;
	int counter = 0;

	while (fscanf(fp, "%d %c %d\n", &pid, &lock_type, &locked_part) != EOF) {
        if (pid != current_pid) {
            counter++;
            fprintf(tempfile, "%d %c %d\n", pid, lock_type, locked_part);
        }
     }

    fclose(fp);

    if (counter == 0) {
        if(remove(lockfile) != 0) {
            printf("Error: unable to delete the file %s", lockfile);
        }
    } else {
        if ((fp = fopen(lockfile, "w+")) == NULL) {
            err_sys("Can't open lockfile!");
        }

        while (fscanf(tempfile, "%d %c %d\n", &pid, &lock_type, &locked_part) != EOF) {
            fprintf(fp, "%d %c %d\n", pid, lock_type, locked_part);
        }
        fclose(fp);
    }

    fclose(tempfile);
    if(remove("/tmp/tmp_lockfile") != 0) {
        printf("Error: unable to delete the file %s", "/tmp/tmp_lockfile");
    }
}

void read_file_part_and_change_password(char* filename, char* username, char* password) {
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
		err_sys("Can't open passwords file");
	}

    while (fscanf(fp, "%s %s\n", usernames[number_of_strings], passwords[number_of_strings]) != EOF) {
        if (strcmp(usernames[number_of_strings], username) == 0) {
            snprintf(passwords[number_of_strings], MAX_PASSWORD_LENGTH, "%s", password);
        }
        number_of_strings++;
    }

    fclose(fp);
}

void write_file_part(char* filename) {
    FILE *fp;

    if ((fp = fopen(filename, "w+")) == NULL) {
		err_sys("Can't open passwords file");
	}

	for (int i = 0; i < number_of_strings; i++) {
        fprintf(fp, "%s %s\n", usernames[i], passwords[i]);
	}

	fclose(fp);
}


int file_exists(char *filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}



void err_sys(const char *message) {
	perror(message);
	exit(1);
}
