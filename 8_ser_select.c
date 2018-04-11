#include <sys/socket.h>
#include <sys/wait.h>
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
#define LISTENQ 10
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
void str_echo(int sockfd);
ssize_t writen(int fd, const void *vptr, size_t n);
void sig_chld(int signo);

int main(int argc, char **argv)
{
    int  maxfd, listenfd, udpfd,connfd;
    pid_t childpid;
    int nready;
    char strcliaddr[INET_ADDRSTRLEN];
    ssize_t n;
    fd_set rset;
    char mesg[MAXLINE];
    const int on = 1;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    if(signal(SIGCHLD,sig_chld) == SIG_ERR)
        handle_error("signal");
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
        handle_error("setsockopt");
    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("bind");
    if(listen(listenfd, LISTENQ) == -1)
        handle_error("listen");

    if((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if(bind(udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("bind");


    maxfd = listenfd < udpfd ? udpfd:listenfd; /* initialize */
    FD_ZERO(&rset);
    for ( ; ; ) {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);
        if((nready=select(maxfd+1, &rset, NULL, NULL, NULL)) == -1)
            if (errno == EINTR)
	       	continue;
            else
	     	handle_error("select");

        if (FD_ISSET(listenfd, &rset)) { /* new client connection */
            clilen = sizeof(cliaddr);
            if((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) == -1 )
                handle_error("accept");
	    if(inet_ntop(AF_INET,&(((struct sockaddr_in *)&cliaddr)->sin_addr),strcliaddr,INET_ADDRSTRLEN) == NULL)
		handle_error("inet_ntop");
	    printf("TCP connected client ipaddress:%s\n",strcliaddr);
            if ( (childpid = fork()) <0)
                handle_error("fork");
            if (childpid == 0) { /* child process */
                if(close(listenfd) == -1)
                    handle_error("close");
                str_echo(connfd); /* process the request */
                exit(0);
            }
            if(close(connfd) == -1)
                handle_error("close");
        }
  
        if (FD_ISSET(udpfd, &rset)) { /* new client connection */
            clilen = sizeof(cliaddr);
            if((n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr *)&cliaddr, &clilen)) == -1)
                handle_error("recvfrom");
	    if(inet_ntop(AF_INET,&(((struct sockaddr_in *)&cliaddr)->sin_addr),strcliaddr,INET_ADDRSTRLEN) == NULL)
		handle_error("inet_ntop");
	    printf("UDP client ipaddress:%s\n",strcliaddr);
            if((sendto(udpfd, mesg, n, 0, (struct sockaddr *)&cliaddr, clilen)) == -1)
                handle_error("sendto");
        }
    }
}

void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
    return;
}
void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];
    again:
        while ( (n = read(sockfd, buf, MAXLINE)) > 0)
            if(writen(sockfd, buf, n) == -1)
                handle_error("writen");
        if (n < 0 && errno == EINTR)
            goto again;
        else if (n < 0)
            errx(1,"str_echo: read error");
}

ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0)
    {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0; /* and call write() again */
            else
                return (-1); /* error */
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}
