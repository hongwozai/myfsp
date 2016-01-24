/**
 * @file   parse.h
 * @brief  配置文件分析的头文件，不对外开放
 * @author luzeya
 * @date   2015-07-23
 */
#ifndef PRIVATE_PARSE_H_INCLUDE
#define PRIVATE_PARSE_H_INCLUDE

#include <stdio.h>
#include "../mycfg.h"

#define LINE_MAX_CHAR (1024)
#define KV_MAX_VALUE (1024)

typedef enum {
    LS_TYPE = 1,                /* 左方括号 */
    RS_TYPE,                    /* 右方括号 */
    COMMENT_TYPE,               /* 注释符号 */
    STRING_TYPE,                /* 字符串 */
    NEWLINE_TYPE,               /* 换行 */
    ASSIGN_TYPE,                 /* 赋值符号= */
    END_TYPE
} TokenType;

typedef enum {
    INITIAL_STATE = 1,          /* 初始状态 */
    COMMENT_STATE,              /* 注释状态 */
    STRING_STATE,               /* 字符串状态 */
} LexState;

typedef struct {
    TokenType type;
    char *string;
} Token;

typedef struct {
    char *key;
    char *value;
} Pair;
typedef struct PairList_tag {
    Pair pair;
    struct PairList_tag *next;
} PairList;

typedef struct {
    char *section_name;
    PairList *pair_list;
} Section;
typedef struct SectionList_tag {
    Section section;
    struct SectionList_tag *next;
} SectionList;

typedef struct {
    Token token;               /* 当前token */
    int buf_exists;
} Parse;

struct Cfg_tag {
    int line_no;
    FILE *fp;
    /* =======lex parse =========*/
    SectionList *section_list;
};

extern void parse_cfg(Cfg *cfg, Parse *parse);

#endif /* PRIVATE_PARSE_H_INCLUDE */
