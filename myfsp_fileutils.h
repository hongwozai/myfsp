/**
 * @file   myfsp_fileutils.h
 * @brief  文件系统相关的函数
 * @author luzeya
 * @date   2015-07-29
 */
#ifndef PRIVATE_MYFSP_FILEUTILS_H
#define PRIVATE_MYFSP_FILEUTILS_H

#include "myfsp.h"

extern void
path_add_dir(char *path, const char *dir);

extern void
path_rm_dir(char *path);

/* 确定当前目录下的文件是否存在 */
/* filename 仅是文件名，从当前目录找 */
extern bool
ensure_file_exist(const char *filename);

extern bool
isdir(const char *filename);

/* 从完整路径，获得文件名 */
extern void
get_filename(const char *filepath, char *filename);

/* 从完整的路径，获得文件的路径名 */
extern void
get_dirpath(const char *filepath, char *dirpath);

/* 递归删除文件夹或删除文件 */
extern bool
rm_file(const char *filename);

/* 发送文件列表 */
extern bool
write_list(int sockfd, int clientfd, const char *dirpath);

/* 发送文件 */
/* 由于有共享文件的发送，所以加上用户名,因为使用用户名作为文件夹储存 */
/* 共享表中的文件路径仅仅相对于用户存储路径 */
/* filename 仅包括文件名, 从当前目录找 */
extern bool
write_file(int sockfd, int clientfd, const char *usr,
           const char *filename);

/* 接受文件 */
/* filename, 包括路径名与文件名 */
/* savepath保存路径，NULL为当前路径，只有客户端用的时候才使用保存路径 */
extern bool
read_file(int sockfd, int clientfd, const char *filename,
          const char *savepath);

#endif /* PRIVATE_MYFSP_FILEUTILS_H */
