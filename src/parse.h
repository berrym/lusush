#include "ltypes.h"

#ifndef __PARSE_H__
#define __PARSE_H__

extern int char_type(char);
extern int do_magic(char);
extern int do_whspc(char);
extern int do_nchar(char);
extern int parse_cmd(CMD *, char *);

enum {
    IS_MAGIC,                   /* magic character */
    IS_WHSPC,                   /* whitespace */
    IS_NCHAR                    /* normal character */
};

#endif
