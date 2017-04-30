#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define BUFSIZE 4096
#define PORT 8090

#define LIFE_ROWS 5
#define LIFE_COLUMNS 5
#define INITIAL_CONF_FILENAME "initialConfiguration"

void err_sys(char *message);
void initLifeBoard(char gameBoard[][LIFE_COLUMNS]);
void startListener(char gameBoard[][LIFE_COLUMNS]);
int initServer();
void gameStep();

void main(int argc, char ** argv) {
    char lifeGameBoard[LIFE_ROWS][LIFE_COLUMNS];
    initLifeBoard(lifeGameBoard);

    int pid = fork();

    if (pid < 0) {
        err_sys("Fork error");
    } else if (pid == 0) {
        // child regular task (update game field every 1 sec)
        while (1) {
            int child_pid = fork();
            if (child_pid < 0)
                err_sys("Fork error");
            else if (child_pid == 0) {
                gameStep();
            } else {
                sleep(5);
                int status;
                pid_t result = waitpid(child_pid, &status, WNOHANG);
                if (result == 0) {
                  err_sys("alive");
                } else if (result == -1) {
                  err_sys("error");
                }
            }
        }

    } else {
        // server listener
        startListener(lifeGameBoard);
    }


}

void gameStep() {
    sleep(4);
    printf("child %d \n", getpid());
    exit(0);
}

void startListener(char gameBoard[][LIFE_COLUMNS]) {
    int sockFd, n, clientlen;
    char *hostaddrp;
    struct sockaddr_in clientaddr;
    struct hostent *clientHostInfo;
    char buf[BUFSIZE];

    sockFd = initServer();

    clientlen = sizeof(clientaddr);
    while (1) {
        bzero(buf, BUFSIZE);

        if ((n = recvfrom(sockFd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen)) < 0)
            err_sys("Error in recvfrom");

        clientHostInfo = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);

		if (clientHostInfo == NULL)
            err_sys("Error on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
            err_sys("Error on inet_ntoa\n");
        printf("server received datagram from %s (%s)\n",
            clientHostInfo->h_name, hostaddrp);
        printf("server received %d/%d bytes: %s\n", (int) strlen(buf), n, buf);

        if ((n = sendto(sockFd, buf, strlen(buf), 0,
	       (struct sockaddr *) &clientaddr, clientlen)) < 0)
            err_sys("Error in sendto");
    }
}

int initServer() {
    int sockFd, reuse = 1;
    struct sockaddr_in serveraddr;

    if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        err_sys("Can't open the socket");
    }

    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR,
	     &reuse , sizeof(int));

	// server Inet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)PORT);

    if (bind(sockFd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        err_sys("Can't bind name to socket");

    return sockFd;
}

void initLifeBoard(char gameBoard[][LIFE_COLUMNS]) {
    int fd, n;
    char newline;

    if ((fd = open(INITIAL_CONF_FILENAME, O_RDONLY)) < 0) {
        err_sys("Can't open file with configuration");
    }

    for (int i = 0; i < LIFE_ROWS; i++) {
        if ((n = read(fd, gameBoard[i], LIFE_COLUMNS * sizeof(char))) < 0) {
            err_sys("Error while reading configuration file (incorrect format)");
        }
        // for newline
        if ((n = read(fd, &newline, sizeof(char))) < 0) {
            err_sys("Error while reading configuration file (incorrect format)");
        }
    }
}

void err_sys(char *message) {
    perror(message);
    exit(1);
}
