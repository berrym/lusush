#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lusush.h"
#include "init.h"
#include "errors.h"
#include "input.h"
#include "linenoise.h"
#include "history.h"
#include "strings.h"
#include "symtable.h"

static char *buf = NULL, *buf2 = NULL; // Input buffers

void free_input_buffers(void)
{
    if (buf)
        free_str(buf);

    if (buf2)
        free_str(buf2);
}

char *ln_gets(void)
{
    char *line = NULL, *tmp = NULL;

    while (true) {
        errno = 0;

        if (!line)
            line = linenoise(get_shell_varp("PS1", "% "));

        if (!line)
            return NULL;

        if (errno == ENOENT)
            if (shell_type() == INTERACTIVE_SHELL)
                exit(EXIT_SUCCESS);

        // Handle line continuations
        if (line[strlen(line) - 1] == '\\') {
            line[strlen(line) - 1] = '\0';
            tmp = line;
            line = linenoise(get_shell_varp("PS2", "> "));
            strcpy(&tmp[strlen(tmp)], line);
            line = tmp;
        } else {
            break;
        }
    }

    // If the line has any text in it, save it in history
    if (line && *line) {
        history_add(line);
        history_save();
    }

    // Return the line
    return line;
}

char *get_input(FILE *in)
{
    size_t buflen = 0, linecap = 0;
    ssize_t linelen = 0;

    // If the buffers have been previously allocated free them
    free_input_buffers();

    // Read a line from either a file or standard input
    if (shell_type() != NORMAL_SHELL) {
        buf2 = ln_gets();
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
                buflen--;
            } else {
                break;
            }
        }

        return buf;
    }

    return buf2;
}
