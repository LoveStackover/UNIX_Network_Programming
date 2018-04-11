//6_ser.c
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
#define LISTENQ 10
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
void str_echo(int sockfd);
ssize_t writen(int fd, const void *vptr, size_t n);


int main(int argc, char **argv)
{
    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE];
    char strcliaddr[INET_ADDRSTRLEN];
    ssize_t n;
    fd_set rset, allset,xset,xallset;
    char buf[MAXLINE];
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
    maxfd = listenfd; /* initialize */
    maxi = -1; /* index into client[] array */
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1; /* -1 indicates available entry */
    FD_ZERO(&allset);
    FD_ZERO(&xallset);
    FD_SET(listenfd, &allset);
    for ( ; ; ) {
        rset = allset; /* structure assignment */
        xset= xallset;
        if((nready=select(maxfd+1, &rset, NULL, &xset, NULL)) == -1)
            handle_error("select");
        if (FD_ISSET(listenfd, &rset)) { /* new client connection */
            clilen = sizeof(cliaddr);
            if((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) == -1 )
                handle_error("accept");

            for (i = 0; i < FD_SETSIZE; i++)
            if (client[i] < 0) {
                client[i] = connfd; /* save descriptor */
                break;
            }
            if (i == FD_SETSIZE)
                errx(1,"too many clients");
            if(inet_ntop(AF_INET,&cliaddr.sin_addr,strcliaddr,INET_ADDRSTRLEN) == NULL)
                handle_error("inet_pton");
            printf("connected client number : %d, ipaddress:%s\n",i,strcliaddr);
            FD_SET(connfd, &allset); /* add new descriptor to set */
            FD_SET(connfd, &xallset); /* add new descriptor to set */
            if (connfd > maxfd)
                maxfd = connfd; /* for select */
            if (i > maxi)
                maxi = i; /* max index in client[] array */
            if (--nready <= 0)
                continue; /* no more readable descriptors */
        }
        for (i = 0; i <= maxi; i++) { /* check all clients for data */
            if ( (sockfd = client[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) {
                if ( (n = read(sockfd, buf, MAXLINE)) < 0)
                    handle_error("read");
                if(n == 0){
                    if(getpeername(sockfd,(struct sockaddr *) &cliaddr, &clilen)== -1)
                        handle_error("getpeername");
                    if(inet_ntop(AF_INET,&cliaddr.sin_addr,strcliaddr,INET_ADDRSTRLEN) == NULL)
                        handle_error("inet_pton");
                    printf("closed client number : %d, ipaddress:%s\n",i,strcliaddr);
                    if(close(sockfd) == -1)
                        handle_error("close");
                    if(i ==  maxi)
                        maxi-=1;

                    FD_CLR(sockfd, &allset);
                    FD_CLR(sockfd, &xallset);
                    client[i] = -1;
                } else
                    if(writen(sockfd, buf, n) == -1)
                        handle_error("writen");
                if (--nready <= 0)
                    break; /* no more readable descriptors */
           }

           if (FD_ISSET(sockfd, &xset)) {
                if((n = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB)) == -1)
                    handle_error("recv");
                buf[n] = 0; /* null terminate */
                printf("read OOB byte\n");
           }
        }
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
