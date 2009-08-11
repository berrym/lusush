#include <stdio.h>
#include "ldefs.h"
#include "ltypes.h"

#ifndef __INPUT_H__
#define __INPUT_H__

#if defined( USING_READLINE )
extern char *rl_gets(const char *);
#endif
extern int get_input(FILE *, CMDLIST *, CMD *);

#endif