#include "ldefs.h"

#ifndef __ENV_H__
#define __ENV_H__

extern int SHELL_TYPE;

extern void env_init(char **);

enum {
    LOGIN_SHELL=1,
    INTERACTIVE_SHELL,
    NORMAL_SHELL
};

#endif
