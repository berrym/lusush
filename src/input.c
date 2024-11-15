#include "../include/input.h"

#include "../include/errors.h"
#include "../include/history.h"
#include "../include/init.h"
#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/strings.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// getline input buffers
static char *buf = NULL, *buf2 = NULL;
size_t linecap = MAXLINE + 1;
ssize_t linelen;

// Free getline input buffers
void free_input_buffers(void) {
    free_str(buf);
    free_str(buf2);
}

// Get a line of input using linenoise
char *ln_gets(void) {
    char *line = NULL, *next = NULL, *tmp = NULL;

    while (true) {
        errno = 0;

        build_prompt();

        if (line == NULL) {
            line = linenoise(get_shell_varp("PS1", "% "));
        }

        if (line == NULL) {
            return NULL;
        }

        if (errno == ENOENT) {
            return NULL;
        }

        // Handle line continuations
        str_strip_trailing_whitespace(line);
        if (line[strlen(line) - 1] == '\\') {
            line[strlen(line) - 1] = '\0';
            if (next == NULL) {
                next = alloc_str(strlen(line) + 1, true);
                strcpy(next, line);
            }
            line = linenoise(get_shell_varp("PS2", "> "));
            if (line == NULL) {
                return NULL;
            }

            if (errno == ENOENT) {
                return NULL;
            }

            tmp =
                realloc(next, (strlen(next) + strlen(line) + 1) * sizeof(char));
            if (tmp == NULL) {
                error_syscall("error: `ln_gets`");
            }
            next = tmp;
            strcat(next, line);
            line = next;
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

// Get a line of input
char *get_input(FILE *in) {
    // Read a line from either a file or standard input
    if (shell_type() != NORMAL_SHELL) {
        buf = ln_gets();
    } else {
        buf = alloc_str(MAXLINE + 1, true);
        buf2 = alloc_str(MAXLINE + 1, true);

        // Read a line of input
        while ((linelen = getline(&buf2, &linecap, in))) {
            if (feof(in)) {
                return NULL;
            }

            if (ferror(in)) {
                error_return("error: `get_input`");
                return NULL;
            }

            if (!*buf) {
                strcpy(buf, buf2);
            } else {
                char *tmp =
                    realloc(buf, (strlen(buf) + linelen + 1) * sizeof(char));
                if (tmp == NULL) {
                    error_syscall("error: `get_line`");
                }
                buf = tmp;
                strcat(buf, buf2);
            }

            str_strip_trailing_whitespace(buf);
            if (buf[strlen(buf) - 1] == '\\') {
                buf[strlen(buf) - 1] = '\0';
            } else {
                break;
            }
        }
    }

    return buf;
}
