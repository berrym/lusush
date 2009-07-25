#include <stdio.h>
#include "ltypes.h"

#ifndef __INPUT_H__
#define __INPUT_H__

extern char *rl_gets(const char *);
extern int get_input(FILE *, CMDLIST *, CMD *);

#endif
