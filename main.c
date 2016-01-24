#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "myfsp.h"
int main(int argc, char **argv)
{
    int sockfd;
    int conn;
    pid_t pid;
    DbConfig dbcfg;

    if (argc > 2 || argc < 1) {
        err("argument error!");
    }
    myfsp_init(&sockfd, "myfsp.log", "myfsp.ini", &dbcfg);
    if (argc == 2 && strncmp(argv[1], "-d", 2) == 0) {
        daemon(1, 1);
        debug("run in the background");
    }
    while (1) {
        conn = anet_tcp_accept(sockfd, 0);
        if (conn == -1 && errno == ETIMEDOUT) {
            err("accept connection timeout!");
            continue;
        } else if (conn == -1) {
            err("accept connection error!");
            goto error;
        }
        pid = fork();
        if (pid < 0) {
            err("Can't fork children process!");
            goto error;
        } else if (pid > 0) {
            /* 父进程 */
            close(conn);        /* 关闭通信端口 */
        } else {
            /* 子进程 */
            close(sockfd);      /* 关闭监听接口 */
            myfsp_accept_request(conn, &dbcfg);
            close(conn);
            exit(0);
        }
    }
    myfsp_close(sockfd);
    return 0;
error:
    myfsp_close(sockfd);
    return 1;
}
