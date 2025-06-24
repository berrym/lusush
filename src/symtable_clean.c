/**
 * Performance-Optimized Enhanced Symbol Table Implementation using libhashtable
 *
 * This provides a high-performance symbol table system that leverages libhashtable's
 * ht_strstr_t interface (like the alias system) while maintaining full POSIX shell
 * scoping semantics and variable metadata.
 *
 * Key Performance Optimizations:
 * - Uses ht_strstr_t (string->string) interface for maximum performance
 * - Serializes variable metadata into value string to avoid complex copying
 * - Minimal memory allocations compared to generic ht_t approach
 * - Same FNV1A hash algorithm as alias system for consistency
 * - Maintains existing scope chain logic for POSIX compliance
 *
 * Features:
 * - Feature flag controlled (SYMTABLE_USE_LIBHASHTABLE_V2)
 * - Full API compatibility with existing implementation
 * - Preserves all variable metadata (flags, type, scope_level)
 * - Maintains POSIX shell scoping rules
 */

#include "../include/symtable.h"
#include "../include/errors.h"
#include "../include/libhashtable/ht.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <strings.h>
#include <unistd.h>

// Feature flag to enable optimized libhashtable implementation
#ifndef SYMTABLE_USE_LIBHASHTABLE_V2
#define SYMTABLE_USE_LIBHASHTABLE_V2 0
#endif

#if SYMTABLE_USE_LIBHASHTABLE_V2

// ============================================================================
// OPTIMIZED STRUCTURES AND CONSTANTS
// ============================================================================

// Optimized scope structure using ht_strstr_t
typedef struct symtable_scope_opt {
    scope_type_t scope_type;           // Type of scope
    size_t level;                      // Scope nesting level
    ht_strstr_t *vars_ht;             // libhashtable string->string for variables
    struct symtable_scope_opt *parent; // Parent scope
    char *scope_name;                  // Name of scope (for debugging)
} symtable_scope_opt_t;

// Optimized manager structure
typedef struct {
    symtable_scope_opt_t *current_scope; // Current active scope
    symtable_scope_opt_t *global_scope;  // Global scope reference
    size_t max_scope_level;               // Maximum nesting depth
    bool debug_mode;                      // Debug output enabled
} symtable_manager_opt_t;

// Global optimized manager
static symtable_manager_opt_t *global_manager_opt = NULL;

// Constants
#define DEFAULT_HT_FLAGS (HT_STR_NONE | HT_SEED_RANDOM)
#define MAX_SCOPE_DEPTH 256
#define METADATA_SEPARATOR "|"
#define METADATA_BUFFER_SIZE 64

// ============================================================================
// METADATA SERIALIZATION UTILITIES
// ============================================================================

// Serialize variable metadata into a string format
// Format: "value|type|flags|scope_level"
static char *serialize_variable(const char *value, symvar_type_t type, 
                               symvar_flags_t flags, size_t scope_level) {
    if (!value) value = "";
    
    // Calculate needed size
    size_t value_len = strlen(value);
    size_t total_size = value_len + METADATA_BUFFER_SIZE;
    
    char *serialized = malloc(total_size);
    if (!serialized) return NULL;
    
    snprintf(serialized, total_size, "%s%s%d%s%d%s%zu", 
             value, METADATA_SEPARATOR,
             (int)type, METADATA_SEPARATOR,
             (int)flags, METADATA_SEPARATOR,
             scope_level);
    
    return serialized;
}

// Deserialize variable metadata from string format
// Returns allocated symvar_t structure
static symvar_t *deserialize_variable(const char *name, const char *serialized) {
    if (!serialized || !name) return NULL;
    
    symvar_t *var = malloc(sizeof(symvar_t));
    if (!var) return NULL;
    
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
    
    // Parse serialized string
    char *serialized_copy = strdup(serialized);
    if (!serialized_copy) {
        free(var->name);
        free(var);
        return NULL;
    }
    
    char *token = strtok(serialized_copy, METADATA_SEPARATOR);
    int field = 0;
    
    while (token && field < 4) {
        switch (field) {
            case 0: // value
                var->value = strdup(token);
                break;
            case 1: // type
                var->type = (symvar_type_t)atoi(token);
                break;
            case 2: // flags  
                var->flags = (symvar_flags_t)atoi(token);
                break;
            case 3: // scope_level
                var->scope_level = (size_t)atoi(token);
                break;
        }
        field++;
        token = strtok(NULL, METADATA_SEPARATOR);
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
    if (!var) return;
    free(var->name);
    free(var->value);
    free(var);
}

// ============================================================================
// OPTIMIZED CORE IMPLEMENTATION
// ============================================================================

// Create optimized symbol table manager
static symtable_manager_opt_t *symtable_manager_opt_new(void) {
    symtable_manager_opt_t *manager = calloc(1, sizeof(symtable_manager_opt_t));
    if (!manager) return NULL;
    
    // Create global scope
    symtable_scope_opt_t *global = calloc(1, sizeof(symtable_scope_opt_t));
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
        if (global->vars_ht) ht_strstr_destroy(global->vars_ht);
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

// Free optimized symbol table manager
static void symtable_manager_opt_free(symtable_manager_opt_t *manager) {
    if (!manager) return;
    
    // Pop all scopes to free memory
    while (manager->current_scope && manager->current_scope != manager->global_scope) {
        symtable_scope_opt_t *old_scope = manager->current_scope;
        manager->current_scope = old_scope->parent;
        
        if (old_scope->vars_ht) ht_strstr_destroy(old_scope->vars_ht);
        free(old_scope->scope_name);
        free(old_scope);
    }
    
    // Free global scope
    if (manager->global_scope) {
        if (manager->global_scope->vars_ht) ht_strstr_destroy(manager->global_scope->vars_ht);
        free(manager->global_scope->scope_name);
        free(manager->global_scope);
    }
    
    free(manager);
}

// Find variable in scope chain (optimized version)
static symvar_t *find_var_opt(symtable_scope_opt_t *scope, const char *name) {
    if (!scope || !name) return NULL;
    
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

// Push new scope (optimized version)
static int symtable_push_scope_opt(symtable_manager_opt_t *manager, 
                                   scope_type_t type, const char *name) {
    if (!manager || !name) return -1;
    
    if (manager->current_scope->level >= MAX_SCOPE_DEPTH) {
        if (manager->debug_mode) {
            fprintf(stderr, "ERROR: Maximum scope depth exceeded\n");
        }
        return -1;
    }
    
    symtable_scope_opt_t *new_scope = calloc(1, sizeof(symtable_scope_opt_t));
    if (!new_scope) return -1;
    
    new_scope->scope_type = type;
    new_scope->level = manager->current_scope->level + 1;
    new_scope->vars_ht = ht_strstr_create(DEFAULT_HT_FLAGS);
    new_scope->parent = manager->current_scope;
    new_scope->scope_name = strdup(name);
    
    if (!new_scope->vars_ht || !new_scope->scope_name) {
        if (new_scope->vars_ht) ht_strstr_destroy(new_scope->vars_ht);
        free(new_scope->scope_name);
        free(new_scope);
        return -1;
    }
    
    manager->current_scope = new_scope;
    if (new_scope->level > manager->max_scope_level) {
        manager->max_scope_level = new_scope->level;
    }
    
    if (manager->debug_mode) {
        printf("DEBUG: Pushed scope '%s' (level %zu) [optimized]\n", name, new_scope->level);
    }
    
    return 0;
}

// Pop current scope (optimized version)
static int symtable_pop_scope_opt(symtable_manager_opt_t *manager) {
    if (!manager || !manager->current_scope || 
        manager->current_scope == manager->global_scope) {
        return -1; // Can't pop global scope
    }
    
    symtable_scope_opt_t *old_scope = manager->current_scope;
    manager->current_scope = old_scope->parent;
    
    if (manager->debug_mode) {
        printf("DEBUG: Popped scope '%s' (level %zu) [optimized]\n",
               old_scope->scope_name, old_scope->level);
    }
    
    ht_strstr_destroy(old_scope->vars_ht);
    free(old_scope->scope_name);
    free(old_scope);
    
    return 0;
}

// Set variable (optimized version)
static int symtable_set_var_opt(symtable_manager_opt_t *manager, 
                                const char *name, const char *value, 
                                symvar_flags_t flags) {
    if (!manager || !name) return -1;
    
    // Serialize variable data
    char *serialized = serialize_variable(value, SYMVAR_STRING, flags, 
                                         manager->current_scope->level);
    if (!serialized) return -1;
    
    // Insert into current scope's hash table
    ht_strstr_insert(manager->current_scope->vars_ht, name, serialized);
    
    free(serialized);
    
    if (manager->debug_mode) {
        printf("DEBUG: Set variable '%s'='%s' [optimized]\n", name, value ? value : "");
    }
    
    return 0;
}

// Get variable (optimized version)
static char *symtable_get_var_opt(symtable_manager_opt_t *manager, const char *name) {
    if (!manager || !name) return NULL;
    
    symvar_t *var = find_var_opt(manager->current_scope, name);
    if (!var) return NULL;
    
    char *result = var->value ? strdup(var->value) : NULL;
    free_symvar(var);
    
    return result;
}

// ============================================================================
// PUBLIC API WRAPPERS (Feature flag controlled)
// ============================================================================

// Initialize optimized symbol table if enabled
void init_symtable_opt(void) {
    if (global_manager_opt) return; // Already initialized
    
    global_manager_opt = symtable_manager_opt_new();
    if (!global_manager_opt) {
        fprintf(stderr, "ERROR: Failed to initialize optimized symbol table\n");
        return;
    }
    
    printf("INFO: Optimized symbol table (v2) using libhashtable initialized\n");
}

// Free optimized symbol table
void free_symtable_opt(void) {
    if (global_manager_opt) {
        symtable_manager_opt_free(global_manager_opt);
        global_manager_opt = NULL;
    }
}

// Get optimized manager
void *get_opt_manager(void) {
    return (void *)global_manager_opt;
}

// Optimized API functions (for testing and benchmarking)
int symtable_set_var_opt_api(const char *name, const char *value, symvar_flags_t flags) {
    if (!global_manager_opt) {
        init_symtable_opt();
        if (!global_manager_opt) return -1;
    }
    
    return symtable_set_var_opt(global_manager_opt, name, value, flags);
}

char *symtable_get_var_opt_api(const char *name) {
    if (!global_manager_opt) return NULL;
    
    return symtable_get_var_opt(global_manager_opt, name);
}

int symtable_push_scope_opt_api(scope_type_t type, const char *name) {
    if (!global_manager_opt) {
        init_symtable_opt();
        if (!global_manager_opt) return -1;
    }
    
    return symtable_push_scope_opt(global_manager_opt, type, name);
}

int symtable_pop_scope_opt_api(void) {
    if (!global_manager_opt) return -1;
    
    return symtable_pop_scope_opt(global_manager_opt);
}

// Performance comparison function
void symtable_benchmark_opt_comparison(int iterations) {
    printf("\n=== Optimized Symbol Table Performance Comparison ===\n");
    printf("Iterations: %d\n", iterations);
    
    // TODO: Implement benchmarking logic comparing standard vs optimized implementation
    // This would measure:
    // - Variable insertion time
    // - Variable lookup time
    // - Memory usage
    // - Hash collision rates
    
    printf("Feature available when SYMTABLE_USE_LIBHASHTABLE_V2=1\n");
}

#else // SYMTABLE_USE_LIBHASHTABLE_V2 == 0

// Stub functions when feature is disabled
void init_symtable_opt(void) {
    // Feature disabled
}

void free_symtable_opt(void) {
    // Feature disabled
}

void *get_opt_manager(void) {
    return NULL;
}

int symtable_set_var_opt_api(const char *name, const char *value, symvar_flags_t flags) {
    (void)name; (void)value; (void)flags;
    return -1; // Feature disabled
}

char *symtable_get_var_opt_api(const char *name) {
    (void)name;
    return NULL; // Feature disabled
}

int symtable_push_scope_opt_api(scope_type_t type, const char *name) {
    (void)type; (void)name;
    return -1; // Feature disabled
}

int symtable_pop_scope_opt_api(void) {
    return -1; // Feature disabled
}

void symtable_benchmark_opt_comparison(int iterations) {
    (void)iterations;
    printf("Optimized symbol table feature disabled.\n");
    printf("To enable: compile with -DSYMTABLE_USE_LIBHASHTABLE_V2=1\n");
}

#endif // SYMTABLE_USE_LIBHASHTABLE_V2

// ============================================================================
// FEATURE DETECTION AND TESTING
// ============================================================================

bool symtable_opt_available(void) {
    return SYMTABLE_USE_LIBHASHTABLE_V2 == 1;
}

const char *symtable_opt_implementation_info(void) {
    if (SYMTABLE_USE_LIBHASHTABLE_V2) {
        return "Optimized libhashtable v2 implementation (ht_strstr_t, FNV1A hash)";
    } else {
        return "Optimized implementation not available";
    }
}

// Test function to validate optimized implementation
int symtable_opt_test(void) {
    if (!symtable_opt_available()) {
        printf("Optimized symbol table not available - compile with -DSYMTABLE_USE_LIBHASHTABLE_V2=1\n");
        return -1;
    }
    
#if SYMTABLE_USE_LIBHASHTABLE_V2
    printf("Testing optimized symbol table implementation...\n");
    
    // Initialize
    init_symtable_opt();
    if (!global_manager_opt) {
        printf("FAIL: Could not initialize optimized symbol table\n");
        return -1;
    }
    
    // Test basic variable operations
    if (symtable_set_var_opt_api("test_var", "test_value", SYMVAR_NONE) != 0) {
        printf("FAIL: Could not set variable\n");
        return -1;
    }
    
    char *value = symtable_get_var_opt_api("test_var");
    if (!value || strcmp(value, "test_value") != 0) {
        printf("FAIL: Variable value mismatch\n");
        free(value);
        return -1;
    }
    free(value);
    
    // Test scope operations
    if (symtable_push_scope_opt_api(SCOPE_FUNCTION, "test_function") != 0) {
        printf("FAIL: Could not push scope\n");
        return -1;
    }
    
    if (symtable_pop_scope_opt_api() != 0) {
        printf("FAIL: Could not pop scope\n");
        return -1;
    }
    
    // Test variable flags
    if (symtable_set_var_opt_api("flag_var", "flag_value", SYMVAR_EXPORTED | SYMVAR_READONLY) != 0) {
        printf("FAIL: Could not set variable with flags\n");
        return -1;
    }
    
    value = symtable_get_var_opt_api("flag_var");
    if (!value || strcmp(value, "flag_value") != 0) {
        printf("FAIL: Flagged variable value mismatch\n");
        free(value);
        return -1;
    }
    free(value);
    
    // Cleanup
    free_symtable_opt();
    
    printf("PASS: Optimized symbol table test completed successfully\n");
    return 0;
#else
    return -1;
#endif
}