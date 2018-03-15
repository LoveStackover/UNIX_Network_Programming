#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
ssize_t writen(int fd, const void *vptr, size_t n);
void str_cli(FILE *fp, int sockfd);
int max(int,int);

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2)
        errx(1,"usage: tcpcli <IPaddress>");

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == -1)
        handle_error("inet_pton");

    if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("connect");
    str_cli(stdin, sockfd);
    exit(0);
}
int max(int a,int b)
{
	return a < b? b:a; 
}
void str_cli(FILE *fp, int sockfd)
{
        int maxfdp1, stdineof;
        fd_set rset;
        char buf[MAXLINE];
        int n;
        stdineof = 0;
        FD_ZERO(&rset);
        for ( ; ; ) {
                if (stdineof == 0)
                FD_SET(fileno(fp), &rset);
                FD_SET(sockfd, &rset);
                maxfdp1 = max(fileno(fp), sockfd) + 1;
                if(select(maxfdp1, &rset, NULL, NULL, NULL) == -1)
                        handle_error("select");
                if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
                        if ( (n = read(sockfd, buf, MAXLINE)) < 0)
                                handle_error("read");
                        if( n == 0){
                                if (stdineof == 1)
                                        return; /* normal termination */
                                else
                                        errx(1,"str_cli: server terminated prematurely");
                        }
                        if( write(fileno(stdout), buf, n) == -1 )
                                handle_error("write stdio");
                }
                if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
                        if ( (n = read(fileno(fp), buf, MAXLINE)) < 0)
                                handle_error("read");
                        if(n == 0){
                                stdineof = 1;
                                if(shutdown(sockfd, SHUT_WR)== -1)
                                        handle_error("shutdown");/* send FIN */
                                FD_CLR(fileno(fp), &rset);
                                continue;
                        }
                        if(writen(sockfd, buf, n) == -1)
                                handle_error("writen");
                }
        }
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
