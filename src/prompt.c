/**
 * prompt.c - routines to build a prompt string
 *
 * Copyright (c) 2015 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lusush.h"
#include "prompt.h"
#include "opts.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define DBGSTR "lusush: prompt.c: "

// ANSI foreground color values
typedef enum {
    BLACK = 30,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE
} FG_COLOR;

// ANSI background color values
typedef enum {
    BG_BLACK = 40,
    BG_RED,
    BG_GREEN,
    BG_YELLOW,
    BG_BLUE,
    BG_MAGENTA,
    BG_CYAN,
    BG_WHITE
} BG_COLOR;

// ANSI text attribute values
typedef enum {
    NONE       = 0,
    BOLD       = 1,
    UNDERSCORE = 4,
    BLINK      = 5,
    REVERSE    = 7,
    CONCEALED  = 8
} TEXT_ATTRIB;

static const char *RESET = "\x1b[0m"; // ANSI color reset
static char *colors      = NULL;      // ANSI color sequence
static FG_COLOR fg_color = WHITE;     // Default foreground color
static BG_COLOR bg_color = BG_BLUE;   // Default background color
static TEXT_ATTRIB attr  = NONE;      // Default text attributes

// Key->value struct for ANSI codes
struct opt_pair {
    char key[15];
    int val;
};

// Key-value table for foreground codes
static const struct opt_pair fg_opts[] = {
    { "BLACK",      BLACK       },
    { "RED",        RED         },
    { "GREEN",      GREEN       },
    { "YELLOW",     YELLOW      },
    { "BLUE",       BLUE        },
    { "MAGENTA",    MAGENTA     },
    { "CYAN",       CYAN        },
    { "WHITE",      WHITE       }
};

// Key-value table for background codes
static const struct opt_pair bg_opts[] = {
    { "BLACK",      BG_BLACK    },
    { "RED",        BG_RED      },
    { "GREEN",      BG_GREEN    },
    { "YELLOW",     BG_YELLOW   },
    { "BLUE",       BG_BLUE     },
    { "MAGENTA",    BG_MAGENTA  },
    { "CYAN",       BG_CYAN     },
    { "WHITE",      BG_WHITE    }
};

// Key-value table for attribute codes
static const struct opt_pair attr_opts[] = {
    { "NONE",       NONE        },
    { "BOLD",       BOLD        },
    { "UNDERSCORE", UNDERSCORE  },
    { "BLINK",      BLINK       },
    { "REVERSE",    REVERSE     },
    { "CONCEALED",  CONCEALED   }
};

/**
 * setprompt_usage:
 *      Print usage information for builtin command setprompt.
 */
static void setprompt_usage(void)
{
    printf("usage:\n\t-h\t\tThis help\n\t");
    printf("-a ATTRIBUTE\tset attribute for prompt\n\t");
    printf("-f COLOR\tset prompt foreground color\n\t");
    printf("-b COLOR\tset prompt background color\n\t");
    printf("-v\t\tshow valid colors and attributes\n");
}

/**
 * build_colors:
 *      Build ANSI escape sequence to set prompt colors.
 */
static void build_colors(void)
{
    if (!colors)
        if ((colors = calloc(14, sizeof(char))) == NULL)
            error_syscall("lusush: prompt.c: build_colors: calloc");

    snprintf(colors, 14, "%c[%u;%u;%um", 0x1b, attr, fg_color, bg_color);
}

/**
 * set_prompt_fg:
 *      Set prompt foreground color.
 */
static void set_prompt_fg(FG_COLOR fg)
{
    fg_color = fg;
}

/**
 * set_prompt_bg:
 *      Set prompt background color.
 */
static void set_prompt_bg(BG_COLOR bg)
{
    bg_color = bg;
}

/**
 * set_prompt_attrib:
 *      Set text attributes for prompt.
 */
static void set_prompt_attr(TEXT_ATTRIB ta)
{
    attr = ta;
}

/**
 * set_prompt:
 *      Create the command prompt.
 */
void set_prompt(int argc, char **argv)
{
    int i = 0;
    // next option
    int nopt = 0;
    // string of valid short options
    const char *sopts = "ha:f:b:v";
    // array describing valid long options
    const struct option lopts[] = {
        { "help",       0, NULL, 'h' },
        { "attributes", 1, NULL, 'a' },
        { "foreground", 1, NULL, 'f' },
        { "background", 1, NULL, 'b' },
        { "valid-opts", 1, NULL, 'v' },
        { NULL,         0, NULL,  0  }
    };

    optind = 1;

    if (argc < 2) {
        setprompt_usage();
        return;
    }

    do {
        nopt = getopt_long(argc, argv, sopts, lopts, NULL);

        switch (nopt) {
        case 'h':
            setprompt_usage();
            break;
        case 'a':
            for (i = 0; i < 6; i++) {
                if (strncmp(optarg, attr_opts[i].key,
                            strnlen(attr_opts[i].key, MAXLINE)) == 0) {
                    set_prompt_attr(attr_opts[i].val);
                    build_prompt();
                }
            }
            break;
        case 'f':
            for (i = 0; i < 8; i++) {
                if (strncmp(optarg, fg_opts[i].key,
                            strnlen(fg_opts[i].key, MAXLINE)) == 0) {
                    set_prompt_fg(fg_opts[i].val);
                    build_prompt();
                }
            }
            break;
        case 'b':
            for (i = 0; i < 8; i++) {
                if (strncmp(optarg, bg_opts[i].key,
                            strnlen(bg_opts[i].key, MAXLINE)) == 0) {
                    set_prompt_bg(bg_opts[i].val);
                    build_prompt();
                }
            }
            break;
        case 'v':
            printf("VALID COLORS:\n");
            for (i = 0; i < 8; i++)
                printf("\t%s\n", fg_opts[i].key);

            printf("VALID ATTRIBUTES:\n");
            for (i = 0; i < 6; i++)
                printf("\t%s\n", attr_opts[i].key);
            break;
        case '?':
            setprompt_usage();
            break;
        case -1:
            break;
        default:
            abort();
        }
    } while (nopt != -1);
}

/**
 * build_prompt:
 *      Builds the user's prompt displaying the current working directory.
 */
void build_prompt(void)
{
    char *cwd = NULL;
    char prompt[MAXLINE] = { '\0' };

    if (!(cwd = getcwd(NULL, 0))) {
        error_syscall("lusush: prompt.c: build_prompt: getcwd");
    }
    else {
        if (opt_is_set(FANCY_PROMPT)) {
            build_colors();
            strncpy(prompt, colors, strnlen(colors, MAXLINE) + 1);
            strncat(prompt, cwd, strnlen(cwd, MAXLINE) + 1);
            strncat(prompt, RESET, strnlen(RESET, MAXLINE) + 1);
            strncat(prompt, "\n", 2);
            strncat(prompt, "% ", 3);
        }
        else {
            strncpy(prompt, cwd, MAXLINE);
            strncat(prompt, "% ", 3);
        }
    }
    setenv("PROMPT", prompt, 1);

    if (cwd)
        free(cwd);

    cwd = NULL;

    if (colors)
        free(colors);

    colors = NULL;
}
