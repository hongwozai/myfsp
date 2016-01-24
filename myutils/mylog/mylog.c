#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "../mylog.h"

typedef struct {
    LogType type;
    int     fileno;
}LogConfig;

static LogConfig log_config = {PRINT, STDOUT_FILENO};

void
mylog_open(LogType type, .../* const char *filename */)
{
    if (type == LOGFILE) {
        va_list ap;
        const char *name;

        va_start(ap, type);
        name = va_arg(ap, const char*);
        va_end(ap);
        /* 如果无法打开文件，那么便输出到屏幕 */
        log_config.fileno = open(name, O_WRONLY|O_APPEND|O_CREAT,
                                 S_IRUSR|S_IWUSR);
        if (log_config.fileno == -1) {
            goto print;
        }
        log_config.type = LOGFILE;
        return;
    }
print:
    log_config.type     = PRINT;
    log_config.fileno   = STDOUT_FILENO;
}

void
mylog_close()
{
    if (log_config.type == LOGFILE){
        close(log_config.fileno);
    }
}

void
mylog_debug(const char *file, const char *function, int line,
          const char *format, ...)
{
    va_list ap;
    char buf[1024];
    char *p;

    assert(NULL != file);
    assert(NULL != function);
    assert(NULL != format);

    va_start(ap, format);
    sprintf(buf, "[%d:%s,%s():%d] ", getpid(), file, function, line);
    p = buf + strlen(buf);
    vsprintf(p, format, ap);
    strcat(p, "\n");
    write(log_config.fileno, buf, strlen(buf));
    va_end(ap);
}

/* log_err 会打印errno的错误信息 */
void mylog_err(const char *file, const char *function,
             int line, const char *format, ...)
{
    va_list ap;
    char buf[1024];
    char *p;

    assert(NULL != file);
    assert(NULL != function);
    assert(NULL != format);

    va_start(ap, format);
    sprintf(buf, "[ERROR:%d:%s,%s():%d] ",
            getpid(), file, function, line);
    p = buf + strlen(buf);
    vsprintf(p, format, ap);
    p += strlen(p);
    sprintf(p, "(%s)\n", strerror(errno));
    write(log_config.fileno, buf, strlen(buf));
    va_end(ap);
}
