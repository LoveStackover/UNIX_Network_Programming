#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#define LISTENQ 10
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
void str_echo(int sockfd);
ssize_t writen(int fd, const void *vptr, size_t n);


int main(int argc, char **argv)
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("bind");
    if(listen(listenfd, LISTENQ) == -1)
        handle_error("listen");
    for ( ; ; )
    {
        clilen = sizeof(cliaddr);
        if((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) == -1 )
            handle_error("accept");
        if ( (childpid = fork()) <0)
            handle_error("fork");
        if(childpid == 0)
        { /* child process */
            if(close(listenfd) == -1)
                handle_error("close");
            str_echo(connfd); /* process the request */
            exit(0);
        }
        else
            if(close(connfd) == -1)
                handle_error("close"); /* parent closes connected socket */
    }
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
