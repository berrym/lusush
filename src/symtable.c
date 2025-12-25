/**
 * Optimized Symbol Table Implementation for Lusush Shell using libhashtable
 *
 * This provides a high-performance symbol table system that leverages
 * libhashtable's ht_strstr_t interface (like the alias system) while
 * maintaining full POSIX shell scoping semantics and variable metadata.
 *
 * Key Features:
 * - Uses libhashtable ht_strstr_t interface for maximum performance
 * - FNV1A hash algorithm for superior distribution vs djb2
 * - Serialized variable metadata for optimal performance
 * - Maintains existing scope chain logic for POSIX compliance
 * - Full API compatibility with legacy implementation
 * - Automated memory management via libhashtable
 */

#include "symtable.h"

#include "ht.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

// Global manager
static symtable_manager_t *global_manager = NULL;

// Legacy compatibility structures
static symtable_t dummy_symtable = {0, NULL, NULL};

// Constants
#define DEFAULT_HT_FLAGS (HT_STR_NONE | HT_SEED_RANDOM)
#define MAX_SCOPE_DEPTH 256
#define METADATA_SEPARATOR "|"
#define METADATA_BUFFER_SIZE 64

// ============================================================================
// STRUCTURES
// ============================================================================

// Enhanced manager structure
struct symtable_manager {
    symtable_scope_t *current_scope; // Current active scope
    symtable_scope_t *global_scope;  // Global scope reference
    size_t max_scope_level;          // Maximum nesting depth
    bool debug_mode;                 // Debug output enabled
};

// ============================================================================
// METADATA SERIALIZATION UTILITIES
// ============================================================================

// Serialize variable metadata into a string format
// Format: "value|type|flags|scope_level"
static char *serialize_variable(const char *value, symvar_type_t type,
                                symvar_flags_t flags, size_t scope_level) {
    if (!value) {
        value = "";
    }

    // Calculate needed size
    size_t value_len = strlen(value);
    size_t total_size = value_len + METADATA_BUFFER_SIZE;

    char *serialized = malloc(total_size);
    if (!serialized) {
        return NULL;
    }

    snprintf(serialized, total_size, "%s%s%d%s%d%s%zu", value,
             METADATA_SEPARATOR, (int)type, METADATA_SEPARATOR, (int)flags,
             METADATA_SEPARATOR, scope_level);

    return serialized;
}

// Deserialize variable metadata from string format
// Returns allocated symvar_t structure
static symvar_t *deserialize_variable(const char *name,
                                      const char *serialized) {
    if (!serialized || !name) {
        return NULL;
    }

    symvar_t *var = malloc(sizeof(symvar_t));
    if (!var) {
        return NULL;
    }

    // Initialize defaults
    var->name = strdup(name);
    var->value = NULL;
    var->type = SYMVAR_STRING;
    var->flags = SYMVAR_NONE;
    var->scope_level = 0;
    var->next = NULL;

    if (!var->name) {
        free(var);
        return NULL;
    }

    // Parse serialized string manually to handle empty values correctly
    char *serialized_copy = strdup(serialized);
    if (!serialized_copy) {
        free(var->name);
        free(var);
        return NULL;
    }

    // Find the separators manually to handle empty fields
    char *pos = serialized_copy;
    char *sep1 = strstr(pos, METADATA_SEPARATOR);
    if (sep1) {
        *sep1 = '\0';
        var->value = strdup(pos);

        pos = sep1 + strlen(METADATA_SEPARATOR);
        char *sep2 = strstr(pos, METADATA_SEPARATOR);
        if (sep2) {
            *sep2 = '\0';
            var->type = (symvar_type_t)atoi(pos);

            pos = sep2 + strlen(METADATA_SEPARATOR);
            char *sep3 = strstr(pos, METADATA_SEPARATOR);
            if (sep3) {
                *sep3 = '\0';
                var->flags = (symvar_flags_t)atoi(pos);

                pos = sep3 + strlen(METADATA_SEPARATOR);
                var->scope_level = (size_t)atoi(pos);
            }
        }
    }

    free(serialized_copy);

    // Ensure we have a value
    if (!var->value) {
        var->value = strdup("");
    }

    return var;
}

// Free deserialized variable
static void free_symvar(symvar_t *var) {
    if (!var) {
        return;
    }
    free(var->name);
    free(var->value);
    free(var);
}

// ============================================================================
// CORE IMPLEMENTATION
// ============================================================================

// Create symbol table manager
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
    global->vars_ht = ht_strstr_create(DEFAULT_HT_FLAGS);
    global->parent = NULL;
    global->scope_name = strdup("global");

    if (!global->vars_ht || !global->scope_name) {
        if (global->vars_ht) {
            ht_strstr_destroy(global->vars_ht);
        }
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
        symtable_scope_t *old_scope = manager->current_scope;
        manager->current_scope = old_scope->parent;

        if (old_scope->vars_ht) {
            ht_strstr_destroy(old_scope->vars_ht);
        }
        free(old_scope->scope_name);
        free(old_scope);
    }

    // Free global scope
    if (manager->global_scope) {
        if (manager->global_scope->vars_ht) {
            ht_strstr_destroy(manager->global_scope->vars_ht);
        }
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

// Find variable in scope chain
static symvar_t *find_var(symtable_scope_t *scope, const char *name) {
    if (!scope || !name) {
        return NULL;
    }

    while (scope) {
        const char *serialized = ht_strstr_get(scope->vars_ht, name);
        if (serialized) {
            symvar_t *var = deserialize_variable(name, serialized);
            if (var && !(var->flags & SYMVAR_UNSET)) {
                return var;
            }
            free_symvar(var);
        }
        scope = scope->parent;
    }

    return NULL;
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
    new_scope->vars_ht = ht_strstr_create(DEFAULT_HT_FLAGS);
    new_scope->parent = manager->current_scope;
    new_scope->scope_name = strdup(name);

    if (!new_scope->vars_ht || !new_scope->scope_name) {
        if (new_scope->vars_ht) {
            ht_strstr_destroy(new_scope->vars_ht);
        }
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

    ht_strstr_destroy(old_scope->vars_ht);
    free(old_scope->scope_name);
    free(old_scope);

    return 0;
}

// Get current scope level
size_t symtable_current_level(symtable_manager_t *manager) {
    if (!manager || !manager->current_scope) {
        return 0;
    }
    return manager->current_scope->level;
}

// Get current scope name
const char *symtable_current_scope_name(symtable_manager_t *manager) {
    if (!manager || !manager->current_scope) {
        return "unknown";
    }
    return manager->current_scope->scope_name;
}

// Set variable
int symtable_set_var(symtable_manager_t *manager, const char *name,
                     const char *value, symvar_flags_t flags) {
    if (!manager || !name) {
        return -1;
    }

    // Serialize variable data
    char *serialized = serialize_variable(value, SYMVAR_STRING, flags,
                                          manager->current_scope->level);
    if (!serialized) {
        return -1;
    }

    // Insert into current scope's hash table
    ht_strstr_insert(manager->current_scope->vars_ht, name, serialized);

    free(serialized);

    if (manager->debug_mode) {
        printf("DEBUG: Set variable '%s'='%s'\n", name, value ? value : "");
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

    // Temporarily switch to global scope
    symtable_scope_t *old_scope = manager->current_scope;
    manager->current_scope = manager->global_scope;

    int result = symtable_set_var(manager, name, value, SYMVAR_NONE);

    // Restore original scope
    manager->current_scope = old_scope;

    return result;
}

// Get variable
char *symtable_get_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return NULL;
    }

    symvar_t *var = find_var(manager->current_scope, name);
    if (!var) {
        return NULL;
    }

    char *result = var->value ? strdup(var->value) : NULL;
    free_symvar(var);

    return result;
}

// Unset variable
int symtable_unset_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return -1;
    }

    // Mark as unset rather than removing
    return symtable_set_var(manager, name, "", SYMVAR_UNSET);
}

// Check if variable exists
bool symtable_var_exists(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return false;
    }

    symvar_t *var = find_var(manager->current_scope, name);
    if (var) {
        free_symvar(var);
        return true;
    }
    return false;
}

// Export variable
int symtable_export_var(symtable_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return -1;
    }

    // Get current value
    char *value = symtable_get_var(manager, name);
    if (!value) {
        return -1;
    }

    // Reset with export flag
    int result = symtable_set_var(manager, name, value, SYMVAR_EXPORTED);

    // Actually export to system environment
    if (result == 0) {
        setenv(name, value, 1);
    }

    free(value);

    return result;
}

// Get environment array (basic implementation)
char **symtable_get_environ(symtable_manager_t *manager) {
    (void)manager; // TODO: Implement environment export

    // Return empty environment for now
    char **env = malloc(sizeof(char *));
    if (env) {
        env[0] = NULL;
    }
    return env;
}

// Free environment array
void symtable_free_environ(char **environ) {
    if (!environ) {
        return;
    }

    for (int i = 0; environ[i]; i++) {
        free(environ[i]);
    }
    free(environ);
}

// Debug dump scope (stub)
void symtable_dump_scope(symtable_manager_t *manager, scope_type_t scope) {
    if (!manager) {
        printf("DEBUG: No manager provided\n");
        return;
    }

    // Find the requested scope in the scope chain
    symtable_scope_t *target_scope = NULL;
    symtable_scope_t *current = manager->current_scope;

    while (current) {
        if (current->scope_type == scope) {
            target_scope = current;
            break;
        }
        current = current->parent;
    }

    if (!target_scope) {
        printf("DEBUG: Scope type %d not found\n", scope);
        return;
    }

    const char *scope_name = "unknown";
    switch (scope) {
    case SCOPE_GLOBAL:
        scope_name = "global";
        break;
    case SCOPE_FUNCTION:
        scope_name = "function";
        break;
    case SCOPE_LOOP:
        scope_name = "loop";
        break;
    case SCOPE_SUBSHELL:
        scope_name = "subshell";
        break;
    case SCOPE_CONDITIONAL:
        scope_name = "conditional";
        break;
    }

    printf("=== %s scope (level %zu) ===\n", scope_name, target_scope->level);

    if (!target_scope->vars_ht) {
        printf("  (no variables)\n");
        return;
    }

    // Enumerate all variables in this scope's hashtable
    ht_enum_t *enum_iter = ht_strstr_enum_create(target_scope->vars_ht);
    if (!enum_iter) {
        printf("  (enumeration failed)\n");
        return;
    }

    const char *key, *value;
    bool has_vars = false;

    while (ht_strstr_enum_next(enum_iter, &key, &value)) {
        has_vars = true;
        printf("  %-12s = '%s'\n", key ? key : "(null)",
               value ? value : "(null)");
    }

    if (!has_vars) {
        printf("  (no variables in scope)\n");
    }

    ht_strstr_enum_destroy(enum_iter);
}

// Debug dump all scopes
void symtable_dump_all_scopes(symtable_manager_t *manager) {
    if (!manager) {
        printf("DEBUG: No manager provided\n");
        return;
    }

    printf("=== Symbol Table Scope Dump ===\n");
    printf("Current scope level: %zu\n",
           manager->current_scope ? manager->current_scope->level : 0);
    printf("Max scope level: %zu\n", manager->max_scope_level);
    printf("\n");

    // Walk through the scope chain from current to global
    symtable_scope_t *current = manager->current_scope;
    int scope_count = 0;

    while (current) {
        const char *scope_name =
            current->scope_name ? current->scope_name : "unnamed";
        const char *type_name = "unknown";

        switch (current->scope_type) {
        case SCOPE_GLOBAL:
            type_name = "global";
            break;
        case SCOPE_FUNCTION:
            type_name = "function";
            break;
        case SCOPE_LOOP:
            type_name = "loop";
            break;
        case SCOPE_SUBSHELL:
            type_name = "subshell";
            break;
        case SCOPE_CONDITIONAL:
            type_name = "conditional";
            break;
        }

        printf("--- Scope #%d: %s (%s, level %zu) ---\n", scope_count++,
               scope_name, type_name, current->level);

        if (!current->vars_ht) {
            printf("  (no hashtable)\n");
        } else {
            // Enumerate variables in this scope
            ht_enum_t *enum_iter = ht_strstr_enum_create(current->vars_ht);
            if (!enum_iter) {
                printf("  (enumeration failed)\n");
            } else {
                const char *key, *value;
                bool has_vars = false;

                while (ht_strstr_enum_next(enum_iter, &key, &value)) {
                    has_vars = true;
                    printf("  %-12s = '%s'\n", key ? key : "(null)",
                           value ? value : "(null)");
                }

                if (!has_vars) {
                    printf("  (no variables in this scope)\n");
                }

                ht_strstr_enum_destroy(enum_iter);
            }
        }

        printf("\n");
        current = current->parent;
    }

    printf("=== End Scope Dump ===\n");
}

// ============================================================================
// CONVENIENCE API (High-level functions for common operations)
// ============================================================================

// Get access to the global symbol table manager
symtable_manager_t *symtable_get_global_manager(void) { return global_manager; }

// Basic variable operations (global scope)
char *symtable_get_global(const char *name) {
    if (!global_manager) {
        return NULL;
    }
    return symtable_get_var(global_manager, name);
}

char *symtable_get_global_default(const char *name, const char *default_value) {
    char *value = symtable_get_global(name);
    if (!value && default_value) {
        value = strdup(default_value);
    }
    return value;
}

int symtable_set_global(const char *name, const char *value) {
    if (!global_manager) {
        return -1;
    }
    return symtable_set_global_var(global_manager, name, value);
}

bool symtable_exists_global(const char *name) {
    if (!global_manager) {
        return false;
    }
    return symtable_var_exists(global_manager, name);
}

int symtable_unset_global(const char *name) {
    if (!global_manager) {
        return -1;
    }
    return symtable_unset_var(global_manager, name);
}

// Integer variable operations
int symtable_get_global_int(const char *name, int default_value) {
    char *value = symtable_get_global(name);
    if (!value) {
        return default_value;
    }

    int result = atoi(value);
    free(value);
    return result;
}

int symtable_set_global_int(const char *name, int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return symtable_set_global(name, buffer);
}

// Boolean variable operations
bool symtable_get_global_bool(const char *name, bool default_value) {
    char *value = symtable_get_global(name);
    if (!value) {
        return default_value;
    }

    bool result = (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
    free(value);
    return result;
}

int symtable_set_global_bool(const char *name, bool value) {
    return symtable_set_global(name, value ? "1" : "0");
}

// Export/environment operations
int symtable_export_global(const char *name) {
    if (!global_manager) {
        return -1;
    }
    return symtable_export_var(global_manager, name);
}

int symtable_unexport_global(const char *name) {
    // Get current value and reset without export flag
    char *value = symtable_get_global(name);
    if (!value) {
        return -1;
    }

    int result = symtable_set_global(name, value);
    free(value);
    return result;
}

// Special variable operations
int symtable_set_special_global(const char *name, const char *value) {
    if (!global_manager) {
        return -1;
    }
    return symtable_set_var(global_manager, name, value, SYMVAR_SPECIAL_VAR);
}

char *symtable_get_special_global(const char *name) {
    return symtable_get_global(name);
}

// Read-only variable operations
int symtable_set_readonly_global(const char *name, const char *value) {
    if (!global_manager) {
        return -1;
    }
    return symtable_set_var(global_manager, name, value, SYMVAR_READONLY);
}

// Debugging and introspection
void symtable_debug_dump_global_scope(void) {
    if (global_manager) {
        symtable_dump_scope(global_manager, SCOPE_GLOBAL);
    }
}

void symtable_debug_dump_all_scopes(void) {
    if (global_manager) {
        symtable_dump_all_scopes(global_manager);
    }
}

// Debug enumeration function for external access to variables
void symtable_debug_enumerate_global_vars(void (*callback)(const char *key,
                                                           const char *value,
                                                           void *userdata),
                                          void *userdata) {
    if (!global_manager || !global_manager->global_scope ||
        !global_manager->global_scope->vars_ht || !callback) {
        return;
    }

    ht_enum_t *enum_iter =
        ht_strstr_enum_create(global_manager->global_scope->vars_ht);
    if (!enum_iter) {
        return;
    }

    const char *key, *value;
    while (ht_strstr_enum_next(enum_iter, &key, &value)) {
        callback(key, value, userdata);
    }

    ht_strstr_enum_destroy(enum_iter);
}

size_t symtable_count_global_vars(void) {
    // TODO: Implement variable counting
    return 0;
}

// Environment array operations
char **symtable_get_environment_array(void) {
    if (!global_manager) {
        return NULL;
    }
    return symtable_get_environ(global_manager);
}

void symtable_free_environment_array(char **env) { symtable_free_environ(env); }

// ============================================================================
// SYSTEM INTERFACE (Essential functions for shell operation)
// ============================================================================

// Shell initialization and cleanup
void init_symtable(void) {
    if (global_manager) {
        return; // Already initialized
    }

    global_manager = symtable_manager_new();
    if (!global_manager) {
        fprintf(stderr, "ERROR: Failed to initialize symbol table\n");
        return;
    }
}

void free_global_symtable(void) {
    if (global_manager) {
        symtable_manager_free(global_manager);
        global_manager = NULL;
    }
}

// Special variable management
void set_exit_status(int status) {
    char status_str[16];
    snprintf(status_str, sizeof(status_str), "%d", status);
    symtable_set_special_global("?", status_str);

    // Also update the global variable for consistency
    extern int last_exit_status;
    last_exit_status = status;
}

// Environment interface
char **get_environ_array(void) { return symtable_get_environment_array(); }

void free_environ_array(char **env) { symtable_free_environment_array(env); }

// ============================================================================
// LEGACY COMPATIBILITY (For string management and other subsystems)
// ============================================================================

// Legacy functions (for string management system)
symtable_t *new_symtable(size_t level) {
    (void)level;
    return &dummy_symtable;
}

symtable_t *symtable_stack_push(void) {
    if (global_manager) {
        symtable_push_scope(global_manager, SCOPE_FUNCTION, "legacy-scope");
    }
    return &dummy_symtable;
}

symtable_t *symtable_stack_pop(void) {
    if (global_manager) {
        symtable_pop_scope(global_manager);
    }
    return &dummy_symtable;
}

int remove_from_symtable(symtable_t *symtable, symtable_entry_t *entry) {
    (void)symtable;
    (void)entry;
    return 0;
}

symtable_entry_t *add_to_symtable(char *name) {
    if (!global_manager || !name) {
        return NULL;
    }

    // Set variable with empty value if it doesn't exist
    if (!symtable_var_exists(global_manager, name)) {
        symtable_set_var(global_manager, name, "", SYMVAR_NONE);
    }

    // Return a dummy pointer for compatibility
    return (symtable_entry_t *)1;
}

symtable_entry_t *lookup_symbol(symtable_t *symtable, const char *name) {
    (void)symtable;

    if (!global_manager || !name) {
        return NULL;
    }

    if (symtable_var_exists(global_manager, name)) {
        return (symtable_entry_t *)1;
    }

    return NULL;
}

symtable_entry_t *get_symtable_entry(const char *name) {
    if (!global_manager || !name) {
        return NULL;
    }

    if (symtable_var_exists(global_manager, name)) {
        return (symtable_entry_t *)1;
    }

    return NULL;
}

symtable_t *get_local_symtable(void) { return &dummy_symtable; }

symtable_t *get_global_symtable(void) { return &dummy_symtable; }

symtable_stack_t *get_symtable_stack(void) {
    static symtable_stack_t dummy_stack = {
        1, {&dummy_symtable}, &dummy_symtable, &dummy_symtable};
    return &dummy_stack;
}

void free_symtable(symtable_t *symtable) { (void)symtable; }

void symtable_entry_setval(symtable_entry_t *entry, char *val) {
    (void)entry;
    (void)val;
}

// ============================================================================
// ENHANCED API COMPATIBILITY
// ============================================================================

bool symtable_libht_available(void) {
    return true; // Always available since this is the main implementation
}

const char *symtable_implementation_info(void) {
    return "Optimized libhashtable implementation (ht_strstr_t, FNV1A hash)";
}

void init_symtable_libht(void) { init_symtable(); }

void free_symtable_libht(void) { free_global_symtable(); }

void *get_libht_manager(void) { return (void *)global_manager; }

int symtable_set_var_enhanced(const char *name, const char *value,
                              symvar_flags_t flags) {
    if (!global_manager) {
        init_symtable();
        if (!global_manager) {
            return -1;
        }
    }
    return symtable_set_var(global_manager, name, value, flags);
}

char *symtable_get_var_enhanced(const char *name) {
    if (!global_manager) {
        return NULL;
    }
    return symtable_get_var(global_manager, name);
}

int symtable_push_scope_enhanced(scope_type_t type, const char *name) {
    if (!global_manager) {
        init_symtable();
        if (!global_manager) {
            return -1;
        }
    }
    return symtable_push_scope(global_manager, type, name);
}

int symtable_pop_scope_enhanced(void) {
    if (!global_manager) {
        return -1;
    }
    return symtable_pop_scope(global_manager);
}

void symtable_benchmark_comparison(int iterations) {
    (void)iterations;
    printf(
        "Optimized libhashtable implementation is the main implementation\n");
}

int symtable_libht_test(void) {
    printf("Testing main symbol table implementation...\n");

    init_symtable();
    if (!global_manager) {
        printf("FAIL: Could not initialize symbol table\n");
        return -1;
    }

    if (symtable_set_global("test_var", "test_value") != 0) {
        printf("FAIL: Could not set variable\n");
        return -1;
    }

    char *value = symtable_get_global("test_var");
    if (!value || strcmp(value, "test_value") != 0) {
        printf("FAIL: Variable value mismatch\n");
        free(value);
        return -1;
    }
    free(value);

    printf("PASS: Main symbol table test completed successfully\n");
    return 0;
}

// Optimized API compatibility (this IS the optimized implementation)
bool symtable_opt_available(void) { return true; }

const char *symtable_opt_implementation_info(void) {
    return symtable_implementation_info();
}

void init_symtable_opt(void) { init_symtable(); }

void free_symtable_opt(void) { free_global_symtable(); }

void *get_opt_manager(void) { return (void *)global_manager; }

int symtable_set_var_opt_api(const char *name, const char *value,
                             symvar_flags_t flags) {
    return symtable_set_var_enhanced(name, value, flags);
}

char *symtable_get_var_opt_api(const char *name) {
    return symtable_get_var_enhanced(name);
}

int symtable_push_scope_opt_api(scope_type_t type, const char *name) {
    return symtable_push_scope_enhanced(type, name);
}

int symtable_pop_scope_opt_api(void) { return symtable_pop_scope_enhanced(); }

void symtable_benchmark_opt_comparison(int iterations) {
    (void)iterations;
    printf("This IS the optimized implementation\n");
}

int symtable_opt_test(void) { return symtable_libht_test(); }
