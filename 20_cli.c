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
#define INET_ADDRSTRLEN 16
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
void recvfrom_alarm(int signo);
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
    socklen_t len;
    int on=1;
    char recv_mesg[MAXLINE];
    char send_mesg[MAXLINE];
    char strclient[INET_ADDRSTRLEN];
    struct sockaddr *preply_addr;
    struct sigaction act;
    fd_set rset;
    sigset_t sigset_alrm, sigset_empty;
    act.sa_handler = recvfrom_alarm;
    sigemptyset(&act.sa_mask);
    act.sa_flags =0;
    FD_ZERO(&rset);
    sigemptyset(&sigset_empty);
    sigemptyset(&sigset_alrm);
    sigaddset(&sigset_alrm, SIGALRM);


    if(sigprocmask(SIG_BLOCK, &sigset_alrm, NULL) == -1)
	handle_error("sigprocmask");

    if (sigaction(SIGALRM, &act, NULL)<0)
        handle_error("sigaction");
    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) == -1)
        handle_error("setsockopt");
    if((preply_addr = malloc(sizeof(struct sockaddr))) == NULL )
        handle_error("malloc");
    while (fgets(send_mesg, MAXLINE, stdin) != NULL) {
        if(sendto(sockfd, send_mesg, strlen(send_mesg), 0, pservaddr, servlen) == -1)
            handle_error("sendto");
        alarm(5);
        for ( ; ; ) {
	    FD_SET(sockfd, &rset);
	    n = pselect(sockfd + 1, &rset, NULL, NULL, NULL, &sigset_empty);
            if (n < 0) {
                if (errno == EINTR)
                {
                    printf("wait too long for replies\n");
                    break; /* waited long enough for replies */
                }
                else
                    handle_error("recvfrom error");
            } else {
		len = sizeof(struct sockaddr);
	        n=recvfrom(sockfd, recv_mesg, MAXLINE, 0, preply_addr, &len);	
                recv_mesg[n] = 0; /* null terminate */
                printf("from %s: %s",inet_ntop(AF_INET,&(((struct sockaddr_in *)preply_addr)->sin_addr),strclient, INET_ADDRSTRLEN), recv_mesg);
            }
        }
      }
      free(preply_addr);
      handle_error("fgets");
}

void recvfrom_alarm(int signo)
{
}
