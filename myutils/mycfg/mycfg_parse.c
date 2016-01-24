#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include "mycfg_parse.h"

static void
token_out_space(char *str)
{
    int len = strlen(str);
    for (str += len-1; *str == ' '; str--);
    str++;
    *str = '\0';
}
static void
scan(Cfg *cfg, Token *token)
{
    char ch;
    int  pos = 0;
    LexState state = INITIAL_STATE;

    assert(NULL != cfg);
    assert(NULL != token);

    for (ch = getc(cfg->fp); !feof(cfg->fp); ch = getc(cfg->fp)) {
        if (state == INITIAL_STATE) {
            if (ch == '#') {
                state = COMMENT_STATE;
            } else if (isalnum(ch)) {
                state = STRING_STATE;
                token->type  =  STRING_TYPE;
                token->string = (char*)malloc(1024);
                if (!token->string) {
                    err("token string malloc error!");
                    exit(1);
                }
                token->string[pos++] = ch;
            } else if (isspace(ch)) {
                if (ch == '\n') {
                    cfg->line_no++;
                    token->type = NEWLINE_TYPE;
                    return;
                }
            } else if (ch == '['){
                token->type = LS_TYPE;
                return;
            } else if (ch == ']') {
                token->type = RS_TYPE;
                return;
            } else if (ch == '='){
                token->type = ASSIGN_TYPE;
                return;
            } else {
                err("line %d:INITIAL STATE error!", cfg->line_no);
                exit(1);
            }
        } else if (state == COMMENT_STATE) {
            if (ch != '\n') {
                continue;
            }
            /* comment状态下的换行符忽略，但行数不忽略 */
            state = INITIAL_STATE;
            cfg->line_no++;
        } else if (state == STRING_STATE) {
            if (ch != ']' && ch != '\n' && ch != '=') {
                token->string[pos++] = ch;
                continue;
            }
            token->string[pos] = '\0';
            ungetc(ch, cfg->fp);
            token_out_space(token->string);
            return;
        } else {
            err("line %d: scan error! error! error!", cfg->line_no);
            exit(1);
        }
    }
    /* 当string后跟结束符时，会跳出循环，所以要修正 */
    if (state == INITIAL_STATE) {
        token->type = END_TYPE;
    }
}

static void
get_token(Cfg *cfg, Parse *parse, Token *token)
{
    if (parse->buf_exists) {
        *token = parse->token;
        parse->buf_exists = 0;
    } else {
        scan(cfg, token);
    }
}
static void
unget_token(Parse *parse, Token *token)
{
    parse->buf_exists = 1;
    parse->token = *token;
}
static void
parse_pair(Cfg *cfg, Parse *parse, Pair *pair)
{
    Token token;

    /* 确定为此产生式 */
    get_token(cfg, parse, &token);
    pair->key = token.string;
    get_token(cfg, parse, &token);
    if (token.type != ASSIGN_TYPE) {
        err("line %d: forget '=' !", cfg->line_no);
        exit(1);
    }
    get_token(cfg, parse, &token);
    if (token.type != STRING_TYPE) {
        err("line %d: key = value !", cfg->line_no);
        exit(1);
    }
    pair->value = token.string;
    get_token(cfg, parse, &token);
    if (token.type != NEWLINE_TYPE && token.type != END_TYPE) {
        err("line %d: must be have a '\n' !", cfg->line_no);
        exit(1);
    }
}
/* 未对pair_list分配内存 */
static void
parse_pair_list(Cfg *cfg, Parse *parse, PairList *pair_list)
{
    Token token;
    PairList *prev = NULL;        /* 用来处理结尾处的next */

    /* 第一次读的一定正确， 这个由上层保证 */
    for (get_token(cfg, parse, &token);
         token.type == STRING_TYPE || token.type == NEWLINE_TYPE;
         get_token(cfg, parse, &token)) {
        if (token.type == NEWLINE_TYPE)
            continue;
        unget_token(parse, &token);
        parse_pair(cfg, parse, &pair_list->pair);
        pair_list->next = (PairList*)malloc(sizeof(PairList));
        if (!pair_list->next) {
            err("malloc error!");
            exit(1);
        }
        prev = pair_list;
        pair_list = pair_list->next;
    }
    unget_token(parse, &token);
    free(prev->next);
    prev->next = NULL;
}
/* 未给section_name分配内存, 需要给token的string分配内存 */
static void
parse_section_name(Cfg *cfg, Parse *parse, Section *section)
{
    Token token;

    get_token(cfg, parse, &token); /* 是[ */
    get_token(cfg, parse, &token);
    if (token.type != STRING_TYPE) {
        err("line %d: section name error!", cfg->line_no);
        exit(1);
    }
    section->section_name = token.string;
    get_token(cfg, parse, &token);
    if (token.type != RS_TYPE) {
        err("line %d: section ] forget!", cfg->line_no);
        exit(1);
    }
    get_token(cfg, parse, &token);
    if (token.type != NEWLINE_TYPE) {
        err("line %d: section name must be a line!", cfg->line_no);
        exit(1);
    }
}
static void
parse_section(Cfg *cfg, Parse *parse, Section *section)
{
    Token token;

    parse_section_name(cfg, parse, section);
    for (get_token(cfg, parse, &token);
         token.type == NEWLINE_TYPE;
         get_token(cfg, parse, &token)){
        continue;
    }
    if (token.type != STRING_TYPE) {
        err("line %d: not have null section.", cfg->line_no);
        exit(1);
    }
    unget_token(parse, &token);
    section->pair_list = (PairList*)malloc(sizeof(PairList));
    if (!section->pair_list) {
        err("parse_section malloc error!");
        exit(1);
    }
    unget_token(parse, &token);
    parse_pair_list(cfg, parse, section->pair_list);
}
static void
parse_section_list(Cfg *cfg, Parse *parse, SectionList *section_list)
{
    Token token;
    SectionList *prev = NULL;

    for (get_token(cfg, parse, &token);
         token.type == LS_TYPE || token.type == NEWLINE_TYPE;
         get_token(cfg, parse, &token)) {
        if (token.type == NEWLINE_TYPE)
            continue;
        unget_token(parse, &token);
        parse_section(cfg, parse, &section_list->section);
        section_list->next = (SectionList*)malloc(sizeof(SectionList));
        if (!section_list->next) {
            err("parse_section_list malloc error!\n");
            exit(1);
        }
        prev = section_list;
        section_list = section_list->next;
    }
    unget_token(parse, &token);
    free(prev->next);
    prev->next = NULL;
}
void
parse_cfg(Cfg *cfg, Parse *parse)
{
    Token token;

    cfg->section_list  = (SectionList*)malloc(sizeof(SectionList));
    if (!cfg->section_list) {
        err("parse malloc error!");
        exit(1);
    }
    get_token(cfg, parse, &token);
    if (token.type != LS_TYPE) {
        err("line %d: cfg must be have a section!", cfg->line_no);
        exit(1);
    }
    unget_token(parse, &token);
    parse_section_list(cfg, parse, cfg->section_list);
    get_token(cfg, parse, &token);
    if (token.type != END_TYPE) {
        err("line before %d: cfg error! error! error!", cfg->line_no);
        exit(1);
    }
}
