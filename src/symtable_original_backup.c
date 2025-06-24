/**
 * Comprehensive Symbol Table Implementation for Lusush Shell
 *
 * This file provides a complete, unified symbol table system that combines:
 * - Modern POSIX-compliant variable scoping
 * - Convenient high-level API functions
 * - Essential system interfaces
 * - Legacy compatibility for string management
 */

#include "../include/symtable.h"

#include "../include/errors.h"
#include "../include/node.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

// Global modern symtable manager
static symtable_manager_t *global_manager = NULL;

// Legacy compatibility structures
static symtable_t dummy_symtable = {0, NULL, NULL};

// Default hash table size and limits
#define DEFAULT_HASH_SIZE 64
#define MAX_SCOPE_DEPTH 256

// ============================================================================
// INTERNAL UTILITIES
// ============================================================================

// Simple hash function for variable names
static size_t hash_name(const char *name, size_t table_size) {
    size_t hash = 5381;
    while (*name) {
        hash = ((hash << 5) + hash) + *name++;
    }
    return hash % table_size;
}

// Find variable in scope chain
static symvar_t *find_var(symtable_scope_t *scope, const char *name) {
    if (!scope || !name) {
        return NULL;
    }

    while (scope) {
        size_t hash = hash_name(name, scope->hash_size);
        symvar_t *var = scope->vars[hash];

        while (var) {
            if (strcmp(var->name, name) == 0 && !(var->flags & SYMVAR_UNSET)) {
                return var;
            }
            var = var->next;
        }

        scope = scope->parent;
    }

    return NULL;
}

// Free all variables in a scope
static void free_scope_vars(symtable_scope_t *scope) {
    if (!scope || !scope->vars) {
        return;
    }

    for (size_t i = 0; i < scope->hash_size; i++) {
        symvar_t *var = scope->vars[i];
        while (var) {
            symvar_t *next = var->next;
            free(var->name);
            free(var->value);
            free(var);
            var = next;
        }
    }
    free(scope->vars);
}

// ============================================================================
// CORE MODERN API IMPLEMENTATION
// ============================================================================

// Create new symbol table manager
symtable_manager_t *symtable_manager_new(void) {
    symtable_manager_t *manager = calloc(1, sizeof(symtable_manager_t));
    if (!manager) {
        return NULL;
    }

    // Create global scope
    symtable_scope_t *global = calloc(1, sizeof(symtable_scope_t));
    if (!global) {
        free(manager);
        return NULL;
    }

    global->scope_type = SCOPE_GLOBAL;
    global->level = 0;
    global->hash_size = DEFAULT_HASH_SIZE;
    global->vars = calloc(DEFAULT_HASH_SIZE, sizeof(symvar_t *));
    global->parent = NULL;
    global->scope_name = strdup("global");

    if (!global->vars || !global->scope_name) {
        free(global->vars);
        free(global->scope_name);
        free(global);
        free(manager);
        return NULL;
    }

    manager->current_scope = global;
    manager->global_scope = global;
    manager->max_scope_level = 0;
    manager->debug_mode = false;

    return manager;
}

// Free symbol table manager
void symtable_manager_free(symtable_manager_t *manager) {
    if (!manager) {
        return;
    }

    // Pop all scopes to free memory
    while (manager->current_scope &&
           manager->current_scope != manager->global_scope) {
        symtable_pop_scope(manager);
    }

    // Free global scope
    if (manager->global_scope) {
        free_scope_vars(manager->global_scope);
        free(manager->global_scope->scope_name);
        free(manager->global_scope);
    }

    free(manager);
}

// Set debug mode
void symtable_manager_set_debug(symtable_manager_t *manager, bool debug) {
    if (manager) {
        manager->debug_mode = debug;
    }
}

// Push new scope
int symtable_push_scope(symtable_manager_t *manager, scope_type_t type,
                        const char *name) {
    if (!manager || !name) {
        return -1;
    }

    if (manager->current_scope->level >= MAX_SCOPE_DEPTH) {
        if (manager->debug_mode) {
            fprintf(stderr, "ERROR: Maximum scope depth exceeded\n");
        }
        return -1;
    }

    symtable_scope_t *new_scope = calloc(1, sizeof(symtable_scope_t));
    if (!new_scope) {
        return -1;
    }

    new_scope->scope_type = type;
    new_scope->level = manager->current_scope->level + 1;
    new_scope->hash_size = DEFAULT_HASH_SIZE;
    new_scope->vars = calloc(DEFAULT_HASH_SIZE, sizeof(symvar_t *));
    new_scope->parent = manager->current_scope;
    new_scope->scope_name = strdup(name);

    if (!new_scope->vars || !new_scope->scope_name) {
        free(new_scope->vars);
        free(new_scope->scope_name);
        free(new_scope);
        return -1;
    }

    manager->current_scope = new_scope;
    if (new_scope->level > manager->max_scope_level) {
        manager->max_scope_level = new_scope->level;
    }

    if (manager->debug_mode) {
        printf("DEBUG: Pushed scope '%s' (level %zu)\n", name,
               new_scope->level);
    }

    return 0;
}

// Pop current scope
int symtable_pop_scope(symtable_manager_t *manager) {
    if (!manager || !manager->current_scope ||
        manager->current_scope == manager->global_scope) {
        return -1; // Can't pop global scope
    }

    symtable_scope_t *old_scope = manager->current_scope;
    manager->current_scope = old_scope->parent;

    if (manager->debug_mode) {
        printf("DEBUG: Popped scope '%s' (level %zu)\n", old_scope->scope_name,
               old_scope->level);
    }

    // Free the old scope
    free_scope_vars(old_scope);
    free(old_scope->scope_name);
    free(old_scope);

    return 0;
}

// Get current scope level
size_t symtable_current_level(symtable_manager_t *manager) {
    return manager ? manager->current_scope->level : 0;
}

// Get current scope name
const char *symtable_current_scope_name(symtable_manager_t *manager) {
    return manager ? manager->current_scope->scope_name : "unknown";
}

// Set variable
int symtable_set_var(symtable_manager_t *manager, const char *name,
                     const char *value, symvar_flags_t flags) {
    if (!manager || !name) {
        return -1;
    }

    symtable_scope_t *scope = manager->current_scope;
    size_t hash = hash_name(name, scope->hash_size);

    // Check if variable already exists in current scope
    symvar_t *var = scope->vars[hash];
    while (var) {
        if (strcmp(var->name, name) == 0) {
            // Update existing variable
            if (var->flags & SYMVAR_READONLY) {
                if (manager->debug_mode) {
                    fprintf(stderr,
                            "ERROR: Cannot modify read-only variable '%s'\n",
                            name);
                }
                return -1;
            }

            free(var->value);
            var->value = strdup(value ? value : "");
            var->flags = (var->flags & ~SYMVAR_UNSET) | flags;

            if (manager->debug_mode) {
                printf("DEBUG: Updated variable '%s' = '%s'\n", name,
                       var->value);
            }
            return 0;
        }
        var = var->next;
    }

    // Create new variable
    var = calloc(1, sizeof(symvar_t));
    if (!var) {
        return -1;
    }

    var->name = strdup(name);
    var->value = strdup(value ? value : "");
    var->type = SYMVAR_STRING;
    var->flags = flags;
    var->scope_level = scope->level;

    if (!var->name || !var->value) {
        free(var->name);
        free(var->value);
        free(var);
        return -1;
    }

    // Insert at head of hash chain
    var->next = scope->vars[hash];
    scope->vars[hash] = var;

    if (manager->debug_mode) {
        printf("DEBUG: Created variable '%s' = '%s' in scope '%s'\n", name,
               value ? value : "", scope->scope_name);
    }

    return 0;
}

// Set local variable
int symtable_set_local_var(symtable_manager_t *manager, const char *name,
                           const char *value) {
    return symtable_set_var(manager, name, value, SYMVAR_LOCAL);
}

// Set global variable
int symtable_set_global_var(symtable_manager_t *manager, const char *name,
                            const char *value) {
    if (!manager || !name) {
        return -1;
    }

    symtable_scope_t *old_scope = manager->current_scope;
    manager->current_scope = manager->global_scope;
    int result = symtable_set_var(manager, name, value, SYMVAR_NONE);
    manager->current_scope = old_scope;

    return result;
}

// Get variable value
char *symtable_get_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return NULL;
    }

    symvar_t *var = find_var(manager->current_scope, name);
    return var ? var->value : NULL;
}

// Check if variable exists
bool symtable_var_exists(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return false;
    }

    symvar_t *var = find_var(manager->current_scope, name);
    return var && !(var->flags & SYMVAR_UNSET);
}

// Unset variable
int symtable_unset_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return -1;
    }

    symvar_t *var = find_var(manager->current_scope, name);
    if (var) {
        var->flags |= SYMVAR_UNSET;
        free(var->value);
        var->value = strdup("");

        if (manager->debug_mode) {
            printf("DEBUG: Unset variable '%s'\n", name);
        }
        return 0;
    }

    return -1;
}

// Export variable
int symtable_export_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return -1;
    }

    symvar_t *var = find_var(manager->current_scope, name);
    if (var) {
        var->flags |= SYMVAR_EXPORTED;
        if (manager->debug_mode) {
            printf("DEBUG: Exported variable '%s'\n", name);
        }
        return 0;
    }

    return -1;
}

// Dump scope
void symtable_dump_scope(symtable_manager_t *manager, scope_type_t scope_type) {
    if (!manager) {
        return;
    }

    const char *type_names[] = {"GLOBAL", "FUNCTION", "LOOP", "SUBSHELL",
                                "CONDITIONAL"};
    symtable_scope_t *scope = manager->current_scope;

    // Find the scope of the requested type
    while (scope && scope->scope_type != scope_type) {
        scope = scope->parent;
    }

    if (!scope) {
        printf("No scope of type %s found\n", type_names[scope_type]);
        return;
    }

    printf("=== %s SCOPE '%s' (level %zu) ===\n", type_names[scope_type],
           scope->scope_name, scope->level);

    for (size_t i = 0; i < scope->hash_size; i++) {
        symvar_t *var = scope->vars[i];
        while (var) {
            if (!(var->flags & SYMVAR_UNSET)) {
                printf("  %s = '%s'", var->name, var->value);
                if (var->flags & SYMVAR_EXPORTED) {
                    printf(" [exported]");
                }
                if (var->flags & SYMVAR_READONLY) {
                    printf(" [readonly]");
                }
                if (var->flags & SYMVAR_LOCAL) {
                    printf(" [local]");
                }
                printf("\n");
            }
            var = var->next;
        }
    }
}

// Dump all scopes
void symtable_dump_all_scopes(symtable_manager_t *manager) {
    if (!manager) {
        return;
    }

    symtable_scope_t *scope = manager->current_scope;
    while (scope) {
        symtable_dump_scope(manager, scope->scope_type);
        scope = scope->parent;
    }
}

// ============================================================================
// CONVENIENCE API IMPLEMENTATION
// ============================================================================

// Get access to the global symbol table manager
symtable_manager_t *symtable_get_global_manager(void) { return global_manager; }

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
        return (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
                strcmp(value, "yes") == 0 || strcmp(value, "on") == 0 ||
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

// ============================================================================
// SYSTEM INTERFACE IMPLEMENTATION
// ============================================================================

// Initialize the symbol table system
void init_symtable(void) {
    if (!global_manager) {
        global_manager = symtable_manager_new();
        if (!global_manager) {
            error_abort("Failed to initialize symbol table");
        }
    }
}

// Free the global symbol table
void free_global_symtable(void) {
    if (global_manager) {
        symtable_manager_free(global_manager);
        global_manager = NULL;
    }
}

// Set exit status special variable
void set_exit_status(int status) {
    if (global_manager) {
        char status_str[16];
        snprintf(status_str, sizeof(status_str), "%d", status);
        symtable_set_var(global_manager, "?", status_str, SYMVAR_NONE);
    }
}

// Get environment array for exec functions
char **get_environ_array(void) {
    // For now, return system environ until symtable_get_environ is implemented
    extern char **environ;
    return environ;
}

// Free environment array
void free_environ_array(char **env) {
    // Don't free system environ
    (void)env;
}

// ============================================================================
// LEGACY COMPATIBILITY IMPLEMENTATION
// ============================================================================

// Add entry to symbol table (legacy interface)
symtable_entry_t *add_to_symtable(char *name) {
    if (!global_manager || !name) {
        return NULL;
    }

    // Set variable with empty value if it doesn't exist
    if (!symtable_var_exists(global_manager, name)) {
        symtable_set_var(global_manager, name, "", SYMVAR_NONE);
    }

    // Return a dummy pointer for compatibility
    // Legacy code just checks for NULL/non-NULL
    return (symtable_entry_t *)1;
}

// Get symbol table entry (legacy interface)
symtable_entry_t *get_symtable_entry(const char *name) {
    if (!global_manager || !name) {
        return NULL;
    }

    if (symtable_var_exists(global_manager, name)) {
        return (symtable_entry_t *)1; // Dummy pointer for compatibility
    }

    return NULL;
}

// Set value of symbol table entry (legacy interface)
void symtable_entry_setval(symtable_entry_t *entry, char *val) {
    // This function is problematic in the legacy interface because it doesn't
    // include the variable name. For compatibility, we ignore it.
    // Legacy code should use symtable_set_global instead.
    (void)entry;
    (void)val;
}

// Lookup symbol in symbol table
symtable_entry_t *lookup_symbol(symtable_t *symtable, const char *name) {
    (void)symtable; // Ignore legacy symtable parameter

    if (!global_manager || !name) {
        return NULL;
    }

    if (symtable_var_exists(global_manager, name)) {
        return (symtable_entry_t *)1; // Dummy pointer for compatibility
    }

    return NULL;
}

// Get global symbol table (legacy interface)
symtable_t *get_global_symtable(void) {
    return &dummy_symtable; // Return dummy for compatibility
}

// Get local symbol table (legacy interface)
symtable_t *get_local_symtable(void) {
    return &dummy_symtable; // Return dummy for compatibility
}

// Get symbol table stack (legacy interface)
symtable_stack_t *get_symtable_stack(void) {
    static symtable_stack_t dummy_stack = {
        1, {&dummy_symtable}, &dummy_symtable, &dummy_symtable};
    return &dummy_stack;
}

// Remove entry from symbol table
int remove_from_symtable(symtable_t *symtable, symtable_entry_t *entry) {
    (void)symtable;
    (void)entry;
    // Legacy interface doesn't provide variable name, so we can't implement
    // this properly
    return 0;
}

// Push new symbol table scope (legacy interface)
symtable_t *symtable_stack_push(void) {
    if (global_manager) {
        symtable_push_scope(global_manager, SCOPE_FUNCTION, "legacy-scope");
    }
    return &dummy_symtable;
}

// Pop symbol table scope (legacy interface)
symtable_t *symtable_stack_pop(void) {
    if (global_manager) {
        symtable_pop_scope(global_manager);
    }
    return &dummy_symtable;
}

// Free symbol table (legacy interface)
void free_symtable(symtable_t *symtable) {
    (void)symtable; // Don't free the global manager
}

// Create new symbol table (legacy interface)
symtable_t *new_symtable(size_t level) {
    (void)level;            // Ignore level parameter
    return &dummy_symtable; // Return dummy for compatibility
}
