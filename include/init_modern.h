/*
 * Modern initialization system for Lusush shell
 * Provides clean interface using modern components while preserving linenoise for interactive input
 */

#ifndef INIT_MODERN_H
#define INIT_MODERN_H

#include "symtable_modern.h"
#include <stdio.h>
#include <stdbool.h>

// Shell type constants (preserved for compatibility)
#define NORMAL_SHELL 0
#define INTERACTIVE_SHELL 1
#define LOGIN_SHELL 2

// Forward declarations
extern bool exit_flag;

// Modern initialization function
// Returns configured symtable_manager for the shell session
// Preserves linenoise for interactive input while using modern components
symtable_manager_t *init_modern(int argc, char **argv, FILE **in);

// Shell type detection (preserved interface)
int shell_type(void);

// Cleanup function for modern components
void cleanup_modern(symtable_manager_t *symtable);

#endif // INIT_MODERN_H