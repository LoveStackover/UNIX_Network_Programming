#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#define INET_ADDRSTRLEN 16
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen);
int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("bind");
    dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    char mesg[MAXLINE];
    for ( ; ; ) {
        len = clilen;
        if((n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len)) == -1)
              handle_error("recvfrom");
        if((sendto(sockfd, mesg, n, 0, pcliaddr, len)) == -1)
              handle_error("sendto");
    }
}
