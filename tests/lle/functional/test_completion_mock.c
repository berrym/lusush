/**
 * Mock Shell Data for Completion Testing
 *
 * Provides minimal mock implementations of shell data structures
 * so completion tests can run standalone without full lush.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Mock builtin structure */
typedef struct {
    char *name;
    void *function;
} builtin_t;

/* Mock builtins array */
static builtin_t mock_builtins[] = {
    {"cd", NULL}, {"echo", NULL}, {"exit", NULL}, {NULL, NULL}};

builtin_t *builtins = mock_builtins;
int builtins_count = 3;

/* Mock aliases hashtable (NULL for now) */
void *aliases = NULL;

/* Mock lookup_alias function */
char *lookup_alias(const char *name) {
    (void)name;
    return NULL; /* No aliases in mock */
}

/* Mock environ for variable completion */
char **environ = NULL;
