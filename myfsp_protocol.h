/**
 * @file   myfsp_protocol.h
 * @brief  协议请求与应答定义
 * @author luzeya
 * @date   2015-07-29
 */
#ifndef PUBLIC_MYFSP_PROTOCOL_H
#define PUBLIC_MYFSP_PROTOCOL_H

#include "myfsp.h"

#define RCVBUFSIZE 512
#define SENDBUFSIZE 512
#define DATABUFSIZE 498

/**
 * fsp请求
 */
#define REQUEST_CD    "A %s\n"    /* %s为目录名 */
#define REQUEST_MKDIR "B %s\n"
#define REQUEST_CANCAL "C %s,%s\n"       /*%s与share同义，取消共享文件 */
#define REQUEST_DOWN_SHARE  "D %s,%s\n"        /* 第一个%s共享的用户名，第二个是共享的文件 */
#define REQUEST_RM  "E %s\n"                /* 删除文件或目录 */
#define REQUEST_SHARE_LIST "F \n"           /* 显示共享列表 */
#define REQUEST_SELF_SHARE_LIST "I \n"           /* 显示自己共享的列表 */
#define REQUEST_GET   "G %s\n"        /* %s为文件名 */
#define REQUEST_REG   "H %s,%s\n"     /* %s为用户名与密码 */
#define REQUEST_NAME  "N %s\n"     /* %s为用户名 */
#define REQUEST_PASS  "P %s\n" /* %s为密码 */
#define REQUEST_PORT  "T %s\n" /* %d为告知服务器的端口号 */
#define REQUEST_LIST  "L \n"        /* 列出当前目录下的文件 */
#define REQUEST_UP    "U %s\n"        /* 上传文件, %s为文件名 */
#define REQUEST_SHARE "S %s,%s\n"        /* 第一个%s为共享用户,第二个是文件名 */
#define REQUEST_QUIT  "Q \n"          /* 退出 */

/**
 * fsp应答
 */
/**
 * 用户登录成功，用户退出, 用户名发送完毕该发送密码，用户登录失败
 * 共享文件成功，共享文件失败
 * 文件开始上传，文件上传失败，文件上传完毕 (服务器打开第二个端口)
 * 文件开始下载，文件下载失败,文件下载完毕 （客户端打开第二个端口）
 * 文件列表开始，文件列表失败，文件列表完毕 (客户端打开第二个端口)
 * 命令错误
 */
#define REPL_100 "100\n"  /* " 用户登录成功" */
#define REPL_101 "101\n"  /* " 用户退出" */
#define REPL_102 "102\n"  /* 用户登录失败 */
#define REPL_110 "110\n"  /* 用户名发送完毕,该发送密码 */
#define REPL_120 "120\n"  /* 用户注册成功 */
#define REPL_121 "121\n"  /* 用户注册失败 */
#define REPL_200 "200\n"  /* 数据连接准备完毕 */
#define REPL_201 "201\n"  /* 数据连接没有准备好 */
#define REPL_300 "300\n"  /* 共享文件成功 */
#define REPL_301 "301\n"  /* 共享文件失败 */
#define REPL_310 "310\n"  /* 取消共享文件成功 */
#define REPL_311 "311\n"  /* 取消共享文件失败 */
#define REPL_320 "320\n"  /* 共享列表开始传输 */
#define REPL_321 "321\n"  /* 共享列表传输失败 */
#define REPL_322 "322\n"  /* 共享列表传输成功 */
#define REPL_330 "330\n"  /* 自己共享列表开始传输 */
#define REPL_331 "331\n"  /* 自己共享列表传输失败 */
#define REPL_332 "332\n"  /* 自己共享列表传输成功 */
#define REPL_400 "400\n"  /* 文件开始上传，打开数据连接 */
#define REPL_401 "401\n"  /* 文件上传失败 */
#define REPL_402 "402\n"  /* 文件上传成功 */
#define REPL_500 "500\n"  /* get文件开始下载，打开数据连接 */
#define REPL_501 "501\n"  /* get文件下载失败 */
#define REPL_502 "502\n"  /* get文件下载成功 */
#define REPL_510 "510\n"  /* down文件开始下载，打开数据连接 */
#define REPL_511 "511\n"  /* down文件下载失败 */
#define REPL_512 "512\n"  /* down文件下载成功 */
#define REPL_600 "600\n"  /* 文件开始列表传输，打开数据连接 */
#define REPL_601 "601\n"  /* 文件列表传输失败 */
#define REPL_602 "602\n"  /* 文件列表传输成功 */
#define REPL_700 "700\n"  /* 切换文件夹 */
#define REPL_701 "701\n"  /* 创建文件夹 */
#define REPL_702 "702\n"  /* 删除文件或文件夹 */
#define REPL_703 "703\n"  /* 切换、创建、删除失败 */
#define REPL_1000 "1000\n"  /* 命令错误 */
#define REPL_1001 "1001\n"  /* 系统错误 */

typedef enum {
    NAME_REQUEST = 1,
    PASS_REQUEST,
    REG_REQUEST,
    CD_REQUEST,
    MKDIR_REQUEST,
    RM_REQUEST,
    LIST_REQUEST,
    PORT_REQUEST,
    GET_REQUEST,
    UP_REQUEST,
    SHARE_REQUEST,
    CANCAL_REQUEST,
    SLIST_REQUEST,
    SELF_SHARE_REQUEST,
    DOWN_REQUEST,
    QUIT_REQUEST,
    UNKNOWN_REQUEST
} RequestType;

/* 分析之后，需要向前两个字符 */
extern void
parse_request_1(char *str, char *req);

extern void
parse_request_2(char *str, char *req1, char *req2);

extern RequestType
parse_request(const char *str);

/* sec为0, 会阻塞 */
extern bool
send_repl(int sockfd, unsigned int sec, char *repl);

/* sec为0, 会阻塞 */
extern bool
send_request(int sockfd, unsigned int sec, char *request);

extern int
recv_request(int sockfd, unsigned int sec, char *str, size_t strlen);

extern int
recv_repl(int sockfd, unsigned int sec, char *str, size_t strlen);

#endif /* PUBLIC_MYFSP_PROTOCOL_H */
