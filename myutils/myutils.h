/**
 * @file   myutils.h
 * @brief  共用库头文件
 * @author luzeya
 * @date   2015-07-27
 */
#ifndef PUBLIC_MYUTILS_H
#define PUBLIC_MYUTILS_H

#ifndef TRUE
#   ifndef FALSE
typedef enum {
    FALSE = 0,
    TRUE = 1
} bool;
#   endif  /* FALSE */
#endif     /* TRUE */

#include "mylog.h"
#include "mycfg.h"
#include "mysock.h"

#endif /* PUBLIC_MYUTILS_H */
