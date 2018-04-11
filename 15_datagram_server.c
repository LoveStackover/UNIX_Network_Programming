#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <signal.h>
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)

char * ptr;
void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen);
void sig_int(int signo)
{
	unlink(ptr);
	exit(0);
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_un servaddr, cliaddr;
    if(argc != 2)
	errx(1,"usage %s <SERVERPATHNAME>",argv[0])
    ptr=argv[1];
    unlink(ptr);
    if(signal(SIGTERM,sig_int) == SIG_ERR)
        handle_error("signal");
    if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path,ptr,sizeof(servaddr.sun_path));
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
