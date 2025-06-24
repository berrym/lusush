#ifndef INIT_H
#define INIT_H

#include <stdbool.h>
#include <stdio.h>

// POSIX-compliant shell types
enum {
    SHELL_NON_INTERACTIVE = 0, // Script files, -c commands, piped input
    SHELL_INTERACTIVE = 1,     // Interactive shell (stdin is terminal)
    SHELL_LOGIN = 2            // Login shell (argv[0] starts with - or -l flag)
};

// Legacy compatibility defines
#define NORMAL_SHELL SHELL_NON_INTERACTIVE
#define INTERACTIVE_SHELL SHELL_INTERACTIVE
#define LOGIN_SHELL SHELL_LOGIN

int init(int, char **, FILE **);
int shell_type(void);
bool is_interactive_shell(void);
bool is_login_shell(void);

#endif
