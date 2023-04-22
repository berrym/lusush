#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exec.h"
#include "errors.h"
#include "history.h"
#include "lusush.h"
#include "strings.h"
#if defined(USING_EDITLINE)
#include <readline.h>
#elif defined (USING_READLINE)
#include <readline/history.h>
#endif

const char *histfn = ".lusushist"; // The history filename

#if defined(USING_READLINE)

/**
 * init_history:
 *      Initialization code for the history list.
 */
void init_history(void)
{
    int fd = 0;
    char *fn = NULL;
    char *home = get_shell_varp("HOME", "");

    using_history();

    fn = alloc_str(MAXLINE, true);
    sprintf(fn, "%s/%s", home, histfn);

    // Make sure the history file exists with proper permissions
    fd = open(fn, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd > 0)
        close(fd);

    // Read the history file
    if (read_history(fn) != 0)
        error_return("init_history");

    free_str(fn);
}

/**
 * read_history_file:
 *      Read the history file adding entries to the history list.
 */
int read_history_file(const char *fn)
{
    FILE *fp = NULL;
    char buf[MAXLINE + 1] = { '\0' };

    if (!fn || !*fn)
        return 1;

    // Open the history file for reading
    if ((fp = fopen(fn, "r")) == NULL) {
        error_return("read_history");
        return 1;
    }

    while (fgets(buf, MAXLINE, fp) != NULL)
        add_history(buf);

    // Check the stream for errors
    if (ferror(fp)) {
        error_return("read_history");
        return 1;
    }

    fclose(fp);

    return 0;
}

/**
 * print_history:
 *      Print an indexed list of all history entries.
 */
void print_history(void)
{
    HIST_ENTRY *h = NULL;

    for (size_t i = 0; (h = history_get(history_base + i)); i++)
        printf("%-5zu\t%s\n", i + 1, h->line);
}

/**
 * destroy_history:
 *      Free all history entries then free the list itself.
 */
void destroy_history(void)
{
    HIST_ENTRY *h = NULL;

    for (size_t i = 0; (h = history_get(history_base + i)); i++) {
        free(h);
        h = NULL;
    }
}

/**
 * lookup_history:
 *      Parse a string for the index of a history entry, then return that enrty.
 */
char *lookup_history(char *s)
{
    HIST_ENTRY *h = NULL;
    ssize_t i = -1;

    sscanf(s, "%zd", &i);
    if (i < 1) {
        error_message("lookup_history: history number must be positive");
        return NULL;
    }

    h = history_get(history_base + (i - 1));
    if (!h) {
        error_message("lookup_history: unable to find history entry");
        return NULL;
    }

    return h->line;
}

#else

// Structure of a history entry
typedef struct hist_entry_s {
    char *line;
    struct hist_entry_s *next;
} hist_entry_s;

// Structure of a history list
typedef struct hist_list_s {
    size_t len;                 // Number of entries in the list
    hist_entry_s *head;         // First entry in the list
} hist_list_s;

hist_list_s *hist_list = NULL;  // The history list

/**
 * init_history:
 *      Initialization code for the history list.
 */
void init_history(void)
{
    int fd = 0;
    char *fn = NULL;
    char *home = get_shell_varp("HOME", "");
    size_t len = strlen(home) + strlen(histfn) + 1;

    // Create a string holding the history filename
    fn = alloc_str(len + 1, true);
    snprintf(fn, len, "%s/%s", home, histfn);

    // Make sure the history file exists with proper permissions
    fd = open(fn, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd > 0)
        close(fd);

    // Allocate room for the history list
    hist_list = calloc(1, sizeof(hist_list_s));
    if (!hist_list) {
        error_return("init_history");
        return;
    }

    hist_list->len = 0;         // Set history length to zero

    // Read any stored history
    if (read_history_file(fn) != 0)
        error_message("init_history: unable to read history file");

    free_str(fn);
}

/**
 * read_history_file:
 *      Read the history file adding entries to the history list.
 */
int read_history_file(const char *fn)
{
    FILE *fp = NULL;
    char buf[MAXLINE + 1] = { '\0' };

    if (!fn || !*fn)
        return 1;

    // Open the history file for reading
    if ((fp = fopen(fn, "r")) == NULL) {
        error_return("read_history");
        return 1;
    }

    while (fgets(buf, MAXLINE, fp) != NULL)
        add_history(buf);

    // Check the stream for errors
    if (ferror(fp)) {
        error_return("read_history");
        return 1;
    }

    fclose(fp);

    return 0;
}

/**
 * destroy_history:
 *      Free all history entries then free the list itself.
 */
void destroy_history(void)
{
    hist_entry_s *curr = NULL, *next = NULL;

    curr = hist_list->head;
    next = NULL;

    while (curr) {
        if (curr->line)
            free_str(curr->line);

        next = curr->next;
        free(curr);
        curr = next;
    }

    if (hist_list) {
        free(hist_list);
        hist_list = NULL;
    }
}

/**
 * add_history:
 *      Add a line of input to history.
 */
void add_history(const char *s)
{
    if (!hist_list) {
        error_message("add_to_history: history not initialized");
        return;
    }

    if (!s || !*s)
        return;

    // Create storage for a history entry and copy s.
    hist_entry_s *new = calloc(1, sizeof(hist_entry_s));
    new->line = strdup(s);
    if (!new->line) {
        error_return("add_to_history");
        return;
    }

    // Update the histpry list
    if (!hist_list->head) {
        hist_list->head = new;
        hist_list->tail = new;
    } else {
        hist_list->tail->next = new;
        hist_list->tail = new;
    }

    // Increase the number of history entries
    hist_list->len++;
}

/**
 * print_history:
 *      Print an indexed list of all history entries.
 */
void print_history(void)
{
    hist_entry_s *h = hist_list->head;

    for (size_t i = 0; i < hist_list->len; i++) {
        printf("%-5zu\t%s\n", i + 1, h->line);
        h = h->next;
    }
}

/**
 * write_history:
 *      Write history to a file.
 */
static void write_history(const char *fn)
{
    hist_entry_s *h = NULL;
    FILE *fp = NULL;

    if (!hist_list)
        return;

    h = hist_list->head;
    fp = NULL;

    // Open the history file for writing
    if ((fp = fopen(fn, "w")) == NULL) {
        error_return("write_history");
        return;
    }

    // Write each history item to the history file
    while (h) {
        fprintf(fp, "%s", h->line);
        h = h->next;
    }

    // Close the file stream
    fclose(fp);
}

/**
 * lookup_history:
 *      Parse a string for the index of a history entry, then return that enrty.
 */
char *lookup_history(char *s)
{
    hist_entry_s *h = hist_list->head;
    ssize_t i = -1;
    sscanf(s, "%zd", &i);

    if (i < 1) {
        error_message("lookup_history: history number must be a positive integer");
        return NULL;
    }

    for (size_t k = 0; k < i; k++)
        h = h->next;

    if (!h) {
        error_message("lookup_history: unable to find history entry");
        return NULL;
    }

    return h->line;
}

#endif

/**
 * save_history:
 *      Call write_history on a file name.
 */
void save_history(void)
{
    char *fn = NULL;
    char *home = get_shell_varp("HOME", "");

    fn = alloc_str(MAXLINE, true);
    sprintf(fn, "%s/%s", home, histfn);

    write_history(fn);
}

/**
 * history_usage:
 *      Print usage information for the history command.
 */
void history_usage(void)
{
    fprintf(stderr, "usage:\n"
            "history\t(print the command history\n"
            "history number\t(execute command in history\n");
}
