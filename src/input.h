#include <stdio.h>
#include "lusush.h"
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#ifndef __INPUT_H__
#define __INPUT_H__

#ifdef HAVE_LIBREADLINE
extern char *rl_gets(const char *);
#endif

extern char *get_input(FILE *, const char *);
extern int do_line(char *, CMD *);

#endif
