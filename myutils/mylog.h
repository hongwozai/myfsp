/**
 * @file   mylog.h
 * @brief  日志系统头文件
 * @author luzeya
 * @date   2015-07-27
 */
#ifndef PUBLIC_MYLOG_H
#define PUBLIC_MYLOG_H

#include "myutils.h"

#ifdef NDEBUG
#define debug(format, arg...)
#else
#define debug(format, arg...)                               \
	mylog_debug(__FILE__,__FUNCTION__,__LINE__,format, ##arg)
#endif

#define err(format, arg...)                               \
	mylog_err(__FILE__,__FUNCTION__,__LINE__,format, ##arg)

typedef enum {
	LOG_WARNING = 1,
	LOG_ERROR,
	LOG_DEBUG
}LogLevel;

typedef enum {
	PRINT = 1,
	LOGFILE,
}LogType;

/* 当type为PRINT,输出到屏幕,其后无参数
 * 当type为LOGFILE时， 其后参数为文件名
 */
extern void mylog_open(LogType type, .../* const char *filename */);

/* 当type为LOGFILE时， 释放文件描述符 */
extern void mylog_close();

extern void mylog_debug(const char *file, const char *function, int line,
          const char *format, ...);

/* log_err 会打印errno的错误信息 */
extern void mylog_error(const char *file, const char *function, int line,
        const char *format, ...);

#endif /* PUBLIC_MYLOG_H */
