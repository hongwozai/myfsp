/**
 * @file   mycfg.h
 * @brief  mycfg配置文件共用头文件
 * @author luzeya
 * @date   2015-07-23
 */
#ifndef PUBLIC_MYCFG_H_INCLUDE
#define PUBLIC_MYCFG_H_INCLUDE

#include "myutils.h"

/* 临时取值用 */
extern bool
mycfg_readint(const char *file, const char *section, const char *key,
              int *value);
extern bool
mycfg_readstring(const char *file, const char *section, const char *key,
                 char *value);

/* 多次取值使用 */
typedef struct Cfg_tag Cfg;
extern void
mycfg_open(Cfg **cfg, const char *file);
extern void
mycfg_close(Cfg *cfg);
extern bool
mycfg_read(Cfg *cfg, const char *section, const char *key,
           char *value);

#endif /* PUBLIC_MYCFG_H_INCLUDE */
