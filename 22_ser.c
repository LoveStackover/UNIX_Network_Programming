#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <net/if.h>
#define INET_ADDRSTRLEN 16
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)


void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen);
ssize_t recvfrom_flags(int fd, void *ptr, size_t nbytes, int *flagsp, struct sockaddr *sa, socklen_t *salenptr, struct sockaddr_in *pktp);


int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        handle_error("socket");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("bind");
    dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    int on=1;
    int flags;
    char mesg[MAXLINE];
    struct sockaddr_in pktp;
    if (setsockopt(sockfd, IPPROTO_IP, IP_RECVORIGDSTADDR, &on, sizeof(on)) < 0)
        handle_error("setsockopt");
    for ( ; ; ) {
        len = clilen;
        if((n = recvfrom_flags(sockfd, mesg, MAXLINE, &flags, pcliaddr, &len,&pktp)) == -1)
              handle_error("recvfrom");
        if((sendto(sockfd, mesg, n, 0, pcliaddr, len)) == -1)
              handle_error("sendto");
        printf("%d-byte datagram from %s", n, inet_ntop(AF_INET,&(((struct sockaddr_in *)pcliaddr)->sin_addr),mesg, sizeof(mesg)));
       	printf(", to %s\n", inet_ntop(AF_INET, &pktp.sin_addr,mesg, sizeof(mesg)));

    
        if (flags & MSG_TRUNC)
              printf(" (datagram truncated)\n");
        if (flags & MSG_CTRUNC)
              printf(" (control info truncated)\n");



    }
}



ssize_t recvfrom_flags(int fd, void *ptr, size_t nbytes, int *flagsp, struct sockaddr *sa, socklen_t *salenptr, struct sockaddr_in *pktp)
{
	struct msghdr msg;
	struct iovec iov[1];
	ssize_t n;
        char mesg[20];
  
	struct cmsghdr *cmptr;
	union {
		struct cmsghdr cm;
		//这里以前的文章讨论过，CMSG_SPACE包含pad
		char control[CMSG_SPACE(sizeof(struct sockaddr_in))];
	} control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_flags = 0;

	msg.msg_name = sa;
	msg.msg_namelen = *salenptr;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	if ( (n = recvmsg(fd, &msg, *flagsp)) < 0)
		return (n);

	*salenptr = msg.msg_namelen; /* pass back results */
	if (pktp)
		bzero(pktp, sizeof(struct sockaddr_in));
	*flagsp = msg.msg_flags; /* pass back results */
	//这三者有一个成立说明我们感兴趣的信息已经不可能在Ancillary Data中了，直接返回
	if (msg.msg_controllen < sizeof(struct cmsghdr) || (msg.msg_flags & MSG_CTRUNC) || pktp == NULL)
		return (n);
	//网络编程之Advanced I/O Functions篇记录过的处理方法
	for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
		if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_ORIGDSTADDR) {
			memcpy(pktp, CMSG_DATA(cmptr), sizeof(struct sockaddr_in));
			continue;
		}

		errx(1, "unknown ancillary data, len = %d, level = %d, type = %d", cmptr->cmsg_len, cmptr->cmsg_level, cmptr->cmsg_type);
	}
	return (n);
}
