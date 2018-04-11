#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <netdb.h>
#include <syslog.h>
#define INET_ADDRSTRLEN 16
#define LISTENQ 10
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
        do { syslog(LOG_ERR,msg"error %m"); exit(EXIT_FAILURE); } while (0)
void str_echo(int sockfd);
ssize_t writen(int fd, const void *vptr, size_t n);

int main(int argc, char **argv)
{
    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE];
    char strcliaddr[INET_ADDRSTRLEN];
    ssize_t n;
    fd_set rset, allset;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;
    int err;
    if(argc != 2)
	errx(1,"error usage 13_ser <service>\n");
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ( (err=getaddrinfo(NULL, argv[1], &hints, &res)) != 0)
        errx(1,"getaddr error for %s\n", gai_strerror(err));
    if(daemon(0,0) == -1)
	handle_error("daemon");
    ressave = res;
    do {
        listenfd =socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0)
            continue; /* error, try next one */
        if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
            handle_error("setsockopt");
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            break; /* success */
        if(close(listenfd) == -1)
            handle_error("close");/* bind error, close and try next one */
    } while ( (res = res->ai_next) != NULL);
    if (res == NULL) /* errno from final socket() or bind() */
    {
        syslog(LOG_ERR,"no valid host address information for %s\n", argv[1]);
	exit(-1);
    }
    freeaddrinfo(ressave);
    if(listen(listenfd, LISTENQ) == -1)
        handle_error("listen");
    maxfd = listenfd; /* initialize */
    maxi = -1; /* index into client[] array */
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1; /* -1 indicates available entry */
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    for ( ; ; ) {
        rset = allset; /* structure assignment */
        if((nready=select(maxfd+1, &rset, NULL, NULL, NULL)) == -1)
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
	    {
                syslog(LOG_ERR,"too many clients");
		exit(-1);
	    }
            if(inet_ntop(AF_INET,&cliaddr.sin_addr,strcliaddr,INET_ADDRSTRLEN) == NULL)
                handle_error("inet_pton");
            syslog(LOG_ERR,"connected client number : %d, ipaddress:%s\n",i,strcliaddr);
            FD_SET(connfd, &allset); /* add new descriptor to set */
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
            syslog(LOG_ERR,"closed client number : %d, ipaddress:%s\n",i,strcliaddr);
                    if(close(sockfd) == -1)
                        handle_error("close");
            if(i ==  maxi)
                maxi-=1;

                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                } else
                    if(writen(sockfd, buf, n) == -1)
                        handle_error("writen");
                if (--nready <= 0)
                    break; /* no more readable descriptors */
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
	{
            syslog(LOG_ERR,"str_echo: read error");
	    exit(-1);
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
