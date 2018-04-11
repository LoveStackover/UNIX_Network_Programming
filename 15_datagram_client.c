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
void dg_cli(FILE *fp, int sockfd, struct sockaddr *pservaddr, socklen_t servlen);
char * ptr;
void sig_int(int signo)
{
    unlink(ptr);
    exit(0);
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_un servaddr,cliaddr;
    if (argc != 3)
        errx(1,"usage: udpcli <SERPATHNAME> <CLIENTPATHNAME>");
    ptr=argv[2];
    if(signal(SIGTERM,sig_int) == SIG_ERR)
	handle_error("signal");

    bzero(&servaddr, sizeof(servaddr));
    bzero(&cliaddr, sizeof(cliaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path,argv[1],sizeof(servaddr.sun_path));

    cliaddr.sun_family = AF_UNIX;
    strncpy(cliaddr.sun_path,ptr,sizeof(cliaddr.sun_path));

    if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        handle_error("socket");
    if(bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) == -1)
        handle_error("bind");
    dg_cli(stdin, sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
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
