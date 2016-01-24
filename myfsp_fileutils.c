#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include "myfsp_fileutils.h"
#include "myfsp_protocol.h"

void
path_add_dir(char *path, const char *dir)
{
    strcat(path, "/");
    strcat(path, dir);
}

void
path_rm_dir(char *path)
{
    char *p;

    p = strrchr(path, '/');
    *p = '\0';
}

bool
isdir(const char *filename)
{
    struct stat st;

    /* 假定文件存在 */
    stat(filename, &st);
    if (S_ISDIR(st.st_mode)) {
        return TRUE;
    }
    return FALSE;
}

bool
ensure_file_exist(const char *filename)
{
    DIR *dir;
    struct dirent *dirent;

    assert(NULL != filename);
    dir = opendir(".");
    if (!dir) {
        err("current dir can't open!");
        return FALSE;
    }
    while ((dirent = readdir(dir)) != NULL) {
        if (strcmp(filename, dirent->d_name) == 0) {
            closedir(dir);
            return TRUE;
        }
    }
    debug("not find file %s!", filename);
    closedir(dir);
    return FALSE;
}

bool
rm_file(const char *filename)
{
    int flag;
    DIR *dir;
    char buf[1024];
    struct stat st;
    struct dirent *dirent;

    if (stat(filename, &st) == -1) {
        err("filename error!");
        return FALSE;
    }
    if (S_ISREG(st.st_mode)) {
        if (unlink(filename) == -1) {
            err("File can't delete!");
        }
    } else if (S_ISDIR(st.st_mode)) {
        dir = opendir(filename);
        if (dir == NULL) {
            err("Can't open dir!");
            return FALSE;
        }
        errno = 0;
        for (dirent = readdir(dir);
             dirent != NULL;
             dirent = readdir(dir)) {
            if (strncmp(dirent->d_name, ".", 1) == 0) {
                continue;
            }
            sprintf(buf, "%s/%s", filename, dirent->d_name);
            if (rm_file(buf) == FALSE) {
                err("shit! rm_file fatal!");
                return FALSE;
            }
        }
        if (errno != 0) {
            err("Read dir error!");
            return FALSE;
        }
        if (rmdir(filename) == -1) {
            err("Can't remove dir!");
            return FALSE;
        }
    } else {
        debug("file type error!");
        return FALSE;
    }
    return TRUE;
}

void
get_filename(const char *filepath, char *filename)
{
    char *p;

    p = strrchr(filepath, '/');
    if (p == NULL) {
        strcpy(filename, filepath);
        return;
    }
    strcpy(filename, ++p);
}

void
get_dirpath(const char *filepath, char *dirpath)
{
    char *p;

    p = strrchr(filepath, '/');
    if (p == NULL) {
        strcpy(dirpath, ".");
    }
    strncpy(dirpath, filepath, p - filepath);
}

static bool
write_file_info(int sockfd, int clientfd, const char *filename)
{
    int status;
    struct stat st;
    char buf[SENDBUFSIZE];

    assert(NULL != filename);
    status = stat(filename, &st);
    if (status == -1) {
        err("Can't get file info!");
        return FALSE;
    }
    sprintf(buf, "%c %ld %s\r\n", S_ISDIR(st.st_mode)?'d':'f',
            st.st_size, filename);
    debug("%s", buf);
    status = anet_tcp_send(clientfd, buf, strlen(buf), 0);
    if (status == -1) {
        err("Can't send file info!");
        return FALSE;
    }
    return TRUE;
}

bool
write_list(int sockfd, int clientfd, const char *dirpath)
{
    DIR *dir;
    bool status;
    struct dirent *dirent;

    dir = opendir(dirpath?dirpath:".");
    if (!dir) {
        err("Can't open dir!");
        close(clientfd);
        return FALSE;
    }
    while ((dirent = readdir(dir)) != NULL) {
        /* if (strncmp(".", dirent->d_name, 1) == 0) { */
        /*     continue; */
        /* } */
        status = write_file_info(sockfd, clientfd, dirent->d_name);
        if (status == FALSE) {
            err("Can't send file list!");
            close(clientfd);
            return FALSE;
        }
    }
    close(clientfd);
    return TRUE;
}

bool
write_file(int sockfd, int clientfd, const char *usr,
           const char *filename)
{
    /* TODO: stat readlink 判断是否是符号连接 */
    int fileno;
    int status;
    char buf[SENDBUFSIZE];

    fileno = open(filename, O_RDONLY);
    if (fileno == -1) {
        err("file can't open!");
        close(clientfd);
        return FALSE;
    }
    while ((status = read(fileno, buf, sizeof(buf)) ) != 0) {
        if (status == -1) {
            err("Can't read file!");
            goto error;
        }
        /* 写入status个字节 */
        status = anet_tcp_send(clientfd, buf, status, 0);
        if (status == -1) {
            err("Can't send file!");
            goto error;
        }
    }
    close(fileno);
    close(clientfd);
    return TRUE;
error:
    close(clientfd);
    unlink(filename);
    close(fileno);
    return FALSE;
}

bool
read_file(int sockfd, int clientfd, const char *filename,
          const char *savepath)
{
    int fileno;
    int status, nwrite;
    char buf[RCVBUFSIZE];

    debug("read file:%s", filename);
    fileno = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    debug("fileno:%d, filename:%s", fileno, filename);
    if (fileno == -1) {
        err("Can't create file!");
        return FALSE;
    }
    /* 0必定是关闭， < sizeof(buf)是也是关闭 */
    /* 但无法终止循环, 因为还有字节为输入 */
    /* 让其再循环一次，其结果必定为0,即可判断 */
    for (status = anet_tcp_recv(clientfd, buf, sizeof(buf), 0);
         status <= sizeof(buf) && status > 0;
         status = anet_tcp_recv(clientfd, buf, sizeof(buf), 0)) {
        nwrite = write(fileno, buf, status);
        if (nwrite == -1) {
            err("write file");
            goto error;
        }
    }
    if (status == -1) {
        err("recv file error!");
        goto error;
    }
    debug("peer conn close!");
    close(fileno);
    close(clientfd);
    return TRUE;
error:
    close(fileno);
    unlink(filename);
    return FALSE;
}
