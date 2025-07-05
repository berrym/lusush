#include "../include/prompt.h"

#include "../include/config.h"
#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/strings.h"
#include "../include/symtable.h"
#include "../include/themes.h"

#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Prompt styles
typedef enum {
    NORMAL_PROMPT,
    COLOR_PROMPT,
    FANCY_PROMPT,
    PRO_PROMPT,
    GIT_PROMPT,
} PROMPT_STYLE;

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
    ANSI_BLINK_OFF = 25,
    ANSI_INVERSE_OFF = 27,
    ANSI_CONCEALED_OFF = 28,
    ANSI_STRIKETHROUGH_OFF = 29,
} TEXT_ATTRIB;

// Key->value struct for ANSI codes
typedef struct {
    const char *key;
    int val;
} prompt_opts;

// Key-value table for prompt styles
static const prompt_opts prompt_styles[] = {
    {"normal", NORMAL_PROMPT},
    { "color",  COLOR_PROMPT},
    { "fancy",  FANCY_PROMPT},
    {   "pro",    PRO_PROMPT},
    {   "git",    GIT_PROMPT},
};
static const int NUM_PROMPT_STYLES =
    sizeof(prompt_styles) / sizeof(prompt_opts);

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
    {"default", ANSI_FG_DEFAULT}
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
    {"default", ANSI_BG_DEFAULT}
};
static const int NUM_BG_OPTS = sizeof(bg_opts) / sizeof(prompt_opts);

// Key-value table for attribute codes
static const prompt_opts attr_opts[] = {
    {             "none",          ANSI_RESET_ON},
    {             "bold",           ANSI_BOLD_ON},
    {          "italics",        ANSI_ITALICS_ON},
    {        "underline",      ANSI_UNDERLINE_ON},
    {            "blink",          ANSI_BLINK_ON},
    {          "inverse",        ANSI_INVERSE_ON},
    {        "concealed",      ANSI_CONCEALED_ON},
    {    "strikethrough",  ANSI_STRIKETHROUGH_ON},
    {         "bold-off",          ANSI_BOLD_OFF},
    {      "italics-off",       ANSI_ITALICS_OFF},
    {    "underline-off",     ANSI_UNDERLINE_OFF},
    {        "blink-off",         ANSI_BLINK_OFF},
    {      "inverse-off",       ANSI_INVERSE_OFF},
    {"strikethrough-off", ANSI_STRIKETHROUGH_OFF}
};
static const int NUM_VALID_ATTRIB = sizeof(attr_opts) / sizeof(prompt_opts);

static const char *RESET = "\x1b[0m"; // ansi color reset
static char *colors = NULL;                      // ansi color sequence
// Static variables for legacy prompt support
static FG_COLOR fg_color = ANSI_FG_GREEN;        // default foreground color
static BG_COLOR bg_color = ANSI_BG_DEFAULT;      // default background color
static TEXT_ATTRIB attr = ANSI_BOLD_ON;          // default text attributes
static PROMPT_STYLE prompt_style = COLOR_PROMPT; // default prompt style
static char PS1[MAXLINE + 1] = "% ";
static char PS2[MAXLINE + 1] = "> ";
static char PS1_ROOT[MAXLINE + 1] = "# ";

// Git status information
typedef struct {
    char branch[256];
    int has_changes;
    int has_staged;
    int has_untracked;
    int ahead;
    int behind;
} git_info_t;

static git_info_t git_info = {0};
static time_t last_git_check = 0;
static const int GIT_CACHE_SECONDS = 5; // Cache git status for 5 seconds

// Legacy setprompt functions removed - use theme system instead

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

    sprintf(colors, "%c[%d;%d;%dm", 0x1b, attr, fg_color, bg_color);

    return 0;
}

// Legacy prompt color/attribute functions removed - use theme system instead

/**
 * run_command:
 *      Execute a command and capture its output.
 *      Returns 0 on success, non-zero on failure.
 */
static int run_command(const char *cmd, char *output, size_t output_size) {
    FILE *fp;
    int status;

    if (output) {
        output[0] = '\0';
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    if (output && output_size > 0) {
        if (fgets(output, output_size, fp) != NULL) {
            // Remove trailing newline
            size_t len = strlen(output);
            if (len > 0 && output[len - 1] == '\n') {
                output[len - 1] = '\0';
            }
        }
    }

    status = pclose(fp);
    return WEXITSTATUS(status);
}

/**
 * get_git_branch:
 *      Get the current git branch name.
 */
static int get_git_branch(char *branch, size_t branch_size) {
    return run_command("git branch --show-current", branch, branch_size);
}

/**
 * get_git_status:
 *      Get git repository status information.
 */
static void get_git_status(git_info_t *info) {
    char output[256];

    // Reset info
    memset(info, 0, sizeof(git_info_t));

    // Check if we're in a git repository
    if (run_command("git rev-parse --git-dir", NULL, 0) != 0) {
        return; // Not in a git repository
    }

    // Get branch name
    if (get_git_branch(info->branch, sizeof(info->branch)) != 0) {
        strcpy(info->branch, "unknown");
    }

    // Check for staged changes
    if (run_command("git diff --cached --quiet", NULL, 0) != 0) {
        info->has_staged = 1;
    }

    // Check for unstaged changes
    if (run_command("git diff --quiet", NULL, 0) != 0) {
        info->has_changes = 1;
    }

    // Check for untracked files
    if (run_command("git ls-files --others --exclude-standard", output,
                    sizeof(output)) == 0 &&
        strlen(output) > 0) {
        info->has_untracked = 1;
    }

    // Check ahead/behind status
    if (run_command("git rev-list --count --left-right @{upstream}...HEAD",
                    output, sizeof(output)) == 0) {
        sscanf(output, "%d\t%d", &info->behind, &info->ahead);
    }
}

/**
 * update_git_info:
 *      Update git information with caching.
 */
void update_git_info(void) {
    time_t now = time(NULL);

    // Use cached info if it's recent enough
    if (now - last_git_check < GIT_CACHE_SECONDS) {
        return;
    }

    get_git_status(&git_info);
    last_git_check = now;
}

/**
 * format_git_prompt:
 *      Format git information for prompt display.
 */
void format_git_prompt(char *git_prompt, size_t size) {
    if (strlen(git_info.branch) == 0) {
        git_prompt[0] = '\0';
        return;
    }

    char status_indicators[32] = "";

    // Add status indicators
    if (git_info.has_staged) {
        strcat(status_indicators, "+");
    }
    if (git_info.has_changes) {
        strcat(status_indicators, "*");
    }
    if (git_info.has_untracked) {
        strcat(status_indicators, "?");
    }

    // Format ahead/behind indicators
    char ahead_behind[32] = "";
    if (git_info.ahead > 0 && git_info.behind > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), "↕%d/%d", git_info.ahead,
                 git_info.behind);
    } else if (git_info.ahead > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), "↑%d", git_info.ahead);
    } else if (git_info.behind > 0) {
        snprintf(ahead_behind, sizeof(ahead_behind), "↓%d", git_info.behind);
    }

    // Combine all git information
    if (strlen(status_indicators) > 0 || strlen(ahead_behind) > 0) {
        snprintf(git_prompt, size, " (%s%s%s%s)", git_info.branch,
                 strlen(status_indicators) > 0 ? " " : "", status_indicators,
                 ahead_behind);
    } else {
        snprintf(git_prompt, size, " (%s)", git_info.branch);
    }
}

/**
 * build_prompt:
 *      Builds the user's prompt, either a fancy colored one with
 *      the current working directory or a plain '% or # '.
 */
void build_prompt(void) {
    char prompt[(MAXLINE * 2) + 1] = {'\0'}; // prompt string

    // Phase 3 Target 2: Try theme-aware prompt generation first
    if (config.theme_name && strlen(config.theme_name) > 0 &&
        theme_get_active() != NULL) {

        if (theme_generate_primary_prompt(prompt, sizeof(prompt))) {
            // Successfully generated themed prompt
            symtable_set_global("PS1", prompt);
            return;
        }
        // If theme generation fails, fall back to traditional prompt
    }

    // Traditional prompt generation (fallback)
    char u[_POSIX_LOGIN_NAME_MAX + 1] = {'\0'}; // username
    char h[_POSIX_HOST_NAME_MAX + 1] = {'\0'};  // hostname
    char d[_POSIX_PATH_MAX + 1] = {'\0'};       // current working directory
    char t[64] = {'\0'};                        // local time

    // Build prompt color sequence
    if (prompt_style == COLOR_PROMPT || prompt_style == FANCY_PROMPT ||
        prompt_style == PRO_PROMPT || prompt_style == GIT_PROMPT) {
        // Build text colors, and then the formatted prompt string
        if (build_colors() > 0) {
            goto fancy_error;
        }
    }

    // Build a prompt string based on style set
    if (prompt_style == FANCY_PROMPT || prompt_style == PRO_PROMPT ||
        prompt_style == GIT_PROMPT) {
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

        time_t tm = time(NULL);
        struct tm *lt = localtime(&tm);
        if (!(strftime(t, sizeof(t), "%c", lt))) {
            error_return("error: `build_prompt`: strftime");
            goto fancy_error;
        }
    }

    if (prompt_style == FANCY_PROMPT) {
        sprintf(prompt, "%s%s@%s in %s%s%s", colors, u, h, d, RESET,
                (getuid() > 0) ? PS1 : PS1_ROOT);
    } else if (prompt_style == PRO_PROMPT) {
        sprintf(prompt, "%s%s@%s\tin\t%s\t%s\n\r%s%s", colors, u, h, d, t,
                (getuid() > 0) ? PS1 : PS1_ROOT, RESET);
    } else if (prompt_style == GIT_PROMPT) {
        char git_prompt[256] = "";
        format_git_prompt(git_prompt, sizeof(git_prompt));
        sprintf(prompt, "%s%s@%s in %s%s%s%s", colors, u, h, d, git_prompt,
                RESET, (getuid() > 0) ? PS1 : PS1_ROOT);
    } else if (prompt_style == COLOR_PROMPT) {
        sprintf(prompt, "%s%s%s", colors, (getuid() > 0) ? PS1 : PS1_ROOT,
                RESET);
    } else {
    fancy_error:
        sprintf(prompt, "%s", (getuid() > 0) ? PS1 : PS1_ROOT);
    }

    // Set the prompt environment variables
    symtable_set_global("PS1", prompt);

    // Phase 3 Target 2: Generate themed PS2 if theme is active
    char ps2_prompt[MAXLINE + 1] = {'\0'};
    if (config.theme_name && strlen(config.theme_name) > 0 &&
        theme_get_active() != NULL) {

        if (theme_generate_secondary_prompt(ps2_prompt, sizeof(ps2_prompt))) {
            // Successfully generated themed PS2
            symtable_set_global("PS2", ps2_prompt);
        } else {
            // Fall back to default PS2
            symtable_set_global("PS2", PS2);
        }
    } else {
        // Use traditional PS2
        symtable_set_global("PS2", PS2);
    }

    if (colors) {
        free(colors);
    }
    colors = NULL;
}
