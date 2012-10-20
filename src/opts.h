#include "lusush.h"

#ifndef __OPTS_H__
#define __OPTS_H__

extern bool set_bool_opt(unsigned int, bool);
extern bool opt_is_set(int);
extern int parse_opts(int, char **);

enum {
    VERBOSE_PRINT,
    COLOR_PROMPT,
    OPT_CNT
};

#endif
