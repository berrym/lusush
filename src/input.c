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

char *buf = NULL, *buf2 = NULL; // Input buffers

void free_input_buffers(void)
{
    if (buf)
        free_str(buf);

    if (buf2)
        free_str(buf2);
}

#if defined(USING_EDITLINE) || defined(USING_READLINE)

char *rl_gets(const char *prompt)
{
    char *s = NULL, *tmp = NULL;

    while (true) {
        if (!s) {
            s = readline(get_shell_varp("PS1", "% "));
            continue;
        }

        // Handle line continuations
        if (s[strlen(s) - 1] == '\\') {
            s[strlen(s) - 1] = '\0';
            tmp = s;
            s = readline(get_shell_varp("PS2", "> "));
            strcpy(&tmp[strlen(tmp)], s);
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
    size_t linecap = MAXLINE;
    ssize_t linelen;

    // If the buffers have been previously allocated free them
    free_input_buffers();

    // Read a line from either a file or standard input
    if (shell_type() != NORMAL_SHELL) {
        buf2 = rl_gets(get_shell_varp("PS1", "% "));
    } else {
        // Allocate memory for a line of input
        buf2 = alloc_str(MAXLINE + 1, true);

        // Allocate memory for extended line of input
        buf = alloc_str(MAXLINE + 1, true);

        // Read a line of input
        while ((linelen = getline(&buf2, &linecap, in))) {
            if (feof(in) || ferror(in))
                return NULL;
            
            strncat(buf, buf2, linelen);
            buflen += linelen;
            buflen += str_strip_trailing_whitespace(buf);

            if (buf[buflen - 1] == '\\') {
                buf[buflen - 1] = '\0';
                buflen -= 1;

                if (shell_type() != NORMAL_SHELL)
                    fprintf(stderr, "%s", get_shell_varp("PS2", "> "));
            } else {
                break;
            }
        }
    
        return buf;
    }

    return buf2;
}

#else  // Not using line editing libraries

char *get_input(FILE *in)
{
    size_t buflen = 0;
    size_t linecap = MAXLINE;
    ssize_t linelen = 0;

    // Free input buffers
    free_input_buffers();

    buf2 = alloc_str(MAXLINE + 1, true);

    // If the shell is interactive print a prompt string
    if (shell_type() != NORMAL_SHELL)
        printf("%s", get_shell_varp("PS1", ""));

    // Read a line of input
    while ((linelen = getline(&buf, &linecap, in))) {
        if (feof(in) || ferror(in))
            return NULL;
        
        strncat(buf2, buf, linelen);
        buflen += linelen;
        buflen += str_strip_trailing_whitespace(buf2);

        // Handle line continuations
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
        add_history(buf2);

    // Return full line read
    return buf2;
}

#endif
