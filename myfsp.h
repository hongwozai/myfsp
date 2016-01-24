/**
 * @file   myfsp.h
 * @brief  文件共享系统
 * @author luzeya
 * @date   2015-07-28
 */
#ifndef PUBLIC_MYFSP_H
#define PUBLIC_MYFSP_H

#include "myutils/myutils.h"
#include "myfsp_db.h"
#include "myfsp_protocol.h"
#include "myfsp_fileutils.h"

extern void
myfsp_init(int *listenfd, char *log_file, char *cfg_filename,
           DbConfig *dbcfg);

extern void
myfsp_close(int sockfd);

extern void
myfsp_accept_request(int sockfd, DbConfig *dbcfg);

#endif /* PUBLIC_MYFSP_H */
