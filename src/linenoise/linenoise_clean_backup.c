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
 * Copyright (c) 2024, Lusush Shell Project - Enhanced for multiline prompts,
 *                     UTF-8 support, ANSI handling, and advanced features
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
 */

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../include/linenoise/linenoise.h"
#include "../../include/linenoise/encodings/utf8.h"
#include "../../include/termcap.h"

/* Function declarations */
int strcasecmp(const char *s1, const char *s2);

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 4096

static char *unsupported_term[] = {"dumb","cons25","emacs",NULL};
static linenoiseCompletionCallback *completionCallback = NULL;
static linenoiseHintsCallback *hintsCallback = NULL;
static linenoiseFreeHintsCallback *freeHintsCallback = NULL;

static struct termios orig_termios; /* In order to restore at exit.*/
static int maskmode = 0; /* Show "***" instead of input. For passwords. */
static int rawmode = 0; /* For atexit() function to check if restore is needed*/
static int mlmode = 0;  /* Multi line mode. Default is single line. */ 
static int atexit_registered = 0; /* Register atexit just 1 time. */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
static char **history = NULL;
static bool history_no_dups = false;

/* Forward declarations */
static char *linenoiseNoTTY(void);
static void refreshLineWithCompletion(struct linenoiseState *ls, linenoiseCompletions *lc, int flags);
static void refreshLineWithFlags(struct linenoiseState *l, int flags);

enum KEY_ACTION{
    KEY_NULL = 0,       /* NULL */
    CTRL_A = 1,         /* Ctrl+a */
    CTRL_B = 2,         /* Ctrl-b */
    CTRL_C = 3,         /* Ctrl-c */
    CTRL_D = 4,         /* Ctrl-d */
    CTRL_E = 5,         /* Ctrl-e */
    CTRL_F = 6,         /* Ctrl-f */
    CTRL_H = 8,         /* Ctrl-h */
    TAB = 9,            /* Tab */
    CTRL_K = 11,        /* Ctrl+k */
    CTRL_L = 12,        /* Ctrl+l */
    ENTER = 13,         /* Enter */
    CTRL_N = 14,        /* Ctrl-n */
    CTRL_P = 16,        /* Ctrl-p */
    CTRL_R = 18,        /* Ctrl+r */
    CTRL_T = 20,        /* Ctrl-t */
    CTRL_U = 21,        /* Ctrl+u */
    CTRL_W = 23,        /* Ctrl+w */
    ESC = 27,           /* Escape */
    BACKSPACE =  127    /* Backspace */
};

static void linenoiseAtExit(void);
int linenoiseHistoryAdd(const char *line);

#define REFRESH_CLEAN (1<<0)    // Clean the old prompt from the screen
#define REFRESH_WRITE (1<<1)    // Rewrite the prompt on the screen.
#define REFRESH_ALL (REFRESH_CLEAN|REFRESH_WRITE) // Do both.

/* ======================= Encoding Functions ====================== */

/* Default encoding functions for ASCII */
static size_t defaultPrevCharLen(const char *buf, size_t buf_len, size_t pos, size_t *col_len) {
    (void)buf;
    (void)buf_len;
    if (col_len != NULL) {
        *col_len = 1;
    }
    if (pos == 0) return 0;
    return 1;
}

static size_t defaultNextCharLen(const char *buf, size_t buf_len, size_t pos, size_t *col_len) {
    (void)buf;
    (void)buf_len;
    (void)pos;
    if (col_len != NULL) {
        *col_len = 1;
    }
    return 1;
}

static size_t defaultReadCode(int fd, char *buf, size_t buf_len, int *c) {
    (void)buf_len;
    if (read(fd, buf, 1) == 1) {
        if (c != NULL) {
            *c = (unsigned char)buf[0];
        }
        return 1;
    }
    return 0;
}

/* Set default encoding functions */
static linenoisePrevCharLen *prevCharLen = defaultPrevCharLen;
static linenoiseNextCharLen *nextCharLen = defaultNextCharLen;
static linenoiseReadCode *readCode = defaultReadCode;

void linenoiseSetEncodingFunctions(linenoisePrevCharLen *prevCharLenFunc,
                                   linenoiseNextCharLen *nextCharLenFunc,
                                   linenoiseReadCode *readCodeFunc) {
    prevCharLen = prevCharLenFunc;
    nextCharLen = nextCharLenFunc;
    readCode = readCodeFunc;
}

/* ======================= ANSI Escape Sequence Handling ====================== */

/* Check if buffer starts with ANSI escape sequence and return its length */
static size_t getAnsiEscapeLen(const char *buf, size_t buf_len) {
    if (buf_len < 2) return 0;
    
    /* ESC[ sequences (CSI - Control Sequence Introducer) */
    if (buf[0] == '\033' && buf[1] == '[') {
        size_t i = 2;
        /* Skip parameter bytes (0x30-0x3F) */
        while (i < buf_len && buf[i] >= 0x30 && buf[i] <= 0x3F) i++;
        /* Skip intermediate bytes (0x20-0x2F) */
        while (i < buf_len && buf[i] >= 0x20 && buf[i] <= 0x2F) i++;
        /* Final byte (0x40-0x7E) */
        if (i < buf_len && buf[i] >= 0x40 && buf[i] <= 0x7E) {
            return i + 1;
        }
    }
    
    /* ESC O sequences */
    if (buf[0] == '\033' && buf[1] == 'O' && buf_len >= 3) {
        return 3;
    }
    
    /* Simple ESC sequences */
    if (buf[0] == '\033' && buf_len >= 2) {
        return 2;
    }
    
    return 0;
}

/* ======================= Display Width Calculation ====================== */

/* Calculate display width of a string, handling ANSI escapes and UTF-8 */
static size_t getDisplayWidth(const char *s, size_t len) {
    size_t width = 0;
    size_t pos = 0;
    
    while (pos < len) {
        /* Skip ANSI escape sequences */
        size_t escape_len = getAnsiEscapeLen(s + pos, len - pos);
        if (escape_len > 0) {
            pos += escape_len;
            continue;
        }
        
        /* Handle tab expansion */
        if (s[pos] == '\t') {
            width += 8 - (width % 8);
            pos++;
            continue;
        }
        
        /* Handle newlines */
        if (s[pos] == '\n' || s[pos] == '\r') {
            pos++;
            continue;
        }
        
        /* Handle UTF-8 characters */
        size_t col_len;
        size_t char_len = nextCharLen(s, len, pos, &col_len);
        if (char_len == 0) break;
        
        width += col_len;
        pos += char_len;
    }
    
    return width;
}

/* Calculate the display width of the last line in a multiline string */
static size_t getLastLineWidth(const char *s, size_t len) {
    size_t pos = 0;
    size_t last_newline_pos = 0;
    bool found_newline = false;
    
    /* Find the last newline */
    while (pos < len) {
        size_t escape_len = getAnsiEscapeLen(s + pos, len - pos);
        if (escape_len > 0) {
            pos += escape_len;
            continue;
        }
        
        if (s[pos] == '\n' || s[pos] == '\r') {
            last_newline_pos = pos + 1;
            found_newline = true;
        }
        pos++;
    }
    
    if (!found_newline) {
        return getDisplayWidth(s, len);
    }
    
    return getDisplayWidth(s + last_newline_pos, len - last_newline_pos);
}

/* Count number of newlines in string (for multiline prompt support) */
static int countNewlines(const char *s, size_t len) {
    int count = 0;
    size_t pos = 0;
    
    while (pos < len) {
        /* Skip ANSI escape sequences */
        size_t escape_len = getAnsiEscapeLen(s + pos, len - pos);
        if (escape_len > 0) {
            pos += escape_len;
            continue;
        }
        
        if (s[pos] == '\n') {
            count++;
            /* Skip \r if it follows \n */
            if (pos + 1 < len && s[pos + 1] == '\r') {
                pos++;
            }
        } else if (s[pos] == '\r') {
            count++;
            /* Skip \n if it follows \r */
            if (pos + 1 < len && s[pos + 1] == '\n') {
                pos++;
            }
        }
        pos++;
    }
    
    return count;
}

/* ======================= Low level terminal handling ====================== */

/* Enable "mask mode". When it is enabled, instead of the input that
 * the user is typing, the terminal will just display a corresponding
 * number of asterisks, like "****". This is useful for passwords and other
 * secrets that should not be displayed. */
void linenoiseMaskModeEnable(void) {
    maskmode = 1;
}

/* Disable mask mode. */
void linenoiseMaskModeDisable(void) {
    maskmode = 0;
}

/* Set if to use or not the multi line mode. */
void linenoiseSetMultiLine(int ml) {
    mlmode = ml;
}

/* Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences. */
static int isUnsupportedTerm(void) {
    char *term = getenv("TERM");
    int j;
    
    if (term == NULL) return 0;
    for (j = 0; unsupported_term[j]; j++)
        if (strcasecmp(term,unsupported_term[j]) == 0) return 1;
    return 0;
}

/* Raw mode: 1960 magic stuff. */
static int enableRawMode(int fd) {
    struct termios raw;
    
    if (!isatty(STDIN_FILENO)) goto fatal;
    if (!atexit_registered) {
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;
    
    raw = orig_termios;  /* modify the original mode */
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
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
    
    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

static void disableRawMode(int fd) {
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd,TCSAFLUSH,&orig_termios) != -1)
        rawmode = 0;
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
static int getCursorPosition(int ifd, int ofd) {
    char buf[32];
    int cols, rows;
    unsigned int i = 0;
    
    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;
    
    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    
    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",&rows,&cols) != 2) return -1;
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
        start = getCursorPosition(ifd,ofd);
        if (start == -1) goto failed;
        
        /* Go to right margin and get position. */
        if (write(ofd,"\x1b[999C",6) != 6) goto failed;
        cols = getCursorPosition(ifd,ofd);
        if (cols == -1) goto failed;
        
        /* Restore position. */
        if (cols > start) {
            char seq[32];
            snprintf(seq,32,"\x1b[%dD",cols-start);
            if (write(ofd,seq,strlen(seq)) == -1) {
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
    if (write(STDOUT_FILENO,"\x1b[H\x1b[2J",7) <= 0) {
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
    for (i = 0; i < lc->len; i++)
        free(lc->cvec[i]);
    if (lc->cvec != NULL)
        free(lc->cvec);
}

/* This is an helper function for linenoiseEdit*() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input. */
static int completeLine(struct linenoiseState *ls, int keypressed) {
    linenoiseCompletions lc = { 0, NULL };
    int nwritten;
    char c = keypressed;

    completionCallback(ls->buf,&lc);
    if (lc.len == 0) {
        linenoiseBeep();
        ls->in_completion = 0;
    } else {
        switch(c) {
            case 9: /* tab */
                if (ls->in_completion == 0) {
                    ls->in_completion = 1;
                    ls->completion_idx = 0;
                } else {
                    ls->completion_idx = (ls->completion_idx+1) % (lc.len+1);
                    if (ls->completion_idx == lc.len) linenoiseBeep();
                }
                c = 0;
                break;
            case 27: /* escape */
                /* Re-show original buffer */
                if (ls->completion_idx < lc.len) refreshLineWithFlags(ls, REFRESH_ALL);
                ls->in_completion = 0;
                c = 0;
                break;
            default:
                /* Update buffer and return */
                if (ls->completion_idx < lc.len) {
                    nwritten = snprintf(ls->buf,ls->buflen,"%s",
                        lc.cvec[ls->completion_idx]);
                    ls->len = ls->pos = nwritten;
                }
                ls->in_completion = 0;
                break;
        }

        /* Show completion or original buffer */
        if (ls->in_completion && ls->completion_idx < lc.len) {
            refreshLineWithCompletion(ls,&lc,REFRESH_ALL);
        } else {
            refreshLineWithFlags(ls, REFRESH_ALL);
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

    copy = malloc(len+1);
    if (copy == NULL) return;
    memcpy(copy,str,len+1);
    cvec = realloc(lc->cvec,sizeof(char*)*(lc->len+1));
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
    char *new = realloc(ab->b,ab->len+len);

    if (new == NULL) return;
    memcpy(new+ab->len,s,len);
    ab->b = new;
    ab->len += len;
}

static void abFree(struct abuf *ab) {
    free(ab->b);
}

/* Helper of refreshSingleLine() and refreshMultiLine() to show hints
 * to the right of the prompt. */
void refreshShowHints(struct abuf *ab, struct linenoiseState *l, int plen) {
    char seq[64];
    if (hintsCallback && plen+l->len < l->cols) {
        int color = -1, bold = 0;
        char *hint = hintsCallback(l->buf,&color,&bold);
        if (hint) {
            int hintlen = strlen(hint);
            int hintmaxlen = l->cols-(plen+l->len);
            if (hintlen > hintmaxlen) hintlen = hintmaxlen;
            if (bold == 1 && color == -1) color = 37;
            if (color != -1 || bold != 0)
                snprintf(seq,64,"\033[%d;%d;49m",bold,color);
            else
                seq[0] = '\0';
            abAppend(ab,seq,strlen(seq));
            abAppend(ab,hint,hintlen);
            if (color != -1 || bold != 0)
                abAppend(ab,"\033[0m",4);
            /* Call the function to free the hint returned. */
            if (freeHintsCallback) freeHintsCallback(hint);
        }
    }
}

/* Single line low level line refresh. */
static void refreshSingleLine(struct linenoiseState *l, int flags) {
    char seq[64];
    size_t plen = getDisplayWidth(l->prompt, strlen(l->prompt));
    int fd = l->ofd;
    char *buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;
    struct abuf ab;

    while((plen+getDisplayWidth(buf, len)) >= l->cols) {
        size_t char_len = nextCharLen(buf, len, 0, NULL);
        if (char_len == 0) break;
        buf += char_len;
        len -= char_len;
        if (pos >= char_len) pos -= char_len;
        else pos = 0;
    }
    while (plen+getDisplayWidth(buf, len) > l->cols) {
        size_t last_char_len = prevCharLen(buf, len, len, NULL);
        if (last_char_len == 0) break;
        len -= last_char_len;
    }

    abInit(&ab);
    /* Cursor to left edge */
    snprintf(seq,sizeof(seq),"\r");
    abAppend(&ab,seq,strlen(seq));

    if (flags & REFRESH_WRITE) {
        /* Write the prompt and the current buffer content */
        abAppend(&ab,l->prompt,strlen(l->prompt));
        if (maskmode == 1) {
            while (len--) abAppend(&ab,"*",1);
        } else {
            abAppend(&ab,buf,len);
        }
        /* Show hints if any. */
        refreshShowHints(&ab,l,plen);
    }

    /* Erase to right */
    snprintf(seq,sizeof(seq),"\x1b[0K");
    abAppend(&ab,seq,strlen(seq));

    if (flags & REFRESH_WRITE) {
        /* Move cursor to original position. */
        size_t cursor_pos = plen + getDisplayWidth(buf, pos);
        snprintf(seq,sizeof(seq),"\r\x1b[%dC", (int)cursor_pos);
        abAppend(&ab,seq,strlen(seq));
    }

    if (write(fd,ab.b,ab.len) == -1) {} /* Can't recover from write error. */
    abFree(&ab);
}

/* Multi line low level line refresh. This is the core multiline implementation. */
static void refreshMultiLine(struct linenoiseState *l, int flags) {
    char seq[64];
    size_t prompt_lines = countNewlines(l->prompt, strlen(l->prompt)) + 1;
    size_t prompt_last_line_width = getLastLineWidth(l->prompt, strlen(l->prompt));
    
    /* Calculate total display width including buffer */
    size_t total_width = prompt_last_line_width + getDisplayWidth(l->buf, l->len);
    size_t cursor_width = prompt_last_line_width + getDisplayWidth(l->buf, l->pos);
    
    /* Calculate rows needed */
    int rows = (int)((total_width + l->cols - 1) / l->cols);
    if (rows == 0) rows = 1;
    rows += (int)prompt_lines - 1;
    
    /* Calculate cursor position */
    int cursor_row = (int)((cursor_width + l->cols - 1) / l->cols);
    cursor_row += (int)prompt_lines - 1;
    int cursor_col = (int)(cursor_width % l->cols);
    
    int fd = l->ofd;
    struct abuf ab;

    /* Update old rows for next refresh */
    int old_rows = l->oldrows;
    l->oldrows = rows;

    abInit(&ab);

    /* Clean up old content if requested */
    if (flags & REFRESH_CLEAN) {
        if (old_rows > 0) {
            /* Clear old multiline content by going up and clearing each line */
            for (int i = 0; i < old_rows; i++) {
                if (i > 0) {
                    abAppend(&ab, "\x1b[A", 3);  /* Move up one line */
                }
                abAppend(&ab, "\r\x1b[2K", 5);   /* Clear entire line */
            }
            /* Return to start */
            abAppend(&ab, "\r", 1);
        } else {
            /* No old content or unknown - clear current line */
            abAppend(&ab, "\r\x1b[2K", 5);
        }
    }

    if (flags & REFRESH_WRITE) {
        /* Write the prompt */
        abAppend(&ab, l->prompt, strlen(l->prompt));
        
        /* Write the buffer content */
        if (maskmode == 1) {
            for (size_t i = 0; i < l->len; i++) {
                abAppend(&ab, "*", 1);
            }
        } else {
            abAppend(&ab, l->buf, l->len);
        }

        /* Show hints if any */
        refreshShowHints(&ab, l, prompt_last_line_width);

        /* Position cursor correctly */
        /* First, figure out where we are now (end of content) */
        int current_row = rows;
        
        /* Move to target cursor position */
        if (current_row != cursor_row) {
            if (cursor_row < current_row) {
                snprintf(seq, sizeof(seq), "\x1b[%dA", current_row - cursor_row);
            } else {
                snprintf(seq, sizeof(seq), "\x1b[%dB", cursor_row - current_row);
            }
            abAppend(&ab, seq, strlen(seq));
        }
        
        /* Set column position */
        snprintf(seq, sizeof(seq), "\r\x1b[%dC", cursor_col);
        abAppend(&ab, seq, strlen(seq));
    }

    /* Update position tracking */
    l->oldcolpos = l->pos;

    if (write(fd,ab.b,ab.len) == -1) {} /* Can't recover from write error. */
    abFree(&ab);
}

/* Called by refreshLineWithCompletion() and linenoiseShow() to render the current
 * edited line with the proposed completion. */
static void refreshLineWithCompletion(struct linenoiseState *ls, linenoiseCompletions *lc, int flags) {
    /* Obtain the table of completions if the caller didn't provide one. */
    linenoiseCompletions ctable = { 0, NULL };
    if (lc == NULL) {
        completionCallback(ls->buf,&ctable);
        lc = &ctable;
    }

    /* Show the edited line with completion if possible, or just refresh. */
    if (ls->completion_idx < lc->len) {
        struct linenoiseState saved = *ls;
        ls->len = ls->pos = strlen(lc->cvec[ls->completion_idx]);
        ls->buf = lc->cvec[ls->completion_idx];
        refreshLineWithFlags(ls,flags);
        ls->len = saved.len;
        ls->pos = saved.pos;
        ls->buf = saved.buf;
    } else {
        refreshLineWithFlags(ls,flags);
    }

    /* Free the completions table if needed. */
    if (lc != &ctable) freeCompletions(&ctable);
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
static void refreshLineWithFlags(struct linenoiseState *l, int flags) {
    if (mlmode)
        refreshMultiLine(l,flags);
    else
        refreshSingleLine(l,flags);
}

/* Utility function to avoid specifying REFRESH_ALL all the times. */
static void refreshLine(struct linenoiseState *l) {
    refreshLineWithFlags(l,REFRESH_ALL);
}

/* Hide the current line, when using the multiplexing API. */
void linenoiseHide(struct linenoiseState *l) {
    if (mlmode)
        refreshMultiLine(l,REFRESH_CLEAN);
    else
        refreshSingleLine(l,REFRESH_CLEAN);
}

/* Show the current line, when using the multiplexing API. */
void linenoiseShow(struct linenoiseState *l) {
    if (l->in_completion) {
        refreshLineWithCompletion(l,NULL,REFRESH_WRITE);
    } else {
        refreshLineWithFlags(l,REFRESH_WRITE);
    }
}

/* Insert the character 'c' at cursor current position. */
int linenoiseEditInsert(struct linenoiseState *l, char c) {
    if (l->len < l->buflen) {
        if (l->len == l->pos) {
            l->buf[l->pos] = c;
            l->pos++;
            l->len++;
            l->buf[l->len] = '\0';
            if ((!mlmode && getDisplayWidth(l->prompt, strlen(l->prompt))+l->len < l->cols && !hintsCallback)) {
                /* Avoid a full update of the line in the
                 * trivial case. */
                char d = (maskmode==1) ? '*' : c;
                if (write(l->ofd,&d,1) == -1) return -1;
            } else {
                refreshLine(l);
            }
        } else {
            memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
            l->buf[l->pos] = c;
            l->len++;
            l->pos++;
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
 * entry as specified by 'dir'. CLEAN IMPLEMENTATION */
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
            l->history_index = history_len-1;
            return;
        }
        strncpy(l->buf,history[history_len - 1 - l->history_index],l->buflen);
        l->buf[l->buflen-1] = '\0';
        l->len = l->pos = strlen(l->buf);
        
        /* Use standard refresh function for consistent clearing and redrawing */
        refreshLineWithFlags(l, REFRESH_ALL);
    }
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
void linenoiseEditDelete(struct linenoiseState *l) {
    if (l->len > 0 && l->pos < l->len) {
        size_t char_len = nextCharLen(l->buf, l->len, l->pos, NULL);
        memmove(l->buf+l->pos,l->buf+l->pos+char_len,l->len-l->pos-char_len);
        l->len -= char_len;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Backspace implementation. */
void linenoiseEditBackspace(struct linenoiseState *l) {
    if (l->pos > 0 && l->len > 0) {
        size_t char_len = prevCharLen(l->buf, l->len, l->pos, NULL);
        memmove(l->buf+l->pos-char_len,l->buf+l->pos,l->len-l->pos);
        l->pos -= char_len;
        l->len -= char_len;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

/* Delete the previous word, maintaining the cursor at the start of the
 * current word. */
void linenoiseEditDeletePrevWord(struct linenoiseState *l) {
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos--;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

/* ========================== Multiplexed API ================================ */

/* This function is part of the multiplexed API of Linenoise. See the top
 * comment on linenoiseEditStart() for more information. */
int linenoiseEditStart(struct linenoiseState *l, int stdin_fd, int stdout_fd, char *buf, size_t buflen, const char *prompt) {
    /* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
    l->in_completion = 0;
    l->ifd = stdin_fd != -1 ? stdin_fd : STDIN_FILENO;
    l->ofd = stdout_fd != -1 ? stdout_fd : STDOUT_FILENO;
    l->buf = buf;
    l->buflen = buflen;
    l->prompt = prompt;
    l->plen = strlen(prompt);
    l->oldcolpos = l->pos = 0;
    l->len = 0;
    l->oldrows = 0;
    l->history_index = 0;

    /* Enter raw mode. */
    if (enableRawMode(l->ifd) == -1) return -1;

    l->cols = getColumns(stdin_fd, stdout_fd);

    /* Buffer starts empty. */
    l->buf[0] = '\0';
    l->buflen--; /* Make sure there is always space for the nulterm */

    /* If stdin is not a tty, stop here with the initialization. */
    if (!isatty(l->ifd)) return 0;

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    linenoiseHistoryAdd("");

    if (write(l->ofd,prompt,l->plen) == -1) return -1;
    return 0;
}

char *linenoiseEditMore = "If you see this, you are misusing the API";

/* This function is part of the multiplexed API of linenoise. */
char *linenoiseEditFeed(struct linenoiseState *l) {
    /* Not a TTY, pass control to line reading without character
     * count limits. */
    if (!isatty(l->ifd)) return linenoiseNoTTY();

    char c;
    int nread;
    char seq[3];

    nread = read(l->ifd,&c,1);
    if (nread <= 0) return NULL;

    /* Only autocomplete when the callback is set. */
    if ((l->in_completion || c == 9) && completionCallback != NULL) {
        c = completeLine(l,c);
        /* Return on errors */
        if (c < 0) return NULL;
        /* Read next character when 0 */
        if (c == 0) return linenoiseEditMore;
    }

    switch(c) {
    case ENTER:    /* enter */
        history_len--;
        free(history[history_len]);
        if (mlmode) linenoiseEditMoveEnd(l);
        if (hintsCallback) {
            /* Force a refresh without hints to leave the previous
             * line as the user typed it after a newline. */
            linenoiseHintsCallback *hc = hintsCallback;
            hintsCallback = NULL;
            refreshLine(l);
            hintsCallback = hc;
        }
        return strdup(l->buf);
    case CTRL_C:     /* ctrl-c */
        errno = EAGAIN;
        return NULL;
    case BACKSPACE:   /* backspace */
    case 8:     /* ctrl-h */
        linenoiseEditBackspace(l);
        break;
    case CTRL_D:     /* ctrl-d, remove char at right of cursor, or if the
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
    case CTRL_T:    /* ctrl-t, swaps current character with previous. */
        if (l->pos > 0 && l->pos < l->len) {
            int aux = l->buf[l->pos-1];
            l->buf[l->pos-1] = l->buf[l->pos];
            l->buf[l->pos] = aux;
            if (l->pos != l->len-1) l->pos++;
            refreshLine(l);
        }
        break;
    case CTRL_B:     /* ctrl-b */
        linenoiseEditMoveLeft(l);
        break;
    case CTRL_F:     /* ctrl-f */
        linenoiseEditMoveRight(l);
        break;
    case CTRL_P:    /* ctrl-p */
        linenoiseEditHistoryNext(l, LINENOISE_HISTORY_PREV);
        break;
    case CTRL_N:    /* ctrl-n */
        linenoiseEditHistoryNext(l, LINENOISE_HISTORY_NEXT);
        break;
    case ESC:    /* escape sequence */
        /* Read the next two bytes representing the escape sequence. */
        if (read(l->ifd,seq,1) == -1) break;
        if (read(l->ifd,seq+1,1) == -1) break;

        /* ESC [ sequences. */
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                /* Extended escape, read additional byte. */
                if (read(l->ifd,seq+2,1) == -1) break;
                if (seq[2] == '~') {
                    switch(seq[1]) {
                    case '3': /* Delete key. */
                        linenoiseEditDelete(l);
                        break;
                    }
                }
            } else {
                switch(seq[1]) {
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
            switch(seq[1]) {
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
        if (linenoiseEditInsert(l,c)) return NULL;
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

/* This is part of the multiplexed linenoise API. */
void linenoiseEditStop(struct linenoiseState *l) {
    if (!isatty(l->ifd)) return;
    disableRawMode(l->ifd);
    printf("\n");
}

/* This just implements a blocking loop for the multiplexed API. */
static char *linenoiseBlockingEdit(int stdin_fd, int stdout_fd, char *buf, size_t buflen, const char *prompt) {
    struct linenoiseState l;

    /* Editing without a buffer is invalid. */
    if (buflen == 0) {
        errno = EINVAL;
        return NULL;
    }

    linenoiseEditStart(&l,stdin_fd,stdout_fd,buf,buflen,prompt);
    char *res;
    while((res = linenoiseEditFeed(&l)) == linenoiseEditMore);
    linenoiseEditStop(&l);
    return res;
}

/* This function is called when linenoise() is called with the standard
 * input file descriptor not attached to a TTY. */
static char *linenoiseNoTTY(void) {
    char *line = NULL;
    size_t len = 0, maxlen = 0;

    while(1) {
        if (len == maxlen) {
            if (maxlen == 0) maxlen = 16;
            maxlen *= 2;
            char *oldval = line;
            line = realloc(line,maxlen);
            if (line == NULL) {
                if (oldval) free(oldval);
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

/* The high level function that is the main API of the linenoise library. */
char *linenoise(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];

    if (!isatty(STDIN_FILENO)) {
        /* Not a tty: read from file / pipe. */
        return linenoiseNoTTY();
    } else if (isUnsupportedTerm()) {
        size_t len;

        printf("%s",prompt);
        fflush(stdout);
        if (fgets(buf,LINENOISE_MAX_LINE,stdin) == NULL) return NULL;
        len = strlen(buf);
        while(len && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
            len--;
            buf[len] = '\0';
        }
        return strdup(buf);
    } else {
        char *retval = linenoiseBlockingEdit(STDIN_FILENO,STDOUT_FILENO,buf,LINENOISE_MAX_LINE,prompt);
        return retval;
    }
}

/* This is just a wrapper the user may want to call in order to make sure
 * the linenoise returned buffer is freed with the same allocator it was
 * created with. Useful when the main program is using an alternative
 * allocator. */
void linenoiseFree(void *ptr) {
    if (ptr == linenoiseEditMore) return; // Protect from API misuse.
    free(ptr);
}

/* ================================ History ================================= */

/* Free the history, but does not reset it. Only used when we have to
 * exit() to avoid memory leaks are reported by valgrind & co. */
static void freeHistory(void) {
    if (history) {
        int j;

        for (j = 0; j < history_len; j++)
            free(history[j]);
        free(history);
    }
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void linenoiseAtExit(void) {
    disableRawMode(STDIN_FILENO);
    freeHistory();
}

/* This is the API call to add a new entry in the linenoise history. */
int linenoiseHistoryAdd(const char *line) {
    char *linecopy;

    if (history_max_len == 0) return 0;

    /* Initialization on first call. */
    if (history == NULL) {
        history = malloc(sizeof(char*)*history_max_len);
        if (history == NULL) return 0;
        memset(history,0,(sizeof(char*)*history_max_len));
    }

    /* Don't add duplicated lines if history_no_dups is set. */
    if (history_no_dups && history_len && !strcmp(history[history_len-1], line)) return 0;

    /* Add an heap allocated copy of the line in the history.
     * If we reached the max length, remove the older line. */
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (history_len == history_max_len) {
        free(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

/* Set the maximum length for the history. */
int linenoiseHistorySetMaxLen(int len) {
    char **new;

    if (len < 1) return 0;
    if (history) {
        int tocopy = history_len;

        new = malloc(sizeof(char*)*len);
        if (new == NULL) return 0;

        /* If we can't copy everything, free the elements we'll not use. */
        if (len < tocopy) {
            int j;

            for (j = 0; j < tocopy-len; j++) free(history[j]);
            tocopy = len;
        }
        memset(new,0,sizeof(char*)*len);
        memcpy(new,history+(history_len-tocopy), sizeof(char*)*tocopy);
        free(history);
        history = new;
    }
    history_max_len = len;
    if (history_len > history_max_len)
        history_len = history_max_len;
    return 1;
}

/* Set history to avoid duplicates */
void linenoiseHistorySetNoDuplicates(bool no_dups) {
    history_no_dups = no_dups;
}

/* Save the history in the specified file. On success 0 is returned
 * otherwise -1 is returned. */
int linenoiseHistorySave(const char *filename) {
    mode_t old_umask = umask(S_IXUSR|S_IRWXG|S_IRWXO);
    FILE *fp;
    int j;

    fp = fopen(filename,"w");
    umask(old_umask);
    if (fp == NULL) return -1;
    chmod(filename,S_IRUSR|S_IWUSR);
    for (j = 0; j < history_len; j++)
        fprintf(fp,"%s\n",history[j]);
    fclose(fp);
    return 0;
}

/* Load the history from the specified file. If the file does not exist
 * zero is returned and no operation is performed. */
int linenoiseHistoryLoad(const char *filename) {
    FILE *fp = fopen(filename,"r");
    char buf[LINENOISE_MAX_LINE];

    if (fp == NULL) return -1;

    while (fgets(buf,LINENOISE_MAX_LINE,fp) != NULL) {
        char *p;

        p = strchr(buf,'\r');
        if (!p) p = strchr(buf,'\n');
        if (p) *p = '\0';
        linenoiseHistoryAdd(buf);
    }
    fclose(fp);
    return 0;
}

/* Get history length */
int linenoiseHistoryLen(void) {
    return history_len;
}

/* Get history line at index */
const char *linenoiseHistoryLine(int index) {
    if (index >= 0 && index < history_len) {
        return history[index];
    }
    return NULL;
}

/* Clear history */
void linenoiseHistoryClear(void) {
    if (history) {
        int j;
        for (j = 0; j < history_len; j++)
            free(history[j]);
        history_len = 0;
    }
}

/* Print history - enhanced API function */
void linenoiseHistoryPrint(void) {
    int j;
    for (j = 0; j < history_len; j++) {
        printf("%4d: %s\n", j, history[j]);
    }
}

/* Get history at index - enhanced API function */
char *linenoiseHistoryGet(int index) {
    if (index >= 0 && index < history_len) {
        return strdup(history[index]);
    }
    return NULL;
}

/* Set history no duplicates mode */
void linenoiseSetHistoryNoDups(int enable) {
    history_no_dups = enable ? true : false;
}

/* History expansion (basic implementation) */
int linenoiseHistoryExpansion(const char *line, char **expanded) {
    /* Simple pass-through for now - can be enhanced later */
    *expanded = strdup(line);
    return 0;
}

/* Reverse history search (placeholder) */
void linenoiseHistoryReverseSearch(struct linenoiseState *l) {
    /* Implementation can be added later for Ctrl+R */
    (void)l;
}

/* Delete history entry */
int linenoiseHistoryDelete(int index) {
    if (index < 0 || index >= history_len) return 0;
    
    free(history[index]);
    if (index < history_len - 1) {
        memmove(history + index, history + index + 1, 
                sizeof(char*) * (history_len - index - 1));
    }
    history_len--;
    return 1;
}

/* Set no duplicates flag */
void linenoiseHistoryNoDups(bool flag) {
    history_no_dups = flag;
}

/* Remove duplicates from history */
int linenoiseHistoryRemoveDups(void) {
    int removed = 0;
    int i, j;
    
    for (i = 0; i < history_len; i++) {
        for (j = i + 1; j < history_len; j++) {
            if (strcmp(history[i], history[j]) == 0) {
                free(history[j]);
                if (j < history_len - 1) {
                    memmove(history + j, history + j + 1,
                            sizeof(char*) * (history_len - j - 1));
                }
                history_len--;
                j--; /* Check the same position again */
                removed++;
            }
        }
    }
    return removed;
}

/* Initialize UTF-8 support */
void linenoiseSetupUtf8(void) {
    linenoiseSetEncodingFunctions(
        linenoiseUtf8PrevCharLen,
        linenoiseUtf8NextCharLen,
        linenoiseUtf8ReadCode
    );
}
