#define _POSIX_C_SOURCE 200809L

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "history.h"
#include "lusush.h"
#include "strings.h"

char *histfn = NULL;

#if defined(USING_EDITLINE_HISTORY) || defined(USING_READLINE_HISTORY)

struct char *hist_list = NULL;

void init_history(void)
{
    // Read the history file
    if (read_history(hist_file_name()) != 0)
        return;
}

void print_history(void)
{
    size_t i = 0;
    HIST_ENTRY *h = NULL;

    for (i = 0; (h = history_get(i + history_base)); i++)
        printf("%5zu\t%s\n", i + 1, h->line);
}

void free_history_list(void)
{
    size_t i = 0;
    HIST_ENTRY *h = NULL;

    for (i = 0; (h = history_get(i + history_base)); i++)
        free(h);
}

#else

struct histable *hist_list = NULL;

void init_history(void)
{
    char *home = get_shell_varp("HOME", "");
    size_t len = strnlen(home, MAXLINE) + 12;
    char *fn = NULL;

    fn = alloc_string(len + 1, true);

    snprintf(fn, len, "%s/.lusushist", home);
    histfn = fn;

    hist_list = calloc(1, sizeof(struct histable));
    hist_list->len = 0;
}

void free_history_list(void)
{
    struct histentry *curr, *next;

    curr = hist_list->head;
    next = NULL;

    while (curr) {
        if (curr->cmd)
            free(curr->cmd);

        next = curr->next;

        if (curr)
            free(curr);

        curr = next;
    }

    if (hist_list)
        free(hist_list);

    if (histfn)
        free(histfn);
}

void add_to_history(const char *s)
{
    if (!hist_list) {
        error_message("add_history: history not initialized");
        return;
    }

    if (!s || !*s)
        return;

    if (strnlen(s, MAXLINE) == 0)
        return;

    struct histentry *new = calloc(1, sizeof(struct histentry));
    new->cmd = alloc_string(MAXLINE + 1, true);

    strncpy(new->cmd, s, strnlen(s, MAXLINE) + 1);
    null_replace_newline(new->cmd);

    if (!hist_list->head) {
        hist_list->head = new;
        hist_list->tail = new;
    } else {
        hist_list->tail->next = new;
        hist_list->tail = new;
    }

    hist_list->len++;
}

void print_history(void)
{
    size_t i = 0;
    struct histentry *entry = hist_list->head;

    while (entry) {
        printf("%5zu\t%s\n", i + 1, entry->cmd);
        entry = entry->next;
        i++;
    }
}

void write_history(const char *fn)
{
    if (!hist_list)
        return;

    struct histentry *entry = hist_list->head;
    FILE *fp = NULL;

    // Open the history file for writing
    if ((fp = fopen(fn, "w")) == NULL) {
        error_return("write_history: fopen");
        return;
    }

    // Write each history item as a new line
    while (entry) {
        fprintf(fp, "%s\n", entry->cmd);
        entry = entry->next;
    }

    // Close the file stream
    fclose(fp);
}

#endif

const char *hist_file_name()
{
    return histfn;
}

int read_history(const char *fn)
{
    if (!histfn || !*histfn)
        return 1;

    int fd = open(histfn, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd > 0)
        close(fd);

    FILE *fp = NULL;            // file stream pointer
    char buf[MAXLINE + 1] = { '\0' };

    // Open the history file for reading
    if ((fp = fopen(fn, "r")) == NULL) {
        error_return("read_hist_file: fopen");
        return 1;
    }

    while (fgets(buf, MAXLINE, fp) != NULL)
        add_to_history(buf);

    // Check the stream for errors
    if (ferror(fp)) {
        error_return("read_history");
        return 1;
    }

    fclose(fp);

    return 0;
}

void save_history(void)
{
    write_history(histfn);
}
