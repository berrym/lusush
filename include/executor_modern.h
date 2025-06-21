/**
 * Modern Execution Engine for POSIX Shell
 * 
 * This execution engine is designed specifically to work with the modern
 * tokenizer and parser. It handles the new AST structure and provides
 * clean, efficient execution of shell commands and control structures.
 */

#ifndef EXECUTOR_MODERN_H
#define EXECUTOR_MODERN_H

#include "parser_modern.h"
#include "symtable_modern.h"
#include "node.h"
#include <stdbool.h>

// Execution context for maintaining state
typedef struct executor_modern {
    bool interactive;                // Interactive mode flag
    bool debug;                     // Debug mode flag
    int exit_status;                // Last command exit status
    const char *error_message;      // Last error message
    bool has_error;                 // Error flag
    symtable_manager_t *symtable;   // Modern symbol table manager
} executor_modern_t;

// Main execution interface
executor_modern_t *executor_modern_new(void);
void executor_modern_free(executor_modern_t *executor);

// Primary execution functions
int executor_modern_execute(executor_modern_t *executor, node_t *ast);
int executor_modern_execute_command_line(executor_modern_t *executor, const char *input);

// Configuration
void executor_modern_set_debug(executor_modern_t *executor, bool debug);
void executor_modern_set_interactive(executor_modern_t *executor, bool interactive);

// Error handling
bool executor_modern_has_error(executor_modern_t *executor);
const char *executor_modern_error(executor_modern_t *executor);

#endif // EXECUTOR_MODERN_H
