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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "lusush.h"
#include "errors.h"
#include "prompt.h"
#include "opts.h"

// ANSI foreground color values
typedef enum {
    ANSI_FG_BLACK   = 30,
    ANSI_FG_RED     = 31,
    ANSI_FG_GREEN   = 32,
    ANSI_FG_YELLOW  = 33,
    ANSI_FG_BLUE    = 34,
    ANSI_FG_MAGENTA = 35,
    ANSI_FG_CYAN    = 36,
    ANSI_FG_WHITE   = 37,
    ANSI_FG_DEFAULT = 39,
} FG_COLOR;

// ANSI background color values
typedef enum {
    ANSI_BG_BLACK   = 40,
    ANSI_BG_RED     = 41,
    ANSI_BG_GREEN   = 42,
    ANSI_BG_YELLOW  = 43,
    ANSI_BG_BLUE    = 44,
    ANSI_BG_MAGENTA = 45,
    ANSI_BG_CYAN    = 46,
    ANSI_BG_WHITE   = 47,
    ANSI_BG_DEFAULT = 49,
} BG_COLOR;

// ANSI text attribute values
typedef enum {
    ANSI_RESET_ON          = 0,
    ANSI_BOLD_ON           = 1,
    ANSI_UNDERLINE_ON      = 4,
    ANSI_BLINK_ON          = 5,
    ANSI_INVERSE_ON        = 7,
    ANSI_CONCEALED_ON      = 8,
    ANSI_STRIKETHROUGH_ON  = 9,
    ANSI_BOLD_OFF          = 21,
    ANSI_ITALICS_OFF       = 23,
    ANSI_UNDERLINE_OFF     = 24,
    ANSI_INVERSE_OFF       = 27,
    ANSI_STRIKETHROUGH_OFF = 29,
} TEXT_ATTRIB;

static const char *RESET = "\x1b[0m"; // ansi color reset
static char *colors      = NULL;      // ansi color sequence
static FG_COLOR fg_color = ANSI_FG_WHITE; // default foreground color
static BG_COLOR bg_color = ANSI_BG_BLUE;  // default background color
static TEXT_ATTRIB attr  = ANSI_RESET_ON; // default text attributes

// Key->value struct for ANSI codes
struct opt_pair {
    char *key;
    int val;
};

// Key-value table for foreground codes
static const struct opt_pair fg_opts[] = {
    { "BLACK",   ANSI_FG_BLACK   },
    { "RED",     ANSI_FG_RED     },
    { "GREEN",   ANSI_FG_GREEN   },
    { "YELLOW",  ANSI_FG_YELLOW  },
    { "BLUE",    ANSI_FG_BLUE    },
    { "MAGENTA", ANSI_FG_MAGENTA },
    { "CYAN",    ANSI_FG_CYAN    },
    { "WHITE",   ANSI_FG_WHITE   }
};

// Key-value table for background codes
static const struct opt_pair bg_opts[] = {
    { "BLACK",   ANSI_BG_BLACK   },
    { "RED",     ANSI_BG_RED     },
    { "GREEN",   ANSI_BG_GREEN   },
    { "YELLOW",  ANSI_BG_YELLOW  },
    { "BLUE",    ANSI_BG_BLUE    },
    { "MAGENTA", ANSI_BG_MAGENTA },
    { "CYAN",    ANSI_BG_CYAN    },
    { "WHITE",   ANSI_BG_WHITE   }
};

// Key-value table for attribute codes
static const struct opt_pair attr_opts[] = {
    { "NONE",       ANSI_RESET_ON     },
    { "BOLD",       ANSI_BOLD_ON      },
    { "UNDERSCORE", ANSI_UNDERLINE_ON },
    { "BLINK",      ANSI_BLINK_ON     },
    { "REVERSE",    ANSI_INVERSE_ON   },
    { "CONCEALED",  ANSI_CONCEALED_ON }
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
static int build_colors(void)
{
    if (!colors) {
        if ((colors = calloc(14, sizeof(char))) == NULL) {
            error_return("build_colors: calloc");
            return 1;
        }
    }
    snprintf(colors, 14, "%c[%u;%u;%um", 0x1b, attr, fg_color, bg_color);
    return 0;
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
    int nopt = 0;                   // next option
    const char *sopts = "ha:f:b:v"; // string of valid short options
    const struct option lopts[] = { // array describing valid long options
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
        case 'h':               // show usage help
            setprompt_usage();
            break;
        case 'a':               // set fancy prompt text attribute
            for (i = 0; i < 6; i++)
                if (strcmp(optarg, attr_opts[i].key) == 0)
                    set_prompt_attr(attr_opts[i].val);
            break;
        case 'f':               // set fancy prompt foreground color
            for (i = 0; i < 8; i++)
                if (strcmp(optarg, fg_opts[i].key) == 0)
                    set_prompt_fg(fg_opts[i].val);
            break;
        case 'b':               // set fancy prompt background color
            for (i = 0; i < 8; i++)
                if (strcmp(optarg, bg_opts[i].key) == 0)
                    set_prompt_bg(bg_opts[i].val);
            break;
        case 'v':               // print valid setprompt options
            printf("VALID COLORS:\n");
            for (i = 0; i < 8; i++)
                printf("\t%s\n", fg_opts[i].key);

            printf("VALID ATTRIBUTES:\n");
            for (i = 0; i < 6; i++)
                printf("\t%s\n", attr_opts[i].key);
            break;
        case -1:
            break;
        default:
            setprompt_usage();
            break;
        }
    } while (nopt != -1);
}

/**
 * build_prompt:
 *      Builds the user's prompt, either a fancy colored one with
 *      the current working directory or a plain '% or # '.
 */
void build_prompt(void)
{
    char u[_POSIX_LOGIN_NAME_MAX + 1] = { '\0' }; // username
    char h[_POSIX_HOST_NAME_MAX + 1] = { '\0' };  // hostname
    char d[_POSIX_PATH_MAX + 1] = { '\0' };       // current working directory
    char prompt[MAXLINE + 1] = { '\0' };          // prompt string

    // Build a prompt string
    if (opt_is_set(FANCY_PROMPT)) {
        // Get user's login name
        if (getlogin_r(u, _POSIX_LOGIN_NAME_MAX) < 0) {
            error_return("build_prompt: getlogin_r");
            goto fancy_error;
        }

        // Get machine host name
        if (gethostname(h, _POSIX_HOST_NAME_MAX) < 0) {
            error_return("build_prompt: gethostname");
            goto fancy_error;
        }

        // Get current working directory
        if (!(getcwd(d, _POSIX_PATH_MAX))) {
            error_return("build_prompt: getcwd");
            goto fancy_error;
        }

        // Build text colors, and then the formatted prompt string
        if (build_colors() > 0)
            goto fancy_error;

        snprintf(prompt, MAXLINE, "%s%s@%s %s%s\n%c ",
                 colors, u, h, d, RESET, (getuid() > 0) ? '%' : '#');
    }
    else {
fancy_error:
        if (getuid() > 0)
            strncpy(prompt, "% ", 3); // normal user prompt
        else
            strncpy(prompt, "# ", 3); // root user prompt
    }

    // Set the PROMPT environment variable
    setenv("PROMPT", prompt, 1);

    if (colors)
        free(colors);

    colors = NULL;
}
