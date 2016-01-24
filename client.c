#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include "myfsp.h"

void
cmd_get_token(char *buf, int *pos, char *token)
{
    /* 0为正常状态，1为a字符串状态 */
    int status = 0;
    int token_pos = 0;

    while (1) {
        if (status == 0) {
            if (isalnum(buf[*pos]) || buf[*pos] == '.' ||
                buf[*pos] == '~' || buf[*pos] == '/') {
                status = 1;
                token[token_pos++] = buf[*pos];
                *pos += 1;
                continue;
            }
            *pos += 1;
            continue;
        } else if (status == 1) {
            if (isalnum(buf[*pos]) || buf[*pos] == '.' ||
                buf[*pos] == '/' || buf[*pos] == '-') {
                token[token_pos++] = buf[*pos];
                *pos += 1;
                continue;
            } else {
                status = 0;
                token[token_pos] = '\0';
                return;
            }
        } else {
            assert(0);
        }
    }
}

int main(void)
{
    char buf[1024];
    char token[128];
    char tmp[128];
    char filename[30];
    int flag;
    bool status;
    int pos = 0;
    int sockfd, listenfd, conn;
    bool isport = FALSE;
    char ip[30];
    int port;

    /* cfg */
    mycfg_readstring("client.ini", "global", "ip", ip);
    mycfg_readint("client.ini", "global", "port", &port);
    /* signal */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    /* client */
    sockfd = anet_tcp_client();
    if (sockfd == -1) {
        err("shit!");
        return 1;
    }
    status = anet_tcp_connect(sockfd, ip, port, 3);
    if (status == FALSE) {
        if (errno == ETIMEDOUT) {
            debug("connect timeout!");
            return 1;
        }
        err("why?");
        debug("connect fail!");
        return 1;
    }
    while (1) {
        printf("> ");
        fgets(buf, sizeof(buf), stdin);
        pos = 0;
        cmd_get_token(buf, &pos, token);
        if (strcmp(token, "usr") == 0) {
            /* 设定用户名 */
            cmd_get_token(buf, &pos, token);
            sprintf(buf, REQUEST_NAME, token);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
        } else if (strcmp(token, "reg") == 0) {
            cmd_get_token(buf, &pos, token);
            cmd_get_token(buf, &pos, tmp);
            sprintf(buf, REQUEST_REG, token, tmp);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
        } else if (strcmp(token, "pass") == 0) {
            /* 设定密码 */
            cmd_get_token(buf, &pos, token);
            sprintf(buf, REQUEST_PASS, token);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            if (buf[2] == '0') {
                debug("login success!");
            } else if (buf[2] == '2') {
                debug("login fail!");
                continue;
            } else {
                debug("shit!");
                break;
            }
            /* 创建服务器 */
            listenfd = anet_tcp_server("127.0.0.1", 0, 0);
            if (listenfd == -1) {
                err("Can't open port!");
                close(listenfd);
                continue;
            }
            getlocalsockaddr(listenfd, ip, &port);
            debug("ip:%s, port:%d", ip, port);
            /* 发送端口 */
            sprintf(tmp, "%d", port);
            sprintf(buf, REQUEST_PORT, tmp);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
            if (buf[2] == '0') {
                debug("port success!");
                isport = TRUE;
                continue;
            } else {
                debug("port not success!");
                continue;
            }
        } else if (strcmp(token, "ls") == 0) {
            if (isport == FALSE) {
                debug("Please run port!");
                continue;
            }
            send_request(sockfd, 0, REQUEST_LIST);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
            conn = anet_tcp_accept(listenfd, 0);
            if (conn == -1) {
                err("accept error!");
                break;
            }
            while (recv_repl(conn, 0, buf, sizeof(buf)) != 0) {
                debug("%s", buf);
            }
            close(conn);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
        } else if (strcmp(token, "port") == 0) {
            if (isport == TRUE) {
                printf("port already!");
                continue;
            }
            cmd_get_token(buf, &pos, token);
            /* 发送端口 */
            sprintf(buf, REQUEST_PORT, token);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
            if (buf[2] == '0') {
                debug("port success!");
            } else {
                debug("port not success!");
                continue;
            }
            /* 创建一个服务器 */
            listenfd = anet_tcp_server("127.0.0.1", atoi(token), 0);
            if (listenfd == -1) {
                err("Can't open port!");
                close(listenfd);
                continue;
            }
            isport = TRUE;
        } else if (strcmp(token, "get") == 0){
            cmd_get_token(buf, &pos, token);
            sprintf(buf, REQUEST_GET, token);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
            if (buf[2] != '0') {
                debug("error!");
                break;
            }
            conn = anet_tcp_accept(listenfd, 0);
            if (conn == -1) {
                err("accept error!");
                break;
            }
            read_file(sockfd, conn, token, NULL);
            close(conn);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
        } else if (strcmp(token, "up") == 0) {
            if (isport == FALSE) {
                debug("Please run port!");
                continue;
            }
            cmd_get_token(buf, &pos, token);
            debug("token:%s", token);
            struct stat st;
            flag = stat(token, &st);
            if (flag == -1 || S_ISDIR(st.st_mode)) {
                err("File can't transmission!");
                continue;
            }
            get_filename(token, filename);
            sprintf(buf, REQUEST_UP, filename);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
            /* 接受文件 */
            conn = anet_tcp_accept(listenfd, 0);
            if (conn == -1) {
                err("accept error!");
                break;
            }
            status = write_file(sockfd, conn, NULL, token);
            if (status == FALSE) {
                err("write file failure!");
            }
            close(conn);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
        } else if (strcmp(token, "cd") == 0) {
            cmd_get_token(buf, &pos, token);
            sprintf(buf, REQUEST_CD, token);
            send_request(sockfd, 0, buf);
            recv_request(sockfd, 0, buf, sizeof(buf));
            debug("buf:%s", buf);
        } else if (strcmp(token, "mkdir") == 0) {
            cmd_get_token(buf, &pos, token);
            sprintf(buf, REQUEST_MKDIR, token);
            send_request(sockfd, 0, buf);
            recv_request(sockfd, 0, buf, sizeof(buf));
            debug("buf:%s", buf);
        } else if (strcmp(token, "rm") == 0) {
            cmd_get_token(buf, &pos, token);
            sprintf(buf, REQUEST_RM, token);
            send_request(sockfd, 0, buf);
            recv_request(sockfd, 0, buf, sizeof(buf));
            debug("buf:%s", buf);
        } else if (strcmp(token, "sls") == 0) {
            send_request(sockfd, 0, REQUEST_SHARE_LIST);
            recv_request(sockfd, 0, buf, sizeof(buf));
            debug("buf:%s", buf);
            conn = anet_tcp_accept(listenfd, 0);
            if (conn == -1) {
                err("accept error!");
                break;
            }
            while (recv_repl(conn, 0, buf, sizeof(buf)) != 0) {
                debug("%s", buf);
            }
            close(conn);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
        } else if (strcmp(token, "slist") == 0) {
            send_request(sockfd, 0, REQUEST_SELF_SHARE_LIST);
            recv_request(sockfd, 0, buf, sizeof(buf));
            debug("buf:%s", buf);
            conn = anet_tcp_accept(listenfd, 0);
            if (conn == -1) {
                err("accept error!");
                break;
            }
            while (recv_repl(conn, 0, buf, sizeof(buf)) != 0) {
                debug("%s", buf);
            }
            close(conn);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
        } else if (strcmp(token, "share") == 0) {
            /* 共享给所有人用all, 第一个是共享给人， 第二个是共享的文件 */
            cmd_get_token(buf, &pos, token);
            cmd_get_token(buf, &pos, tmp);
            sprintf(buf, REQUEST_SHARE, token, tmp);
            send_request(sockfd, 0, buf);
            recv_request(sockfd, 0, buf, sizeof(buf));
            debug("buf:%s", buf);
        } else if (strcmp(token, "cancal") == 0) {
            /* 取消共享给所有人，用all */
            cmd_get_token(buf, &pos, token);
            cmd_get_token(buf, &pos, tmp);
            sprintf(buf, REQUEST_CANCAL, token, tmp);
            send_request(sockfd, 0, buf);
            recv_request(sockfd, 0, buf, sizeof(buf));
            debug("buf:%s", buf);
        } else if (strcmp(token, "down") == 0){
            cmd_get_token(buf, &pos, token);
            cmd_get_token(buf, &pos, tmp);
            sprintf(buf, REQUEST_DOWN_SHARE, token, tmp);
            send_request(sockfd, 0, buf);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
            if (buf[2] != '0') {
                debug("error!");
                break;
            }
            conn = anet_tcp_accept(listenfd, 0);
            if (conn == -1) {
                err("accept error!");
                break;
            }
            read_file(sockfd, conn, tmp, NULL);
            close(conn);
            recv_repl(sockfd, 0, buf, sizeof(buf));
            debug("%s", buf);
        } else if (strcmp(token, "quit") == 0) {
            send_request(sockfd, 0, REQUEST_QUIT);
            printf("Bye\n");
            break;
        } else {
            printf("Please reinput!\n");
            continue;
        }
    }
    close(sockfd);
    return 0;
}
