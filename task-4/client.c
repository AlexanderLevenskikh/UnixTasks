#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define HOSTNAME "localhost"
#define PORT 8090

#define LIFE_ROWS 10
#define LIFE_COLUMNS 10

void err_sys(char *message);

void main(int argc, char ** argv) {
    int sockFd, n, serverlen, i, j;
    char *hostName, *hostaddrp;
    struct sockaddr_in serveraddr;
    struct hostent *serverHostInfo;
    char gameBoard[LIFE_ROWS*LIFE_COLUMNS], buf[1];

    buf[0] = 'A';

    if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        err_sys("Can't open the socket");
    }

    serverHostInfo = gethostbyname(HOSTNAME);
    if (serverHostInfo == NULL) {
        fprintf(stderr,"Error, no such host as %s\n", HOSTNAME);
        exit(0);
    }
	// server Inet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)serverHostInfo->h_addr,
	  (char *)&serveraddr.sin_addr.s_addr, serverHostInfo->h_length);
    serveraddr.sin_port = htons(PORT);

    // send
    serverlen = sizeof(serveraddr);
    if ((n = sendto(sockFd, buf, sizeof(buf), 0, (const struct sockaddr *) &serveraddr, serverlen)) < 0)
      err_sys("Error in sendto");

    // receive
    if ((n = recvfrom(sockFd, gameBoard, LIFE_ROWS*LIFE_COLUMNS*sizeof(char), 0, (struct sockaddr *) &serveraddr, &serverlen)) < 0)
      err_sys("ERROR in recvfrom");

      printf("Current game state\n");
    for (i=0; i<LIFE_ROWS; i++) {
        for (j=0; j<LIFE_COLUMNS; j++) {
            printf("%c", gameBoard[i*LIFE_ROWS + j]);
        }
        printf("\n");
    }
    printf("\n");
    exit(0);
}

void err_sys(char *message) {
    perror(message);
    exit(1);
}
