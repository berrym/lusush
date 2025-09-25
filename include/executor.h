/**
 * Execution Engine for POSIX Shell
 *
 * This execution engine is designed specifically to work with the
 * tokenizer and parser. It handles the AST structure and provides
 * clean, efficient execution of shell commands and control structures.
 */

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "node.h"
#include "parser.h"
#include "symtable.h"

#include <stdbool.h>
#include <sys/types.h>

// Function parameter definition
typedef struct function_param {
    char *name;                    // Parameter name
    char *default_value;          // Default value (NULL if required)
    bool is_required;             // True if parameter is required
    struct function_param *next;  // Next parameter in list
} function_param_t;

// Function definition storage
typedef struct function_def {
    char *name;                   // Function name
    node_t *body;                 // Function body AST
    function_param_t *params;     // Parameter list (NULL for no params)
    int param_count;              // Number of parameters
    struct function_def *next;    // Next function in list
} function_def_t;

// Job states
typedef enum { JOB_RUNNING, JOB_STOPPED, JOB_DONE } job_state_t;

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
    LOOP_NORMAL,  // Normal execution
    LOOP_BREAK,   // Break out of loop
    LOOP_CONTINUE // Continue to next iteration
} loop_control_t;

// Execution context for maintaining state
typedef struct executor {
    bool interactive;             // Interactive mode flag
    bool debug;                   // Debug mode flag
    int exit_status;              // Last command exit status
    const char *error_message;    // Last error message
    bool has_error;               // Error flag
    symtable_manager_t *symtable; // Symbol table manager
    function_def_t *functions;    // Function definition table
    job_t *jobs;                  // Job control list
    int next_job_id;              // Next job ID to assign
    pid_t shell_pgid;             // Shell process group ID
    loop_control_t loop_control;  // Loop control state
    int loop_depth;               // Current loop nesting depth
    
    // Script execution context for debugging
    char *current_script_file;    // Current script file being executed
    int current_script_line;      // Current line number in script
    bool in_script_execution;     // True if executing from script file

    // Expansion error tracking
    bool expansion_error;         // True if error occurred during expansion
    int expansion_exit_status;    // Exit status from expansion errors

} executor_t;

// Main execution interface
executor_t *executor_new(void);
executor_t *executor_new_with_symtable(symtable_manager_t *symtable);
void executor_free(executor_t *executor);

// Primary execution functions
int executor_execute(executor_t *executor, node_t *ast);
int executor_execute_command_line(executor_t *executor, const char *input);

// Configuration
void executor_set_debug(executor_t *executor, bool debug);
void executor_set_interactive(executor_t *executor, bool interactive);
void executor_set_symtable(executor_t *executor, symtable_manager_t *symtable);

// Error handling
bool executor_has_error(executor_t *executor);
const char *executor_error(executor_t *executor);

// Variable expansion functions
char *expand_if_needed(executor_t *executor, const char *text);

// Job control functions
int executor_execute_background(executor_t *executor, node_t *command);
job_t *executor_add_job(executor_t *executor, pid_t pgid,
                        const char *command_line);
void executor_update_job_status(executor_t *executor);
job_t *executor_find_job(executor_t *executor, int job_id);
void executor_remove_job(executor_t *executor, int job_id);
int executor_builtin_jobs(executor_t *executor, char **argv);
int executor_builtin_fg(executor_t *executor, char **argv);
int executor_builtin_bg(executor_t *executor, char **argv);

// Function parameter management utilities
function_param_t *create_function_param(const char *name, const char *default_value);
void free_function_params(function_param_t *params);

// Script context management for debugging
void executor_set_script_context(executor_t *executor, const char *script_file, int line_number);
void executor_clear_script_context(executor_t *executor);
const char *executor_get_current_script_file(executor_t *executor);
int executor_get_current_script_line(executor_t *executor);

#endif // EXECUTOR_H
