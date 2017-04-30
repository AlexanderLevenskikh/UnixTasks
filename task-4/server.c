#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 4096
#define PORT 8090

void err_sys(char *message);

void main(int argc, char ** argv) {
    int sockFd, n, reuse = 1, clientlen;
    char *hostaddrp;
    struct sockaddr_in serveraddr, clientaddr;
    struct hostent *clientHostInfo;
    char buf[BUFSIZE];

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

void err_sys(char *message) {
    perror(message);
    exit(1);
}
