#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#define LISTENQ 10
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error_en(en, msg) \
	  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
static pthread_key_t rl_key;
static pthread_once_t rl_once = PTHREAD_ONCE_INIT;
typedef struct {
    int  rl_cnt; /* initialize to 0 */
    char *rl_bufptr; /* initialize to rl_buf */
    char rl_buf[MAXLINE];
} Rline;

void str_echo(int sockfd);
ssize_t writen(int fd, const void *vptr, size_t n);
ssize_t readline(int fd, void *vptr, size_t maxlen);
static ssize_t my_read(Rline *tsd,int fd, char *ptr);
void * doit(void *arg);
static void readline_once(void);
static void readline_destructor(void *ptr);

int main(int argc, char **argv)
{
    int listenfd, connfd,error;
    pthread_t tid;
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
        if((error=pthread_create(&tid, NULL,&doit, connfd)) != 0)
            handle_error_en(error,"pthread_create");
	printf("Thread fo client:%d\n",tid);

    }
}


void * doit(void *arg)
{
    int error;
    int connfd = (int) arg;
    if((error = pthread_detach(pthread_self())) != 0){
       handle_error_en(error,"pthread_detach");
    }
    str_echo(connfd); /* same function as before */
    if(close(connfd) == -1)
       handle_error("close"); /* done with connected socket */
    return (NULL);
}

void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];
    while ((n = readline(sockfd, buf, MAXLINE)) > 0)
        if(writen(sockfd, buf, n) == -1)
            handle_error("writen");
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

static ssize_t my_read(Rline *tsd,int fd, char *ptr)
{
    if(tsd->rl_cnt <= 0) {
        again:
            if ((tsd->rl_cnt = read(fd, tsd->rl_buf, sizeof(tsd->rl_buf))) < 0) {
                if (errno == EINTR)
                    goto again;
                return (-1);
            } else if (tsd->rl_cnt == 0)
                return (0);
            tsd->rl_bufptr = tsd->rl_buf;
    }
    tsd->rl_cnt--;
    *ptr = *tsd->rl_bufptr++;
    return (1);
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;
    ptr = vptr;
    Rline *tsd;
    int error;

    if((error = pthread_once(&rl_once, readline_once)) != 0)
        handle_error_en(error,"pthread_once");
    if ( (tsd = pthread_getspecific(rl_key)) == NULL) {
        if((tsd = calloc(1, sizeof(Rline))) == NULL)
            handle_error("calloc");
        if((error = pthread_setspecific(rl_key, tsd)) != 0)
            handle_error_en(error,"pthread_setspecific");
    }
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(tsd,fd, &c)) == 1) {
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

static void readline_destructor(void *ptr)
{
    free(ptr);
}

static void readline_once(void)
{
    int error;
    if((error = pthread_key_create(&rl_key, readline_destructor)) != 0)
	handle_error_en(error,"pthread_key_create");
}
