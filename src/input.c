#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lusush.h"
#include "errors.h"
#include "input.h"
#include "init.h"
#include "history.h"
#include "prompt.h"
#include "strings.h"
#include "symtable.h"

char *buf = NULL, *buf2 = NULL;

void free_input_buffers(void)
{
    if (buf)
        free(buf);

    if (buf2)
        free(buf2);

    buf = buf2 = NULL;
}

#if  !defined(USING_EDITLINE) && !defined(USING_READLINE)

char *get_input(FILE *in)
{
    size_t buflen = 0;
    size_t linecap = 0;
    ssize_t linelen;

    // Free input buf2fers
    free_input_buffers();

    buf = alloc_string(MAXLINE + 1, true);

    buf2 = alloc_string(MAXLINE + 1, true);

    // If the shell is interactive print a prompt string
    if (shell_type() != NORMAL_SHELL)
        printf("%s", get_shell_varp("PS1", ""));

    // Read a line of input
    while ((linelen = getline(&buf, &linecap, in))) {
        if (feof(in) || ferror(in))
            return NULL;
        strncat(buf2, buf, linelen);
        buflen += linelen;
        buflen -= strip_trailing_whspc(buf2);
        if (buf2[buflen - 1] == '\\') {
            buf2[buflen - 1] = '\0';
            buflen -= 1;
            if (shell_type() != NORMAL_SHELL)
                fprintf(stderr, "%s", get_shell_varp("PS2", ""));
        } else {
            break;
        }
    }

    // Add line to command history
    if (in == stdin && *buf2)
        add_to_history(buf2);

    // Return full line read
    return buf2;
}

#endif

#if defined(USING_EDITLINE) || defined(USING_READLINE)

char *rl_gets(const char *prompt)
{
    // A line of input
    char *s = NULL;
    char *tmp = NULL;

    while (true) {
        if (!s) {
            s = readline(get_shell_varp("PS1", ""));
            continue;
        }

        if (s[strnlen(s, MAXLINE) - 1] == '\\') {
            s[strnlen(s, MAXLINE) - 1] = '\0';
            tmp = s;
            s = readline(get_shell_varp("PS2", ""));
            strncpy(&tmp[strnlen(tmp, MAXLINE)], s, MAXLINE);
            s = tmp;
        } else {
            break;
        }
    }

    // If the line has any text in it, save it in history
    if (s && *s)
        add_history(s);

    // Return the line
    return s;
}

char *get_input(FILE *in)
{
    size_t buflen = 0;
    size_t linecap = 0;
    ssize_t linelen;

    // If the buffers have been previously allocated free them
    free_input_buffers();

    // Read a line from either a file or standard input
    if (shell_type() != NORMAL_SHELL) {
        buf2 = rl_gets(get_shell_varp("PS1", ""));
    } else {
        // Allocate memory for a line of input
        buf2 = alloc_string(MAXLINE + 1, true);

        // Allocate memory for extended line of input
        buf = alloc_string(MAXLINE + 1, true);

        // Read a line of input
        while ((linelen = getline(&buf2, &linecap, in))) {
            if (feof(in) || ferror(in))
                return NULL;
            strncat(buf, buf2, linelen);
            buflen += linelen;
            buflen -= strip_trailing_whspc(buf);
            if (buf[buflen - 1] == '\\') {
                buf[buflen - 1] = '\0';
                buflen -= 1;
            } else {
                break;
            }
        }
    
        return buf;
    }

    return buf2;
}

#endif
