/**
 * @file   myfsp_db.h
 * @brief  数据库模块
 * @author luzeya
 * @date   2015-07-28
 */
#ifndef PRIVATE_MYFSP_DB_H
#define PRIVATE_MYFSP_DB_H

#include "myutils/myutils.h"

typedef struct {
    char usr[30];
    char passwd[30];
    char db[30];
    unsigned int port;
} DbConfig;

extern void
db_config(DbConfig *config, const char *usr, const char *passwd,
                 const char *db, unsigned int port);

extern bool
db_register_usr(DbConfig *config,const char *name, const char *passwd);

extern bool
db_login_usr(DbConfig *config, const char *name, const char *passwd,
             bool *result);

/**
 * @param name
 * @param tousr NULL 为所有用户
 * @param filepath NULL 为所有文件
 */
extern bool
db_share_file(DbConfig *config, const char *name, const char *tousr,
              int isdir, const char *filepath);

extern bool
db_share_filepath(DbConfig *config, const char *name, const char *tousr,
                  const char *filename, char *filepath);
/**
 * @param name
 * @param tousr NULL 为所有用户
 * @param filepath NULL 为所有文件
 */
extern bool
db_cancal_share(DbConfig *config, const char *name, const char *tousr,
                const char *filepath);

extern bool
db_self_share_list(DbConfig *config, int sockfd, const char *name);

extern bool
db_share_list(DbConfig *config, int sockfd, const char *name);

#endif /* PRIVATE_MYFSP_DB_H */
