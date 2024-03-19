#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <sys/types.h>
#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/errors.h"
#include "../include/input.h"
#include "../include/third_party/linenoise.h"
#include "../include/history.h"
#include "../include/strings.h"
#include "../include/symtable.h"

ssize_t getline(char **restrict lineptr, size_t *restrict n,
                FILE *restrict stream);

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
    char *line = NULL, *line2 = NULL;

    line2 = alloc_str(MAXLINE + 1, true);

    while (true) {
        errno = 0;

        if (line == NULL)
            line = linenoise(get_shell_varp("PS1", "% "));

        if (line == NULL)
            return NULL;

        if (errno == ENOENT)
            if (shell_type() == INTERACTIVE_SHELL)
                exit(EXIT_SUCCESS);

        // Handle line continuations
        str_strip_trailing_whitespace(line);
        if (line[strlen(line) - 1] == '\\') {
            line[strlen(line) - 1] = '\0';
            if (!*line2) {
                line2 = line;
            } else {
                strcat(line2, line);
            }

            line = linenoise(get_shell_varp("PS2", "> "));
            if (line == NULL)
                return NULL;

            if (errno == ENOENT)
                if (shell_type() == INTERACTIVE_SHELL)
                    exit(EXIT_SUCCESS);

            line2 = realloc(line2, strlen(line2) + strlen(line) + 1);
            strcat(line2, line);
            line = line2;
        } else {
            break;
        }
    }

    // If the line has any text in it, save it in history
    if (*line) {
        history_add(line);
        history_save();
    }

    // Return the line
    return line;
}

char *get_input(FILE *in)
{
    // If the buffers have been previously allocated free them
    free_input_buffers();

    // Read a line from either a file or standard input
    if (shell_type() != NORMAL_SHELL) {
        buf2 = ln_gets();
    } else {
        size_t linecap = 0;
        ssize_t linelen;

        buf = alloc_str(MAXLINE + 1, true);
        buf2 = alloc_str(MAXLINE + 1, true);

        // Read a line of input
        while ((linelen = getline(&buf2, &linecap, in))) {
            if (feof(in) || ferror(in)) {
                error_return("lusush: get_input");
                exit(EXIT_FAILURE);
            }

            if (!*buf) {
                char *tmp = buf;
                buf = buf2;
                buf2 = tmp;
            } else {
                buf = realloc(buf, strlen(buf) + strlen(buf2) + 1);
                strcat(buf, buf2);
            }

            str_strip_trailing_whitespace(buf);
            if (buf[strlen(buf) - 1] == '\\') {
                buf[strlen(buf) - 1] = '\0';
            } else {
                break;
            }
        }

        return buf;
    }

    return buf2;
}
