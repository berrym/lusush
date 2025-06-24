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
#include "symtable.h"
#include "node.h"
#include <stdbool.h>
#include <sys/types.h>

// Function definition storage
typedef struct function_def {
    char *name;
    node_t *body;
    struct function_def *next;
} function_def_t;

// Job states
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

// Process in a job
typedef struct process {
    pid_t pid;
    char *command;
    int status;
    struct process *next;
} process_t;

// Job control structure
typedef struct job {
    int job_id;
    pid_t pgid;
    job_state_t state;
    bool foreground;
    process_t *processes;
    char *command_line;
    struct job *next;
} job_t;

// Loop control states
typedef enum {
    LOOP_NORMAL,    // Normal execution
    LOOP_BREAK,     // Break out of loop
    LOOP_CONTINUE   // Continue to next iteration
} loop_control_t;

// Execution context for maintaining state
typedef struct executor_modern {
    bool interactive;                // Interactive mode flag
    bool debug;                     // Debug mode flag
    int exit_status;                // Last command exit status
    const char *error_message;      // Last error message
    bool has_error;                 // Error flag
    symtable_manager_t *symtable;   // Modern symbol table manager
    function_def_t *functions;      // Function definition table
    job_t *jobs;                    // Job control list
    int next_job_id;                // Next job ID to assign
    pid_t shell_pgid;               // Shell process group ID
    loop_control_t loop_control;    // Loop control state
    int loop_depth;                 // Current loop nesting depth
} executor_modern_t;

// Main execution interface
executor_modern_t *executor_modern_new(void);
executor_modern_t *executor_modern_new_with_symtable(symtable_manager_t *symtable);
void executor_modern_free(executor_modern_t *executor);

// Primary execution functions
int executor_modern_execute(executor_modern_t *executor, node_t *ast);
int executor_modern_execute_command_line(executor_modern_t *executor, const char *input);

// Configuration
void executor_modern_set_debug(executor_modern_t *executor, bool debug);
void executor_modern_set_interactive(executor_modern_t *executor, bool interactive);
void executor_modern_set_symtable(executor_modern_t *executor, symtable_manager_t *symtable);

// Error handling
bool executor_modern_has_error(executor_modern_t *executor);
const char *executor_modern_error(executor_modern_t *executor);

// Variable expansion functions
char *expand_if_needed_modern(executor_modern_t *executor, const char *text);

// Job control functions
int executor_modern_execute_background(executor_modern_t *executor, node_t *command);
job_t *executor_modern_add_job(executor_modern_t *executor, pid_t pgid, const char *command_line);
void executor_modern_update_job_status(executor_modern_t *executor);
job_t *executor_modern_find_job(executor_modern_t *executor, int job_id);
void executor_modern_remove_job(executor_modern_t *executor, int job_id);
int executor_modern_builtin_jobs(executor_modern_t *executor, char **argv);
int executor_modern_builtin_fg(executor_modern_t *executor, char **argv);
int executor_modern_builtin_bg(executor_modern_t *executor, char **argv);

#endif // EXECUTOR_MODERN_H
