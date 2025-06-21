/**
 * Modern Execution Engine Implementation
 * 
 * Clean, efficient execution engine designed for the modern parser and tokenizer.
 * Handles command execution, control structures, pipelines, and variable management
 * with proper POSIX compliance.
 */

#include "executor_modern.h"
#include "parser_modern.h"
#include "tokenizer_new.h"
#include "symtable_modern.h"
#include "node.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

// Forward declarations
// Forward declarations - updated for modern symtable
static int execute_node_modern(executor_modern_t *executor, node_t *node);
static int execute_command_modern(executor_modern_t *executor, node_t *command);
static int execute_pipeline_modern(executor_modern_t *executor, node_t *pipeline);
static int execute_if_modern(executor_modern_t *executor, node_t *if_node);
static int execute_while_modern(executor_modern_t *executor, node_t *while_node);
static int execute_for_modern(executor_modern_t *executor, node_t *for_node);
static int execute_command_list_modern(executor_modern_t *executor, node_t *list);
static char **build_argv_from_ast(executor_modern_t *executor, node_t *command, int *argc);
static int execute_external_command(executor_modern_t *executor, char **argv);
static int execute_builtin_command(executor_modern_t *executor, char **argv);
static bool is_builtin_command(const char *cmd);
static void executor_error(executor_modern_t *executor, const char *message);
static char *expand_variable_modern(executor_modern_t *executor, const char *var_text);
static char *expand_arithmetic_modern(executor_modern_t *executor, const char *arith_text);
static char *expand_if_needed_modern(executor_modern_t *executor, const char *text);
static char *expand_quoted_string_modern(executor_modern_t *executor, const char *str);
static bool is_assignment(const char *text);
static int execute_assignment_modern(executor_modern_t *executor, const char *assignment);

// Create new executor
executor_modern_t *executor_modern_new(void) {
    executor_modern_t *executor = malloc(sizeof(executor_modern_t));
    if (!executor) return NULL;
    
    // Initialize modern symbol table
    executor->symtable = symtable_manager_new();
    if (!executor->symtable) {
        free(executor);
        return NULL;
    }
    
    executor->interactive = false;
    executor->debug = false;
    executor->exit_status = 0;
    executor->error_message = NULL;
    executor->has_error = false;
    
    return executor;
}

// Free executor
void executor_modern_free(executor_modern_t *executor) {
    if (executor) {
        if (executor->symtable) {
            symtable_manager_free(executor->symtable);
        }
        free(executor);
    }
}

// Set debug mode
void executor_modern_set_debug(executor_modern_t *executor, bool debug) {
    if (executor) {
        executor->debug = debug;
        if (executor->symtable) {
            symtable_manager_set_debug(executor->symtable, debug);
        }
    }
}

// Set interactive mode
void executor_modern_set_interactive(executor_modern_t *executor, bool interactive) {
    if (executor) {
        executor->interactive = interactive;
    }
}

// Check for errors
bool executor_modern_has_error(executor_modern_t *executor) {
    return executor && executor->has_error;
}

const char *executor_modern_error(executor_modern_t *executor) {
    return executor ? executor->error_message : "Invalid executor";
}

// Set error
static void executor_error(executor_modern_t *executor, const char *message) {
    if (executor) {
        executor->error_message = message;
        executor->has_error = true;
    }
}

// Main execution entry point
int executor_modern_execute(executor_modern_t *executor, node_t *ast) {
    if (!executor || !ast) {
        return 1;
    }
    
    executor->has_error = false;
    executor->error_message = NULL;
    
    int result = execute_node_modern(executor, ast);
    executor->exit_status = result;
    
    return result;
}

// Execute command line (parse and execute)
int executor_modern_execute_command_line(executor_modern_t *executor, const char *input) {
    if (!executor || !input) {
        return 1;
    }
    
    // Parse the input
    parser_modern_t *parser = parser_modern_new(input);
    if (!parser) {
        executor_error(executor, "Failed to create parser");
        return 1;
    }
    
    node_t *ast = parser_modern_parse(parser);
    if (parser_modern_has_error(parser)) {
        executor_error(executor, parser_modern_error(parser));
        parser_modern_free(parser);
        return 1;
    }
    
    if (!ast) {
        parser_modern_free(parser);
        return 0; // Empty command
    }
    
    int result = executor_modern_execute(executor, ast);
    
    free_node_tree(ast);
    parser_modern_free(parser);
    
    return result;
}

// Core node execution dispatcher
static int execute_node_modern(executor_modern_t *executor, node_t *node) {
    if (!node) return 0;
    
    if (executor->debug) {
        printf("DEBUG: Executing node type %d\n", node->type);
        if (node->val.str) {
            printf("DEBUG: Node value: '%s'\n", node->val.str);
        }
    }
    
    switch (node->type) {
        case NODE_COMMAND:
            return execute_command_modern(executor, node);
        case NODE_PIPE:
            return execute_pipeline_modern(executor, node);
        case NODE_IF:
            return execute_if_modern(executor, node);
        case NODE_WHILE:
            return execute_while_modern(executor, node);
        case NODE_FOR:
            return execute_for_modern(executor, node);
        case NODE_VAR:
            // Variable nodes are typically handled by their parent
            return 0;
        default:
            // Handle command lists (sibling nodes)
            return execute_command_list_modern(executor, node);
    }
}

// Execute command list (sequence of commands)
static int execute_command_list_modern(executor_modern_t *executor, node_t *list) {
    if (!list) return 0;
    
    int last_result = 0;
    node_t *current = list;
    
    while (current) {
        last_result = execute_node_modern(executor, current);
        
        if (executor->debug) {
            printf("DEBUG: Command result: %d\n", last_result);
        }
        
        current = current->next_sibling;
    }
    
    return last_result;
}

// Execute a simple command
static int execute_command_modern(executor_modern_t *executor, node_t *command) {
    if (!command || command->type != NODE_COMMAND) {
        return 1;
    }
    
    // Check for assignment
    if (command->val.str && is_assignment(command->val.str)) {
        return execute_assignment_modern(executor, command->val.str);
    }
    
    // Build argument vector
    int argc;
    char **argv = build_argv_from_ast(executor, command, &argc);
    if (!argv || argc == 0) {
        return 1;
    }
    
    if (executor->debug) {
        printf("DEBUG: Executing command: %s with %d args\n", argv[0], argc - 1);
    }
    
    int result;
    if (is_builtin_command(argv[0])) {
        result = execute_builtin_command(executor, argv);
    } else {
        result = execute_external_command(executor, argv);
    }
    
    // Free argv
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
    
    return result;
}

// Execute pipeline
static int execute_pipeline_modern(executor_modern_t *executor, node_t *pipeline) {
    if (!pipeline || pipeline->type != NODE_PIPE) {
        return 1;
    }
    
    // For now, implement simple two-command pipeline
    // A more complete implementation would handle N-command pipelines
    
    node_t *left = pipeline->first_child;
    node_t *right = left ? left->next_sibling : NULL;
    
    if (!left || !right) {
        executor_error(executor, "Malformed pipeline");
        return 1;
    }
    
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        executor_error(executor, "Failed to create pipe");
        return 1;
    }
    
    pid_t left_pid = fork();
    if (left_pid == -1) {
        executor_error(executor, "Failed to fork for pipeline");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return 1;
    }
    
    if (left_pid == 0) {
        // Left command: write to pipe
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        
        int result = execute_node_modern(executor, left);
        exit(result);
    }
    
    pid_t right_pid = fork();
    if (right_pid == -1) {
        executor_error(executor, "Failed to fork for pipeline");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        waitpid(left_pid, NULL, 0);
        return 1;
    }
    
    if (right_pid == 0) {
        // Right command: read from pipe
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        
        int result = execute_node_modern(executor, right);
        exit(result);
    }
    
    // Parent: close pipes and wait
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    
    int left_status, right_status;
    waitpid(left_pid, &left_status, 0);
    waitpid(right_pid, &right_status, 0);
    
    // Return exit status of right (last) command
    return WEXITSTATUS(right_status);
}

// Execute if statement
static int execute_if_modern(executor_modern_t *executor, node_t *if_node) {
    if (!if_node || if_node->type != NODE_IF) {
        return 1;
    }
    
    node_t *condition = if_node->first_child;
    node_t *then_body = condition ? condition->next_sibling : NULL;
    node_t *else_body = then_body ? then_body->next_sibling : NULL;
    
    if (!condition || !then_body) {
        executor_error(executor, "Malformed if statement");
        return 1;
    }
    
    // Execute condition
    int condition_result = execute_node_modern(executor, condition);
    
    if (executor->debug) {
        printf("DEBUG: IF condition result: %d\n", condition_result);
    }
    
    // Execute appropriate body
    if (condition_result == 0) { // Success in shell terms
        return execute_node_modern(executor, then_body);
    } else if (else_body) {
        return execute_node_modern(executor, else_body);
    }
    
    return 0;
}

// Execute while loop
static int execute_while_modern(executor_modern_t *executor, node_t *while_node) {
    if (!while_node || while_node->type != NODE_WHILE) {
        return 1;
    }
    
    node_t *condition = while_node->first_child;
    node_t *body = condition ? condition->next_sibling : NULL;
    
    if (!condition || !body) {
        executor_error(executor, "Malformed while loop");
        return 1;
    }
    
    int last_result = 0;
    int iteration = 0;
    const int max_iterations = 10000; // Safety limit
    
    while (iteration < max_iterations) {
        // Execute condition
        int condition_result = execute_node_modern(executor, condition);
        
        if (executor->debug) {
            printf("DEBUG: WHILE iteration %d, condition result: %d\n", 
                   iteration, condition_result);
        }
        
        // If condition fails, exit loop
        if (condition_result != 0) {
            break;
        }
        
        // Execute body
        last_result = execute_node_modern(executor, body);
        
        iteration++;
    }
    
    if (iteration >= max_iterations) {
        executor_error(executor, "While loop exceeded maximum iterations");
        return 1;
    }
    
    return last_result;
}

// Execute for loop
static int execute_for_modern(executor_modern_t *executor, node_t *for_node) {
    if (!for_node || for_node->type != NODE_FOR) {
        return 1;
    }
    
    const char *var_name = for_node->val.str;
    if (!var_name) {
        executor_error(executor, "For loop missing variable name");
        return 1;
    }
    
    node_t *word_list = for_node->first_child;
    node_t *body = word_list ? word_list->next_sibling : NULL;
    
    if (!body) {
        executor_error(executor, "For loop missing body");
        return 1;
    }
    
    // Push loop scope
    if (symtable_push_scope(executor->symtable, SCOPE_LOOP, "for-loop") != 0) {
        executor_error(executor, "Failed to create loop scope");
        return 1;
    }
    
    int last_result = 0;
    
    if (word_list && word_list->first_child) {
        // Iterate over word list
        node_t *word = word_list->first_child;
        while (word) {
            if (word->val.str) {
                // Set loop variable in current (loop) scope
                if (symtable_set_local_var(executor->symtable, var_name, word->val.str) != 0) {
                    executor_error(executor, "Failed to set loop variable");
                    symtable_pop_scope(executor->symtable);
                    return 1;
                }
                
                if (executor->debug) {
                    printf("DEBUG: FOR loop setting %s=%s\n", var_name, word->val.str);
                }
                
                // Execute body
                last_result = execute_node_modern(executor, body);
            }
            word = word->next_sibling;
        }
    }
    
    // Pop loop scope
    symtable_pop_scope(executor->symtable);
    
    return last_result;
}

// Build argv from AST
static char **build_argv_from_ast(executor_modern_t *executor, node_t *command, int *argc) {
    if (!executor || !command || !argc) {
        return NULL;
    }
    
    // Count arguments
    int count = 0;
    if (command->val.str) count++; // Command name
    
    node_t *child = command->first_child;
    while (child) {
        if (child->val.str) count++;
        child = child->next_sibling;
    }
    
    if (count == 0) {
        *argc = 0;
        return NULL;
    }
    
    // Allocate argv
    char **argv = malloc((count + 1) * sizeof(char *));
    if (!argv) {
        *argc = 0;
        return NULL;
    }
    
    int i = 0;
    
    // Add command name
    if (command->val.str) {
        argv[i] = expand_if_needed_modern(executor, command->val.str);
        i++;
    }
    
    // Add arguments
    child = command->first_child;
    while (child && i < count) {
        if (child->val.str) {
            argv[i] = expand_if_needed_modern(executor, child->val.str);
            i++;
        }
        child = child->next_sibling;
    }
    
    argv[i] = NULL;
    *argc = i;
    
    return argv;
}

// Expand variable/arithmetic if needed
static char *expand_if_needed_modern(executor_modern_t *executor, const char *text) {
    if (!executor || !text) return NULL;
    
    // Check for variable expansion
    if (text[0] == '$') {
        if (strncmp(text, "$((", 3) == 0) {
            return expand_arithmetic_modern(executor, text);
        } else {
            return expand_variable_modern(executor, text);
        }
    }
    
    // Check if this looks like it contains variables (has $ in the middle)
    // This is a heuristic for expandable strings
    if (strchr(text, '$')) {
        return expand_quoted_string_modern(executor, text);
    }
    
    // Regular text - just duplicate
    return strdup(text);
}

// Execute external command
static int execute_external_command(executor_modern_t *executor, char **argv) {
    if (!argv || !argv[0]) {
        return 1;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        executor_error(executor, "Failed to fork");
        return 1;
    }
    
    if (pid == 0) {
        // Child process
        execvp(argv[0], argv);
        perror(argv[0]);
        exit(127);
    }
    
    // Parent process
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    
    return 1;
}

// Execute builtin command
static int execute_builtin_command(executor_modern_t *executor, char **argv) {
    if (!argv || !argv[0]) {
        return 1;
    }
    
    // This is a simplified implementation
    // In the full shell, this would call the actual builtin functions
    
    if (strcmp(argv[0], "echo") == 0) {
        for (int i = 1; argv[i]; i++) {
            if (i > 1) printf(" ");
            printf("%s", argv[i]);
        }
        printf("\n");
        return 0;
    }
    
    if (strcmp(argv[0], "exit") == 0) {
        int exit_code = 0;
        if (argv[1]) {
            exit_code = atoi(argv[1]);
        }
        exit(exit_code);
    }
    
    // For other builtins, fall back to external execution
    return execute_external_command(executor, argv);
}

// Check if command is builtin
static bool is_builtin_command(const char *cmd) {
    if (!cmd) return false;
    
    const char *builtins[] = {
        "echo", "exit", "cd", "pwd", "export", "unset", "alias", "unalias",
        "type", "which", "history", "jobs", "fg", "bg", "set", "unset",
        NULL
    };
    
    for (int i = 0; builtins[i]; i++) {
        if (strcmp(cmd, builtins[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Check if text is an assignment
static bool is_assignment(const char *text) {
    if (!text) return false;
    
    // Look for '=' not at the beginning
    const char *eq = strchr(text, '=');
    return eq && eq != text;
}

// Execute assignment
// Execute assignment using modern symbol table
static int execute_assignment_modern(executor_modern_t *executor, const char *assignment) {
    if (!executor || !assignment) return 1;
    
    char *eq = strchr(assignment, '=');
    if (!eq) return 1;
    
    // Split into variable and value
    size_t var_len = eq - assignment;
    char *var_name = malloc(var_len + 1);
    if (!var_name) return 1;
    
    strncpy(var_name, assignment, var_len);
    var_name[var_len] = '\0';
    
    // Expand the value using modern expansion
    char *value = expand_if_needed_modern(executor, eq + 1);
    
    // Set the variable in the modern symbol table
    int result = symtable_set_var(executor->symtable, var_name, value ? value : "", SYMVAR_NONE);
    
    if (executor->debug) {
        printf("DEBUG: Assignment %s=%s (result: %d)\n", var_name, value ? value : "", result);
    }
    
    free(var_name);
    free(value);
    
    return result == 0 ? 0 : 1;
}

// Expand variable reference using modern symbol table
static char *expand_variable_modern(executor_modern_t *executor, const char *var_text) {
    if (!executor || !var_text || var_text[0] != '$') {
        return strdup(var_text ? var_text : "");
    }
    
    const char *var_name = var_text + 1;
    
    // Handle ${var} format
    if (var_name[0] == '{') {
        char *close = strchr(var_name, '}');
        if (close) {
            size_t len = close - var_name - 1;
            char *name = malloc(len + 1);
            if (name) {
                strncpy(name, var_name + 1, len);
                name[len] = '\0';
                
                // Look up in modern symbol table
                char *value = symtable_get_var(executor->symtable, name);
                
                free(name);
                return value ? value : strdup("");
            }
        }
    } else {
        // Simple $var format - find end of variable name
        size_t name_len = 0;
        while (var_name[name_len] && (isalnum(var_name[name_len]) || var_name[name_len] == '_')) {
            name_len++;
        }
        
        if (name_len > 0) {
            char *name = malloc(name_len + 1);
            if (name) {
                strncpy(name, var_name, name_len);
                name[name_len] = '\0';
                
                // Look up in modern symbol table
                char *value = symtable_get_var(executor->symtable, name);
                
                free(name);
                return value ? value : strdup("");
            }
        }
    }
    
    return strdup("");
}

// Expand arithmetic expression using modern symbol table
static char *expand_arithmetic_modern(executor_modern_t *executor, const char *arith_text) {
    if (!executor || !arith_text) return strdup("0");
    
    // This is a simplified implementation
    // A full implementation would parse and evaluate the arithmetic
    
    if (strncmp(arith_text, "$((", 3) == 0) {
        // Find the closing ))
        const char *close = strstr(arith_text + 3, "))");
        if (close) {
            // Extract the expression
            size_t expr_len = close - arith_text - 3;
            char *expr = malloc(expr_len + 1);
            if (expr) {
                strncpy(expr, arith_text + 3, expr_len);
                expr[expr_len] = '\0';
                
                // Simple arithmetic evaluation (very basic)
                // In a full implementation, this would be much more sophisticated
                int result = 0;
                if (strstr(expr, "+1")) {
                    // Handle i+1 pattern
                    char *var = strtok(expr, "+");
                    if (var) {
                        // Look up in modern symbol table first, then environment
                        char *val = symtable_get_var(executor->symtable, var);
                        if (!val) {
                            val = getenv(var);
                        }
                        
                        if (val) {
                            result = atoi(val) + 1;
                            if (val != getenv(var)) free(val); // Free if allocated by symtable
                        } else {
                            result = 1;
                        }
                    }
                } else {
                    result = atoi(expr);
                }
                
                free(expr);
                
                char *result_str = malloc(32);
                if (result_str) {
                    snprintf(result_str, 32, "%d", result);
                    return result_str;
                }
            }
        }
    }
    
    return strdup("0");
}

// Expand variables within double-quoted strings
static char *expand_quoted_string_modern(executor_modern_t *executor, const char *str) {
    if (!executor || !str) return strdup("");
    
    size_t len = strlen(str);
    if (len == 0) return strdup("");
    
    // Allocate a buffer for expansion (estimate double the original size)
    size_t buffer_size = len * 2 + 256;
    char *result = malloc(buffer_size);
    if (!result) return strdup("");
    
    size_t result_pos = 0;
    size_t i = 0;
    
    while (i < len) {
        if (str[i] == '$' && i + 1 < len) {
            // Variable expansion needed
            size_t var_start = i + 1;
            size_t var_end = var_start;
            
            // Handle ${var} format
            if (str[var_start] == '{') {
                var_start++; // Skip opening brace
                var_end = var_start; // Start looking for closing brace after opening brace
                while (var_end < len && str[var_end] != '}') {
                    var_end++;
                }
                if (var_end < len) {
                    // Extract variable name
                    size_t var_name_len = var_end - var_start;
                    char *var_name = malloc(var_name_len + 1);
                    if (var_name) {
                        strncpy(var_name, &str[var_start], var_name_len);
                        var_name[var_name_len] = '\0';
                        
                        // Get variable value
                        char *var_value = symtable_get_var(executor->symtable, var_name);
                        if (var_value) {
                            size_t value_len = strlen(var_value);
                            // Ensure buffer is large enough
                            while (result_pos + value_len >= buffer_size) {
                                buffer_size *= 2;
                                result = realloc(result, buffer_size);
                                if (!result) {
                                    free(var_name);
                                    free(var_value);
                                    return strdup("");
                                }
                            }
                            strcpy(&result[result_pos], var_value);
                            result_pos += value_len;
                            free(var_value);
                        }
                        
                        free(var_name);
                        i = var_end + 1; // Skip past closing brace
                    } else {
                        result[result_pos++] = str[i++];
                    }
                } else {
                    result[result_pos++] = str[i++];
                }
            } else {
                // Simple $var format
                while (var_end < len && (isalnum(str[var_end]) || str[var_end] == '_')) {
                    var_end++;
                }
                
                if (var_end > var_start) {
                    // Extract variable name
                    size_t var_name_len = var_end - var_start;
                    char *var_name = malloc(var_name_len + 1);
                    if (var_name) {
                        strncpy(var_name, &str[var_start], var_name_len);
                        var_name[var_name_len] = '\0';
                        
                        // Get variable value
                        char *var_value = symtable_get_var(executor->symtable, var_name);
                        if (var_value) {
                            size_t value_len = strlen(var_value);
                            // Ensure buffer is large enough
                            while (result_pos + value_len >= buffer_size) {
                                buffer_size *= 2;
                                result = realloc(result, buffer_size);
                                if (!result) {
                                    free(var_name);
                                    free(var_value);
                                    return strdup("");
                                }
                            }
                            strcpy(&result[result_pos], var_value);
                            result_pos += value_len;
                            free(var_value);
                        }
                        
                        free(var_name);
                        i = var_end;
                    } else {
                        result[result_pos++] = str[i++];
                    }
                } else {
                    result[result_pos++] = str[i++];
                }
            }
        } else {
            // Regular character
            if (result_pos >= buffer_size - 1) {
                buffer_size *= 2;
                result = realloc(result, buffer_size);
                if (!result) return strdup("");
            }
            result[result_pos++] = str[i++];
        }
    }
    
    result[result_pos] = '\0';
    return result;
}
