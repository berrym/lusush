/**
 * Enhanced Symbol Table Implementation using libhashtable for Lusush Shell
 *
 * This provides an improved symbol table system that leverages libhashtable's
 * FNV1A hash algorithm and optimized collision handling while maintaining
 * full POSIX shell scoping semantics.
 *
 * Features:
 * - Uses libhashtable's generic ht_t interface for symvar_t storage
 * - Maintains existing scope chain logic for POSIX compliance
 * - FNV1A hash algorithm for better distribution than djb2
 * - Built-in collision handling and memory management
 * - Feature flag controlled (SYMTABLE_USE_LIBHASHTABLE)
 * - Full API compatibility with existing implementation
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

// Feature flag to enable libhashtable implementation
#ifndef SYMTABLE_USE_LIBHASHTABLE
#define SYMTABLE_USE_LIBHASHTABLE 0
#endif

#if SYMTABLE_USE_LIBHASHTABLE

// ============================================================================
// LIBHASHTABLE-ENHANCED STRUCTURES
// ============================================================================

// Enhanced scope structure using libhashtable
typedef struct symtable_scope_libht {
    scope_type_t scope_type;           // Type of scope
    size_t level;                      // Scope nesting level
    ht_t *vars_ht;                     // libhashtable for variables
    struct symtable_scope_libht *parent; // Parent scope
    char *scope_name;                  // Name of scope (for debugging)
} symtable_scope_libht_t;

// Enhanced manager structure
typedef struct {
    symtable_scope_libht_t *current_scope; // Current active scope
    symtable_scope_libht_t *global_scope;  // Global scope reference
    size_t max_scope_level;                 // Maximum nesting depth
    bool debug_mode;                        // Debug output enabled
} symtable_manager_libht_t;

// Global enhanced manager
static symtable_manager_libht_t *global_manager_libht = NULL;

// Default limits
#define DEFAULT_HT_FLAGS (HT_STR_NONE)
#define MAX_SCOPE_DEPTH 256

// ============================================================================
// LIBHASHTABLE CALLBACK FUNCTIONS
// ============================================================================

// Hash function for variable names (uses libhashtable's FNV1A)
static uint64_t symvar_name_hash(const void *key, uint64_t seed) {
    return fnv1a_hash_str(key, seed);
}

// Key equality function for variable names
static bool symvar_name_equal(const void *key1, const void *key2) {
    return strcmp((const char *)key1, (const char *)key2) == 0;
}

// Key copy function - duplicate string
static void *symvar_name_copy(const void *key) {
    return strdup((const char *)key);
}

// Key free function - free string
static void symvar_name_free(const void *key) {
    free((void *)key);
}

// Value copy function - duplicate symvar_t
static void *symvar_copy(const void *value) {
    const symvar_t *src = (const symvar_t *)value;
    if (!src) return NULL;
    
    symvar_t *copy = malloc(sizeof(symvar_t));
    if (!copy) return NULL;
    
    copy->name = src->name ? strdup(src->name) : NULL;
    copy->value = src->value ? strdup(src->value) : NULL;
    copy->type = src->type;
    copy->flags = src->flags;
    copy->scope_level = src->scope_level;
    copy->next = NULL; // Not used in libhashtable version
    
    return copy;
}

// Value free function - free symvar_t
static void symvar_free(const void *value) {
    symvar_t *var = (symvar_t *)value;
    if (!var) return;
    
    free(var->name);
    free(var->value);
    free(var);
}

// Callback structure for libhashtable
static const ht_callbacks_t symvar_callbacks = {
    .key_copy = symvar_name_copy,
    .key_free = symvar_name_free,
    .val_copy = symvar_copy,
    .val_free = symvar_free
};

// ============================================================================
// ENHANCED CORE IMPLEMENTATION
// ============================================================================

// Create enhanced symbol table manager
static symtable_manager_libht_t *symtable_manager_libht_new(void) {
    symtable_manager_libht_t *manager = calloc(1, sizeof(symtable_manager_libht_t));
    if (!manager) {
        return NULL;
    }
    
    // Create global scope
    symtable_scope_libht_t *global = calloc(1, sizeof(symtable_scope_libht_t));
    if (!global) {
        free(manager);
        return NULL;
    }
    
    global->scope_type = SCOPE_GLOBAL;
    global->level = 0;
    global->vars_ht = ht_create(symvar_name_hash, symvar_name_equal, 
                                &symvar_callbacks, DEFAULT_HT_FLAGS);
    global->parent = NULL;
    global->scope_name = strdup("global");
    
    if (!global->vars_ht || !global->scope_name) {
        if (global->vars_ht) ht_destroy(global->vars_ht);
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

// Helper function to free all variables in a scope
// Note: libhashtable will handle cleanup via callbacks, but we need to enumerate first
static void free_scope_variables(ht_t *vars_ht) {
    // libhashtable will call our free callbacks when ht_destroy is called
    // No manual cleanup needed here
}

// Free enhanced symbol table manager
static void symtable_manager_libht_free(symtable_manager_libht_t *manager) {
    if (!manager) return;
    
    // Pop all scopes to free memory
    while (manager->current_scope && manager->current_scope != manager->global_scope) {
        symtable_scope_libht_t *old_scope = manager->current_scope;
        manager->current_scope = old_scope->parent;
        
        if (old_scope->vars_ht) {
            
            ht_destroy(old_scope->vars_ht);
        }
        free(old_scope->scope_name);
        free(old_scope);
    }
    
    // Free global scope
    if (manager->global_scope) {
        if (manager->global_scope->vars_ht) {
            ht_destroy(manager->global_scope->vars_ht);
        }
        free(manager->global_scope->scope_name);
        free(manager->global_scope);
    }
    
    free(manager);
}

// Find variable in scope chain (libhashtable version)
static symvar_t *find_var_libht(symtable_scope_libht_t *scope, const char *name) {
    if (!scope || !name) return NULL;
    
    while (scope) {
        symvar_t *var = (symvar_t *)ht_get(scope->vars_ht, name);
        if (var && !(var->flags & SYMVAR_UNSET)) {
            return var;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

// Push new scope (libhashtable version)
static int symtable_push_scope_libht(symtable_manager_libht_t *manager, 
                                     scope_type_t type, const char *name) {
    if (!manager || !name) return -1;
    
    if (manager->current_scope->level >= MAX_SCOPE_DEPTH) {
        if (manager->debug_mode) {
            fprintf(stderr, "ERROR: Maximum scope depth exceeded\n");
        }
        return -1;
    }
    
    symtable_scope_libht_t *new_scope = calloc(1, sizeof(symtable_scope_libht_t));
    if (!new_scope) return -1;
    
    new_scope->scope_type = type;
    new_scope->level = manager->current_scope->level + 1;
    new_scope->vars_ht = ht_create(symvar_name_hash, symvar_name_equal,
                                   &symvar_callbacks, DEFAULT_HT_FLAGS);
    new_scope->parent = manager->current_scope;
    new_scope->scope_name = strdup(name);
    
    if (!new_scope->vars_ht || !new_scope->scope_name) {
        if (new_scope->vars_ht) ht_destroy(new_scope->vars_ht);
        free(new_scope->scope_name);
        free(new_scope);
        return -1;
    }
    
    manager->current_scope = new_scope;
    if (new_scope->level > manager->max_scope_level) {
        manager->max_scope_level = new_scope->level;
    }
    
    if (manager->debug_mode) {
        printf("DEBUG: Pushed scope '%s' (level %zu) [libhashtable]\n", 
               name, new_scope->level);
    }
    
    return 0;
}

// Pop current scope (libhashtable version)
static int symtable_pop_scope_libht(symtable_manager_libht_t *manager) {
    if (!manager || !manager->current_scope || 
        manager->current_scope == manager->global_scope) {
        return -1; // Can't pop global scope
    }
    
    symtable_scope_libht_t *old_scope = manager->current_scope;
    manager->current_scope = old_scope->parent;
    
    if (manager->debug_mode) {
        printf("DEBUG: Popped scope '%s' (level %zu) [libhashtable]\n",
               old_scope->scope_name, old_scope->level);
    }
    
    if (old_scope->vars_ht) {
        
        ht_destroy(old_scope->vars_ht);
    }
    free(old_scope->scope_name);
    free(old_scope);
    
    return 0;
}

// Set variable (libhashtable version)
static int symtable_set_var_libht(symtable_manager_libht_t *manager, 
                                  const char *name, const char *value, 
                                  symvar_flags_t flags) {
    if (!manager || !name) return -1;
    
    // Create new variable on stack (libhashtable will copy it)
    symvar_t new_var = {0};
    new_var.name = strdup(name);
    new_var.value = value ? strdup(value) : strdup("");
    new_var.type = SYMVAR_STRING;
    new_var.flags = flags;
    new_var.scope_level = manager->current_scope->level;
    new_var.next = NULL;
    
    if (!new_var.name || !new_var.value) {
        free(new_var.name);
        free(new_var.value);
        return -1;
    }
    
    // Insert into current scope's hash table (libhashtable will copy key and value)
    ht_insert(manager->current_scope->vars_ht, name, &new_var);
    
    // Clean up our temporary copy
    free(new_var.name);
    free(new_var.value);
    
    if (manager->debug_mode) {
        printf("DEBUG: Set variable '%s'='%s' [libhashtable]\n", name, value ? value : "");
    }
    
    return 0;
}

// Get variable (libhashtable version)
static char *symtable_get_var_libht(symtable_manager_libht_t *manager, const char *name) {
    if (!manager || !name) return NULL;
    
    symvar_t *var = find_var_libht(manager->current_scope, name);
    if (!var) return NULL;
    
    return var->value ? strdup(var->value) : NULL;
}

// ============================================================================
// PUBLIC API WRAPPERS (Feature flag controlled)
// ============================================================================

// Initialize enhanced symbol table if enabled
void init_symtable_libht(void) {
    if (global_manager_libht) return; // Already initialized
    
    global_manager_libht = symtable_manager_libht_new();
    if (!global_manager_libht) {
        fprintf(stderr, "ERROR: Failed to initialize enhanced symbol table\n");
        return;
    }
    
    printf("INFO: Enhanced symbol table using libhashtable initialized\n");
}

// Free enhanced symbol table
void free_symtable_libht(void) {
    if (global_manager_libht) {
        symtable_manager_libht_free(global_manager_libht);
        global_manager_libht = NULL;
    }
}

// Get enhanced manager
void *get_libht_manager(void) {
    return (void *)global_manager_libht;
}

// Enhanced API functions (for testing and benchmarking)
int symtable_set_var_enhanced(const char *name, const char *value, symvar_flags_t flags) {
    if (!global_manager_libht) {
        init_symtable_libht();
        if (!global_manager_libht) return -1;
    }
    
    return symtable_set_var_libht(global_manager_libht, name, value, flags);
}

char *symtable_get_var_enhanced(const char *name) {
    if (!global_manager_libht) return NULL;
    
    return symtable_get_var_libht(global_manager_libht, name);
}

int symtable_push_scope_enhanced(scope_type_t type, const char *name) {
    if (!global_manager_libht) {
        init_symtable_libht();
        if (!global_manager_libht) return -1;
    }
    
    return symtable_push_scope_libht(global_manager_libht, type, name);
}

int symtable_pop_scope_enhanced(void) {
    if (!global_manager_libht) return -1;
    
    return symtable_pop_scope_libht(global_manager_libht);
}

// Performance comparison function
void symtable_benchmark_comparison(int iterations) {
    printf("\n=== Symbol Table Performance Comparison ===\n");
    printf("Iterations: %d\n", iterations);
    
    // TODO: Implement benchmarking logic comparing old vs new implementation
    // This would measure:
    // - Variable insertion time
    // - Variable lookup time
    // - Memory usage
    // - Hash collision rates
    
    printf("Feature available when SYMTABLE_USE_LIBHASHTABLE=1\n");
}

#else // SYMTABLE_USE_LIBHASHTABLE == 0

// Stub functions when feature is disabled
void init_symtable_libht(void) {
    // Feature disabled
}

void free_symtable_libht(void) {
    // Feature disabled
}

void *get_libht_manager(void) {
    return NULL;
}

int symtable_set_var_enhanced(const char *name, const char *value, symvar_flags_t flags) {
    (void)name; (void)value; (void)flags;
    return -1; // Feature disabled
}

char *symtable_get_var_enhanced(const char *name) {
    (void)name;
    return NULL; // Feature disabled
}

int symtable_push_scope_enhanced(scope_type_t type, const char *name) {
    (void)type; (void)name;
    return -1; // Feature disabled
}

int symtable_pop_scope_enhanced(void) {
    return -1; // Feature disabled
}

void symtable_benchmark_comparison(int iterations) {
    (void)iterations;
    printf("Enhanced symbol table feature disabled.\n");
    printf("To enable: compile with -DSYMTABLE_USE_LIBHASHTABLE=1\n");
}

#endif // SYMTABLE_USE_LIBHASHTABLE

// ============================================================================
// FEATURE DETECTION AND TESTING
// ============================================================================

bool symtable_libht_available(void) {
    return SYMTABLE_USE_LIBHASHTABLE == 1;
}

const char *symtable_implementation_info(void) {
    if (SYMTABLE_USE_LIBHASHTABLE) {
        return "Enhanced libhashtable implementation (FNV1A hash)";
    } else {
        return "Standard implementation (djb2-like hash)";
    }
}

// Test function to validate enhanced implementation
int symtable_libht_test(void) {
    if (!symtable_libht_available()) {
        printf("Enhanced symbol table not available - compile with -DSYMTABLE_USE_LIBHASHTABLE=1\n");
        return -1;
    }
    
#if SYMTABLE_USE_LIBHASHTABLE
    printf("Testing enhanced symbol table implementation...\n");
    
    // Initialize
    init_symtable_libht();
    if (!global_manager_libht) {
        printf("FAIL: Could not initialize enhanced symbol table\n");
        return -1;
    }
    
    // Test basic variable operations
    if (symtable_set_var_enhanced("test_var", "test_value", SYMVAR_NONE) != 0) {
        printf("FAIL: Could not set variable\n");
        return -1;
    }
    
    char *value = symtable_get_var_enhanced("test_var");
    if (!value || strcmp(value, "test_value") != 0) {
        printf("FAIL: Variable value mismatch\n");
        free(value);
        return -1;
    }
    free(value);
    
    // Test scope operations
    if (symtable_push_scope_enhanced(SCOPE_FUNCTION, "test_function") != 0) {
        printf("FAIL: Could not push scope\n");
        return -1;
    }
    
    if (symtable_pop_scope_enhanced() != 0) {
        printf("FAIL: Could not pop scope\n");
        return -1;
    }
    
    // Cleanup
    free_symtable_libht();
    
    printf("PASS: Enhanced symbol table test completed successfully\n");
    return 0;
#else
    return -1;
#endif
}