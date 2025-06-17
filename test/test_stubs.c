#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Minimal in-memory alias table for testing
#define MAX_ALIASES 64
#define MAX_ALIAS_NAME 64
#define MAX_ALIAS_VALUE 256

typedef struct {
    char name[MAX_ALIAS_NAME];
    char value[MAX_ALIAS_VALUE];
    bool in_use;
} alias_entry_t;

static alias_entry_t alias_table[MAX_ALIASES];

// Stub for global parsing_alias
bool parsing_alias = false;

// Stubs for symtable functions
void *get_symtable_entry(const char *name) { return NULL; }
void *add_to_symtable(const char *name, const char *val) { return NULL; }
void *remove_from_symtable(const char *name) { return NULL; }
void *symtable_entry_setval(void *entry, const char *val) { return NULL; }

// Stubs for shell variable functions
int get_shell_vari(char *name, int def) { return def; }

// Stubs for globbing
int has_glob_chars(const char *s) { return 0; }
char **get_filename_matches(const char *pattern, int *count) { *count = 0; return NULL; }

// Stubs for arithmetic expansion
char *arithm_expand(char *expr) { return NULL; }

// Stubs for pattern matching
int match_suffix(const char *a, const char *b) { return 0; }
int match_prefix(const char *a, const char *b) { return 0; }

// Stubs for symtable creation
void *new_symtable(void) { return NULL; }

// Minimal alias lookup
char *lookup_alias(const char *name) {
    for (int i = 0; i < MAX_ALIASES; ++i) {
        if (alias_table[i].in_use && strcmp(alias_table[i].name, name) == 0) {
            return alias_table[i].value;
        }
    }
    return NULL;
}

// Minimal alias management
void init_aliases(void) {
    for (int i = 0; i < MAX_ALIASES; ++i) {
        alias_table[i].in_use = false;
    }
}

bool set_alias(const char *key, const char *val) {
    for (int i = 0; i < MAX_ALIASES; ++i) {
        if (alias_table[i].in_use && strcmp(alias_table[i].name, key) == 0) {
            strncpy(alias_table[i].value, val, MAX_ALIAS_VALUE - 1);
            alias_table[i].value[MAX_ALIAS_VALUE - 1] = '\0';
            return true;
        }
    }
    for (int i = 0; i < MAX_ALIASES; ++i) {
        if (!alias_table[i].in_use) {
            strncpy(alias_table[i].name, key, MAX_ALIAS_NAME - 1);
            alias_table[i].name[MAX_ALIAS_NAME - 1] = '\0';
            strncpy(alias_table[i].value, val, MAX_ALIAS_VALUE - 1);
            alias_table[i].value[MAX_ALIAS_VALUE - 1] = '\0';
            alias_table[i].in_use = true;
            return true;
        }
    }
    return false;
}

void free_aliases(void) {
    for (int i = 0; i < MAX_ALIASES; ++i) {
        alias_table[i].in_use = false;
    }
}
