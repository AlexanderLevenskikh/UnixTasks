#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 4096
#define HOSTNAME "localhost"
#define PORT 8090

void err_sys(char *message);

void main(int argc, char ** argv) {
    int sockFd, n, serverlen;
    char *hostName, *hostaddrp;
    struct sockaddr_in serveraddr;
    struct hostent *serverHostInfo;
    char buf[BUFSIZE];

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

    bzero(buf, BUFSIZE);
    printf("Please enter msg: ");
    fgets(buf, BUFSIZE, stdin);

    // send
    serverlen = sizeof(serveraddr);
    if ((n = sendto(sockFd, buf, strlen(buf), 0, (const struct sockaddr *) &serveraddr, serverlen)) < 0)
      err_sys("Error in sendto");

    // receive
    if ((n = recvfrom(sockFd, buf, strlen(buf), 0, (struct sockaddr *) &serveraddr, &serverlen)) < 0)
      err_sys("ERROR in recvfrom");

    printf("Echo from server: %s", buf);
    exit(0);
}

void err_sys(char *message) {
    perror(message);
    exit(1);
}
