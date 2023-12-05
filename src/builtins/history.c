#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "exec.h"
#include "errors.h"
#include "history.h"
#include "lusush.h"
#include "strings.h"
#include "linenoise.h"

static const char *histfn = ".lusushist"; // The history filename

static char *__get_histfilename(void)
{
    char *fn = NULL;
    char *home = get_shell_varp("HOME", "");
    if (!home) {
        error_return("history_save");
        return NULL;
    }

    fn = alloc_str(MAXLINE, true);
    if (!fn) {
        error_return("__get_histfilename");
        return NULL;
    }

    sprintf(fn, "%s/%s", home, histfn);

    return fn;
}

/**
 * init_history:
 *      Initialization code for history.
 */
void init_history(void)
{
    char *fn = __get_histfilename();
    int fd = 0;

    // Make sure the history file exists with proper permissions
    fd = open(fn, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd > 0)
        close(fd);
    chmod(fn, S_IRUSR | S_IWUSR);

    // Set maximum lines of history
    linenoiseHistorySetMaxLen(100);

    // Read the history file
    if (linenoiseHistoryLoad(fn) != 0)
        error_return("init_history");

    free_str(fn);
}

/**
 * history_add:
 *      Add a line of history to the linenoise history buffer.
 */
void history_add(const char *line)
{
    if (!line)
        return;

    linenoiseHistoryAdd(line);
}

/**
 * history_save:
 *      Call write_history on a file name.
 */
void history_save(void)
{
    char *fn = __get_histfilename();
    linenoiseHistorySave(fn);
    free_str(fn);
}

/**
 * history_print:
 *      Print an indexed list of all history entries.
 */
void history_print(void)
{
    linenoiseHistoryPrint();
}

/**
 * history_lookup:
 *      Parse a string for the index of a history entry, then return that enrty.
 */
char *history_lookup(const char *s)
{
    char *line = NULL;
    int idx = -1;

    idx = atoi(s);
    if (idx < 1) {
        error_message("history_lookup: history index must be a positive value");
        return NULL;
    }
    idx -= 2;
    if (idx < 0)
        idx = 0;

    line = linenoiseHistoryGet(idx);
    if (!line) {
        error_return("history_lookup");
        return NULL;
    }

    return line;
}

/**
 * history_usage:
 *      Print usage information for the history command.
 */
void history_usage(void)
{
    fprintf(stderr, "usage:\n"
            "history\t\t(print the command history)\n"
            "history index\t(execute command in history)\n");
}
