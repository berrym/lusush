#ifndef EXEC_H
#define EXEC_H

#include "../include/node.h"

// Command info structure for improved command handling
typedef struct command_info {
    char **argv;       // Argument vector
    size_t argc;       // Argument count
    size_t capacity;   // Allocated capacity
} command_info_t;

// Execution functions for different node types
int do_basic_command(node_t *n);
int do_redirect(node_t *n);
int do_if_clause(node_t *n);
int do_for_loop(node_t *n);
int do_while_loop(node_t *n);
int do_until_loop(node_t *n);
int do_case_clause(node_t *n);
int do_function_def(node_t *n);
int do_subshell(node_t *n);
int do_background(node_t *n);
int do_and_or(node_t *n);
int do_list(node_t *n);
int do_group(node_t *n);

// Helper functions
char *search_path(char *fn);
int do_exec_cmd(int argc, char **argv);
int apply_redirects(node_t *n);
int restore_redirects(void);

#endif
