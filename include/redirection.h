#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "node.h"
#include <stdbool.h>

/**
 * I/O Redirection Header for Lusush Shell
 * 
 * Provides comprehensive I/O redirection functionality including:
 * - Basic output redirection (>)
 * - Append redirection (>>)
 * - Input redirection (<)
 * - Error redirection (2>, 2>>)
 * - Combined redirection (&>)
 * - Here strings (<<<)
 * - Here documents (<<, <<-)
 */
#include "node.h"
#include "executor.h"

// Main redirection setup function
// Called by the executor to setup all redirections for a command
int setup_redirections(executor_t *executor, node_t *command);

// Utility functions for redirection handling
bool is_redirection_node(node_t *node);
int count_redirections(node_t *command);

// Redirection state management
typedef struct redirection_state {
    int saved_stdin;     // Saved stdin file descriptor
    int saved_stdout;    // Saved stdout file descriptor
    int saved_stderr;    // Saved stderr file descriptor
    bool stdin_saved;    // Whether stdin was saved
    bool stdout_saved;   // Whether stdout was saved
    bool stderr_saved;   // Whether stderr was saved
} redirection_state_t;

// Save and restore file descriptors for proper cleanup
int save_file_descriptors(redirection_state_t *state);
int restore_file_descriptors(redirection_state_t *state);

// Error handling
void redirection_error(const char *message);

#endif /* REDIRECTION_H */