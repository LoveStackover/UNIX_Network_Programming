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

void dg_cli(FILE *fp, int sockfd, struct sockaddr *pservaddr, socklen_t servlen);          
int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    if (argc != 2)
        errx(1,"usage: udpcli <IPaddress>");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if(inet_pton(AF_INET,argv[1], &servaddr.sin_addr) != 1)
        handle_error("inet_pton");
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        handle_error("socket");
    dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    exit(0);
}

void dg_cli(FILE *fp, int sockfd, struct sockaddr *pservaddr, socklen_t servlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE + 1];
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        if(sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen) == -1)
            handle_error("sendto");
        if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))== -1)
            handle_error("recvfrom");
        recvline[n] = 0; /* null terminate */
        if(fputs(recvline, stdout) == EOF)
            handle_error("fputs");
    }
    handle_error("fgets");
}
