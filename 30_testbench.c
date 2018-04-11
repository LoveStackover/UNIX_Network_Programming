#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#define KEY 0x1 /* key for first message queue */
#define MAXLINE 20
#define handle_error(msg) \
	          do { perror(msg); exit(EXIT_FAILURE); } while (0)


#define MAXN 16384 /* max # bytes to request from server */

typedef struct {
    long mtype;
    char mtext[MAXLINE];
} Mymsg;

ssize_t writen(int fd, const void *vptr, size_t n);
ssize_t readn(int fd, void *vptr, size_t n);
void sig_int(int );


int ident;

int main(int argc, char **argv)
{
    int i, j, sockfd, nchildren, nloops, nbytes;
    pid_t pid;
    in_port_t port;
    Mymsg msg;
    ssize_t n;
    struct sockaddr_in servaddr;
    char request[MAXLINE], reply[MAXN];
    if (argc != 6)
        errx(1, "usage: client <hostname or IPaddr> <port> <#children> <#loops/child> <#bytes/request>");
    port = atoi(argv[2]);
    nchildren = atoi(argv[3]);
    nloops = atoi(argv[4]);
    nbytes = atoi(argv[5]);
    snprintf(request, sizeof(request), "%d\n", nbytes); /* newline at end */
    
    if(signal(SIGINT, sig_int) == SIG_ERR)
	handle_error("signal");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == -1)
    	handle_error("inet_pton");

    if((ident=msgget(KEY,IPC_CREAT|0660)) == -1 )
	handle_error("msgget");

    for (i = 0; i < nchildren; i++) {
        if ((pid = fork()) <0)
            handle_error("fork");
        if ( pid == 0) { /* child */
	    //debug
	    //printf("child %d start\n",i);
            for (j = 0; j < nloops; j++) {
                if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
                    handle_error("socket");
                if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
                    handle_error("connect");
                if(writen(sockfd, request, strlen(request)) == -1)
                    handle_error("writen");
                if ( (n = readn(sockfd, reply, nbytes)) != nbytes)
                    handle_error("readn");
		//debug
		//printf("childe %d, read bytes:%d\n",i,n);
                if(close(sockfd) == -1)
                    handle_error("close");
            }
	   // debug
           // printf("child %d done\n", i);
            exit(0);
        }
    /* parent loops around to fork() again */
    }
    while (wait(NULL) > 0);
        if (errno != ECHILD)
             errx(1,"wait error");
    msg.mtype=1;
    memcpy(msg.mtext,"start",6);
    if (msgsnd(ident,&msg,MAXLINE,0) == -1 )
	handle_error("msgrcv");
    if (msgrcv(ident,&msg,MAXLINE,2,0) ==  -1)
	handle_error("msgrcv");
    if (msgctl(ident,IPC_RMID,NULL) == -1)
	handle_error("msgctl");
    exit(0);
}
void sig_int(int signo)
{
    if (msgctl(ident,IPC_RMID,NULL) == -1)                                                                           
	handle_error("msgctl");
    exit(0);   
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


ssize_t readn(int fd, void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nread;
    char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0; /* and call read() again */
            else
                return (-1);
        } else if (nread == 0)
            break; /* EOF */
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft); /* return >= 0 */
}
