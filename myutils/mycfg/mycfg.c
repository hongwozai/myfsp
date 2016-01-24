#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "../mycfg.h"
#include "../mylog.h"
#include "mycfg_parse.h"

void
mycfg_open(Cfg **cfg, const char *file)
{
    Parse parse;

    *cfg = (Cfg*)malloc(sizeof(Cfg));
    if (!*cfg) {
        err("memory alloc error!\n");
        exit(1);
    }
    parse.buf_exists = 0;
    (*cfg)->line_no = 1;
    (*cfg)->fp = fopen(file, "r");
    if (!(*cfg)->fp) {
        err("file can't open.");
        exit(1);
    }
    parse_cfg(*cfg, &parse);
}

static void
mycfg_free_pair(Pair *p)
{
    assert(NULL != p);
    free(p->key);
    free(p->value);
}
static void
mycfg_free_pair_list(PairList *pl)
{
    PairList *p;

    assert(NULL != pl);
    do
    {
        p = pl;
        pl = pl->next;
        mycfg_free_pair(&p->pair);
        free(p);
    } while (pl);
}
static void
mycfg_free_section(Section *s)
{
    assert(NULL != s);
    free(s->section_name);
    mycfg_free_pair_list(s->pair_list);
}
static void
mycfg_free_section_list(SectionList *sl)
{
    SectionList *p;

    assert(NULL != sl);
    do
    {
        p = sl;
        sl = sl->next;
        mycfg_free_section(&p->section);
        free(p);
    } while (sl);
}
void
mycfg_close(Cfg *cfg)
{
    if (cfg) {
        fclose(cfg->fp);
        mycfg_free_section_list(cfg->section_list);
        free(cfg);
    }
}

bool
mycfg_read(Cfg *cfg, const char *section, const char *key, char *value)
{
    SectionList *s;
    PairList    *p;

    for (s = cfg->section_list; s; s = s->next)
        if (strcmp(s->section.section_name, section) == 0)
            for (p = s->section.pair_list; p; p = p->next)
                if (strcmp(p->pair.key, key) == 0) {
                    strcpy(value, p->pair.value);
                    return TRUE;
                }
    return FALSE;
}

bool
mycfg_readint(const char *file, const char *section, const char *key,
              int *value)
{
    Cfg *cfg;
    bool flag;
    char buf[43];

    mycfg_open(&cfg, file);
    flag = mycfg_read(cfg, section, key, buf);
    mycfg_close(cfg);
    *value = atoi(buf);
    return flag;
}
bool
mycfg_readstring(const char *file, const char *section, const char *key,
                 char *value)
{
    Cfg *cfg;
    bool flag;

    mycfg_open(&cfg, file);
    flag = mycfg_read(cfg, section, key, value);
    mycfg_close(cfg);
    return flag;
}
