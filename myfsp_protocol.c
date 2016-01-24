#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "myfsp_protocol.h"
#include "myfsp.h"

void
parse_request_1(char *str, char *req)
{
    strcpy(req, str+2);
}

void
parse_request_2(char *str, char *req1, char *req2)
{
    char *p;
    char buf[30];

    strcpy(buf, str+2);
    p = strtok(buf, ",");
    strcpy(req1, p);
    p = strtok(NULL, ",");
    strcpy(req2, p);
}

RequestType
parse_request(const char *str)
{
    switch(*str) {
    case 'N':
        return NAME_REQUEST;
    case 'P':
        return PASS_REQUEST;
    case 'A':
        return CD_REQUEST;
    case 'B':
        return MKDIR_REQUEST;
    case 'E':
        return RM_REQUEST;
    case 'F':
        return SLIST_REQUEST;
    case 'G':
        return GET_REQUEST;
    case 'H':
        return REG_REQUEST;
    case 'I':
        return SELF_SHARE_REQUEST;
    case 'L':
        return LIST_REQUEST;
    case 'T':
        return PORT_REQUEST;
    case 'U':
        return UP_REQUEST;
    case 'S':
        return SHARE_REQUEST;
    case 'C':
        return CANCAL_REQUEST;
    case 'D':
        return DOWN_REQUEST;
    case 'Q':
        return QUIT_REQUEST;
    default:
        return UNKNOWN_REQUEST;
    }
}

bool
send_repl(int sockfd, unsigned int sec, char *repl)
{
    int status;

    status = anet_tcp_send(sockfd, repl, strlen(repl), sec);
    if (status == -1) {
        if (errno == ETIMEDOUT) {
            err("send repl timeout!");
            return FALSE;
        }
        err("send repl fail!");
        return FALSE;
    }
    return TRUE;
}

bool
send_request(int sockfd, unsigned int sec, char *request)
{
    return send_repl(sockfd, sec, request);
}

int
recv_request(int sockfd, unsigned int sec, char *str, size_t strlen)
{
    int flag;
    char *p;

    flag = anet_tcp_readline(sockfd, str, strlen, sec);
    if (flag == 0 || flag == -1) {
        return flag;
    }
    p = strchr(str, '\n');
    *p = '\0';
    return flag;
}

int
recv_repl(int sockfd, unsigned int sec, char *str, size_t strlen)
{
    return recv_request(sockfd, sec, str, strlen);
}
