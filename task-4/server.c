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
#include <sys/mman.h>

#define BUFSIZE 4096
#define PORT 8090

#define LIFE_ROWS 5
#define LIFE_COLUMNS 5
#define INITIAL_CONF_FILENAME "initialConfiguration"

void err_sys(char *message);
void initLifeBoard();
void startListener();
int initServer();
void gameStep();
void exitHandler(int sig);

volatile char *lifeGameBoard;

void main(int argc, char ** argv) {

    // space for shared board
    lifeGameBoard = mmap(0, LIFE_ROWS*LIFE_COLUMNS*sizeof(char), PROT_READ|PROT_WRITE,
              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!lifeGameBoard) {
        err_sys("Mmap failed");
    }
    memset((void *)lifeGameBoard, '.', LIFE_ROWS*LIFE_COLUMNS*sizeof(char));

    initLifeBoard();

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
                sleep(2);
                int status;
                pid_t result = waitpid(child_pid, &status, WNOHANG);
                if (result == 0) {
                  err_sys("alive");
                } else if (result == -1) {
                  err_sys("error");
                } else {
                    for (int i = 0; i < LIFE_ROWS; i++) {
                        printf("\n");
                        for (int j = 0; j < LIFE_COLUMNS; j++)
                            printf("%c", lifeGameBoard[i*LIFE_ROWS+j]);
                    }
                    printf("\n\n");
                }
            }
        }

    } else {
        // server listener
        signal(SIGINT, exitHandler);
        startListener();
    }


}

void exitHandler(int sig) {
	printf("Exit program\n");
	munmap(lifeGameBoard , LIFE_ROWS * LIFE_COLUMNS * sizeof(char));
	exit(0);
}

int getAliveAroundNumber(int x, int y) {
    int result = 0, x1, y1;

    for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++) {
        x1 = x+i; y1 = y+j;
        if ((x1 || y1) && x1 >= 0 && x1 < LIFE_COLUMNS && y1 >= 0 && y1 < LIFE_ROWS && lifeGameBoard[y1*LIFE_ROWS+x1] == 'x')
            result++;
    }
    return result;
}

void gameStep() {
    char lifeGameBoardCopy[LIFE_ROWS][LIFE_COLUMNS];

    for (int i = 0; i < LIFE_ROWS; i++) {
        for (int j = 0; j < LIFE_COLUMNS; j++) {
            int number = getAliveAroundNumber(j, i);
            if (number == 3) lifeGameBoardCopy[i][j] = 'x';
            if (number < 2 || number > 3) lifeGameBoardCopy[i][j] = '.';
            if (number == 2) lifeGameBoardCopy[i][j] = lifeGameBoard[i*LIFE_ROWS+j];
        }
    }


    for (int i = 0; i < LIFE_ROWS; i++)
    for (int j = 0; j < LIFE_COLUMNS; j++)
        lifeGameBoard[i*LIFE_ROWS+j] = lifeGameBoardCopy[i][j];

    exit(0);
}

void startListener() {
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

void initLifeBoard() {
    int fd, n;
    char newline;
    char buffer[BUFSIZE];

    if ((fd = open(INITIAL_CONF_FILENAME, O_RDONLY)) < 0) {
        err_sys("Can't open file with configuration");
    }

    if ((n = read(fd, buffer, LIFE_ROWS*(LIFE_COLUMNS+1)*sizeof(char))) < 0) {
        err_sys("Error while reading configuration file (incorrect format)");
    }

    for (int i = 0; i < LIFE_ROWS; i++)
    for (int j = 0; j < LIFE_COLUMNS; j++) {
        lifeGameBoard[i*LIFE_ROWS + j] = buffer[i*(LIFE_ROWS+1) + j];
    }


}

void err_sys(char *message) {
    perror(message);
    exit(1);
}
