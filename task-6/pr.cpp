#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/file.h>

#define MAX_LENGTH_FILENAME 255
#define MAX_USERNAME_LENGTH 100
#define MAX_PASSWORD_LENGTH 50
#define MAX_USERNAMES_COUNT 4096
#define BUFSIZE 4096

char usernames[MAX_USERNAMES_COUNT][MAX_USERNAME_LENGTH];
char passwords[MAX_USERNAMES_COUNT][MAX_PASSWORD_LENGTH];

char *filename, *username, *password, *begin_of_part_str, *end_of_part_str;
int begin_of_part, end_of_part;
char lockfile[MAX_LENGTH_FILENAME];
char locklockfile[MAX_LENGTH_FILENAME];

int number_of_strings = 0;

void change_password(int, int);
bool is_file_part_locked(char *, int, int);
int file_exists(char *);
void lock_file(char *, char);
void lock_file_part(char *, char, int, int);
void unlock_file(char *);
void unlock_file_part(char *);

void change_password_in_file_part(char *, char *, char *, int, int);

void err_sys(const char*);

int main(int argc, char ** argv) {
	if (argc != 6) {
		err_sys("Incorrect arguments number. Usage: ./pr filename username password begin_of_part end_of_part");
	}

	filename = argv[1];
	username = argv[2];
	password = argv[3];
	begin_of_part_str = argv[4];
	end_of_part_str = argv[5];

	char *end_pointer;
	begin_of_part = strtol(begin_of_part_str, &end_pointer, 10);
	end_of_part = strtol(end_of_part_str, &end_pointer, 10);


	int filename_length = (sizeof(filename) / sizeof(char));
	if (filename_length > MAX_LENGTH_FILENAME) {
		err_sys("Incorrect filename length.");
	}

    snprintf(lockfile, sizeof lockfile, "%s%s", filename, ".lck");
    snprintf(locklockfile, sizeof lockfile, "%s%s", filename, ".lck.lck");

	change_password(begin_of_part, end_of_part);

	return 0;
}

void change_password(int begin_of_part, int end_of_part) {
    while (file_exists(lockfile)) {
        printf("%s exists. Wait.\n", locklockfile);
        fflush(stdout);
        sleep(5);
	}

	lock_file(locklockfile, 'w');
	sleep(5);

	while(is_file_part_locked(lockfile, begin_of_part, end_of_part)) {
        unlock_file(locklockfile);
		sleep(5);
		lock_file(locklockfile, 'w');
	}
    lock_file_part(lockfile, 'w', begin_of_part, end_of_part);
    sleep(5);

    unlock_file(locklockfile);
    sleep(5);

    change_password_in_file_part(filename, username, password, begin_of_part-1, end_of_part-1);
    sleep(5);

    lock_file(locklockfile, 'w');
    sleep(5);
    unlock_file_part(lockfile);
    sleep(5);
    unlock_file(locklockfile);
    sleep(5);
}

bool is_file_part_locked(char * lockfile, int begin_of_part, int end_of_part) {
    FILE *fp;
    bool result = false;

    if (file_exists(lockfile)) {
        FILE *f = fopen(lockfile, "r");

        int pid, begin_of_locked_part, end_of_locked_part;
        char lock_type;

        while (fscanf(f, "%d %c %d %d\n", &pid, &lock_type, &begin_of_locked_part, &end_of_locked_part) != EOF) {
            if (((begin_of_part < begin_of_locked_part) && (end_of_part < begin_of_locked_part))
                || ((begin_of_locked_part < begin_of_part) && (end_of_locked_part < begin_of_part))) {
                    result = true;
                    break;
                }
        }
        result = true;
    }

    return result;
}

void lock_file(char* lockfile, char operation) {
    int pid = getpid();

    FILE *f = fopen(lockfile, "w");
    if (f != NULL) {
        fprintf(f, "%d %c\n", pid, operation);
        fclose(f);
        printf("Create file %s. PID: %d, operation: %c\n", lockfile, pid, operation);
    } else {
        err_sys("Error in file creating");
    }
}

void unlock_file(char *lockfile) {
    if(remove(lockfile) != 0) {
        printf("Error: unable to delete the file %s\n", lockfile);
    }
    printf("Remove file %s.\n", lockfile);
}

void lock_file_part(char* lockfile, char operation, int begin_of_part, int end_of_part) {
    FILE *f = fopen(lockfile, "a");
    int pid = getpid();

    if (f != NULL) {
        fprintf(f, "%d %c %d %d\n", pid, operation, begin_of_part, end_of_part);
        printf("Add lock in file %s. PID: %d, operation: %c\n, part: (%d..%d).\n", lockfile, pid, operation, begin_of_part, end_of_part);
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

	int pid, begin_of_locked_part, end_of_locked_part;
	char lock_type;
	int counter = 0;

	while (fscanf(fp, "%d %c %d %d\n", &pid, &lock_type, &begin_of_locked_part, &end_of_locked_part) != EOF) {
        if (pid != current_pid) {
            counter++;
            fprintf(tempfile, "%d %c %d %d\n", pid, lock_type, begin_of_locked_part, end_of_locked_part);
        }
     }

    fclose(fp);

    if (counter == 0) {
        if(remove(lockfile) != 0) {
            printf("Error: unable to delete the file %s\n", lockfile);
        }
    } else {
        if ((fp = fopen(lockfile, "w+")) == NULL) {
            err_sys("Can't open lockfile!");
        }

        while (fscanf(tempfile, "%d %c %d %d\n", &pid, &lock_type, &begin_of_locked_part, &end_of_locked_part) != EOF) {
            fprintf(fp, "%d %c %d %d\n", pid, lock_type, begin_of_locked_part, end_of_locked_part);
        }
        fclose(fp);
    }

    printf("Remove lock from file %s. PID: %d.\n", lockfile, current_pid);

    fclose(tempfile);
    if(remove("/tmp/tmp_lockfile") != 0) {
        printf("Error: unable to delete the file %s\n", "/tmp/tmp_lockfile");
    }
}

void change_password_in_file_part(char* filename, char* username, char* password, int begin_of_part, int end_of_part) {
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
		err_sys("Can't open passwords file");
	}

	if (flock(fileno(fp), LOCK_EX | LOCK_NB) < 0) {
        err_sys("Can't set exclusive lock\n");
    }

    while (fscanf(fp, "%s %s\n", usernames[number_of_strings], passwords[number_of_strings]) != EOF) {
        if (number_of_strings >= begin_of_part && number_of_strings <= end_of_part) {
            if (strcmp(usernames[number_of_strings], username) == 0) {
                printf("username %s password %s will be changed to %s.\n", usernames[number_of_strings], passwords[number_of_strings], password);
                snprintf(passwords[number_of_strings], MAX_PASSWORD_LENGTH, "%s", password);
            }
        }

        number_of_strings++;
    }

    fclose(fp);
    if ((fp = fopen(filename, "w+")) == NULL) {
		err_sys("Can't open passwords file");
	}

	for (int i = 0; i < number_of_strings; i++) {
        fprintf(fp, "%s %s\n", usernames[i], passwords[i]);
	}

	flock (fileno(fp), LOCK_UN);
	printf("Password changed\n");

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
