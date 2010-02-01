#include <stdio.h>
#include "ldefs.h"
#include "ltypes.h"
#ifdef USING_READLINE
#include <readline/readline.h>
#endif

#ifndef __INPUT_H__
#define __INPUT_H__

#ifdef USING_READLINE
extern char *rl_gets(const char *);
#endif

extern char *get_input(FILE *, const char *);
extern int do_line(char *, CMD *);

#endif
