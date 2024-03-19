#ifndef INIT_H
#define INIT_H

#include <stdio.h>

// Types of shell instances
enum { LOGIN_SHELL, INTERACTIVE_SHELL, NORMAL_SHELL };

int init(int, char **, FILE **);
int shell_type(void);

#endif
