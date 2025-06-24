/**
 * Unified Symbol Table Convenience Interface
 * 
 * This header provides a simplified, unified interface for symbol table operations
 * that wraps the modern symtable API. It's designed to ease migration from the
 * legacy symtable interface while providing access to modern functionality.
 * 
 * Key Features:
 * - Simple function names for common operations
 * - Automatic handling of the global symbol table manager
 * - Proper default value handling
 * - Type-safe integer operations
 * - Direct access to modern API when needed
 * 
 * Migration Path:
 * - get_shell_varp(name, default) → symtable_get_global_default(name, default)
 * - set_shell_varp(name, value) → symtable_set_global(name, value)
 * - get_shell_vari(name, default) → symtable_get_global_int(name, default)
 * - set_shell_vari(name, value) → symtable_set_global_int(name, value)
 * - export_shell_var(name) → symtable_export_global(name)
 */

#ifndef SYMTABLE_UNIFIED_H
#define SYMTABLE_UNIFIED_H

#include "symtable_modern.h"
#include <stdbool.h>

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

// Convenience macros for backward compatibility
#define get_global_var(name) symtable_get_global(name)
#define set_global_var(name, value) symtable_set_global(name, value)
#define get_global_var_default(name, def) symtable_get_global_default(name, def)
#define export_global_var(name) symtable_export_global(name)

// Advanced operations (direct modern API access)
#define symtable_manager() symtable_get_global_manager()
#define symtable_push_function_scope(name) symtable_push_scope(symtable_manager(), SCOPE_FUNCTION, name)
#define symtable_push_loop_scope(name) symtable_push_scope(symtable_manager(), SCOPE_LOOP, name)
#define symtable_push_subshell_scope(name) symtable_push_scope(symtable_manager(), SCOPE_SUBSHELL, name)
#define symtable_pop_current_scope() symtable_pop_scope(symtable_manager())

// Debugging and introspection
void symtable_debug_dump_global_scope(void);
void symtable_debug_dump_all_scopes(void);
size_t symtable_count_global_vars(void);

// Environment array operations
char **symtable_get_environment_array(void);
void symtable_free_environment_array(char **env);

#endif // SYMTABLE_UNIFIED_H