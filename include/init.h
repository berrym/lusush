#include <stdio.h>

#ifndef INIT_H
#define INIT_H

// Types of shell instances
enum {
    LOGIN_SHELL,
    INTERACTIVE_SHELL,
    NORMAL_SHELL
};

int init(int, char **, FILE **);
int shell_type(void);

#endif
