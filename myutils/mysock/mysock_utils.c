#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include "../mysock.h"
#include "../mylog.h"

ssize_t
readn(int sockfd, void *buf, size_t count)
{
    size_t nleft = count;
    char *tmp = (char *)buf;
    while (nleft > 0) {
        ssize_t nread = read(sockfd, tmp, nleft);
        if (nread == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        } else if (nread == 0) {
            /* 对方关闭 */
            return count - nleft;
        } else {
            nleft -= nread;
            tmp   += nread;
        }
    }
    return count;
}

ssize_t
writen(int sockfd, void *buf, size_t count)
{
    size_t nleft = count;
    char *tmp = (char *)buf;
    while (nleft > 0) {
        ssize_t nwrite = write(sockfd, tmp, nleft);
        if (nwrite == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        } else if (nwrite == 0) {
            /* 对方关闭 */
            return count - nleft;
        } else {
            nleft -= nwrite;
            tmp   += nwrite;
        }
    }
    return count;
}

ssize_t
readline(int sockfd, void *buf, size_t maxline)
{
    int i;
    int ret, nread;
    char *bufp = (char *)buf;
    int nleft = maxline;
    while (nleft > 0) {
        ret = recv(sockfd, bufp, nleft, MSG_PEEK);
        if (ret < 0) {
            return -1;
        } else if (ret == 0) {
            return 0;
        }
        /* 看看有无\n，然后再做决定 */
        nread = ret;
        for (i = 0; i < ret; i++) {
            if (bufp[i] == '\n') {
                /* 有\n, 直接返回 */
                ret = readn(sockfd, bufp, i+1);
                /* 因为已经在缓冲区内了，所以ret不等于i+1就一定错误 */
                if (ret != i+1) {
                    return -1;
                }
                /* 读到一行, 返回 */
                return ret;
            }
        }
        /* 无\n, 将buf中的全部读完后，继续再读(循环) */
        ret = readn(sockfd, bufp, nread);
        if (ret != nread) {
            return -1;
        }
        nleft -= nread;
        bufp += nread;
    }
    return ret;
}

bool
active_nonblock(int sockfd)
{
    int ret;
    int flag = fcntl(sockfd, F_GETFL);
    if (flag == -1)
        return FALSE;
    flag |= O_NONBLOCK;
    ret = fcntl(sockfd, F_SETFL, flag);
    if (ret == -1)
        return FALSE;
    return TRUE;
}

bool
deactive_nonblock(int sockfd)
{
    int ret;
    int flag = fcntl(sockfd, F_GETFL);
    if (flag == -1)
        return FALSE;
    flag &= ~O_NONBLOCK;
    ret = fcntl(sockfd, F_SETFL, flag);
    if (ret == -1)
        return FALSE;
    return TRUE;
}

bool
read_timeout(int sockfd, unsigned int sec)
{
    int ret;
    fd_set rset;
    void *timeout;
    struct timeval tval;

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    tval.tv_sec  = sec;
    tval.tv_usec = 0;
    timeout = sec?&tval:NULL;
    do {
        ret = select(sockfd+1, &rset, NULL, NULL, timeout);
    } while (ret == -1 && errno == EINTR);
    /* timeout */
    if (ret == 0) {
        errno = ETIMEDOUT;
        return FALSE;
    }
    if (ret == -1)
        return FALSE;
    return TRUE;
}

bool
write_timeout(int sockfd, unsigned int sec)
{
    int ret;
    fd_set wset;
    void *timeout;
    struct timeval tval;

    FD_ZERO(&wset);
    FD_SET(sockfd, &wset);
    tval.tv_sec  = sec;
    tval.tv_usec = 0;
    timeout = sec?&tval:NULL;
    do {
        ret = select(sockfd+1, NULL, &wset, NULL, timeout);
    } while (ret == -1 && errno == EINTR);
    /* timeout */
    if (ret == 0) {
        errno = ETIMEDOUT;
        return FALSE;
    }
    /* other error */
    if (ret == -1)
        return FALSE;
    return TRUE;
}

bool
connect_timeout(int sockfd, struct sockaddr_in *addr, unsigned int sec)
{
    int ret;

    if (active_nonblock(sockfd) == FALSE)
        goto error;
    ret = connect(sockfd, (struct sockaddr*)addr,
                  sizeof(struct sockaddr_in));
    if (ret == -1 && errno == EINPROGRESS) {
        int sret;
        fd_set wset;
        struct timeval tval;
        void *timeout;

        FD_ZERO(&wset);
        FD_SET(sockfd, &wset);
        tval.tv_sec  = sec;
        tval.tv_usec = 0;
        timeout = sec?&tval:NULL;
        do {
            sret = select(sockfd+1, NULL, &wset, NULL, timeout);
        } while (sret == -1 && errno == EINTR);
        if (sret == -1)
            goto error;
        else if (sret == 0) {
            errno = ETIMEDOUT;
            goto error;
        } else if (sret == 1) {
            int optval;
            socklen_t optlen = sizeof(optval);

            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
                           &optval, &optlen) != 0) {
                goto error;
            }
            if (optval != 0) {
                errno = optval;
                goto error;
            }
        }
    } else if (ret == -1) {
        goto error;
    }
    if (deactive_nonblock(sockfd) == FALSE)
        goto error;
    return TRUE;
error:
    return FALSE;
}

int
accept_timeout(int sockfd, struct sockaddr_in *addr, unsigned int sec)
{
    int ret = 0;
    fd_set rset;
    struct timeval time;
    void *timeout;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    time.tv_sec = sec;
    time.tv_usec = 0;
    timeout = sec?&time:NULL;
    do {
        ret = select(sockfd+1, &rset, NULL, NULL, timeout);
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) {
        goto error;
    } else if (ret == 0) {
        ret = -1;
        errno = ETIMEDOUT;
    } else {
        ret = accept(sockfd, (struct sockaddr*)addr,
                     addr?&addrlen:NULL);
        /* 可以直接返回ret */
    }
    return ret;
error:
    return -1;
}

static void
pr_addr(struct sockaddr_in *addr)
{
    debug("ip: %s:%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
}

bool
getlocalsockaddr(int sockfd, char *ip, int *port)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        err("getsockname failure");
        return FALSE;
    }
    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);
    return TRUE;
}

bool
pr_sockaddr(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        err("getsockname failure");
        return FALSE;
    }
    pr_addr(&addr);
    return TRUE;
}

bool
pr_peeraddr(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getpeername(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        err("getpeername failure");
        return FALSE;
    }
    pr_addr(&addr);
    return TRUE;
}
