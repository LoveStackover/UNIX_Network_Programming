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
#include <sys/ioctl.h>
#define INET_ADDRSTRLEN 16
#define SERV_PORT 9877
#define MAXLINE 20
#define handle_error(msg) \
          do { perror(msg); exit(EXIT_FAILURE); } while (0)
void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen);
int mcast_join(int sockfd, const struct sockaddr *grp, const char *ifname, u_int ifindex);
int main(int argc, char **argv)
{
    int sockfd;
    int on=1;
    if (argc != 2)
        errx(1,"usage: udpserv <mcast address>");
    struct sockaddr_in servaddr, mcastaddr,cliaddr;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        handle_error("socket");
    if(setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on)) == -1)
	      handle_error("setsockopt");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("bind");

    bzero(&mcastaddr, sizeof(mcastaddr));
    mcastaddr.sin_family = AF_INET;
    if(inet_pton(AF_INET,argv[1], &mcastaddr.sin_addr) != 1)
        handle_error("inet_pton");

    if(mcast_join(sockfd,(struct sockaddr *)&mcastaddr,NULL,0) == -1)
        handle_error("mcast_join");
    dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    char mesg[MAXLINE];
    for ( ; ; ) {
        len = clilen;
        if((n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len)) == -1)
              handle_error("recvfrom");
        if((sendto(sockfd, mesg, n, 0, pcliaddr, len)) == -1)
              handle_error("sendto");
    }
}

int mcast_join(int sockfd, const struct sockaddr *grp, const char *ifname, u_int ifindex)
{
    switch (grp->sa_family) {
        case AF_INET:{
            struct ip_mreq mreq,mreq1;
            struct ifreq ifreq;
	    int len = sizeof(mreq);
            //grp的sin_addr填充mreq结构体imr_multiaddr成员,也就是组播地址
            memcpy(&mreq.imr_multiaddr,&((const struct sockaddr_in *) grp)->sin_addr, sizeof(struct in_addr));
            //根据接口index或者名字，获取接口地址来填充mreq的imr_interface成员，如果两个参数都没指定，则接口地址为INADDR_ANY。
            if (ifindex > 0) {
                if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
                    errno = ENXIO; /* i/f index not found */
                    return (-1);
                }
                goto doioctl;
            } else if (ifname != NULL) {
                strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
                doioctl:
                    if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
                        return (-1);
                    memcpy(&mreq.imr_interface,&((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr, sizeof(struct in_addr));
            } else
                mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            return (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)));
	}
        case AF_INET6:{
            //IPv6与IPv4有点区别，在上一篇也提及，这里代码反映了这一区别
            struct ipv6_mreq mreq6;
            memcpy(&mreq6.ipv6mr_multiaddr,&((const struct sockaddr_in6 *) grp)->sin6_addr,sizeof(struct in6_addr));
            if (ifindex > 0) {
                mreq6.ipv6mr_interface = ifindex;
            } else if (ifname != NULL) {
                if ( (mreq6.ipv6mr_interface = if_nametoindex(ifname)) == 0){
                    errno = ENXIO; /* i/f name not found */
                    return (-1);
                }
            } else
                mreq6.ipv6mr_interface = 0;
            return (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)));
	}
        default:{
            errno = EAFNOSUPPORT;
            return (-1);
	}
    }
}
