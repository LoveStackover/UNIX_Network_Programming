#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
ssize_t writen(int fd, const void *vptr, size_t n);
void str_cli(FILE *fp, int sockfd);
ssize_t readline(int fd, void *vptr, size_t maxlen);
static ssize_t my_read(int fd, char *ptr) ;
static int read_cnt;
static char *read_ptr;
static char read_buf[MAXLINE];

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_un servaddr;

    if (argc != 2)
        errx(1,"usage: tcpcli <PATHNAME>");

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path,argv[1],sizeof(servaddr.sun_path));
    if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("connect");
    str_cli(stdin, sockfd);
    exit(0);
}

void str_cli(FILE *fp, int sockfd)
{
    char sendline[MAXLINE], recvline[MAXLINE];
    int n;
    while (fgets(sendline, MAXLINE, fp) != NULL)
    {
        if(writen(sockfd, sendline, strlen(sendline)) == -1)
            handle_error("writen");
        if((n=readline(sockfd, recvline, MAXLINE)) == -1)
            handle_error("readline");
        else if(n == 0)
            errx(1,"str_cli: server terminated prematurely");
        else
            if(fputs(recvline, stdout) == EOF)
                errx(1,"error in fputs");
    }
    errx(1,"fgets:end of file or error");
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


static ssize_t my_read(int fd, char *ptr)
{
    if (read_cnt <= 0) {
        again:
            if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
                if (errno == EINTR)
                    goto again;
                return (-1);
            } else if (read_cnt == 0)
                return (0);
            read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;
    return (1);
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;
    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(fd, &c)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break; /* newline is stored, like fgets() */
        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1); /* EOF, n - 1 bytes were read */
        } else
            return (-1); /* error, errno set by read() */
    }
    *ptr = 0; /* null terminate like fgets() */
    return (n);
}
