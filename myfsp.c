#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "myfsp.h"

void
myfsp_init(int *listenfd, char *log_file, char *cfg_file,
           DbConfig *dbcfg)
{
    Cfg *cfg;
    int flag;
    bool status;
    char ip[15];
    char port[15];
    char dbport[15];
    char blacklog[15];
    char filesystem[20];

    /* 日志系统初始化 */
    if (log_file == NULL) {
        mylog_open(PRINT);
    } else {
        /* 打开失败，还输出到屏幕 */
        mylog_open(LOGFILE, log_file);
    }
    /* 读取配置 */
    debug("Read configuration file...");
    mycfg_open(&cfg, cfg_file);
    mycfg_read(cfg, "server", "ip", ip);
    mycfg_read(cfg, "server", "port", port);
    mycfg_read(cfg, "server", "blacklog", blacklog);
    mycfg_read(cfg, "server", "filesystem", filesystem);
    /* 读取数据库配置 */
    mycfg_read(cfg, "db", "usr", dbcfg->usr);
    mycfg_read(cfg, "db", "passwd", dbcfg->passwd);
    mycfg_read(cfg, "db", "db", dbcfg->db);
    mycfg_read(cfg, "db", "port", dbport);
    dbcfg->port = atoi(dbport);
    mycfg_close(cfg);
    /* 创建工作目录 */
    status = ensure_file_exist(filesystem);
    if (status == FALSE) {
        mkdir(filesystem, S_IRUSR|S_IWUSR|S_IXUSR);
    }
    flag = chdir(filesystem);
    if (flag == -1) {
        err("filesystem Can't use!");
        return;
    }
    /* 信号处理 */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    /* signal(SIGABRT, SIG_IGN); */
    /* signal(SIGINT, SIG_IGN); */
    /* 启动服务器 */
    debug("Initialize server...");
    debug("ip:%s,port:%s,blacklog:%s", ip, port, blacklog);
    *listenfd = anet_tcp_server(ip, atoi(port), atoi(blacklog));
    if (*listenfd == -1) {
        err("Server initialization failure!");
        exit(1);
    }
    debug("Server initialization success.");
}

void
myfsp_close(int sockfd)
{
    if (sockfd >= 0) {
        close(sockfd);
    }
    mylog_close();
}


static int
make_client_connection(int sockfd, const char *tmp_port)
{
    int clientfd;
    bool status;
    char *ip;
    short port;
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);

    getpeername(sockfd, (struct sockaddr*)&addr, &socklen);
    ip = inet_ntoa(addr.sin_addr);
    port = atoi(tmp_port);
    clientfd = anet_tcp_client();
    if (clientfd == -1) {
        err("create client socket error!");
        return -1;
    }
    status = anet_tcp_connect(clientfd, ip, port, 0);
    if (status == FALSE) {
        err("make client connection error!");
        return -1;
    }
    return clientfd;
}

void
myfsp_accept_request(int sockfd, DbConfig *dbcfg)
{
    int  flag;
    int clientfd;
    char name[30];              /* 存储用户姓名 */
    char passwd[30];            /* 存储用户密码 */
    char port[30];              /* 存储客户端数据连接的端口号 */
    char filename[30];          /* 存储要下载的文件名 */
    char tousr[30];
    bool isport = FALSE;                /* 判断客户端是否打开了端口 */
    bool islogin = FALSE;       /* 判断客户端是否登录 */
    bool result = FALSE;        /* 结果记录 */
    bool status;                /* 函数运行是否成功 */
    char str[RCVBUFSIZE];       /* 接受缓冲 */
    char current_dir[1024];     /* 存放当前目录 */
    char filesystem[1024];      /* 系统存放文件的根目录 */
    char tmp[1024];
    char tmp1[1024];

    getcwd(filesystem, sizeof(filesystem));
    while (1) {
        flag = recv_request(sockfd, 0, str, sizeof(str));
        if (flag == 0) {
            debug("Peer connection close!");
            return;
        } else if (flag == -1) {
            err("readline error!");
            debug("Peer exit!");
            return;
        }
        debug("request:%s", str);
        switch (parse_request(str)) {
        case REG_REQUEST:
            parse_request_2(str, name, passwd);
            status = db_register_usr(dbcfg, name, passwd);
            if (status == FALSE) {
                err("user register failure!");
                send_repl(sockfd, 0, REPL_121);
                break;
            }
            sprintf(tmp, "%s/%s", filesystem, name);
            if (mkdir(tmp, S_IRUSR|S_IWUSR|S_IXUSR) != 0) {
                err("register failure!");
                send_repl(sockfd, 0, REPL_121);
                break;
            }
            memset(name, 0, sizeof(name));
            memset(passwd, 0, sizeof(passwd));
            send_repl(sockfd, 0, REPL_120);
            break;
        case NAME_REQUEST:
            parse_request_1(str, name);
            send_repl(sockfd, 0,REPL_110);
            break;
        case PASS_REQUEST:
            parse_request_1(str, passwd);
            status = db_login_usr(dbcfg, name, passwd, &result);
            if (status == FALSE) {
                err("db login error!");
                send_repl(sockfd, 0, REPL_1001);
                return;
            }
            if (result == TRUE) {
                islogin = TRUE;
                flag = chdir(name);
                if (flag == -1) {
                    err("%s Can't access!", name);
                    send_repl(sockfd, 0, REPL_1001);
                    return;
                }
                strcpy(current_dir, name);
                debug("usr login success!,dir:%s", current_dir);
                send_repl(sockfd, 0, REPL_100);
            } else {
                islogin = FALSE;
                debug("usr login failure!");
                send_repl(sockfd, 0, REPL_102);
            }
            break;
        case PORT_REQUEST:
            parse_request_1(str, port);
            isport = TRUE;
            send_repl(sockfd, 0, REPL_200);
            break;
        case LIST_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_601);
                break;
            }
            if (isport == FALSE) {
                debug("usr not port!");
                send_repl(sockfd, 0, REPL_601);
                break;
            }
            send_repl(sockfd, 0, REPL_600);
            clientfd = make_client_connection(sockfd, port);
            if (clientfd == -1) {
                err("make client failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            status = write_list(sockfd, clientfd, NULL);
            if (status == FALSE) {
                err("write list failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            debug("write list finished");
            send_repl(sockfd, 0, REPL_602);
            break;
        case SLIST_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_321);
                break;
            }
            if (isport == FALSE) {
                debug("usr not port!");
                send_repl(sockfd, 0, REPL_321);
                break;
            }
            send_repl(sockfd, 0, REPL_320);
            clientfd = make_client_connection(sockfd, port);
            if (clientfd == -1) {
                err("make client failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            status = db_share_list(dbcfg, clientfd, name);
            if (status == FALSE) {
                err("share list failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            debug("share list finished");
            send_repl(sockfd, 0, REPL_322);
            break;
        case SELF_SHARE_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_331);
                break;
            }
            if (isport == FALSE) {
                debug("usr not port!");
                send_repl(sockfd, 0, REPL_331);
                break;
            }
            send_repl(sockfd, 0, REPL_330);
            clientfd = make_client_connection(sockfd, port);
            if (clientfd == -1) {
                err("make client failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            status = db_self_share_list(dbcfg, clientfd, name);
            if (status == FALSE) {
                err("selft share list failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            debug("self share list finished");
            send_repl(sockfd, 0, REPL_332);
            break;
        case GET_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_501);
                break;
            }
            if (isport == FALSE) {
                debug("usr not port!");
                send_repl(sockfd, 0, REPL_501);
                break;
            }
            parse_request_1(str, filename);
            send_repl(sockfd, 0, REPL_500);
            clientfd = make_client_connection(sockfd, port);
            if (clientfd == -1) {
                err("make client failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            status = write_file(sockfd, clientfd, NULL, filename);
            if (status == FALSE) {
                err("write file failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            debug("write file finished");
            send_repl(sockfd, 0, REPL_502);
            break;
        case CD_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_501);
                break;
            }
            parse_request_1(str, filename);
            if (chdir(filename) != 0) {
                err("chdir failure!");
                send_repl(sockfd, 0, REPL_703);
                break;
            }
            if (strcmp(filename, "..") == 0) {
                path_rm_dir(current_dir);
                debug("dir:%s", current_dir);
            } else {
                path_add_dir(current_dir, filename);
                debug("dir:%s", current_dir);
            }
            send_repl(sockfd, 0, REPL_700);
            break;
        case MKDIR_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_501);
                break;
            }
            parse_request_1(str, filename);
            if (mkdir(filename, S_IRUSR|S_IWUSR|S_IXUSR) != 0) {
                err("mkdir failure");
                send_repl(sockfd, 0, REPL_703);
                break;
            }
            send_repl(sockfd, 0, REPL_701);
            break;
        case RM_REQUEST:
            /* TODO:删除的同时也要查看是否共享，共享则取消 */
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_501);
                break;
            }
            parse_request_1(str, filename);
            status = ensure_file_exist(filename);
            if (status == FALSE) {
                debug("File not exists!");
                send_repl(sockfd, 0, REPL_703);
                break;
            }
            if (rm_file(filename) == FALSE) {
                debug("File or Dir don't remove!");
                send_repl(sockfd, 0, REPL_703);
                break;
            }
            send_repl(sockfd, 0, REPL_702);
            break;
        case UP_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_401);
                break;
            }
            if (isport == FALSE) {
                debug("usr not port!");
                send_repl(sockfd, 0, REPL_401);
                break;
            }
            parse_request_1(str, filename);
            send_repl(sockfd, 0, REPL_400);
            clientfd = make_client_connection(sockfd, port);
            if (clientfd == -1) {
                err("make client failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            status = read_file(sockfd, clientfd, filename, NULL);
            if (status == FALSE) {
                err("up file failure");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            debug("up file finished");
            send_repl(sockfd, 0, REPL_402);
            break;
        case SHARE_REQUEST:
            /* TODO: . 代表当前目录 */
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_301);
                break;
            }
            parse_request_2(str, tousr, filename);
            debug("%s,%s", tousr, filename);
            flag = isdir(filename);
            strcpy(tmp, current_dir);
            path_add_dir(tmp, filename);
            status = db_share_file(dbcfg, name, tousr,
                                   flag == TRUE?1:0, tmp);
            if (status == FALSE) {
                err("db share file error!");
                send_repl(sockfd, 0, REPL_301);
                break;
            }
            send_repl(sockfd, 0, REPL_300);
            debug("share file success!");
            break;
        case CANCAL_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_311);
                break;
            }
            parse_request_2(str, tousr, filename);
            debug("%s,%s", tousr, filename);
            status = db_cancal_share(dbcfg, name, tousr, filename);
            if (status == FALSE) {
                err("db cancal share file error!");
                send_repl(sockfd, 0, REPL_311);
                break;
            }
            send_repl(sockfd, 0, REPL_310);
            debug("cancal share file success!");
            break;
        case DOWN_REQUEST:
            if (islogin == FALSE) {
                debug("usr not login!");
                send_repl(sockfd, 0, REPL_511);
                break;
            }
            if (isport == FALSE) {
                debug("usr not port!");
                send_repl(sockfd, 0, REPL_511);
                break;
            }
            parse_request_2(str, tousr,filename);
            send_repl(sockfd, 0, REPL_510);
            clientfd = make_client_connection(sockfd, port);
            if (clientfd == -1) {
                err("make client failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            status = db_share_filepath(dbcfg, tousr, name, filename, tmp);
            if (status == FALSE) {
                err("db share filepath failure");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            sprintf(tmp1, "%s/%s", filesystem, tmp);
            status = write_file(sockfd, clientfd, NULL, tmp1);
            if (status == FALSE) {
                err("write file failure!");
                send_repl(sockfd, 0, REPL_1001);
                break;
            }
            debug("write file finished");
            send_repl(sockfd, 0, REPL_512);
            break;
        case QUIT_REQUEST:
            send_repl(sockfd, 0, REPL_101);
            debug("quit!");
            return;
        case UNKNOWN_REQUEST:
            debug("I don't know!");
            send_repl(sockfd, 0, REPL_1000);
            break;
        default:
            assert(0);
            break;
        }
    }
}
