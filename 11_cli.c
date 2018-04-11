#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
ssize_t writen(int fd, const void *vptr, size_t n);
void str_cli(FILE *fp, int sockfd);
int max(int,int);

int main(int argc, char **argv)
{
    int sockfd,err;
    struct sockaddr_in servaddr;
    char strcliaddr[INET_ADDRSTRLEN];

    if (argc != 3)
        errx(1,"usage: tcpcli <hostname> <service>");
 
    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((err=getaddrinfo(argv[1], argv[2], &hints, &res)) != 0)
        errx(1,"getaddrinfo error for %s", gai_strerror(err));
    ressave = res;
    
    do {
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue; /* ignore this one */
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break; /* success */
	else
	    perror("connect");
        if(close(sockfd) == -1)
            handle_error("close");
    } while ( (res = res->ai_next) != NULL);
    if (res == NULL) /* errno set from final connect() */
        errx(1,"no valid host address for %s",argv[1]);
    freeaddrinfo(ressave);

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
