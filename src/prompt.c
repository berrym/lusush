#include "../include/prompt.h"

#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/strings.h"
#include "../include/symtable.h"

#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// ANSI foreground color values
typedef enum {
    ANSI_FG_BLACK = 30,
    ANSI_FG_RED = 31,
    ANSI_FG_GREEN = 32,
    ANSI_FG_YELLOW = 33,
    ANSI_FG_BLUE = 34,
    ANSI_FG_MAGENTA = 35,
    ANSI_FG_CYAN = 36,
    ANSI_FG_WHITE = 37,
    ANSI_FG_DEFAULT = 39,
    FG_NONE = 0,
} FG_COLOR;

// ANSI background color values
typedef enum {
    ANSI_BG_BLACK = 40,
    ANSI_BG_RED = 41,
    ANSI_BG_GREEN = 42,
    ANSI_BG_YELLOW = 43,
    ANSI_BG_BLUE = 44,
    ANSI_BG_MAGENTA = 45,
    ANSI_BG_CYAN = 46,
    ANSI_BG_WHITE = 47,
    ANSI_BG_DEFAULT = 49,
    BG_NONE = 0,
} BG_COLOR;

// ANSI text attribute values
typedef enum {
    ANSI_RESET_ON = 0,
    ANSI_BOLD_ON = 1,
    ANSI_ITALICS_ON = 3,
    ANSI_UNDERLINE_ON = 4,
    ANSI_BLINK_ON = 5,
    ANSI_INVERSE_ON = 7,
    ANSI_CONCEALED_ON = 8,
    ANSI_STRIKETHROUGH_ON = 9,
    ANSI_BOLD_OFF = 21,
    ANSI_ITALICS_OFF = 23,
    ANSI_UNDERLINE_OFF = 24,
    ANSI_INVERSE_OFF = 27,
    ANSI_STRIKETHROUGH_OFF = 29,
} TEXT_ATTRIB;

// Key->value struct for ANSI codes
typedef struct {
    const char *key;
    int val;
} prompt_opts;

// Key-value table for foreground codes
static const prompt_opts fg_opts[] = {
    {  "black",   ANSI_FG_BLACK},
    {    "red",     ANSI_FG_RED},
    {  "green",   ANSI_FG_GREEN},
    { "yellow",  ANSI_FG_YELLOW},
    {   "blue",    ANSI_FG_BLUE},
    {"magenta", ANSI_FG_MAGENTA},
    {   "cyan",    ANSI_FG_CYAN},
    {  "white",   ANSI_FG_WHITE},
    {   "none",               0}
};
static const int NUM_FG_OPTS = sizeof(fg_opts) / sizeof(prompt_opts);

// Key-value table for background codes
static const prompt_opts bg_opts[] = {
    {  "black",   ANSI_BG_BLACK},
    {    "red",     ANSI_BG_RED},
    {  "green",   ANSI_BG_GREEN},
    { "yellow",  ANSI_BG_YELLOW},
    {   "blue",    ANSI_BG_BLUE},
    {"magenta", ANSI_BG_MAGENTA},
    {   "cyan",    ANSI_BG_CYAN},
    {  "white",   ANSI_BG_WHITE},
    {   "none",               0}
};
static const int NUM_BG_OPTS = sizeof(bg_opts) / sizeof(prompt_opts);

// Key-value table for attribute codes
static const prompt_opts attr_opts[] = {
    {             "none",          ANSI_RESET_ON},
    {             "bold",           ANSI_BOLD_ON},
    {           "italics",        ANSI_ITALICS_ON},
    {        "underline",      ANSI_UNDERLINE_ON},
    {            "blink",          ANSI_BLINK_ON},
    {          "reverse",        ANSI_INVERSE_ON},
    {        "concealed",      ANSI_CONCEALED_ON},
    {         "bold_off",          ANSI_BOLD_OFF},
    {      "italics_off",       ANSI_ITALICS_OFF},
    {    "underline_off",     ANSI_UNDERLINE_OFF},
    {      "inverse_off",       ANSI_INVERSE_OFF},
    {"strikethrough_off", ANSI_STRIKETHROUGH_OFF}
};
static const int NUM_VALID_ATTRIB = sizeof(attr_opts) / sizeof(prompt_opts);

static const char *RESET = "\x1b[0m";     // ansi color reset
static char *colors = NULL;               // ansi color sequence
static FG_COLOR fg_color = ANSI_FG_CYAN;  // default foreground color
static BG_COLOR bg_color = BG_NONE; // default background color
static TEXT_ATTRIB attr = ANSI_BOLD_ON;   // default text attributes

/**
 * setprompt_usage:
 *      Print usage information for builtin command setprompt.
 */
static void setprompt_usage(void) {
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
static int build_colors(void) {
    if (colors == NULL) {
        colors = alloc_str(32, false);
        if (colors == NULL) {
            return 1;
        }
    }

    if (bg_color == BG_NONE) {
        sprintf(colors, "%c[%d;%dm", 0x1b, attr, fg_color);
    } else {
        sprintf(colors, "%c[%d;%d;%dm", 0x1b, attr, fg_color, bg_color);
    }

    return 0;
}

/**
 * set_prompt_fg:
 *      Set prompt foreground color.
 */
static void set_prompt_fg(FG_COLOR fg) { fg_color = fg; }

/**
 * set_prompt_bg:
 *      Set prompt background color.
 */
static void set_prompt_bg(BG_COLOR bg) { bg_color = bg; }

/**
 * set_prompt_attrib:
 *      Set text attributes for prompt.
 */
static void set_prompt_attr(TEXT_ATTRIB ta) { attr = ta; }

/**
 * set_prompt:
 *      Create the command prompt.
 */
void set_prompt(int argc, char **argv) {
    int i = 0;
    int nopt = 0;                   // next option
    const char *sopts = "ha:f:b:v"; // string of valid short options
    // array describing valid long options
    const struct option lopts[] = {
        {      "help", 0, NULL, 'h'},
        {"attributes", 1, NULL, 'a'},
        {"foreground", 1, NULL, 'f'},
        {"background", 1, NULL, 'b'},
        {"valid-opts", 1, NULL, 'v'},
        {        NULL, 0, NULL,   0}
    };

    optind = 1;

    if (argc < 2) {
        setprompt_usage();
        return;
    }

    do {
        nopt = getopt_long(argc, argv, sopts, lopts, NULL);

        switch (nopt) {
        case 'h': // show usage help
            setprompt_usage();
            break;
        case 'a': // set fancy prompt text attribute
            for (i = 0; i < NUM_VALID_ATTRIB; i++) {
                if (strcmp(optarg, attr_opts[i].key) == 0) {
                    set_prompt_attr(attr_opts[i].val);
                }
            }
            break;
        case 'f': // set fancy prompt foreground color
            for (i = 0; i < NUM_FG_OPTS; i++) {
                if (strcmp(optarg, fg_opts[i].key) == 0) {
                    set_prompt_fg(fg_opts[i].val);
                }
            }
            break;
        case 'b': // set fancy prompt background color
            for (i = 0; i < NUM_BG_OPTS; i++) {
                if (strcmp(optarg, bg_opts[i].key) == 0) {
                    set_prompt_bg(bg_opts[i].val);
                }
            }
            break;
        case 'v': // print valid setprompt options
            printf("VALID COLORS:\n");
            for (i = 0; i < NUM_FG_OPTS; i++) {
                printf("\t%s\n", fg_opts[i].key);
            }

            printf("VALID ATTRIBUTES:\n");
            for (i = 0; i < NUM_VALID_ATTRIB; i++) {
                printf("\t%s\n", attr_opts[i].key);
            }
            break;
        case -1:
            break;
        default:
            setprompt_usage();
            break;
        }
    } while (nopt != -1);

    build_prompt();
}

/**
 * build_prompt:
 *      Builds the user's prompt, either a fancy colored one with
 *      the current working directory or a plain '% or # '.
 */
void build_prompt(void) {
    char u[_POSIX_LOGIN_NAME_MAX + 1] = {'\0'}; // username
    char h[_POSIX_HOST_NAME_MAX + 1] = {'\0'};  // hostname
    char d[_POSIX_PATH_MAX + 1] = {'\0'};       // current working directory
    char prompt[MAXLINE + 1] = {'\0'};          // prompt string

    // Build a prompt string
    if (fancy_prompt == true) {
        // Get user's login name
        if (getlogin_r(u, _POSIX_LOGIN_NAME_MAX) < 0) {
            error_return("error: `build_prompt`: getlogin_r");
            goto fancy_error;
        }

        // Get machine host name
        if (gethostname(h, _POSIX_HOST_NAME_MAX) < 0) {
            error_return("error: `build_prompt`: gethostname");
            goto fancy_error;
        }

        // Get current working directory
        if (!(getcwd(d, _POSIX_PATH_MAX))) {
            error_return("error: `build_prompt`: getcwd");
            goto fancy_error;
        }

        // Build text colors, and then the formatted prompt string
        if (build_colors() > 0) {
            goto fancy_error;
        }

        sprintf(prompt, "%s%s@%s in %s%s%c ", colors, u, h, d, RESET,
                (getuid() > 0) ? '%' : '#');
    } else {
    fancy_error:
        if (getuid() > 0) {
            strcpy(prompt, "% "); // normal user prompt
        } else {
            strcpy(prompt, "# "); // root user prompt
        }
    }

    // Set the prompt environment variables
    symtable_entry_t *prompt_entry = add_to_symtable("PS1");
    symtable_entry_t *prompt2_entry = add_to_symtable("PS2");

    if (getuid() > 0) {
        symtable_entry_setval(prompt_entry, prompt);
    } else {
        symtable_entry_setval(prompt_entry, prompt);
    }

    symtable_entry_setval(prompt2_entry, "> ");

    if (colors) {
        free(colors);
    }

    colors = NULL;
}
