#include <stdio.h>
#include "ldefs.h"
#include "ltypes.h"

#ifndef __INPUT_H__
#define __INPUT_H__

#if defined( USING_READLINE )
#include <readline/readline.h>
extern char *rl_gets(const char *);
#endif

extern char *get_input(FILE *, const char *);
extern int do_line(char *, CMDLIST *, CMD *);

#endif
