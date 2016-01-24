#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include "myfsp_db.h"
#include "myfsp_fileutils.h"

void
db_config(DbConfig *config, const char *usr, const char *passwd,
          const char *db, unsigned int port)
{
    strcpy(config->usr, usr);
    strcpy(config->passwd, passwd);
    strcpy(config->db, db);
    config->port   = port;
}

bool
db_register_usr(DbConfig *config, const char *name, const char *password)
{
    MYSQL *mysql = NULL;
    char insert[1024];

    assert(NULL != config);
    assert(NULL != name);
    assert(NULL != password);
    if (!(mysql = mysql_init(NULL))) {
        goto error;
    }
    if (!mysql_real_connect(mysql, "127.0.0.1", config->usr,
                            config->passwd, config->db, config->port,
                            NULL, 0)) {
        goto error;
    }
    sprintf(insert, "insert into usr values('%s', '%s');",
            name, password);
    debug("%s", insert);
    if (mysql_real_query(mysql, insert, strlen(insert))) {
        goto error;
    }
    mysql_close(mysql);
    return TRUE;
 error:
    err("%s", mysql_error(mysql));
    mysql_close(mysql);
    return FALSE;
}

bool
db_login_usr(DbConfig *config, const char *name, const char *password,
             bool *result)
{
    MYSQL *mysql = NULL;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char select[1024];

    assert(NULL != config);
    assert(NULL != name);
    assert(NULL != password);
    if (!(mysql = mysql_init(NULL))) {
        goto error;
    }
    if (!mysql_real_connect(mysql, "127.0.0.1", config->usr,
                            config->passwd, config->db, config->port,
                            NULL, 0)) {
        goto error;
    }
    sprintf(select, "select name, passwd from usr where name = '%s' and passwd = '%s';",
            name, password);
    debug("%s", select);
    if (mysql_real_query(mysql, select, strlen(select))) {
        goto error;
    }
    res = mysql_store_result(mysql);
    if (!res) {
        if (mysql_errno(mysql)) {
            goto error;
        }
        /* select不会出现这种种状况 */
        assert(0);
        return TRUE;
    }
    row = mysql_fetch_row(res);
    if (!row) {
        *result = FALSE;
        return TRUE;
    }
    mysql_close(mysql);
    *result = TRUE;
    return TRUE;
 error:
    err("%s", mysql_error(mysql));
    return FALSE;

}

/**
 * @param name
 * @param tousr NULL 为所有用户
 * @param filepath NULL 为所有文件
 */
bool
db_share_file(DbConfig *config, const char *name, const char *tousr,
              int isdir, const char *filepath)
{
    MYSQL *mysql = NULL;
    char insert[1024];
    char filename[54];

    assert(NULL != config);
    assert(NULL != name);
    assert(NULL != tousr);
    assert(NULL != filepath);
    if (!(mysql = mysql_init(NULL))) {
        goto error;
    }
    if (!mysql_real_connect(mysql, "127.0.0.1", config->usr,
                            config->passwd, config->db, config->port,
                            NULL, 0)) {
        goto error;
    }
    get_filename(filepath, filename);
    debug("%s,%s", filepath, filename);
    sprintf(insert, "insert into share values('%s','%s', '%s', '%c','%s');",
            name, tousr, filename, isdir?'t':'f',filepath);
    debug("%s", insert);
    if (mysql_real_query(mysql, insert, strlen(insert))) {
        goto error;
    }
    mysql_close(mysql);
    return TRUE;
 error:
    err("%s", mysql_error(mysql));
    mysql_close(mysql);
    return FALSE;
}

/**
 * @param name
 * @param tousr NULL 为所有用户
 * @param filepath NULL 为所有文件
 */
bool
db_cancal_share(DbConfig *config, const char *name, const char *tousr,
                const char *filename)
{
    MYSQL *mysql = NULL;
    char delete[1024];
    my_ulonglong affect;

    assert(NULL != config);
    assert(NULL != name);
    assert(NULL != tousr);
    assert(NULL != filename);
    if (!(mysql = mysql_init(NULL))) {
        goto error;
    }
    if (!mysql_real_connect(mysql, "127.0.0.1", config->usr,
                            config->passwd, config->db, config->port,
                            NULL, 0)) {
        goto error;
    }
    sprintf(delete, "delete from share where fromusr = '%s' and tousr = '%s' and filename = '%s';",
            name, tousr, filename);
    debug("%s", delete);
    if (mysql_real_query(mysql, delete, strlen(delete))) {
        goto error;
    }
    affect = mysql_affected_rows(mysql);
    if (!affect) {
        goto error;
    }
    mysql_close(mysql);
    return TRUE;
 error:
    err("%s", mysql_error(mysql));
    mysql_close(mysql);
    return FALSE;
}

bool
db_share_filepath(DbConfig *config, const char *name, const char *tousr,
                  const char *filename, char *filepath)
{
    MYSQL *mysql = NULL;
    char select[1024];
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;

    assert(NULL != config);
    assert(NULL != name);
    assert(NULL != tousr);
    assert(NULL != filename);
    if (!(mysql = mysql_init(NULL))) {
        goto error;
    }
    if (!mysql_real_connect(mysql, "127.0.0.1", config->usr,
                            config->passwd, config->db, config->port,
                            NULL, 0)) {
        goto error;
    }
    sprintf(select, "select filepath from share where fromusr = '%s' and filename = '%s' and tousr in ('%s', 'all')",
            name, filename, tousr);
    debug("%s", select);
    if (mysql_real_query(mysql, select, strlen(select))) {
        goto error;
    }
    res = mysql_store_result(mysql);
    if (!res) {
        if (mysql_errno(mysql)) {
            goto error;
        }
        /* select不会出现这种种状况 */
        assert(0);
        return FALSE;
    }
    row = mysql_fetch_row(res);
    if (!row) {
        return FALSE;
    }
    debug("%s", row[0]);
    strcpy(filepath, row[0]);
    mysql_close(mysql);
    return TRUE;
 error:
    err("%s", mysql_error(mysql));
    mysql_close(mysql);
    return FALSE;
}

bool
db_self_share_list(DbConfig *config, int sockfd, const char *name)
{
    MYSQL *mysql = NULL;
    char select[1024];
    char buf[1024];
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;

    assert(NULL != config);
    assert(NULL != name);
    if (!(mysql = mysql_init(NULL))) {
        goto error;
    }
    if (!mysql_real_connect(mysql, "127.0.0.1", config->usr,
                            config->passwd, config->db, config->port,
                            NULL, 0)) {
        goto error;
    }
    sprintf(select, "select tousr, filename, isdir from share where fromusr = '%s';", name);
    debug("%s", select);
    if (mysql_real_query(mysql, select, strlen(select))) {
        goto error;
    }
    res = mysql_store_result(mysql);
    if (!res) {
        if (mysql_errno(mysql)) {
            goto error;
        }
        /* select不会出现这种种状况 */
        assert(0);
        return FALSE;
    }
    for (row = mysql_fetch_row(res);
         row;
         row = mysql_fetch_row(res)) {
        sprintf(buf, "%s %s %s\n", row[0], row[1], row[2]);
        int flag = anet_tcp_send(sockfd, buf, strlen(buf), 0);
        if (flag == -1 || flag == 0) {
            debug("shit");
        }
    }
    mysql_close(mysql);
    close(sockfd);
    return TRUE;
error:
    err("%s", mysql_error(mysql));
    mysql_close(mysql);
    return FALSE;
}

bool
db_share_list(DbConfig *config, int sockfd, const char *name)
{
    MYSQL *mysql = NULL;
    char select[1024];
    char buf[1024];
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;

    assert(NULL != config);
    assert(NULL != name);
    if (!(mysql = mysql_init(NULL))) {
        goto error;
    }
    if (!mysql_real_connect(mysql, "127.0.0.1", config->usr,
                            config->passwd, config->db, config->port,
                            NULL, 0)) {
        goto error;
    }
    sprintf(select, "select fromusr, tousr, filename, isdir from share where fromusr <> '%s' and tousr = 'all' or tousr = '%s'", name, name);
    debug("%s", select);
    if (mysql_real_query(mysql, select, strlen(select))) {
        goto error;
    }
    res = mysql_store_result(mysql);
    if (!res) {
        if (mysql_errno(mysql)) {
            goto error;
        }
        /* select不会出现这种种状况 */
        assert(0);
        return FALSE;
    }
    for (row = mysql_fetch_row(res);
         row;
         row = mysql_fetch_row(res)) {
        sprintf(buf, "%s %s %s %s\n", row[0], row[1], row[2], row[3]);
        int flag = anet_tcp_send(sockfd, buf, strlen(buf), 0);
        if (flag == -1 || flag == 0) {
            debug("shit");
        }
    }
    mysql_close(mysql);
    close(sockfd);
    return TRUE;
error:
    err("%s", mysql_error(mysql));
    mysql_close(mysql);
    return FALSE;
}
