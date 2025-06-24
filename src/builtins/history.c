#include "../../include/history.h"

#include "../../include/errors.h"
#include "../../include/linenoise/linenoise.h"
#include "../../include/lusush.h"
#include "../../include/strings.h"
#include "../../include/symtable_unified.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *histfn = ".lusushist"; // The history filename

static char *__get_histfilename(void) {
    char *fn = NULL;
    char *home = symtable_get_global_default("HOME", "");
    if (home == NULL) {
        error_return("error: `history_save`");
        return NULL;
    }

    fn = alloc_str(MAXLINE + 1, true);
    if (fn == NULL) {
        error_return("error: `__get_histfilename`");
        return NULL;
    }

    sprintf(fn, "%s/%s", home, histfn);

    return fn;
}

/**
 * init_history:
 *      Initialization code for history.
 */
void init_history(void) {
    char *fn = __get_histfilename();
    int fd = 0;

    // Make sure the history file exists with proper permissions
    fd = open(fn, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd > 0) {
        close(fd);
    }
    chmod(fn, S_IRUSR | S_IWUSR);

    // Set maximum lines of history
    linenoiseHistorySetMaxLen(1000);

    // Read the history file
    if (linenoiseHistoryLoad(fn) != 0) {
        error_return("error: `init_history`");
    }

    linenoiseHistoryRemoveDups();

    free_str(fn);
}

/**
 * history_add:
 *      Add a line of history to the linenoise history buffer.
 */
void history_add(const char *line) {
    if (line == NULL) {
        return;
    }

    linenoiseHistoryAdd(line);
}

/**
 * history_save:
 *      Call write_history on a file name.
 */
void history_save(void) {
    char *fn = __get_histfilename();
    linenoiseHistorySave(fn);
    free_str(fn);
}

/**
 * history_print:
 *      Print an indexed list of all history entries.
 */
void history_print(void) { linenoiseHistoryPrint(); }

/**
 * history_lookup:
 *      Parse a string for the index of a history entry, then return that enrty.
 */
char *history_lookup(const char *s) {
    char *line = NULL;
    int idx = -1;

    idx = atoi(s);
    if (idx < 1) {
        error_message("error: `history_lookup`: history index must be a "
                      "positive value");
        return NULL;
    }
    idx--;

    line = linenoiseHistoryGet(idx);
    if (line == NULL) {
        error_return("error: `history_lookup`");
        return NULL;
    }

    return line;
}

/**
 * history_usage:
 *      Print usage information for the history command.
 */
void history_usage(void) {
    fprintf(stderr, "usage:\n"
                    "history\t\t(print the command history)\n"
                    "history index\t(execute command in history)\n");
}
