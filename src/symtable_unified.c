/**
 * Unified Symbol Table Convenience Interface Implementation
 * 
 * This file implements the unified symbol table interface that wraps the modern
 * symtable API for easier use throughout the codebase. It provides simple,
 * consistent functions for common symbol table operations.
 */

#include "../include/symtable_unified.h"
#include "../include/symtable_modern.h"
#include "../include/errors.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// External reference to global manager (defined in symtable.c for now)
extern symtable_manager_t *get_global_symtable_manager(void);

// Get access to the global symbol table manager
symtable_manager_t *symtable_get_global_manager(void) {
    return get_global_symtable_manager();
}

// Basic variable operations (global scope)
char *symtable_get_global(const char *name) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (!manager || !name) {
        return NULL;
    }
    return symtable_get_var(manager, name);
}

char *symtable_get_global_default(const char *name, const char *default_value) {
    char *value = symtable_get_global(name);
    if (value && strlen(value) > 0) {
        return value;
    }
    return (char *)default_value;
}

int symtable_set_global(const char *name, const char *value) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (!manager || !name) {
        return -1;
    }
    return symtable_set_var(manager, name, value ? value : "", SYMVAR_NONE);
}

bool symtable_exists_global(const char *name) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (!manager || !name) {
        return false;
    }
    return symtable_var_exists(manager, name);
}

int symtable_unset_global(const char *name) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (!manager || !name) {
        return -1;
    }
    return symtable_unset_var(manager, name);
}

// Integer variable operations
int symtable_get_global_int(const char *name, int default_value) {
    char *value = symtable_get_global(name);
    if (value && strlen(value) > 0) {
        return atoi(value);
    }
    return default_value;
}

int symtable_set_global_int(const char *name, int value) {
    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%d", value);
    return symtable_set_global(name, value_str);
}

// Boolean variable operations
bool symtable_get_global_bool(const char *name, bool default_value) {
    char *value = symtable_get_global(name);
    if (value) {
        return (strcmp(value, "1") == 0 || 
                strcmp(value, "true") == 0 || 
                strcmp(value, "yes") == 0 ||
                strcmp(value, "on") == 0 ||
                atoi(value) != 0);
    }
    return default_value;
}

int symtable_set_global_bool(const char *name, bool value) {
    return symtable_set_global(name, value ? "1" : "0");
}

// Export/environment operations
int symtable_export_global(const char *name) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (!manager || !name) {
        return -1;
    }
    return symtable_export_var(manager, name);
}

int symtable_unexport_global(const char *name) {
    // TODO: symtable_unexport_var not implemented yet in modern API
    // For now, just return success
    (void)name;
    return 0;
}

// Special variable operations
int symtable_set_special_global(const char *name, const char *value) {
    // TODO: symtable_set_special_var not implemented yet in modern API
    // For now, use regular variable setting
    return symtable_set_global(name, value);
}

char *symtable_get_special_global(const char *name) {
    // TODO: symtable_get_special_var not implemented yet in modern API
    // For now, use regular variable getting
    return symtable_get_global(name);
}

// Read-only variable operations
int symtable_set_readonly_global(const char *name, const char *value) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (!manager || !name) {
        return -1;
    }
    return symtable_set_var(manager, name, value ? value : "", SYMVAR_READONLY);
}

// Debugging and introspection
void symtable_debug_dump_global_scope(void) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (manager) {
        symtable_dump_scope(manager, SCOPE_GLOBAL);
    }
}

void symtable_debug_dump_all_scopes(void) {
    symtable_manager_t *manager = symtable_get_global_manager();
    if (manager) {
        symtable_dump_all_scopes(manager);
    }
}

size_t symtable_count_global_vars(void) {
    // TODO: symtable_count_vars not implemented yet in modern API
    // For now, return a placeholder value
    return 0;
}

// Environment array operations
char **symtable_get_environment_array(void) {
    // TODO: symtable_get_environ not implemented yet in modern API
    // For now, use system environ as fallback
    extern char **environ;
    return environ;
}

void symtable_free_environment_array(char **env) {
    // TODO: symtable_free_environ not implemented yet in modern API
    // For system environ, we don't need to free it
    (void)env;
}