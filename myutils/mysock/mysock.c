#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../mylog.h"
#include "../mysock.h"

int
anet_tcp_send(int sockfd, void *buf, size_t buflen, unsigned int sec)
{
    bool ret;

    ret = write_timeout(sockfd, sec);
    if (ret == TRUE) {
        return writen(sockfd, buf, buflen);
    }
    /* ret为FALSE时， write_timeout会设置errno */
    return -1;
}

int
anet_tcp_recv(int sockfd, void *buf, size_t buflen, unsigned int sec)
{
    bool ret;

    ret = read_timeout(sockfd, sec);
    if (ret == TRUE) {
        return readn(sockfd, buf, buflen);
    }
    return -1;
}

int
anet_tcp_readline(int sockfd, void *buf, size_t maxline, unsigned int sec)
{
    bool ret;

    ret = read_timeout(sockfd, sec);
    if (ret == TRUE) {
        return readline(sockfd, buf, maxline);
    }
    return -1;
}

int
anet_tcp_client()
{
    int sockfd;
    int optval;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        err("sockfd failure");
        return -1;
    }
    optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) == -1){
        err("address reuse failure");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

bool
anet_tcp_connect(int sockfd, const char *ip, short port, unsigned int sec)
{
	bool ret;
	struct sockaddr_in addr;

	addr.sin_family        =   AF_INET;
	addr.sin_addr.s_addr   =   inet_addr(ip);
	addr.sin_port          =   htons(port);
	ret = connect_timeout(sockfd, &addr, sec);
	return ret;
}

int
anet_tcp_server(const char *ip, short port, int blacklog)
{
	int sockfd, optval;
	struct sockaddr_in sockaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		err("socket failure");
		return -1;
	}
	optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) == -1){
		err("address reuse failure");
        close(sockfd);
		return -1;
	}
	sockaddr.sin_family       =  AF_INET;
	sockaddr.sin_addr.s_addr  =  inet_addr(ip==NULL?"127.0.0.1":ip);
	sockaddr.sin_port         =  htons(port);
    if (port != 0) {
        if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1) {
            err("bind failure");
            close(sockfd);
            return -1;
        }
    }
    if (listen(sockfd, blacklog?blacklog:SOMAXCONN) == -1) {
        err("listen failure");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int
anet_tcp_accept(int sockfd, unsigned int sec)
{
    return accept_timeout(sockfd, NULL, sec);
}
