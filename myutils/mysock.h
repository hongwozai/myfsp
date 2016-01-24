/**
 * @file   mysock.h
 * @brief  sock通讯库
 * @author luzeya
 * @date   2015-07-27
 */
#ifndef PUBLIC_MYFSS_H
#define PUBLIC_MYFSS_H

#include <sys/types.h>
#include <arpa/inet.h>
#include "myutils.h"

/* 共用的发送与接受库，没有处理粘包问题 */
/* 返回的值小于buflen, 大于等于0,是对方关闭 */
/* 超时返回-2 */
/* 返回-1的情况都是错误 */
extern int anet_tcp_send(int sockfd, void *buf, size_t buflen,
                         unsigned int sec);
extern int anet_tcp_recv(int sockfd, void *buf, size_t buflen,
                         unsigned int sec);
extern int anet_tcp_readline(int sockfd, void *buf, size_t maxline,
                             unsigned int sec);
/* 返回客户端的fd, 失败返回-1 */
extern int anet_tcp_client();
extern bool anet_tcp_connect(int sockfd, const char *ip, short port,
                            unsigned int sec);

/* ip为NULL, 则为本机127.0.0.1 */
/* port为0, 则为INADDR_ANY */
/* blacklog 为0, 则为默认的数值SOMAXCONN */
/* 返回sockfd，失败返回-1,并设置errno */
extern int anet_tcp_server(const char *ip, short port, int blacklog);
/* 返回对等连接的fd， */
extern int anet_tcp_accept(int sockfd, unsigned int sec);

extern bool getlocalsockaddr(int sockfd, char *ip, int *port);
extern bool pr_sockaddr(int sockfd);
extern bool pr_peeraddr(int sockfd);
/* 成功返回大于等于0的数,（等于0小于buf就是连接关闭）
 *  失败返回-1,
 *  对方关闭返回小于count,大于等于0的数
 * 所以判断时的方式及前后为
 * -1 错误
 * <count 关闭
 */
extern ssize_t readn(int sockfd, void *buf, size_t count);

/* 成功返回大于等于0的数,（等于0小于buf就是连接关闭）
 *  失败返回-1,
 *  对方关闭返回小于count,大于等于0的数
 * 所以判断时的方式及前后为
 * -1 错误
 * <count 关闭
 */
extern ssize_t writen(int sockfd, void *buf, size_t count);

/* 读出的行带\n, 成功返回读的字节， 错误返回-1, 对方关闭返回0 */
extern ssize_t readline(int sockfd, void *buf, size_t maxline);

/* 返回TRUE, 或者FALSE */
extern bool active_nonblock(int sockfd);

extern bool deactive_nonblock(int sockfd);

/* 当sec为0时， 默认阻塞 */
extern bool read_timeout(int sockfd, unsigned int sec);

/* 当sec为0时， 默认阻塞 */
extern bool write_timeout(int sockfd, unsigned int sec);
/* 当sec为0时， 默认阻塞 */
extern bool connect_timeout(int sockfd, struct sockaddr_in *addr, unsigned int sec);
/* 连接成功返回socket， 失败返回-1, 并设置errno */
/* 当sec为0时， 默认阻塞 */
extern int accept_timeout(int sockfd, struct sockaddr_in *addr, unsigned int sec);

#endif /* PUBLIC_MYFSS_H */
