/**
 * Comprehensive Symbol Table Implementation for Lusush Shell
 *
 * This provides a unified, modern POSIX-compliant variable scoping system that
 * handles:
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

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

// Forward declarations
typedef struct symtable_scope symtable_scope_t;
typedef struct symvar symvar_t;

// Variable types
typedef enum {
    SYMVAR_STRING,  // Regular string variable
    SYMVAR_INTEGER, // Integer variable (for arithmetic)
    SYMVAR_ARRAY,   // Array variable (bash extension)
    SYMVAR_FUNCTION // Function definition
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
    SCOPE_GLOBAL,     // Global shell scope
    SCOPE_FUNCTION,   // Function local scope
    SCOPE_LOOP,       // Loop iteration scope (for/while)
    SCOPE_SUBSHELL,   // Subshell scope
    SCOPE_CONDITIONAL // Conditional execution scope (if/case)
} scope_type_t;

// Variable entry structure
struct symvar {
    char *name;           // Variable name
    char *value;          // Variable value (string representation)
    symvar_type_t type;   // Variable type
    symvar_flags_t flags; // Variable flags
    size_t scope_level;   // Scope level where defined
    symvar_t *next;       // Next variable in hash chain
};

// Symbol table scope structure
struct symtable_scope {
    scope_type_t scope_type;  // Type of scope
    size_t level;             // Scope nesting level
    size_t hash_size;         // Hash table size
    symvar_t **vars;          // Hash table of variables
    symtable_scope_t *parent; // Parent scope
    char *scope_name;         // Name of scope (for debugging)
};

// Symbol table manager
typedef struct {
    symtable_scope_t *current_scope; // Current active scope
    symtable_scope_t *global_scope;  // Global scope reference
    size_t max_scope_level;          // Maximum nesting depth
    bool debug_mode;                 // Debug output enabled
} symtable_manager_t;

// Legacy compatibility structures (for string management system)
typedef enum {
    SYM_STR,
    SYM_FUNC,
} symbol_type_t;

typedef struct symtable_entry {
    char *name;
    symbol_type_t val_type;
    char *val;
    unsigned int flags;
    struct symtable_entry *next;
    struct node *func_body;
} symtable_entry_t;

typedef struct {
    size_t level;
    symtable_entry_t *head, *tail;
} symtable_t;

#define MAX_SYMTAB 256
typedef struct {
    size_t symtable_count;
    symtable_t *symtable_list[MAX_SYMTAB];
    symtable_t *global_symtable, *local_symtable;
} symtable_stack_t;

// ============================================================================
// CORE MODERN API
// ============================================================================

// Manager lifecycle
symtable_manager_t *symtable_manager_new(void);
void symtable_manager_free(symtable_manager_t *manager);
void symtable_manager_set_debug(symtable_manager_t *manager, bool debug);

// Scope management
int symtable_push_scope(symtable_manager_t *manager, scope_type_t type,
                        const char *name);
int symtable_pop_scope(symtable_manager_t *manager);
size_t symtable_current_level(symtable_manager_t *manager);
const char *symtable_current_scope_name(symtable_manager_t *manager);

// Low-level variable operations
int symtable_set_var(symtable_manager_t *manager, const char *name,
                     const char *value, symvar_flags_t flags);
int symtable_set_local_var(symtable_manager_t *manager, const char *name,
                           const char *value);
int symtable_set_global_var(symtable_manager_t *manager, const char *name,
                            const char *value);
char *symtable_get_var(symtable_manager_t *manager, const char *name);
int symtable_unset_var(symtable_manager_t *manager, const char *name);
bool symtable_var_exists(symtable_manager_t *manager, const char *name);

// Export/environment operations
int symtable_export_var(symtable_manager_t *manager, const char *name);
char **symtable_get_environ(symtable_manager_t *manager);
void symtable_free_environ(char **environ);

// Debugging and introspection
void symtable_dump_scope(symtable_manager_t *manager, scope_type_t scope);
void symtable_dump_all_scopes(symtable_manager_t *manager);

// ============================================================================
// CONVENIENCE API (High-level functions for common operations)
// ============================================================================

// Get access to the global symbol table manager
symtable_manager_t *symtable_get_global_manager(void);

// Basic variable operations (global scope)
char *symtable_get_global(const char *name);
char *symtable_get_global_default(const char *name, const char *default_value);
int symtable_set_global(const char *name, const char *value);
bool symtable_exists_global(const char *name);
int symtable_unset_global(const char *name);

// Integer variable operations
int symtable_get_global_int(const char *name, int default_value);
int symtable_set_global_int(const char *name, int value);

// Boolean variable operations
bool symtable_get_global_bool(const char *name, bool default_value);
int symtable_set_global_bool(const char *name, bool value);

// Export/environment operations
int symtable_export_global(const char *name);
int symtable_unexport_global(const char *name);

// Special variable operations
int symtable_set_special_global(const char *name, const char *value);
char *symtable_get_special_global(const char *name);

// Read-only variable operations
int symtable_set_readonly_global(const char *name, const char *value);

// Debugging and introspection
void symtable_debug_dump_global_scope(void);
void symtable_debug_dump_all_scopes(void);
size_t symtable_count_global_vars(void);

// Environment array operations
char **symtable_get_environment_array(void);
void symtable_free_environment_array(char **env);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define symtable_set(mgr, name, value)                                         \
    symtable_set_var(mgr, name, value, SYMVAR_NONE)
#define symtable_get(mgr, name) symtable_get_var(mgr, name)
#define symtable_export(mgr, name) symtable_export_var(mgr, name)

// Backward compatibility macros
#define get_global_var(name) symtable_get_global(name)
#define set_global_var(name, value) symtable_set_global(name, value)
#define get_global_var_default(name, def) symtable_get_global_default(name, def)
#define export_global_var(name) symtable_export_global(name)

// Advanced operations (direct modern API access)
#define symtable_manager() symtable_get_global_manager()
#define symtable_push_function_scope(name)                                     \
    symtable_push_scope(symtable_manager(), SCOPE_FUNCTION, name)
#define symtable_push_loop_scope(name)                                         \
    symtable_push_scope(symtable_manager(), SCOPE_LOOP, name)
#define symtable_push_subshell_scope(name)                                     \
    symtable_push_scope(symtable_manager(), SCOPE_SUBSHELL, name)
#define symtable_pop_current_scope() symtable_pop_scope(symtable_manager())

// ============================================================================
// SYSTEM INTERFACE (Essential functions for shell operation)
// ============================================================================

// Shell initialization and cleanup
void init_symtable(void);
void free_global_symtable(void);

// Special variable management
void set_exit_status(int status);

// Environment interface
char **get_environ_array(void);
void free_environ_array(char **env);

// ============================================================================
// LEGACY COMPATIBILITY (For string management and other subsystems)
// ============================================================================

// Legacy flag definitions
#define FLAG_EXPORT (1 << 0)
#define FLAG_READONLY (1 << 1)
#define FLAG_CMD_EXPORT (1 << 2)
#define FLAG_LOCAL (1 << 3)
#define FLAG_ALLCAPS (1 << 4)
#define FLAG_ALLSMALL (1 << 5)
#define FLAG_FUNCTRACE (1 << 6)
#define FLAG_INTVAL (1 << 7)
#define FLAG_SPECIAL_VAR (1 << 8)
#define FLAG_TEMP_VAR (1 << 9)

// Legacy functions (for string management system)
symtable_t *new_symtable(size_t level);
symtable_t *symtable_stack_push(void);
symtable_t *symtable_stack_pop(void);
int remove_from_symtable(symtable_t *symtable, symtable_entry_t *entry);
symtable_entry_t *add_to_symtable(char *name);
symtable_entry_t *lookup_symbol(symtable_t *symtable, const char *name);
symtable_entry_t *get_symtable_entry(const char *name);
symtable_t *get_local_symtable(void);
symtable_t *get_global_symtable(void);
symtable_stack_t *get_symtable_stack(void);
void free_symtable(symtable_t *symtable);
void symtable_entry_setval(symtable_entry_t *entry, char *val);

#endif // SYMTABLE_H
