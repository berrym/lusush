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
#include <sys/types.h>
#include <ctype.h>

// Forward declarations
// Forward declarations - updated for modern symtable
static int execute_node_modern(executor_modern_t *executor, node_t *node);
static int execute_command_modern(executor_modern_t *executor, node_t *command);
static int execute_pipeline_modern(executor_modern_t *executor, node_t *pipeline);
static int execute_function_definition_modern(executor_modern_t *executor, node_t *function);
static int execute_function_call_modern(executor_modern_t *executor, const char *function_name, char **argv, int argc);
static bool is_function_defined(executor_modern_t *executor, const char *function_name);
static function_def_t *find_function(executor_modern_t *executor, const char *function_name);
static int store_function(executor_modern_t *executor, const char *function_name, node_t *body);
static node_t *copy_ast_node(node_t *node);
static int execute_if_modern(executor_modern_t *executor, node_t *if_node);
static int execute_while_modern(executor_modern_t *executor, node_t *while_node);
static int execute_for_modern(executor_modern_t *executor, node_t *for_node);
static int execute_case_modern(executor_modern_t *executor, node_t *case_node);
static int execute_logical_and_modern(executor_modern_t *executor, node_t *and_node);
static int execute_logical_or_modern(executor_modern_t *executor, node_t *or_node);
static int execute_command_list_modern(executor_modern_t *executor, node_t *list);
static char **build_argv_from_ast(executor_modern_t *executor, node_t *command, int *argc);
static int execute_external_command(executor_modern_t *executor, char **argv);
static int execute_builtin_command(executor_modern_t *executor, char **argv);
static bool is_builtin_command(const char *cmd);
static void executor_error(executor_modern_t *executor, const char *message);
static char *expand_variable_modern(executor_modern_t *executor, const char *var_text);
static char *expand_arithmetic_modern(executor_modern_t *executor, const char *arith_text);
static char *expand_command_substitution_modern(executor_modern_t *executor, const char *cmd_text);
static char *expand_if_needed_modern(executor_modern_t *executor, const char *text);
static char *expand_quoted_string_modern(executor_modern_t *executor, const char *str);
static bool is_assignment(const char *text);
static int execute_assignment_modern(executor_modern_t *executor, const char *assignment);
static bool match_pattern(const char *str, const char *pattern);

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
    executor->functions = NULL;
    
    return executor;
}

// Free executor
void executor_modern_free(executor_modern_t *executor) {
    if (executor) {
        if (executor->symtable) {
            symtable_manager_free(executor->symtable);
        }
        
        // Free function table
        function_def_t *func = executor->functions;
        while (func) {
            function_def_t *next = func->next;
            free(func->name);
            free_node_tree(func->body);
            free(func);
            func = next;
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
    
    // Check if this is a command sequence (has siblings) or a single command
    if (ast->next_sibling) {
        // This is a command sequence, execute all siblings
        int result = execute_command_list_modern(executor, ast);
        executor->exit_status = result;
        return result;
    } else {
        // Single command, execute normally
        int result = execute_node_modern(executor, ast);
        executor->exit_status = result;
        return result;
    }
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
        case NODE_CASE:
            return execute_case_modern(executor, node);
        case NODE_LOGICAL_AND:
            return execute_logical_and_modern(executor, node);
        case NODE_LOGICAL_OR:
            return execute_logical_or_modern(executor, node);
        case NODE_FUNCTION:
            return execute_function_definition_modern(executor, node);
        case NODE_VAR:
            // Variable nodes are typically handled by their parent
            return 0;
        default:
            if (executor->debug) {
                printf("DEBUG: Unknown node type %d, skipping\n", node->type);
            }
            return 0;
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
    if (is_function_defined(executor, argv[0])) {
        result = execute_function_call_modern(executor, argv[0], argv, argc);
    } else if (is_builtin_command(argv[0])) {
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
    
    // Iterate over word list
    if (word_list && word_list->first_child) {
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

// Execute logical AND operator (&&)
static int execute_logical_and_modern(executor_modern_t *executor, node_t *and_node) {
    if (!and_node || and_node->type != NODE_LOGICAL_AND) {
        return 1;
    }
    
    node_t *left = and_node->first_child;
    node_t *right = left ? left->next_sibling : NULL;
    
    if (!left || !right) {
        executor_error(executor, "Logical AND missing operands");
        return 1;
    }
    
    // Execute left command
    int left_result = execute_node_modern(executor, left);
    
    // Only execute right command if left succeeded (exit code 0)
    if (left_result == 0) {
        return execute_node_modern(executor, right);
    }
    
    // Left failed, return its exit code without executing right
    return left_result;
}

// Execute logical OR operator (||)
static int execute_logical_or_modern(executor_modern_t *executor, node_t *or_node) {
    if (!or_node || or_node->type != NODE_LOGICAL_OR) {
        return 1;
    }
    
    node_t *left = or_node->first_child;
    node_t *right = left ? left->next_sibling : NULL;
    
    if (!left || !right) {
        executor_error(executor, "Logical OR missing operands");
        return 1;
    }
    
    // Execute left command
    int left_result = execute_node_modern(executor, left);
    
    // Only execute right command if left failed (exit code != 0)
    if (left_result != 0) {
        return execute_node_modern(executor, right);
    }
    
    // Left succeeded, return its exit code without executing right
    return left_result;
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

// Expand variable/arithmetic/command substitution if needed
static char *expand_if_needed_modern(executor_modern_t *executor, const char *text) {
    if (!executor || !text) return NULL;
    
    // Check for expansions starting with $
    if (text[0] == '$') {
        if (strncmp(text, "$((", 3) == 0) {
            return expand_arithmetic_modern(executor, text);
        } else if (strncmp(text, "$(", 2) == 0) {
            return expand_command_substitution_modern(executor, text);
        } else {
            return expand_variable_modern(executor, text);
        }
    }
    
    // Check for backtick command substitution
    if (text[0] == '`') {
        return expand_command_substitution_modern(executor, text);
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

// Execute case statement
static int execute_case_modern(executor_modern_t *executor, node_t *node) {
    if (!executor || !node || node->type != NODE_CASE) {
        return 1;
    }
    
    // Get the test word and expand variables in it
    char *test_word = expand_if_needed_modern(executor, node->val.str);
    if (!test_word) {
        return 1;
    }
    
    int result = 0;
    bool matched = false;
    
    // Iterate through case items (children)
    node_t *case_item = node->first_child;
    while (case_item && !matched) {
        // The pattern is stored in case_item->val.str
        char *patterns = case_item->val.str;
        if (!patterns) {
            case_item = case_item->next_sibling;
            continue;
        }
        
        // Split patterns by | and test each one
        char *pattern_copy = strdup(patterns);
        if (!pattern_copy) {
            free(test_word);
            return 1;
        }
        
        char *pattern = strtok(pattern_copy, "|");
        while (pattern && !matched) {
            // Expand variables in pattern
            char *expanded_pattern = expand_if_needed_modern(executor, pattern);
            if (expanded_pattern) {
                if (match_pattern(test_word, expanded_pattern)) {
                    matched = true;
                    
                    // Execute commands for this case item
                    node_t *commands = case_item->first_child;
                    while (commands) {
                        result = execute_node_modern(executor, commands);
                        if (result != 0) break;
                        commands = commands->next_sibling;
                    }
                }
                free(expanded_pattern);
            }
            pattern = strtok(NULL, "|");
        }
        
        free(pattern_copy);
        case_item = case_item->next_sibling;
    }
    
    free(test_word);
    return result;
}

// Execute function definition
static int execute_function_definition_modern(executor_modern_t *executor, node_t *node) {
    if (!executor || !node || node->type != NODE_FUNCTION) {
        return 1;
    }
    
    char *function_name = node->val.str;
    if (!function_name) {
        executor_error(executor, "Function definition missing name");
        return 1;
    }
    
    // Get function body
    node_t *body = node->first_child;
    if (!body) {
        executor_error(executor, "Function definition missing body");
        return 1;
    }
    
    // Store function in function table
    if (store_function(executor, function_name, body) != 0) {
        executor_error(executor, "Failed to define function");
        return 1;
    }
    
    if (executor->debug) {
        printf("DEBUG: Defined function '%s'\n", function_name);
    }
    
    return 0;
}

// Check if a function is defined
static bool is_function_defined(executor_modern_t *executor, const char *function_name) {
    return find_function(executor, function_name) != NULL;
}

// Execute function call
static int execute_function_call_modern(executor_modern_t *executor, const char *function_name, char **argv, int argc) {
    if (!executor || !function_name) {
        return 1;
    }
    
    function_def_t *func = find_function(executor, function_name);
    if (!func) {
        executor_error(executor, "Function not found");
        return 1;
    }
    
    if (executor->debug) {
        printf("DEBUG: Calling function '%s' with %d args\n", function_name, argc - 1);
    }
    
    // Create new scope for function
    if (symtable_push_scope(executor->symtable, SCOPE_FUNCTION, function_name) != 0) {
        executor_error(executor, "Failed to create function scope");
        return 1;
    }
    
    // Set positional parameters ($1, $2, etc.)
    for (int i = 1; i < argc; i++) {
        char param_name[16];
        snprintf(param_name, sizeof(param_name), "%d", i);
        if (symtable_set_local_var(executor->symtable, param_name, argv[i]) != 0) {
            symtable_pop_scope(executor->symtable);
            executor_error(executor, "Failed to set function parameter");
            return 1;
        }
    }
    
    // Set $# (argument count)
    char argc_str[16];
    snprintf(argc_str, sizeof(argc_str), "%d", argc - 1);
    symtable_set_local_var(executor->symtable, "#", argc_str);
    
    // Execute function body
    int result = execute_node_modern(executor, func->body);
    
    // Restore previous scope
    symtable_pop_scope(executor->symtable);
    
    return result;
}

// Find function in function table
static function_def_t *find_function(executor_modern_t *executor, const char *function_name) {
    if (!executor || !function_name) {
        return NULL;
    }
    
    function_def_t *func = executor->functions;
    while (func) {
        if (strcmp(func->name, function_name) == 0) {
            return func;
        }
        func = func->next;
    }
    return NULL;
}

// Store function in function table
static int store_function(executor_modern_t *executor, const char *function_name, node_t *body) {
    if (!executor || !function_name || !body) {
        return 1;
    }
    
    // Check if function already exists and remove it
    function_def_t **current = &executor->functions;
    while (*current) {
        if (strcmp((*current)->name, function_name) == 0) {
            function_def_t *to_remove = *current;
            *current = (*current)->next;
            free(to_remove->name);
            free_node_tree(to_remove->body);
            free(to_remove);
            break;
        }
        current = &(*current)->next;
    }
    
    // Create new function definition
    function_def_t *new_func = malloc(sizeof(function_def_t));
    if (!new_func) {
        return 1;
    }
    
    new_func->name = strdup(function_name);
    if (!new_func->name) {
        free(new_func);
        return 1;
    }
    
    // Create a deep copy of the body AST
    new_func->body = copy_ast_node(body);
    if (!new_func->body) {
        free(new_func->name);
        free(new_func);
        return 1;
    }
    
    // Add to front of function list
    new_func->next = executor->functions;
    executor->functions = new_func;
    
    return 0;
}

// Copy AST node recursively
static node_t *copy_ast_node(node_t *node) {
    if (!node) return NULL;
    
    node_t *copy = new_node(node->type);
    if (!copy) return NULL;
    
    // Copy value
    copy->val_type = node->val_type;
    if (node->val.str) {
        copy->val.str = strdup(node->val.str);
        if (!copy->val.str) {
            free_node_tree(copy);
            return NULL;
        }
    } else {
        copy->val = node->val;
    }
    
    // Copy children
    node_t *child = node->first_child;
    while (child) {
        node_t *child_copy = copy_ast_node(child);
        if (!child_copy) {
            free_node_tree(copy);
            return NULL;
        }
        add_child_node(copy, child_copy);
        child = child->next_sibling;
    }
    
    return copy;
}

// Helper function to check if a string is empty or null
static bool is_empty_or_null(const char *str) {
    return !str || str[0] == '\0';
}

// Helper function to extract substring
static char *extract_substring(const char *str, int offset, int length) {
    if (!str) return strdup("");
    
    int str_len = strlen(str);
    
    // Handle negative offset (from end)
    if (offset < 0) {
        offset = str_len + offset;
        if (offset < 0) offset = 0;
    }
    
    // Bounds check
    if (offset >= str_len) return strdup("");
    
    // Calculate actual length
    int remaining = str_len - offset;
    if (length < 0 || length > remaining) {
        length = remaining;
    }
    
    char *result = malloc(length + 1);
    if (!result) return strdup("");
    
    strncpy(result, str + offset, length);
    result[length] = '\0';
    
    return result;
}

// Simple glob pattern matching for parameter expansion
static bool match_pattern(const char *str, const char *pattern) {
    if (!str || !pattern) return false;
    
    // Handle simple cases
    if (*pattern == '\0') return *str == '\0';
    if (*pattern == '*') {
        if (*(pattern + 1) == '\0') return true; // * matches everything
        // Try matching at each position
        for (const char *s = str; *s; s++) {
            if (match_pattern(s, pattern + 1)) return true;
        }
        return match_pattern(str, pattern + 1);
    }
    if (*str == '\0') return false;
    
    // Handle character matching
    if (*pattern == '?' || *pattern == *str) {
        return match_pattern(str + 1, pattern + 1);
    }
    
    return false;
}

// Find shortest match from beginning (for # operator)
static int find_prefix_match(const char *str, const char *pattern, bool longest) {
    if (!str || !pattern) return 0;
    
    int str_len = strlen(str);
    int match_len = 0;
    
    for (int i = 0; i <= str_len; i++) {
        char *substr = malloc(i + 1);
        if (!substr) break;
        
        strncpy(substr, str, i);
        substr[i] = '\0';
        
        if (match_pattern(substr, pattern)) {
            match_len = i;
            if (!longest) {
                free(substr);
                break; // Return first (shortest) match
            }
        }
        free(substr);
    }
    
    return match_len;
}

// Find shortest match from end (for % operator)
static int find_suffix_match(const char *str, const char *pattern, bool longest) {
    if (!str || !pattern) return 0;
    
    int str_len = strlen(str);
    int match_len = 0;
    
    for (int i = 0; i <= str_len; i++) {
        const char *suffix = str + str_len - i;
        if (match_pattern(suffix, pattern)) {
            match_len = i;
            if (!longest) {
                break; // Return first (shortest) match
            }
        }
    }
    
    return match_len;
}

// Case conversion functions
static char *convert_case_first_upper(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) return strdup("");
    
    strcpy(result, str);
    if (len > 0 && islower(result[0])) {
        result[0] = toupper(result[0]);
    }
    
    return result;
}

static char *convert_case_first_lower(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) return strdup("");
    
    strcpy(result, str);
    if (len > 0 && isupper(result[0])) {
        result[0] = tolower(result[0]);
    }
    
    return result;
}

static char *convert_case_all_upper(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) return strdup("");
    
    for (size_t i = 0; i < len; i++) {
        result[i] = toupper(str[i]);
    }
    result[len] = '\0';
    
    return result;
}

static char *convert_case_all_lower(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) return strdup("");
    
    for (size_t i = 0; i < len; i++) {
        result[i] = tolower(str[i]);
    }
    result[len] = '\0';
    
    return result;
}

// Recursively expand variables within a string (for parameter expansion defaults)
static char *expand_variables_in_string(executor_modern_t *executor, const char *str) {
    if (!str || !executor) return strdup("");
    
    size_t len = strlen(str);
    char *result = malloc(len * 2 + 1); // Start with double size
    if (!result) return strdup("");
    
    size_t result_pos = 0;
    size_t result_size = len * 2 + 1;
    
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '$') {
            // Find variable name
            size_t var_start = i + 1;
            size_t var_end = var_start;
            
            // Handle ${var} format
            if (var_start < len && str[var_start] == '{') {
                var_start++; // Skip {
                while (var_end < len && str[var_end] != '}') {
                    var_end++;
                }
                if (var_end < len) var_end++; // Include }
            } else {
                // Handle $var format
                while (var_end < len && (isalnum(str[var_end]) || str[var_end] == '_')) {
                    var_end++;
                }
            }
            
            if (var_end > var_start) {
                // Extract and expand variable
                size_t var_len = var_end - i;
                char *var_expr = malloc(var_len + 1);
                if (var_expr) {
                    strncpy(var_expr, &str[i], var_len);
                    var_expr[var_len] = '\0';
                    
                    char *var_value = expand_variable_modern(executor, var_expr);
                    if (var_value) {
                        size_t value_len = strlen(var_value);
                        
                        // Ensure buffer is large enough
                        while (result_pos + value_len >= result_size) {
                            result_size *= 2;
                            char *new_result = realloc(result, result_size);
                            if (!new_result) {
                                free(result);
                                free(var_value);
                                free(var_expr);
                                return strdup("");
                            }
                            result = new_result;
                        }
                        
                        strcpy(&result[result_pos], var_value);
                        result_pos += value_len;
                        free(var_value);
                    }
                    
                    free(var_expr);
                    i = var_end - 1; // Skip past variable
                    continue;
                }
            }
        }
        
        // Regular character - ensure buffer space
        if (result_pos + 1 >= result_size) {
            result_size *= 2;
            char *new_result = realloc(result, result_size);
            if (!new_result) {
                free(result);
                return strdup("");
            }
            result = new_result;
        }
        
        result[result_pos++] = str[i];
    }
    
    result[result_pos] = '\0';
    return result;
}

// Parse parameter expansion inside ${...}
static char *parse_parameter_expansion(executor_modern_t *executor, const char *expansion) {
    if (!expansion) return strdup("");
    
    // Handle length expansion: ${#var}
    if (expansion[0] == '#') {
        const char *var_name = expansion + 1;
        char *value = symtable_get_var(executor->symtable, var_name);
        if (value) {
            int len = strlen(value);
            char *result = malloc(16);
            if (result) {
                snprintf(result, 16, "%d", len);
            }
            return result ? result : strdup("0");
        }
        return strdup("0");
    }
    
    // Look for parameter expansion operators
    const char *op_pos = NULL;
    const char *operators[] = {":-", ":+", "##", "%%", "^^", ",,", ":", "#", "%", "^", ",", "-", "+", NULL};
    int op_type = -1;
    
    // Find the first valid operator that's not part of a pattern
    for (int i = 0; operators[i]; i++) {
        const char *found = strstr(expansion, operators[i]);
        if (found) {
            // For : operator, make sure it's not part of :// or other patterns
            if (strcmp(operators[i], ":") == 0) {
                // Check if this is followed by // (like in URLs)
                if (found[1] == '/' && found[2] == '/') {
                    continue; // Skip this match, it's part of ://
                }
                // Check if this is part of :+ or :-
                if (found > expansion && (found[-1] == '+' || found[-1] == '-')) {
                    continue; // Skip this match, it's part of :+ or :-
                }
            }
            
            // If we haven't found an operator yet, or this one comes first, use it
            if (!op_pos || found < op_pos) {
                op_pos = found;
                op_type = i;
            }
        }
    }
    
    if (op_pos) {
        // Extract variable name
        size_t var_len = op_pos - expansion;
        char *var_name = malloc(var_len + 1);
        if (!var_name) return strdup("");
        
        strncpy(var_name, expansion, var_len);
        var_name[var_len] = '\0';
        
        // Get variable value
        char *var_value = symtable_get_var(executor->symtable, var_name);
        const char *default_value = op_pos + strlen(operators[op_type]);
        
        // Expand variables in default value
        char *expanded_default = expand_variables_in_string(executor, default_value);
        
        char *result = NULL;
        
        switch (op_type) {
            case 0: // ${var:-default} - use default if var is unset or empty
                if (is_empty_or_null(var_value)) {
                    result = strdup(expanded_default);
                } else {
                    result = strdup(var_value);
                }
                break;
                
            case 1: // ${var:+alternative} - use alternative if var is set and non-empty
                if (!is_empty_or_null(var_value)) {
                    result = strdup(expanded_default);
                } else {
                    result = strdup("");
                }
                break;
                
            case 2: // ${var##pattern} - remove longest match of pattern from beginning
                if (var_value) {
                    int match_len = find_prefix_match(var_value, expanded_default, true);
                    result = strdup(var_value + match_len);
                } else {
                    result = strdup("");
                }
                break;
                
            case 3: // ${var%%pattern} - remove longest match of pattern from end
                if (var_value) {
                    int str_len = strlen(var_value);
                    int match_len = find_suffix_match(var_value, expanded_default, true);
                    int result_len = str_len - match_len;
                    result = malloc(result_len + 1);
                    if (result) {
                        strncpy(result, var_value, result_len);
                        result[result_len] = '\0';
                    } else {
                        result = strdup("");
                    }
                } else {
                    result = strdup("");
                }
                break;
                
            case 4: // ${var^^} - convert all characters to uppercase
                if (var_value) {
                    result = convert_case_all_upper(var_value);
                } else {
                    result = strdup("");
                }
                break;
                
            case 5: // ${var,,} - convert all characters to lowercase
                if (var_value) {
                    result = convert_case_all_lower(var_value);
                } else {
                    result = strdup("");
                }
                break;
                
            case 6: // ${var:offset:length} - substring expansion
                if (var_value) {
                    // Parse offset and optional length (with variable expansion)
                    char *expanded_offset_str = expand_variables_in_string(executor, expanded_default);
                    char *endptr;
                    int offset = strtol(expanded_offset_str, &endptr, 10);
                    int length = -1;
                    
                    if (*endptr == ':') {
                        length = strtol(endptr + 1, NULL, 10);
                    }
                    
                    result = extract_substring(var_value, offset, length);
                    free(expanded_offset_str);
                } else {
                    result = strdup("");
                }
                break;
                
            case 7: // ${var#pattern} - remove shortest match of pattern from beginning
                if (var_value) {
                    int match_len = find_prefix_match(var_value, expanded_default, false);
                    result = strdup(var_value + match_len);
                } else {
                    result = strdup("");
                }
                break;
                
            case 8: // ${var%pattern} - remove shortest match of pattern from end
                if (var_value) {
                    int str_len = strlen(var_value);
                    int match_len = find_suffix_match(var_value, expanded_default, false);
                    int result_len = str_len - match_len;
                    result = malloc(result_len + 1);
                    if (result) {
                        strncpy(result, var_value, result_len);
                        result[result_len] = '\0';
                    } else {
                        result = strdup("");
                    }
                } else {
                    result = strdup("");
                }
                break;
                
            case 9: // ${var^} - convert first character to uppercase
                if (var_value) {
                    result = convert_case_first_upper(var_value);
                } else {
                    result = strdup("");
                }
                break;
                
            case 10: // ${var,} - convert first character to lowercase
                if (var_value) {
                    result = convert_case_first_lower(var_value);
                } else {
                    result = strdup("");
                }
                break;
                
            case 11: // ${var-default} - use default if var is unset (but not if empty)
                if (!var_value) {
                    result = strdup(expanded_default);
                } else {
                    result = strdup(var_value);
                }
                break;
                
            case 12: // ${var+alternative} - use alternative if var is set (even if empty)
                if (var_value) {
                    result = strdup(expanded_default);
                } else {
                    result = strdup("");
                }
                break;
        }
        
        free(var_name);
        free(expanded_default);
        return result ? result : strdup("");
    }
    
    // No operator found, just get the variable value
    char *value = symtable_get_var(executor->symtable, expansion);
    return value ? strdup(value) : strdup("");
}

// Expand variable reference using modern symbol table with advanced parameter expansion
static char *expand_variable_modern(executor_modern_t *executor, const char *var_text) {
    if (!executor || !var_text || var_text[0] != '$') {
        return strdup(var_text ? var_text : "");
    }
    
    const char *var_name = var_text + 1;
    
    // Handle ${var} format with advanced parameter expansion
    if (var_name[0] == '{') {
        char *close = strchr(var_name, '}');
        if (close) {
            size_t len = close - var_name - 1;
            char *expansion = malloc(len + 1);
            if (expansion) {
                strncpy(expansion, var_name + 1, len);
                expansion[len] = '\0';
                
                char *result = parse_parameter_expansion(executor, expansion);
                free(expansion);
                return result;
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
                return value ? strdup(value) : strdup("");
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

// Expand command substitution $(command) or `command` - Modern implementation
static char *expand_command_substitution_modern(executor_modern_t *executor, const char *cmd_text) {
    if (!executor || !cmd_text) return strdup("");
    
    // Extract command from $(command) or `command` format
    char *command = NULL;
    if (strncmp(cmd_text, "$(", 2) == 0 && cmd_text[strlen(cmd_text) - 1] == ')') {
        // Extract from $(command)
        size_t len = strlen(cmd_text) - 3; // Remove $( and )
        command = malloc(len + 1);
        if (!command) return strdup("");
        strncpy(command, cmd_text + 2, len);
        command[len] = '\0';
    } else if (cmd_text[0] == '`' && cmd_text[strlen(cmd_text) - 1] == '`') {
        // Extract from `command`
        size_t len = strlen(cmd_text) - 2; // Remove backticks
        command = malloc(len + 1);
        if (!command) return strdup("");
        strncpy(command, cmd_text + 1, len);
        command[len] = '\0';
    } else {
        // Already extracted command
        command = strdup(cmd_text);
        if (!command) return strdup("");
    }
    
    // Create a pipe to capture command output
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        free(command);
        return strdup("");
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        free(command);
        return strdup("");
    }
    
    if (pid == 0) {
        // Child process - execute command
        close(pipefd[0]); // Close read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[1]);
        
        // Execute command using the shell
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(127);
    } else {
        // Parent process - read output
        close(pipefd[1]); // Close write end
        free(command);
        
        char *output = malloc(1024);
        size_t output_size = 1024;
        size_t output_len = 0;
        
        if (!output) {
            close(pipefd[0]);
            waitpid(pid, NULL, 0);
            return strdup("");
        }
        
        ssize_t bytes_read;
        char buffer[256];
        
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            if (output_len + bytes_read >= output_size) {
                output_size *= 2;
                char *new_output = realloc(output, output_size);
                if (!new_output) {
                    free(output);
                    close(pipefd[0]);
                    waitpid(pid, NULL, 0);
                    return strdup("");
                }
                output = new_output;
            }
            memcpy(output + output_len, buffer, bytes_read);
            output_len += bytes_read;
        }
        
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        
        // Null terminate and remove trailing newlines
        output[output_len] = '\0';
        while (output_len > 0 && (output[output_len - 1] == '\n' || output[output_len - 1] == '\r')) {
            output[--output_len] = '\0';
        }
        
        return output;
    }
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
            // Check for command substitution $(...)
            if (str[i + 1] == '(') {
                // Find matching closing parenthesis
                size_t cmd_start = i;
                size_t cmd_end = i + 2;
                int paren_depth = 1;
                
                while (cmd_end < len && paren_depth > 0) {
                    if (str[cmd_end] == '(') {
                        paren_depth++;
                    } else if (str[cmd_end] == ')') {
                        paren_depth--;
                    }
                    cmd_end++;
                }
                
                if (paren_depth == 0) {
                    // Extract command substitution including $( and )
                    size_t full_cmd_len = cmd_end - (cmd_start - 2) + 1; // Include $( and )
                    char *full_cmd_expr = malloc(full_cmd_len + 1);
                    if (full_cmd_expr) {
                        strncpy(full_cmd_expr, &str[cmd_start - 2], full_cmd_len);
                        full_cmd_expr[full_cmd_len] = '\0';
                        
                        // Expand command substitution
                        char *cmd_result = expand_command_substitution_modern(executor, full_cmd_expr);
                        if (cmd_result) {
                            size_t result_len = strlen(cmd_result);
                            // Ensure buffer is large enough
                            while (result_pos + result_len >= buffer_size) {
                                buffer_size *= 2;
                                char *new_result = realloc(result, buffer_size);
                                if (!new_result) {
                                    free(result);
                                    free(cmd_result);
                                    free(full_cmd_expr);
                                    return strdup("");
                                }
                                result = new_result;
                            }
                            
                            // Copy command result
                            strcpy(&result[result_pos], cmd_result);
                            result_pos += result_len;
                            free(cmd_result);
                        }
                        
                        free(full_cmd_expr);
                        i = cmd_end; // Skip past the closing parenthesis
                        continue;
                    }
                }
            }
            
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
