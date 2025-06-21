/**
 * Modern POSIX Symbol Table Implementation
 * 
 * This provides a clean, POSIX-compliant variable scoping system that handles:
 * - Global variables (persistent across commands)
 * - Local variables (function scope)
 * - Loop variables (for/while scope)
 * - Subshell variables (isolated scope)
 * - Environment variables (exported variables)
 * - Special variables ($?, $!, $$, etc.)
 * 
 * POSIX Scoping Rules:
 * 1. Global scope: Variables persist across commands
 * 2. Function scope: Local variables shadow global ones
 * 3. Loop scope: Loop variables are local to the loop context
 * 4. Subshell scope: Copy of parent scope, changes don't propagate up
 * 5. Environment scope: Exported variables available to child processes
 */

#ifndef SYMTABLE_MODERN_H
#define SYMTABLE_MODERN_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct symtable_modern symtable_modern_t;
typedef struct symvar_modern symvar_modern_t;

// Variable types
typedef enum {
    SYMVAR_STRING,      // Regular string variable
    SYMVAR_INTEGER,     // Integer variable (for arithmetic)
    SYMVAR_ARRAY,       // Array variable (bash extension)
    SYMVAR_FUNCTION     // Function definition
} symvar_type_t;

// Variable flags
typedef enum {
    SYMVAR_NONE = 0,
    SYMVAR_EXPORTED = (1 << 0),    // Variable is exported to environment
    SYMVAR_READONLY = (1 << 1),    // Variable is read-only
    SYMVAR_LOCAL = (1 << 2),       // Variable is local to current scope
    SYMVAR_SPECIAL_VAR = (1 << 3), // Special system variable
    SYMVAR_UNSET = (1 << 4)        // Variable is explicitly unset
} symvar_flags_t;

// Scope types for different contexts
typedef enum {
    SCOPE_GLOBAL,       // Global shell scope
    SCOPE_FUNCTION,     // Function local scope
    SCOPE_LOOP,         // Loop iteration scope (for/while)
    SCOPE_SUBSHELL,     // Subshell scope
    SCOPE_CONDITIONAL   // Conditional execution scope (if/case)
} scope_type_t;

// Variable entry structure
struct symvar_modern {
    char *name;                  // Variable name
    char *value;                 // Variable value (string representation)
    symvar_type_t type;          // Variable type
    symvar_flags_t flags;        // Variable flags
    size_t scope_level;          // Scope level where defined
    symvar_modern_t *next;       // Next variable in hash chain
};

// Symbol table scope structure
struct symtable_modern {
    scope_type_t scope_type;     // Type of scope
    size_t level;                // Scope nesting level
    size_t hash_size;            // Hash table size
    symvar_modern_t **vars;      // Hash table of variables
    symtable_modern_t *parent;   // Parent scope
    char *scope_name;            // Name of scope (for debugging)
};

// Symbol table manager
typedef struct {
    symtable_modern_t *current_scope;   // Current active scope
    symtable_modern_t *global_scope;    // Global scope reference
    size_t max_scope_level;             // Maximum nesting depth
    bool debug_mode;                    // Debug output enabled
} symtable_manager_t;

// Public API Functions

// Manager lifecycle
symtable_manager_t *symtable_manager_new(void);
void symtable_manager_free(symtable_manager_t *manager);
void symtable_manager_set_debug(symtable_manager_t *manager, bool debug);

// Scope management
int symtable_push_scope(symtable_manager_t *manager, scope_type_t type, const char *name);
int symtable_pop_scope(symtable_manager_t *manager);
size_t symtable_current_level(symtable_manager_t *manager);
const char *symtable_current_scope_name(symtable_manager_t *manager);

// Variable operations
int symtable_set_var(symtable_manager_t *manager, const char *name, const char *value, symvar_flags_t flags);
int symtable_set_local_var(symtable_manager_t *manager, const char *name, const char *value);
int symtable_set_global_var(symtable_manager_t *manager, const char *name, const char *value);
char *symtable_get_var(symtable_manager_t *manager, const char *name);
int symtable_unset_var(symtable_manager_t *manager, const char *name);
bool symtable_var_exists(symtable_manager_t *manager, const char *name);

// Export/environment operations
int symtable_export_var(symtable_manager_t *manager, const char *name);
int symtable_unexport_var(symtable_manager_t *manager, const char *name);
char **symtable_get_environ(symtable_manager_t *manager);
void symtable_free_environ(char **environ);

// Special variable support
int symtable_set_special_var(symtable_manager_t *manager, const char *name, const char *value);
char *symtable_get_special_var(symtable_manager_t *manager, const char *name);

// Utility functions
void symtable_dump_scope(symtable_manager_t *manager, scope_type_t scope);
void symtable_dump_all_scopes(symtable_manager_t *manager);
size_t symtable_count_vars(symtable_manager_t *manager, scope_type_t scope);

// Convenience macros for common operations
#define symtable_set(mgr, name, value) symtable_set_var(mgr, name, value, SYMVAR_NONE)
#define symtable_get(mgr, name) symtable_get_var(mgr, name)
#define symtable_export(mgr, name) symtable_export_var(mgr, name)

#endif // SYMTABLE_MODERN_H
