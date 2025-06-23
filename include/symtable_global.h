/*
 * Global Symbol Table Management for Lusush Shell
 * 
 * This module provides a global modern symtable that replaces the legacy
 * symtable system while maintaining the same interface expectations.
 * It bridges the gap between legacy code that expects global symtable
 * access and the modern symtable_manager system.
 */

#ifndef SYMTABLE_GLOBAL_H
#define SYMTABLE_GLOBAL_H

#include "symtable_modern.h"
#include <stdbool.h>

// Global symtable manager access
int init_global_symtable(void);
symtable_manager_t *get_global_symtable_manager(void);
void free_global_symtable(void);

// Modern global variable operations
int set_global_var(const char *name, const char *value);
char *get_global_var(const char *name);
int export_global_var(const char *name, const char *value);
bool global_var_exists(const char *name);
int unset_global_var(const char *name);

// Environment operations
char **get_global_environ(void);
void free_global_environ(char **environ);

// Shell variable convenience functions
char *get_shell_varp(char *name, char *default_value);
int get_shell_vari(char *name, int default_value);
bool get_shell_varb(const char *name, bool default_value);

// Special variables
int set_special_var(const char *name, const char *value);
void set_exit_status(int status);

// Debug
void dump_global_symtable(void);

// Legacy compatibility functions (for gradual migration)
void init_symtable(void);
void *add_to_symtable(const char *name);
void symtable_entry_setval(void *entry, const char *value);
void *get_symtable_entry(const char *name);

#endif // SYMTABLE_GLOBAL_H