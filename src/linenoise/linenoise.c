/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2023, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward n chars
 *
 * CUB (CUrsor Backward)
 *    Sequence: ESC [ n D
 *    Effect: moves cursor backward n chars
 *
 * The following is used to get the terminal width if getting
 * the width with the TIOCGWINSZ ioctl fails
 *
 * DSR (Device Status Report)
 *    Sequence: ESC [ 6 n
 *    Effect: reports the current cusor position as ESC [ n ; m R
 *            where n is the row and m is the column
 *
 * When multi line mode is enabled, we also use an additional escape
 * sequence. However multi line editing is disabled by default.
 *
 * CUU (Cursor Up)
 *    Sequence: ESC [ n A
 *    Effect: moves cursor up of n chars.
 *
 * CUD (Cursor Down)
 *    Sequence: ESC [ n B
 *    Effect: moves cursor down of n chars.
 *
 * When linenoiseClearScreen() is called, two additional escape sequences
 * are used in order to clear the screen and position the cursor at home
 * position.
 *
 * CUP (Cursor position)
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED (Erase display)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 *
 * Copyright 2024 Michael Berry - written and maintained for the lusush shell
 *                                many feature added to original codebase,
 *                                utf8, ANSI escape sequences in prompt,
 * multi-line prompt in mlmode, modified history api, etc
 *
 */

#include "../../include/linenoise/linenoise.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 4096
#define UNUSED(x) (void)(x)

int strcasecmp(const char *string1, const char *string2);
static char *unsupported_term[] = {"dumb", "cons25", "emacs", NULL};
static linenoiseCompletionCallback *completionCallback = NULL;
static linenoiseHintsCallback *hintsCallback = NULL;
static linenoiseFreeHintsCallback *freeHintsCallback = NULL;
static char *linenoiseNoTTY(void);
static void refreshLineWithCompletion(struct linenoiseState *ls,
                                      linenoiseCompletions *lc, int flags);
static void refreshLineWithFlags(struct linenoiseState *l, int flags);

static struct termios orig_termios; /* In order to restore at exit.*/
static int maskmode = 0; /* Show "***" instead of input. For passwords. */
static int rawmode = 0; /* For atexit() function to check if restore is needed*/
static int mlmode = 0;  /* Multi line mode. Default is single line. */
static int promptnewlines = 0;
static int atexit_registered = 0; /* Register atexit just 1 time. */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
static char **history = NULL;
static bool history_no_dups = false;

/* Enhanced history features */
static int reverse_search_mode = 0;
static char reverse_search_query[256] = {0};
static int reverse_search_index = -1;
static char *reverse_search_original_line = NULL;

enum KEY_ACTION {
    KEY_NULL = 0,   /* NULL */
    CTRL_A = 1,     /* Ctrl+a */
    CTRL_B = 2,     /* Ctrl-b */
    CTRL_C = 3,     /* Ctrl-c */
    CTRL_D = 4,     /* Ctrl-d */
    CTRL_E = 5,     /* Ctrl-e */
    CTRL_F = 6,     /* Ctrl-f */
    CTRL_H = 8,     /* Ctrl-h */
    TAB = 9,        /* Tab */
    CTRL_K = 11,    /* Ctrl+k */
    CTRL_L = 12,    /* Ctrl+l */
    ENTER = 13,     /* Enter */
    CTRL_N = 14,    /* Ctrl-n */
    CTRL_P = 16,    /* Ctrl-p */
    CTRL_R = 18,    /* Ctrl+r */
    CTRL_T = 20,    /* Ctrl-t */
    CTRL_U = 21,    /* Ctrl+u */
    CTRL_W = 23,    /* Ctrl+w */
    ESC = 27,       /* Escape */
    BACKSPACE = 127 /* Backspace */
};

static void linenoiseAtExit(void);
int linenoiseHistoryAdd(const char *line);
#define REFRESH_CLEAN (1 << 0) // Clean the old prompt from the screen
#define REFRESH_WRITE (1 << 1) // Rewrite the prompt on the screen.
#define REFRESH_ALL (REFRESH_CLEAN | REFRESH_WRITE) // Do both.
static void refreshLine(struct linenoiseState *l);

/* Debugging macro. */
#if 0
FILE *lndebug_fp = NULL;
#define lndebug(...)                                                           \
    do {                                                                       \
        if (lndebug_fp == NULL) {                                              \
            lndebug_fp = fopen("/tmp/lndebug.txt", "a");                       \
            fprintf(                                                           \
                lndebug_fp,                                                    \
                "[%d %d %d] p: %d, rows: %d, rpos: %d, max: %d, oldmax: %d\n", \
                (int)l->len, (int)l->pos, (int)l->oldcolpos, plen, rows, rpos, \
                (int)l->oldrows, old_rows);                                    \
        }                                                                      \
        fprintf(lndebug_fp, ", " __VA_ARGS__);                                 \
        fflush(lndebug_fp);                                                    \
    } while (0)
#else
#define lndebug(fmt, ...)
#endif

/* ========================== Encoding functions =============================
 */

/* Get byte length and column length of the previous character */
static size_t defaultPrevCharLen(const char *buf, size_t buf_len, size_t pos,
                                 size_t *col_len) {
    UNUSED(buf);
    UNUSED(buf_len);
    UNUSED(pos);
    if (col_len != NULL) {
        *col_len = 1;
    }
    return 1;
}

/* Get byte length and column length of the next character */
static size_t defaultNextCharLen(const char *buf, size_t buf_len, size_t pos,
                                 size_t *col_len) {
    UNUSED(buf);
    UNUSED(buf_len);
    UNUSED(pos);
    if (col_len != NULL) {
        *col_len = 1;
    }
    return 1;
}

/* Read bytes of the next character */
static size_t defaultReadCode(int fd, char *buf, size_t buf_len, int *c) {
    if (buf_len < 1) {
        return -1;
    }
    int nread = read(fd, &buf[0], 1);
    if (nread == 1) {
        *c = buf[0];
    }
    return nread;
}

/* Set default encoding functions */
static linenoisePrevCharLen *prevCharLen = defaultPrevCharLen;
static linenoiseNextCharLen *nextCharLen = defaultNextCharLen;
static linenoiseReadCode *readCode = defaultReadCode;

/* Set used defined encoding functions */
void linenoiseSetEncodingFunctions(linenoisePrevCharLen *prevCharLenFunc,
                                   linenoiseNextCharLen *nextCharLenFunc,
                                   linenoiseReadCode *readCodeFunc) {
    prevCharLen = prevCharLenFunc;
    nextCharLen = nextCharLenFunc;
    readCode = readCodeFunc;
}

/* Get column length from begining of buffer to current byte position */
static size_t columnPos(const char *buf, size_t buf_len, size_t pos) {
    size_t ret = 0;
    size_t off = 0;
    while (off < pos) {
        size_t col_len;
        size_t len = nextCharLen(buf, buf_len, off, &col_len);
        off += len;
        ret += col_len;
    }
    return ret;
}

/* Get column length from begining of buffer to current byte position for
 * multiline mode*/
static size_t columnPosForMultiLine(const char *buf, size_t buf_len, size_t pos,
                                    size_t cols, size_t ini_pos) {
    size_t ret = 0;
    size_t colwid = ini_pos;

    size_t off = 0;
    while (off < buf_len) {
        size_t col_len;
        size_t len = nextCharLen(buf, buf_len, off, &col_len);

        int dif = (int)(colwid + col_len) - (int)cols;
        if (dif > 0) {
            ret += dif;
            colwid = col_len;
        } else if (dif == 0) {
            colwid = 0;
        } else {
            colwid += col_len;
        }

        if (off >= pos) {
            break;
        }
        off += len;
        ret += col_len;
    }

    return ret;
}

/* ======================= Low level terminal handling ====================== */

/* Enable "mask mode". When it is enabled, instead of the input that
 * the user is typing, the terminal will just display a corresponding
 * number of asterisks, like "****". This is useful for passwords and other
 * secrets that should not be displayed. */
void linenoiseMaskModeEnable(void) { maskmode = 1; }

/* Disable mask mode. */
void linenoiseMaskModeDisable(void) { maskmode = 0; }

/* Set if to use or not the multi line mode. */
void linenoiseSetMultiLine(int ml) { mlmode = ml; }

/* Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences. */
static int isUnsupportedTerm(void) {
    char *term = getenv("TERM");
    int j;

    if (term == NULL) {
        return 0;
    }
    for (j = 0; unsupported_term[j]; j++) {
        if (!strcasecmp(term, unsupported_term[j])) {
            return 1;
        }
    }
    return 0;
}

/* Raw mode: 1960 magic shit. */
static int enableRawMode(int fd) {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) {
        goto fatal;
    }
    if (!atexit_registered) {
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd, &orig_termios) == -1) {
        goto fatal;
    }

    raw = orig_termios; /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) {
        goto fatal;
    }
    rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

static void disableRawMode(int fd) {
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd, TCSAFLUSH, &orig_termios) != -1) {
        rawmode = 0;
    }
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
static int getCursorPosition(int ifd, int ofd) {
    char buf[32];
    int cols, rows;
    unsigned int i = 0;

    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) {
        return -1;
    }

    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf) - 1) {
        if (read(ifd, buf + i, 1) != 1) {
            break;
        }
        if (buf[i] == 'R') {
            break;
        }
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[') {
        return -1;
    }
    if (sscanf(buf + 2, "%d;%d", &rows, &cols) != 2) {
        return -1;
    }
    return cols;
}

/* Try to get the number of columns in the current terminal, or assume 80
 * if it fails. */
static int getColumns(int ifd, int ofd) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int start, cols;

        /* Get the initial position so we can restore it later. */
        start = getCursorPosition(ifd, ofd);
        if (start == -1) {
            goto failed;
        }

        /* Go to right margin and get position. */
        if (write(ofd, "\x1b[999C", 6) != 6) {
            goto failed;
        }
        cols = getCursorPosition(ifd, ofd);
        if (cols == -1) {
            goto failed;
        }

        /* Restore position. */
        if (cols > start) {
            char seq[32];
            snprintf(seq, 32, "\x1b[%dD", cols - start);
            if (write(ofd, seq, strlen(seq)) == -1) {
                /* Can't recover... */
            }
        }
        return cols;
    } else {
        return ws.ws_col;
    }

failed:
    return 80;
}

/* Clear the screen. Used to handle ctrl+l */
void linenoiseClearScreen(void) {
    if (write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7) <= 0) {
        /* nothing to do, just to avoid warning. */
    }
}

/* Beep, used for completion when there is nothing to complete or when all
 * the choices were already shown. */
static void linenoiseBeep(void) {
    fprintf(stderr, "\x7");
    fflush(stderr);
}

/* ============================== Completion ================================ */

/* Free a list of completion option populated by linenoiseAddCompletion(). */
static void freeCompletions(linenoiseCompletions *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++) {
        free(lc->cvec[i]);
    }
    if (lc->cvec != NULL) {
        free(lc->cvec);
    }
}

/* Called by completeLine() and linenoiseShow() to render the current
 * edited line with the proposed completion. If the current completion table
 * is already available, it is passed as second argument, otherwise the
 * function will use the callback to obtain it.
 *
 * Flags are the same as refreshLine*(), that is REFRESH_* macros. */
static void refreshLineWithCompletion(struct linenoiseState *ls,
                                      linenoiseCompletions *lc, int flags) {
    /* Obtain the table of completions if the caller didn't provide one. */
    linenoiseCompletions ctable = {0, NULL};
    if (lc == NULL) {
        completionCallback(ls->buf, &ctable);
        lc = &ctable;
    }

    /* Show the edited line with completion if possible, or just refresh. */
    if (ls->completion_idx < lc->len) {
        struct linenoiseState saved = *ls;
        ls->len = ls->pos = strlen(lc->cvec[ls->completion_idx]);
        ls->buf = lc->cvec[ls->completion_idx];
        refreshLineWithFlags(ls, flags);
        ls->len = saved.len;
        ls->pos = saved.pos;
        ls->buf = saved.buf;
    } else {
        refreshLineWithFlags(ls, flags);
    }

    /* Free the completions table if needed. */
    if (lc != &ctable) {
        freeCompletions(&ctable);
    }
}

/* Enhanced completion mode flag - check shell option */
extern bool get_enhanced_completion(void);

/* Completion categorization */
typedef enum {
    COMPLETION_BUILTIN,
    COMPLETION_FUNCTION,
    COMPLETION_EXTERNAL,
    COMPLETION_FILE,
    COMPLETION_DIRECTORY,
    COMPLETION_VARIABLE,
    COMPLETION_UNKNOWN
} completion_category_t;

/* Categorize completion based on type */
static completion_category_t categorize_completion(const char *completion) {
    if (!completion) {
        return COMPLETION_UNKNOWN;
    }

    /* Check for built-in commands */
    const char *builtins[] = {
        "cd",    "echo",  "exit",  "export", "pwd",   "set",      "unset",
        "test",  "alias", "bg",    "fg",     "jobs",  "kill",     "read",
        "shift", "true",  "false", "return", "break", "continue", "eval",
        "exec",  "hash",  "type",  "ulimit", "umask", "wait",     NULL};

    for (int i = 0; builtins[i]; i++) {
        if (strcmp(completion, builtins[i]) == 0) {
            return COMPLETION_BUILTIN;
        }
    }

    /* Check if it's a file/directory */
    struct stat st;
    if (stat(completion, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return COMPLETION_DIRECTORY;
        } else {
            return COMPLETION_FILE;
        }
    }

    /* Check if it looks like a variable */
    if (completion[0] == '$' ||
        (strlen(completion) > 0 && strchr(completion, '=') != NULL)) {
        return COMPLETION_VARIABLE;
    }

    /* Assume external command if not categorized */
    return COMPLETION_EXTERNAL;
}

/* Get category display string */
static const char *get_category_display(completion_category_t category) {
    switch (category) {
    case COMPLETION_BUILTIN:
        return "builtin";
    case COMPLETION_FUNCTION:
        return "function";
    case COMPLETION_EXTERNAL:
        return "command";
    case COMPLETION_FILE:
        return "file";
    case COMPLETION_DIRECTORY:
        return "directory";
    case COMPLETION_VARIABLE:
        return "variable";
    default:
        return "item";
    }
}

/* Enhanced single-line menu completion display */
static void displayCompletionMenu(linenoiseCompletions *lc,
                                  struct linenoiseState *ls,
                                  size_t current_idx) {
    if (!lc || lc->len == 0 || current_idx >= lc->len) {
        return;
    }

    const char *current = lc->cvec[current_idx];
    completion_category_t category = categorize_completion(current);
    const char *category_str = get_category_display(category);

    /* Clear to end of line and display menu info */
    printf("\033[K \033[2m[%zu/%zu %s]\033[0m", current_idx + 1, lc->len,
           category_str);

    /* Move cursor back to end of completion text */
    char menu_info[64];
    snprintf(menu_info, sizeof(menu_info), " [%zu/%zu %s]", current_idx + 1,
             lc->len, category_str);
    printf("\033[%dD", (int)strlen(menu_info));
}

/* Simple completion display (fallback) */
static void displayCompletionsSimple(linenoiseCompletions *lc,
                                     struct linenoiseState *ls,
                                     size_t current_idx) {
    if (!lc || lc->len == 0) {
        return;
    }

    if (lc->len == 1) {
        /* Single completion - auto-complete */
        return;
    } else if (lc->len <= 8) {
        /* Few completions - show all simply */
        printf("\n");
        for (size_t i = 0; i < lc->len; i++) {
            if (i == current_idx) {
                printf("\033[7m%s\033[0m", lc->cvec[i]);
            } else {
                printf("%s", lc->cvec[i]);
            }
            if (i < lc->len - 1) {
                printf("  ");
            }
        }
        printf("\n");
    } else {
        /* Many completions - just show count */
        printf("\n[%zu/%zu] %s (TAB: next, ESC: cancel)\n", current_idx + 1,
               lc->len, lc->cvec[current_idx]);
    }
}

/* Main completion display function */
static void displayCompletionsPage(linenoiseCompletions *lc,
                                   struct linenoiseState *ls
                                   __attribute__((unused)),
                                   size_t page __attribute__((unused)),
                                   size_t current_idx) {
    if (!lc || lc->len == 0) {
        return;
    }

    (void)page; /* Suppress unused parameter warning */

    if (lc->len == 1) {
        /* Single completion - auto-complete */
        return;
    }

    /* Use enhanced menu if enabled, otherwise simple display */
    if (get_enhanced_completion()) {
        displayCompletionMenu(lc, ls, current_idx);
    } else {
        displayCompletionsSimple(lc, ls, current_idx);
    }
}

/* This is an helper function for linenoiseEdit*() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 *
 * The state of the editing is encapsulated into the pointed linenoiseState
 * structure as described in the structure definition.
 *
 * If the function returns non-zero, the caller should handle the
 * returned value as a byte read from the standard input, and process
 * it as usually: this basically means that the function may return a byte
 * read from the termianl but not processed. Otherwise, if zero is returned,
 * the input was consumed by the completeLine() function to navigate the
 * possible completions, and the caller should read for the next characters
 * from stdin. */
static int completeLine(struct linenoiseState *ls, int keypressed) {
    linenoiseCompletions lc = {0, NULL};
    int nwritten;
    char c = keypressed;

    completionCallback(ls->buf, &lc);
    if (lc.len == 0) {
        linenoiseBeep();
        ls->in_completion = 0;
    } else {
        switch (c) {
        case 9: /* tab */
            if (ls->in_completion == 0) {
                ls->in_completion = 1;
                ls->completion_idx = 0;
                ls->completion_lines = 0;

                /* Enhanced completion behavior */
                if (lc.len == 1) {
                    /* Only one completion - use it immediately */
                    nwritten = snprintf(ls->buf, ls->buflen, "%s", lc.cvec[0]);
                    ls->len = ls->pos = nwritten;
                    ls->in_completion = 0;
                    c = 0;
                } else {
                    /* Show completions */
                    if (get_enhanced_completion()) {
                        /* Enhanced mode: just update the prompt line */
                        refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                        displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                    } else {
                        /* Simple mode: show completion list */
                        displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                        refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                    }
                    c = 0;
                }
            } else {
                /* Navigate through completions */
                ls->completion_idx = (ls->completion_idx + 1) % lc.len;
                if (get_enhanced_completion()) {
                    /* Enhanced mode: update prompt line only */
                    refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                    displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                } else {
                    /* Simple mode: show updated list if many items */
                    if (lc.len > 8) {
                        displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                    }
                    refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                }
                c = 0;
            }
            break;
        case 16: /* Ctrl+P - previous completion */
            if (ls->in_completion) {
                if (ls->completion_idx == 0) {
                    ls->completion_idx = lc.len - 1;
                } else {
                    ls->completion_idx--;
                }
                if (get_enhanced_completion()) {
                    /* Enhanced mode: update prompt line only */
                    refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                    displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                } else {
                    /* Simple mode: show updated list if many items */
                    if (lc.len > 8) {
                        displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                    }
                    refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                }
                c = 0;
            }
            break;
        case 14: /* Ctrl+N - smart jump forward */
            if (ls->in_completion && lc.len > 6) {
                /* Jump forward by 5 items for faster navigation */
                size_t jump_size = (lc.len > 20) ? 5 : 1;
                ls->completion_idx = (ls->completion_idx + jump_size) % lc.len;
                if (get_enhanced_completion()) {
                    refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                    displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                } else {
                    if (lc.len > 8) {
                        displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
                    }
                    refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
                }
                c = 0;
            }
            break;
        case 27: /* escape */
            /* Clear completion display and restore original buffer */
            if (get_enhanced_completion()) {
                /* Enhanced mode: just clear the menu info */
                printf("\033[K");
            } else {
                /* Simple mode: clear any multi-line display */
                printf("\033[2K\r");
            }
            refreshLine(ls);
            ls->in_completion = 0;
            c = 0;
            break;
        default:
            /* Update buffer and return */
            if (ls->completion_idx < lc.len) {
                nwritten = snprintf(ls->buf, ls->buflen, "%s",
                                    lc.cvec[ls->completion_idx]);
                ls->len = ls->pos = nwritten;
            }
            ls->in_completion = 0;
            break;
        }

        /* Show completion or original buffer */
        if (ls->in_completion && ls->completion_idx < lc.len) {
            refreshLineWithCompletion(ls, &lc, REFRESH_ALL);
            if (get_enhanced_completion()) {
                displayCompletionsPage(&lc, ls, 0, ls->completion_idx);
            }
        } else {
            refreshLine(ls);
        }
    }

    freeCompletions(&lc);
    return c; /* Return last read character */
}

/* Register a callback function to be called for tab-completion. */
void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn) {
    completionCallback = fn;
}

/* Register a hits function to be called to show hits to the user at the
 * right of the prompt. */
void linenoiseSetHintsCallback(linenoiseHintsCallback *fn) {
    hintsCallback = fn;
}

/* Register a function to free the hints returned by the hints callback
 * registered with linenoiseSetHintsCallback(). */
void linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *fn) {
    freeHintsCallback = fn;
}

/* This function is used by the callback function registered by the user
 * in order to add completion options given the input string when the
 * user typed <tab>. See the example.c source code for a very easy to
 * understand example. */
void linenoiseAddCompletion(linenoiseCompletions *lc, const char *str) {
    size_t len = strlen(str);
    char *copy, **cvec;

    copy = malloc(len + 1);
    if (copy == NULL) {
        return;
    }
    memcpy(copy, str, len + 1);
    cvec = realloc(lc->cvec, sizeof(char *) * (lc->len + 1));
    if (cvec == NULL) {
        free(copy);
        return;
    }
    lc->cvec = cvec;
    lc->cvec[lc->len++] = copy;
}

/* =========================== Line editing ================================= */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
    char *b;
    int len;
};

static void abInit(struct abuf *ab) {
    ab->b = NULL;
    ab->len = 0;
}

static void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) {
        return;
    }
    memcpy(new + ab->len, s, len);
    ab->b = new;
    ab->len += len;
}

static void abFree(struct abuf *ab) { free(ab->b); }

/* Helper of refreshSingleLine() and refreshMultiLine() to show hints
 * to the right of the prompt. */
void refreshShowHints(struct abuf *ab, struct linenoiseState *l, int pcollen) {
    char seq[64];
    size_t collen = pcollen + columnPos(l->buf, l->len, l->len);
    if (hintsCallback && collen < l->cols) {
        int color = -1, bold = 0;
        char *hint = hintsCallback(l->buf, &color, &bold);
        if (hint) {
            int hintlen = strlen(hint);
            int hintmaxlen = l->cols - collen;
            if (hintlen > hintmaxlen) {
                hintlen = hintmaxlen;
            }
            if (bold == 1 && color == -1) {
                color = 37;
            }
            if (color != -1 || bold != 0) {
                snprintf(seq, 64, "\033[%d;%d;49m", bold, color);
            } else {
                seq[0] = '\0';
            }
            abAppend(ab, seq, strlen(seq));
            abAppend(ab, hint, hintlen);
            if (color != -1 || bold != 0) {
                abAppend(ab, "\033[0m", 4);
            }
            /* Call the function to free the hint returned. */
            if (freeHintsCallback) {
                freeHintsCallback(hint);
            }
        }
    }
}

/* Check if text is an ANSI escape sequence
 */
static int isAnsiEscape(const char *buf, size_t buf_len, size_t *len) {
    if (buf_len > 2 && !memcmp("\033[", buf, 2)) {
        size_t off = 2;
        while (off < buf_len) {
            switch (buf[off++]) {
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'J':
            case 'K':
            case 'S':
            case 'T':
            case 'f':
            case 'm':
                *len = off;
                return 1;
            }
        }
    }
    return 0;
}

/* Get column length of prompt text
 */
static size_t promptTextColumnLen(const char *prompt, size_t plen) {
    char buf[LINENOISE_MAX_LINE];
    size_t buf_len = 0;
    size_t offset = 0;
    size_t colpos = 0;
    size_t ret = 0;
    size_t cols = getColumns(STDIN_FILENO, STDOUT_FILENO);
    size_t plen2 = plen;
    promptnewlines = 0;
    while (plen2 >= cols) {
        promptnewlines++;
        plen2 -= cols;
    }
    while (offset < plen) {
        size_t len;
        if (isAnsiEscape(prompt + offset, plen - offset, &len)) {
            offset += len;
            continue;
        }
        if (prompt[offset] == '\t') {
            offset++;
            colpos += 8 - (colpos % 8);
            continue;
        }
        if (prompt[offset] == '\r') {
            colpos = 0;
            offset++;
            continue;
        }
        if (prompt[offset] == '\n') {
            promptnewlines++;
            offset++;
            continue;
        }
        buf[buf_len++] = prompt[offset++];
        colpos++;
    }
    if (promptnewlines) {
        ret = columnPosForMultiLine(buf, buf_len, colpos, cols, buf_len);
    } else {
        ret = columnPos(buf, buf_len, colpos);
    }
    return ret;
}

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 *
 * Flags is REFRESH_* macros. The function can just remove the old
 * prompt, just write it, or both. */
static void refreshSingleLine(struct linenoiseState *l, int flags) {
    char seq[64];
    size_t pcollen = promptTextColumnLen(l->prompt, strlen(l->prompt));
    int fd = l->ofd;
    char *buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;
    struct abuf ab;

    /* Bottom-line protection: Robust approach that doesn't interfere with cursor positioning */
    if ((flags & REFRESH_WRITE) && isatty(fd)) {
        struct winsize ws;
        if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 2) {
            /* Simple margin creation without cursor interference */
            static int protection_done = 0;
            if (!protection_done) {
                /* Just ensure we have some margin - don't track cursor position */
                write(fd, "\x1b[999;1H\n\x1b[A", 9);
                protection_done = 1;
            }
        }
    }

    while ((pcollen + columnPos(buf, len, pos)) >= l->cols) {
        int chlen = nextCharLen(buf, len, 0, NULL);
        buf += chlen;
        len -= chlen;
        pos -= chlen;
    }
    while (pcollen + columnPos(buf, len, len) > l->cols) {
        len -= prevCharLen(buf, len, len, NULL);
    }

    abInit(&ab);

    if (promptnewlines) {
        lndebug("go down %d", promptnewlines);
        snprintf(seq, 64, "\x1b[%dB", promptnewlines);
        abAppend(&ab, seq, strlen(seq));
        for (int j = 0; j < promptnewlines; j++) {
            lndebug("clear+up");
            snprintf(seq, 64, "\r\x1b[0K\x1b[1A");
            abAppend(&ab, seq, strlen(seq));
        }
        lndebug("clear");
        snprintf(seq, 64, "\r\x1b[0K");
        abAppend(&ab, seq, strlen(seq));
    }

    /* Cursor to left edge */
    snprintf(seq, sizeof(seq), "\r");
    abAppend(&ab, seq, strlen(seq));

    if (flags & REFRESH_WRITE) {
        /* Write the prompt and the current buffer content */
        abAppend(&ab, l->prompt, strlen(l->prompt));
        if (maskmode == 1) {
            while (len--) {
                abAppend(&ab, "*", 1);
            }
        } else {
            abAppend(&ab, buf, len);
        }
        /* Show hits if any. */
        refreshShowHints(&ab, l, pcollen);
    }

    /* Erase to right */
    snprintf(seq, sizeof(seq), "\x1b[0K");
    abAppend(&ab, seq, strlen(seq));

    if (flags & REFRESH_WRITE) {
        /* Move cursor to original position with bounds checking */
        int cursor_pos = (int)(columnPos(buf, len, pos) + pcollen);
        if (cursor_pos >= 0 && cursor_pos < l->cols) {
            snprintf(seq, sizeof(seq), "\r\x1b[%dC", cursor_pos);
            abAppend(&ab, seq, strlen(seq));
        } else {
            /* Fallback: just go to beginning of line */
            snprintf(seq, sizeof(seq), "\r");
            abAppend(&ab, seq, strlen(seq));
        }
    }

    if (write(fd, ab.b, ab.len) == -1) {
    } /* Can't recover from write error. */
    abFree(&ab);
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 *
 * Flags is REFRESH_* macros. The function can just remove the old
 * prompt, just write it, or both. */
static void refreshMultiLine(struct linenoiseState *l, int flags) {
    char seq[64];
    size_t pcollen = promptTextColumnLen(l->prompt, strlen(l->prompt));
    int colpos =
        columnPosForMultiLine(l->buf, l->len, l->len, l->cols, pcollen);
    int colpos2; /* cursor column position. */
    int rows = (pcollen + colpos + l->cols - 1) /
               l->cols; /* rows used by current buf. */
    rows += promptnewlines;
    int rpos =
        (pcollen + l->oldcolpos + l->cols) / l->cols; /* cursor relative row. */
    rpos += promptnewlines;
    int rpos2; /* rpos after refresh. */
    int col;   /* colum position, zero-based. */
    int old_rows;
    if (l->oldrows) {
        old_rows = l->oldrows;
    } else {
        old_rows = rows;
    }
    int fd = l->ofd, j;
    struct abuf ab;

    l->oldrows = rows;

    /* First step: clear all the lines used before. To do so start by
     * going to the last row. */
    abInit(&ab);

    if (flags & REFRESH_CLEAN) {
        if (old_rows - rpos > 0) {
            lndebug("go down %d", old_rows - rpos);
            snprintf(seq, 64, "\x1b[%dB", old_rows - rpos);
            abAppend(&ab, seq, strlen(seq));
        }

        /* Now for every row clear it, go up. */
        for (j = 0; j < old_rows - 1; j++) {
            lndebug("clear+up");
            snprintf(seq, 64, "\r\x1b[0K\x1b[1A");
            abAppend(&ab, seq, strlen(seq));
        }
    }

    if (flags & REFRESH_ALL) {
        /* Clean the top line. */
        lndebug("clear");
        snprintf(seq, 64, "\r\x1b[0K");
        abAppend(&ab, seq, strlen(seq));
    }

    /* Get column length to cursor position */
    colpos2 = columnPosForMultiLine(l->buf, l->len, l->pos, l->cols, pcollen);

    if (flags & REFRESH_WRITE) {
        /* Write the prompt and the current buffer content */
        abAppend(&ab, l->prompt, strlen(l->prompt));
        if (maskmode == 1) {
            unsigned int i;
            for (i = 0; i < l->len; i++) {
                abAppend(&ab, "*", 1);
            }
        } else {
            abAppend(&ab, l->buf, l->len);
        }

        /* Show hits if any. */
        refreshShowHints(&ab, l, pcollen);

        /* If we are at the very end of the screen with our prompt, we need to
         * emit a newline and move the prompt to the first column. */
        if (l->pos && l->pos == l->len && (colpos2 + pcollen) % l->cols == 0) {
            lndebug("<newline>");
            abAppend(&ab, "\n", 1);
            snprintf(seq, 64, "\r");
            abAppend(&ab, seq, strlen(seq));
            rows++;
            if (rows > (int)l->oldrows) {
                l->oldrows = rows;
            }
        }

        /* Move cursor to right position. */
        rpos2 = (pcollen + colpos2 + l->cols) /
                l->cols; /* Current cursor relative row */
        rpos2 += promptnewlines;
        lndebug("rpos2 %d", rpos2);

        /* Go up till we reach the expected positon. */
        if (rows - rpos2 > 0) {
            lndebug("go-up %d", rows - rpos2);
            snprintf(seq, 64, "\x1b[%dA", rows - rpos2);
            abAppend(&ab, seq, strlen(seq));
        }

        /* Set column. */
        col = (pcollen + colpos2) % l->cols;
        lndebug("set col %d", 1 + col);
        if (col) {
            snprintf(seq, 64, "\r\x1b[%dC", col);
        } else {
            snprintf(seq, 64, "\r");
        }
        abAppend(&ab, seq, strlen(seq));
    }

    lndebug("\n");
    l->oldcolpos = colpos2;

    if (write(fd, ab.b, ab.len) == -1) {
    } /* Can't recover from write error. */
    abFree(&ab);
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
static void refreshLineWithFlags(struct linenoiseState *l, int flags) {
    if (mlmode) {
        refreshMultiLine(l, flags);
    } else {
        refreshSingleLine(l, flags);
    }
}

/* Utility function to avoid specifying REFRESH_ALL all the times. */
static void refreshLine(struct linenoiseState *l) {
    refreshLineWithFlags(l, REFRESH_ALL);
}

/* Hide the current line, when using the multiplexing API. */
void linenoiseHide(struct linenoiseState *l) {
    if (mlmode) {
        refreshMultiLine(l, REFRESH_CLEAN);
    } else {
        refreshSingleLine(l, REFRESH_CLEAN);
    }
}

/* Show the current line, when using the multiplexing API. */
void linenoiseShow(struct linenoiseState *l) {
    if (l->in_completion) {
        refreshLineWithCompletion(l, NULL, REFRESH_WRITE);
    } else {
        refreshLineWithFlags(l, REFRESH_WRITE);
    }
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
int linenoiseEditInsert(struct linenoiseState *l, const char *cbuf, int clen) {
    if (l->len + clen <= l->buflen) {
        if (reverse_search_mode) {
            /* Add character to search query */
            size_t query_len = strlen(reverse_search_query);
            if (query_len < sizeof(reverse_search_query) - 1) {
                reverse_search_query[query_len] = cbuf[0];
                reverse_search_query[query_len + 1] = '\0';

                /* Search for match */
                for (int i = history_len - 1; i >= 0; i--) {
                    if (history[i] &&
                        strstr(history[i], reverse_search_query)) {
                        strncpy(l->buf, history[i], LINENOISE_MAX_LINE - 1);
                        l->buf[LINENOISE_MAX_LINE - 1] = '\0';
                        l->len = strlen(l->buf);
                        l->pos = l->len;
                        reverse_search_index = i;

                        char search_prompt[512];
                        snprintf(
                            search_prompt, sizeof(search_prompt),
                            "(reverse-i-search)`%s': ", reverse_search_query);
                        l->prompt = search_prompt;
                        l->plen = strlen(search_prompt);

                        refreshLineWithFlags(l, REFRESH_CLEAN | REFRESH_WRITE);
                        return 0;
                    }
                }

                /* No match found - beep */
                if (write(l->ofd, "\x7", 1) == -1) {
                    /* ignore */
                }
            }
            return 0;
        }

        if (l->len == l->pos) {
            memcpy(&l->buf[l->pos], cbuf, clen);
            l->pos += clen;
            l->len += clen;
            ;
            l->buf[l->len] = '\0';
            if ((!mlmode &&
                 promptTextColumnLen(l->prompt, l->plen) +
                         columnPos(l->buf, l->len, l->len) <
                     l->cols &&
                 !hintsCallback)) {
                /* Avoid a full update of the line in the
                 * trivial case. */
                if (maskmode == 1) {
                    static const char d = '*';
                    if (write(l->ofd, &d, 1) == -1) {
                        return -1;
                    }
                } else {
                    if (write(l->ofd, cbuf, clen) == -1) {
                        return -1;
                    }
                }
            } else {
                refreshLine(l);
            }
        } else {
            memmove(l->buf + l->pos + clen, l->buf + l->pos, l->len - l->pos);
            memcpy(&l->buf[l->pos], cbuf, clen);
            l->pos += clen;
            l->len += clen;
            l->buf[l->len] = '\0';
            refreshLine(l);
        }
    }
    return 0;
}

/* Move cursor on the left. */
void linenoiseEditMoveLeft(struct linenoiseState *l) {
    if (l->pos > 0) {
        l->pos -= prevCharLen(l->buf, l->len, l->pos, NULL);
        refreshLine(l);
    }
}

/* Move cursor on the right. */
void linenoiseEditMoveRight(struct linenoiseState *l) {
    if (l->pos != l->len) {
        l->pos += nextCharLen(l->buf, l->len, l->pos, NULL);
        refreshLine(l);
    }
}

/* Move cursor to the start of the line. */
void linenoiseEditMoveHome(struct linenoiseState *l) {
    if (l->pos != 0) {
        l->pos = 0;
        refreshLine(l);
    }
}

/* Move cursor to the end of the line. */
void linenoiseEditMoveEnd(struct linenoiseState *l) {
    if (l->pos != l->len) {
        l->pos = l->len;
        refreshLine(l);
    }
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
void linenoiseEditHistoryNext(struct linenoiseState *l, int dir) {
    if (history_len > 1) {
        /* Update the current history entry before to
         * overwrite it with the next one. */
        free(history[history_len - 1 - l->history_index]);
        history[history_len - 1 - l->history_index] = strdup(l->buf);
        /* Show the new entry */
        l->history_index += (dir == LINENOISE_HISTORY_PREV) ? 1 : -1;
        if (l->history_index < 0) {
            l->history_index = 0;
            return;
        } else if (l->history_index >= history_len) {
            l->history_index = history_len - 1;
            return;
        }
        strncpy(l->buf, history[history_len - 1 - l->history_index], l->buflen);
        l->buf[l->buflen - 1] = '\0';
        l->len = l->pos = strlen(l->buf);
        
        /* Always use full refresh for maximum stability */
        refreshLine(l);
    }
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
void linenoiseEditDelete(struct linenoiseState *l) {
    if (l->len > 0 && l->pos < l->len) {
        int chlen = nextCharLen(l->buf, l->len, l->pos, NULL);
        memmove(l->buf + l->pos, l->buf + l->pos + chlen,
                l->len - l->pos - chlen);
        l->len -= chlen;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Backspace implementation. */
void linenoiseEditBackspace(struct linenoiseState *l) {
    if (l->pos > 0 && l->len > 0) {
        int chlen = prevCharLen(l->buf, l->len, l->pos, NULL);
        memmove(l->buf + l->pos - chlen, l->buf + l->pos, l->len - l->pos);
        l->pos -= chlen;
        l->len -= chlen;
        l->buf[l->len] = '\0';
        
        /* Always use full refresh for maximum stability */
        refreshLine(l);
    }
}

/* Delete the previosu word, maintaining the cursor at the start of the
 * current word. */
void linenoiseEditDeletePrevWord(struct linenoiseState *l) {
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos - 1] == ' ') {
        l->pos--;
    }
    while (l->pos > 0 && l->buf[l->pos - 1] != ' ') {
        l->pos--;
    }
    diff = old_pos - l->pos;
    memmove(l->buf + l->pos, l->buf + old_pos, l->len - old_pos + 1);
    l->len -= diff;
    refreshLine(l);
}

/* This function is part of the multiplexed API of Linenoise, that is used
 * in order to implement the blocking variant of the API but can also be
 * called by the user directly in an event driven program. It will:
 *
 * 1. Initialize the linenoise state passed by the user.
 * 2. Put the terminal in RAW mode.
 * 3. Show the prompt.
 * 4. Return control to the user, that will have to call linenoiseEditFeed()
 *    each time there is some data arriving in the standard input.
 *
 * The user can also call linenoiseEditHide() and linenoiseEditShow() if it
 * is required to show some input arriving asyncronously, without mixing
 * it with the currently edited line.
 *
 * When linenoiseEditFeed() returns non-NULL, the user finished with the
 * line editing session (pressed enter CTRL-D/C): in this case the caller
 * needs to call linenoiseEditStop() to put back the terminal in normal
 * mode. This will not destroy the buffer, as long as the linenoiseState
 * is still valid in the context of the caller.
 *
 * The function returns 0 on success, or -1 if writing to standard output
 * fails. If stdin_fd or stdout_fd are set to -1, the default is to use
 * STDIN_FILENO and STDOUT_FILENO.
 */
int linenoiseEditStart(struct linenoiseState *l, int stdin_fd, int stdout_fd,
                       char *buf, size_t buflen, const char *prompt) {
    /* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
    l->in_completion = 0;
    l->ifd = stdin_fd != -1 ? stdin_fd : STDIN_FILENO;
    l->ofd = stdout_fd != -1 ? stdout_fd : STDOUT_FILENO;
    l->buf = buf;
    l->buflen = buflen;
    l->prompt = prompt;
    size_t raw_plen = strlen(prompt);
    l->plen = promptTextColumnLen(prompt, raw_plen);
    l->oldcolpos = l->pos = 0;
    l->len = 0;

    /* Enter raw mode. */
    if (enableRawMode(l->ifd) == -1) {
        return -1;
    }

    l->cols = getColumns(stdin_fd, stdout_fd);
    l->oldrows = 0;
    l->history_index = 0;

    /* Buffer starts empty. */
    l->buf[0] = '\0';
    l->buflen--; /* Make sure there is always space for the nulterm */

    /* If stdin is not a tty, stop here with the initialization. We
     * will actually just read a line from standard input in blocking
     * mode later, in linenoiseEditFeed(). */
    if (!isatty(l->ifd)) {
        return 0;
    }

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    linenoiseHistoryAdd("");

    /* Initialize reverse search state */
    reverse_search_mode = 0;
    reverse_search_query[0] = '\0';
    reverse_search_index = -1;
    if (reverse_search_original_line) {
        free(reverse_search_original_line);
        reverse_search_original_line = NULL;
    }

    if (write(l->ofd, prompt, raw_plen) == -1) {
        return -1;
    }
    return 0;
}

/* Enhanced history search functionality */
static int linenoiseReverseSearch(struct linenoiseState *l) {
    if (!reverse_search_mode) {
        /* Enter reverse search mode */
        reverse_search_mode = 1;
        reverse_search_query[0] = '\0';
        reverse_search_index = history_len - 1;

        /* Save original line */
        if (reverse_search_original_line) {
            free(reverse_search_original_line);
        }
        reverse_search_original_line = malloc(l->len + 1);
        if (reverse_search_original_line) {
            memcpy(reverse_search_original_line, l->buf, l->len);
            reverse_search_original_line[l->len] = '\0';
        }

        /* Clear buffer and show search prompt */
        l->buf[0] = '\0';
        l->len = 0;
        l->pos = 0;

        /* Update prompt to show reverse search */
        char search_prompt[512];
        snprintf(search_prompt, sizeof(search_prompt),
                 "(reverse-i-search)`': ");
        l->prompt = search_prompt;
        l->plen = strlen(search_prompt);

        refreshLineWithFlags(l, REFRESH_CLEAN | REFRESH_WRITE);
        return 0;
    }

    /* Already in reverse search - find next match */
    if (reverse_search_index > 0) {
        reverse_search_index--;
        for (int i = reverse_search_index; i >= 0; i--) {
            if (history[i] && strstr(history[i], reverse_search_query)) {
                /* Found match */
                strncpy(l->buf, history[i], LINENOISE_MAX_LINE - 1);
                l->buf[LINENOISE_MAX_LINE - 1] = '\0';
                l->len = strlen(l->buf);
                l->pos = l->len;
                reverse_search_index = i;

                char search_prompt[512];
                snprintf(search_prompt, sizeof(search_prompt),
                         "(reverse-i-search)`%s': ", reverse_search_query);
                l->prompt = search_prompt;
                l->plen = strlen(search_prompt);

                refreshLineWithFlags(l, REFRESH_CLEAN | REFRESH_WRITE);
                return 0;
            }
        }
    }

    /* No more matches found - beep */
    if (write(l->ofd, "\x7", 1) == -1) {
        /* ignore */
    }
    return 0;
}

static void linenoiseExitReverseSearch(struct linenoiseState *l,
                                       int accept_match) {
    if (!reverse_search_mode) {
        return;
    }

    reverse_search_mode = 0;

    if (!accept_match && reverse_search_original_line) {
        /* Restore original line */
        strncpy(l->buf, reverse_search_original_line, LINENOISE_MAX_LINE - 1);
        l->buf[LINENOISE_MAX_LINE - 1] = '\0';
        l->len = strlen(l->buf);
        l->pos = l->len;
    }

    /* Clean up */
    if (reverse_search_original_line) {
        free(reverse_search_original_line);
        reverse_search_original_line = NULL;
    }

    reverse_search_query[0] = '\0';
    reverse_search_index = -1;
}

/* History expansion support */
int linenoiseHistoryExpansion(const char *line, char **expanded) {
    if (!line || !expanded) {
        return 0;
    }

    /* Simple history expansion for !! and !n patterns */
    if (line[0] == '!' && line[1] != '\0') {
        if (line[1] == '!') {
            /* !! - last command */
            if (history_len > 1) {
                *expanded = strdup(history[history_len - 2]);
                return 1;
            }
        } else if (isdigit(line[1])) {
            /* !n - command number n */
            int num = atoi(line + 1);
            if (num > 0 && num <= history_len) {
                *expanded = strdup(history[num - 1]);
                return 1;
            }
        }
    }

    return 0;
}

/* Public interface for reverse search */
void linenoiseHistoryReverseSearch(struct linenoiseState *l) {
    linenoiseReverseSearch(l);
}

char *linenoiseEditMore =
    "If you see this, you are misusing the API: when linenoiseEditFeed() is "
    "called, if it returns linenoiseEditMore the user is yet editing the line. "
    "See the README file for more information.";

/* This function is part of the multiplexed API of linenoise, see the top
 * comment on linenoiseEditStart() for more information. Call this function
 * each time there is some data to read from the standard input file
 * descriptor. In the case of blocking operations, this function can just be
 * called in a loop, and block.
 *
 * The function returns linenoiseEditMore to signal that line editing is still
 * in progress, that is, the user didn't yet pressed enter / CTRL-D. Otherwise
 * the function returns the pointer to the heap-allocated buffer with the
 * edited line, that the user should free with linenoiseFree().
 *
 * On special conditions, NULL is returned and errno is populated:
 *
 * EAGAIN if the user pressed Ctrl-C
 * ENOENT if the user pressed Ctrl-D
 *
 * Some other errno: I/O error.
 */
char *linenoiseEditFeed(struct linenoiseState *l) {
    /* Not a TTY, pass control to line reading without character
     * count limits. */
    if (!isatty(l->ifd)) {
        return linenoiseNoTTY();
    }

    int c;
    int nread;
    char cbuf[32]; // large enough for any encoding?
    char seq[3];

    nread = readCode(l->ifd, cbuf, sizeof(cbuf), &c);
    if (nread <= 0) {
        return NULL;
    }

    /* Only autocomplete when the callback is set. It returns < 0 when
     * there was an error reading from fd. Otherwise it will return the
     * character that should be handled next. */
    if ((l->in_completion || c == 9) && completionCallback != NULL) {
        c = completeLine(l, c);
        /* Return on errors */
        if (c < 0) {
            return NULL;
        }
        /* Read next character when 0 */
        if (c == 0) {
            return linenoiseEditMore;
        }
    }

    switch (c) {
    case ENTER: /* enter */
        if (reverse_search_mode) {
            /* Accept current match and exit reverse search */
            linenoiseExitReverseSearch(l, 1);
            /* Note: prompt will be restored by calling code */
        }

        history_len--;
        free(history[history_len]);
        if (mlmode) {
            linenoiseEditMoveEnd(l);
        }
        if (hintsCallback) {
            /* Force a refresh without hints to leave the previous
             * line as the user typed it after a newline. */
            linenoiseHintsCallback *hc = hintsCallback;
            hintsCallback = NULL;
            refreshLine(l);
            hintsCallback = hc;
        }
        return strdup(l->buf);
    case CTRL_C: /* ctrl-c */
        errno = EAGAIN;
        return NULL;
    case BACKSPACE: /* backspace */
    case 8:         /* ctrl-h */
        linenoiseEditBackspace(l);
        break;
    case CTRL_D: /* ctrl-d, remove char at right of cursor, or if the
                    line is empty, act as end-of-file. */
        if (l->len > 0) {
            linenoiseEditDelete(l);
        } else {
            history_len--;
            free(history[history_len]);
            errno = ENOENT;
            return NULL;
        }
        break;
    case CTRL_T: /* ctrl-t, swaps current character with previous. */
        if (l->pos > 0 && l->pos < l->len) {
            int aux = l->buf[l->pos - 1];
            l->buf[l->pos - 1] = l->buf[l->pos];
            l->buf[l->pos] = aux;
            if (l->pos != l->len - 1) {
                l->pos++;
            }
            refreshLine(l);
        }
        break;
    case CTRL_B: /* ctrl-b */
        linenoiseEditMoveLeft(l);
        break;
    case CTRL_F: /* ctrl-f */
        linenoiseEditMoveRight(l);
        break;
    case CTRL_P: /* ctrl-p */
        linenoiseEditHistoryNext(l, LINENOISE_HISTORY_PREV);
        break;
    case CTRL_N: /* ctrl-n */
        linenoiseEditHistoryNext(l, LINENOISE_HISTORY_NEXT);
        break;
    case CTRL_R: /* reverse search */
        if (reverse_search_mode) {
            /* Continue search for next match */
            linenoiseReverseSearch(l);
        } else {
            /* Start reverse search */
            linenoiseReverseSearch(l);
        }
        break;
    case ESC: /* escape sequence */
        if (reverse_search_mode) {
            /* Exit reverse search on escape */
            linenoiseExitReverseSearch(l, 0);
            /* Note: prompt will be restored by calling code */
            refreshLineWithFlags(l, REFRESH_CLEAN | REFRESH_WRITE);
            break;
        }
        /* Read the next two bytes representing the escape sequence.
         * Use two calls to handle slow terminals returning the two
         * chars at different times. */
        if (read(l->ifd, seq, 1) == -1) {
            break;
        }
        if (read(l->ifd, seq + 1, 1) == -1) {
            break;
        }

        /* ESC [ sequences. */
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                /* Extended escape, read additional byte. */
                if (read(l->ifd, seq + 2, 1) == -1) {
                    break;
                }
                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '3': /* Delete key. */
                        linenoiseEditDelete(l);
                        break;
                    }
                }
            } else {
                switch (seq[1]) {
                case 'A': /* Up */
                    linenoiseEditHistoryNext(l, LINENOISE_HISTORY_PREV);
                    break;
                case 'B': /* Down */
                    linenoiseEditHistoryNext(l, LINENOISE_HISTORY_NEXT);
                    break;
                case 'C': /* Right */
                    linenoiseEditMoveRight(l);
                    break;
                case 'D': /* Left */
                    linenoiseEditMoveLeft(l);
                    break;
                case 'H': /* Home */
                    linenoiseEditMoveHome(l);
                    break;
                case 'F': /* End*/
                    linenoiseEditMoveEnd(l);
                    break;
                }
            }
        }

        /* ESC O sequences. */
        else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H': /* Home */
                linenoiseEditMoveHome(l);
                break;
            case 'F': /* End*/
                linenoiseEditMoveEnd(l);
                break;
            }
        }
        break;
    default:
        if (linenoiseEditInsert(l, cbuf, nread)) {
            return NULL;
        }
        break;
    case CTRL_U: /* Ctrl+u, delete the whole line. */
        l->buf[0] = '\0';
        l->pos = l->len = 0;
        refreshLine(l);
        break;
    case CTRL_K: /* Ctrl+k, delete from current to end of line. */
        l->buf[l->pos] = '\0';
        l->len = l->pos;
        refreshLine(l);
        break;
    case CTRL_A: /* Ctrl+a, go to the start of the line */
        linenoiseEditMoveHome(l);
        break;
    case CTRL_E: /* ctrl+e, go to the end of the line */
        linenoiseEditMoveEnd(l);
        break;
    case CTRL_L: /* ctrl+l, clear screen */
        linenoiseClearScreen();
        refreshLine(l);
        break;
    case CTRL_W: /* ctrl+w, delete previous word */
        linenoiseEditDeletePrevWord(l);
        break;
    }
    return linenoiseEditMore;
}

/* This is part of the multiplexed linenoise API. See linenoiseEditStart()
 * for more information. This function is called when linenoiseEditFeed()
 * returns something different than NULL. At this point the user input
 * is in the buffer, and we can restore the terminal in normal mode. */
void linenoiseEditStop(struct linenoiseState *l) {
    if (!isatty(l->ifd)) {
        return;
    }
    disableRawMode(l->ifd);
    printf("\n");
}

/* This just implements a blocking loop for the multiplexed API.
 * In many applications that are not event-drivern, we can just call
 * the blocking linenoise API, wait for the user to complete the editing
 * and return the buffer. */
static char *linenoiseBlockingEdit(int stdin_fd, int stdout_fd, char *buf,
                                   size_t buflen, const char *prompt) {
    struct linenoiseState l;

    /* Editing without a buffer is invalid. */
    if (buflen == 0) {
        errno = EINVAL;
        return NULL;
    }

    linenoiseEditStart(&l, stdin_fd, stdout_fd, buf, buflen, prompt);
    char *res;
    while ((res = linenoiseEditFeed(&l)) == linenoiseEditMore)
        ;
    linenoiseEditStop(&l);
    return res;
}

/* This special mode is used by linenoise in order to print scan codes
 * on screen for debugging / development purposes. It is implemented
 * by the linenoise_example program using the --keycodes option. */
void linenoisePrintKeyCodes(void) {
    char quit[4];

    printf("Linenoise key codes debugging mode.\n"
           "Press keys to see scan codes. Type 'quit' at any time to exit.\n");
    if (enableRawMode(STDIN_FILENO) == -1) {
        return;
    }
    memset(quit, ' ', 4);
    while (1) {
        char c;
        int nread;

        nread = read(STDIN_FILENO, &c, 1);
        if (nread <= 0) {
            continue;
        }
        memmove(quit, quit + 1, sizeof(quit) - 1); /* shift string to left. */
        quit[sizeof(quit) - 1] = c; /* Insert current char on the right. */
        if (memcmp(quit, "quit", sizeof(quit)) == 0) {
            break;
        }

        printf("'%c' %02x (%d) (type quit to exit)\n",
               isprint((int)c) ? c : '?', (int)c, (int)c);
        printf("\r"); /* Go left edge manually, we are in raw mode. */
        fflush(stdout);
    }
    disableRawMode(STDIN_FILENO);
}

/* This function is called when linenoise() is called with the standard
 * input file descriptor not attached to a TTY. So for example when the
 * program using linenoise is called in pipe or with a file redirected
 * to its standard input. In this case, we want to be able to return the
 * line regardless of its length (by default we are limited to 4k). */
static char *linenoiseNoTTY(void) {
    char *line = NULL;
    size_t len = 0, maxlen = 0;

    while (1) {
        if (len == maxlen) {
            if (maxlen == 0) {
                maxlen = 16;
            }
            maxlen *= 2;
            char *oldval = line;
            line = realloc(line, maxlen);
            if (line == NULL) {
                if (oldval) {
                    free(oldval);
                }
                return NULL;
            }
        }
        int c = fgetc(stdin);
        if (c == EOF || c == '\n') {
            if (c == EOF && len == 0) {
                free(line);
                return NULL;
            } else {
                line[len] = '\0';
                return line;
            }
        } else {
            line[len] = c;
            len++;
        }
    }
}

/* The high level function that is the main API of the linenoise library.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of stupid terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions. */
char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];

    if (!isatty(STDIN_FILENO)) {
        /* Not a tty: read from file / pipe. In this mode we don't want any
         * limit to the line size, so we call a function to handle that. */
        return linenoiseNoTTY();
    } else if (isUnsupportedTerm()) {
        size_t len;

        printf("%s", prompt);
        fflush(stdout);
        if (fgets(buf, LINENOISE_MAX_LINE, stdin) == NULL) {
            return NULL;
        }
        len = strlen(buf);
        while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            len--;
            buf[len] = '\0';
        }
        return strdup(buf);
    } else {
        char *retval = linenoiseBlockingEdit(STDIN_FILENO, STDOUT_FILENO, buf,
                                             LINENOISE_MAX_LINE, prompt);
        return retval;
    }
}

/* This is just a wrapper the user may want to call in order to make sure
 * the linenoise returned buffer is freed with the same allocator it was
 * created with. Useful when the main program is using an alternative
 * allocator. */
void linenoiseFree(void *ptr) {
    if (ptr == linenoiseEditMore) {
        return; // Protect from API misuse.
    }
    free(ptr);
}

/* ================================ History ================================= */

/* Free the history, but does not reset it. Only used when we have to
 * exit() to avoid memory leaks are reported by valgrind & co. */
static void freeHistory(void) {
    if (history) {
        int j;

        for (j = 0; j < history_len; j++) {
            free(history[j]);
        }
        free(history);
    }
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void linenoiseAtExit(void) {
    disableRawMode(STDIN_FILENO);
    freeHistory();
}

/* Set the maximum length for the history. This function can be called even
 * if there is already some history, the function will make sure to retain
 * just the latest 'len' elements if the new history length value is smaller
 * than the amount of items already inside the history. */
int linenoiseHistorySetMaxLen(int len) {
    char **new;

    if (len < 1) {
        return 0;
    }
    if (history) {
        int tocopy = history_len;

        new = malloc(sizeof(char *) * len);
        if (new == NULL) {
            return 0;
        }

        /* If we can't copy everything, free the elements we'll not use. */
        if (len < tocopy) {
            int j;

            for (j = 0; j < tocopy - len; j++) {
                free(history[j]);
            }
            tocopy = len;
        }
        memset(new, 0, sizeof(char *) * len);
        memcpy(new, history + (history_len - tocopy), sizeof(char *) * tocopy);
        free(history);
        history = new;
    }
    history_max_len = len;
    if (history_len > history_max_len) {
        history_len = history_max_len;
    }
    return 1;
}

/* Save the history in the specified file. On success 0 is returned
 * otherwise -1 is returned. */
int linenoiseHistorySave(const char *filename) {
    mode_t old_umask = umask(S_IXUSR | S_IRWXG | S_IRWXO);
    FILE *fp;
    int j;

    fp = fopen(filename, "w");
    umask(old_umask);
    if (fp == NULL) {
        return -1;
    }
    chmod(filename, S_IRUSR | S_IWUSR);
    for (j = 0; j < history_len; j++) {
        fprintf(fp, "%s\n", history[j]);
    }
    fclose(fp);
    return 0;
}

/* Load the history from the specified file. If the file does not exist
 * zero is returned and no operation is performed.
 *
 * If the file exists and the operation succeeded 0 is returned, otherwise
 * on error -1 is returned. */
int linenoiseHistoryLoad(const char *filename) {
    FILE *fp = fopen(filename, "r");
    char buf[LINENOISE_MAX_LINE];

    if (fp == NULL) {
        return -1;
    }

    while (fgets(buf, LINENOISE_MAX_LINE, fp) != NULL) {
        char *p;

        p = strchr(buf, '\r');
        if (!p) {
            p = strchr(buf, '\n');
        }
        if (p) {
            *p = '\0';
        }
        linenoiseHistoryAdd(buf);
    }
    fclose(fp);
    return 0;
}

/* This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle. */
int linenoiseHistoryAdd(const char *line) {
    char *linecopy;

    if (history_max_len == 0) {
        return 0;
    }

    /* Initialization on first call. */
    if (history == NULL) {
        history = malloc(sizeof(char *) * history_max_len);
        if (history == NULL) {
            return 0;
        }
        memset(history, 0, (sizeof(char *) * history_max_len));
    }

    /* Don't add duplicated entry twice in a row. */
    if (history_len && strcmp(history[history_len - 1], line) == 0) {
        return 0;
    }

    // If no history duplicates option is set
    // Search for a duplicate. Remove from history.
    // Note that we are cutting the new array short by one.
    if (history_no_dups) {
        int len = history_len - 2;
        for (int i = 1; i < len; i++) {
            if (history[i] == NULL) {
                break;
            }
            if (strcmp(history[i], line) == 0) {
                linenoiseHistoryDelete(i);
            }
        }
    }

    /* Add an heap allocated copy of the line in the history.
     * If we reached the max length, remove the older line. */
    linecopy = strdup(line);
    if (!linecopy) {
        return 0;
    }
    if (history_len == history_max_len) {
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (history_max_len - 1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

/* Enhanced history functions */
void linenoiseSetHistoryNoDups(int enable) {
    history_no_dups = enable ? true : false;
}

void linenoiseHistoryPrint(void) {
    for (int i = 0; i < history_len; i++) {
        if (history[i]) {
            printf("%-5d\t%s\n", i + 1, history[i]);
        }
    }
}

char *linenoiseHistoryGet(int index) {
    if (index < 0 || index >= history_max_len) {
        fprintf(stderr, "error: `linenoiseHistoryGet`: index %d out of range\n",
                index);
        return NULL;
    }
    char *line = NULL;
    if (history) {
        if (history[index]) {
            line = strdup(history[index]);
            if (line == NULL) {
                perror("error: `linenoiseHistoryGet`");
                return NULL;
            }
            line[strlen(history[index])] = '\0';
        }
    } else {
        fprintf(stderr,
                "error: `linenoiseHistoryGet`: history not initialized\n");
    }
    return line;
}

int linenoiseHistoryDelete(int index) {
    if (index < 0 || index >= history_max_len) {
        fprintf(stderr, "error: `linenoiseHistoryGet`: index %d out of range\n",
                index);
        return -1;
    }

    if (history[index] == NULL) {
        return 0;
    }

    free(history[index]);

    for (int i = index; i < history_max_len; i++) {
        if ((i + 1) == history_max_len) {
            history[i] = NULL;
        } else {
            history[i] = history[i + 1];
        }
    }

    history_len--;

    return 0;
}

void linenoiseHistoryNoDups(bool flag) {
    history_no_dups = flag;
    if (history_no_dups) {
        linenoiseHistoryRemoveDups();
    }
}

int linenoiseHistoryRemoveDups() {
    int len = history_len - 2;
    for (int i = len; i > 0; i--) {
        for (int j = i - 1; j > 0; j--) {
            if (history[i] == NULL || history[j] == NULL) {
                continue;
            }
            if (strcmp(history[i], history[j]) == 0) {
                linenoiseHistoryDelete(j);
            }
        }
    }

    return 0;
}
