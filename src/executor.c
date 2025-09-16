/**
 * Modern Execution Engine Implementation
 *
 * Clean, efficient execution engine designed for the modern parser and
 * tokenizer. Handles command execution, control structures, pipelines, and
 * variable management with proper POSIX compliance.
 */

#define _POSIX_C_SOURCE 200809L

#include "../include/executor.h"

#include "../include/alias.h"
#include "../include/arithmetic.h"
#include "../include/autocorrect.h"
#include "../include/builtins.h"
#include "../include/config.h"
#include "../include/debug.h"
#include "../include/init.h"
#include "../include/lusush.h"
#include "../include/node.h"
#include "../include/parser.h"
#include "../include/redirection.h"
#include "../include/signals.h"
#include "../include/strings.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Global executor pointer for job control builtins
executor_t *current_executor = NULL;

// Forward declarations
// Forward declarations - updated for symtable
static int execute_node(executor_t *executor, node_t *node);
static int execute_command(executor_t *executor, node_t *command);
static int execute_pipeline(executor_t *executor, node_t *pipeline);
static int execute_function_definition(executor_t *executor, node_t *function);
static int execute_function_call(executor_t *executor,
                                 const char *function_name, char **argv,
                                 int argc);
static bool is_function_defined(executor_t *executor,
                                const char *function_name);
static function_def_t *find_function(executor_t *executor,
                                     const char *function_name);
static int store_function(executor_t *executor, const char *function_name,
                          node_t *body, function_param_t *params, int param_count);
static int validate_function_parameters(function_def_t *func, char **argv, int argc);
static node_t *copy_ast_node(node_t *node);
static node_t *copy_ast_chain(node_t *node);
static int execute_if(executor_t *executor, node_t *if_node);
static int execute_while(executor_t *executor, node_t *while_node);
static int execute_until(executor_t *executor, node_t *until_node);
static int execute_for(executor_t *executor, node_t *for_node);
static int execute_case(executor_t *executor, node_t *case_node);
static int execute_logical_and(executor_t *executor, node_t *and_node);
static int execute_logical_or(executor_t *executor, node_t *or_node);
static int execute_command_list(executor_t *executor, node_t *list);
static char **build_argv_from_ast(executor_t *executor, node_t *command,
                                  int *argc);
static int execute_external_command(executor_t *executor, char **argv);
static int execute_external_command_with_redirection(executor_t *executor,
                                                     char **argv,
                                                     bool redirect_stderr);
static bool is_stdout_captured(void);
static bool has_stdout_redirections(node_t *command);
static int execute_builtin_with_captured_stdout(executor_t *executor,
                                                char **argv, node_t *command);

static int add_to_argv_list(char ***argv_list, int *argv_count,
                            int *argv_capacity, char *arg);
static int execute_external_command_with_setup(executor_t *executor,
                                               char **argv,
                                               bool redirect_stderr,
                                               node_t *command);
static int execute_builtin_command(executor_t *executor, char **argv);
static int execute_brace_group(executor_t *executor, node_t *group);
static int execute_subshell(executor_t *executor, node_t *subshell);
static bool is_builtin_command(const char *cmd);
static void set_executor_error(executor_t *executor, const char *message);
static char *expand_variable(executor_t *executor, const char *var_text);
static char *expand_tilde(const char *text);
static char **expand_glob_pattern(const char *pattern, int *expanded_count);
static bool needs_glob_expansion(const char *str);
static char **expand_brace_pattern(const char *pattern, int *expanded_count);
static bool needs_brace_expansion(const char *str);
static void initialize_job_control(executor_t *executor);
static char *expand_arithmetic(executor_t *executor, const char *arith_text);
static char *expand_command_substitution(executor_t *executor,
                                         const char *cmd_text);
static node_t *copy_node_simple(node_t *original);
static void copy_function_definitions(executor_t *dest, executor_t *src);
char *expand_if_needed(executor_t *executor, const char *text);
static char *expand_quoted_string(executor_t *executor, const char *str);
static bool is_assignment(const char *text);
static int execute_assignment(executor_t *executor, const char *assignment);
static bool match_pattern(const char *str, const char *pattern);

// Create new executor with global symtable
executor_t *executor_new(void) {
    executor_t *executor = malloc(sizeof(executor_t));
    if (!executor) {
        return NULL;
    }

    // Use global symbol table manager from modernized legacy interface
    executor->symtable = symtable_get_global_manager();
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
    initialize_job_control(executor);

    return executor;
}

// Create new executor with external symtable
executor_t *executor_new_with_symtable(symtable_manager_t *symtable) {
    executor_t *executor = malloc(sizeof(executor_t));
    if (!executor) {
        return NULL;
    }

    // Use provided symtable
    executor->symtable = symtable;

    executor->interactive = false;
    executor->debug = false;
    executor->exit_status = 0;
    executor->error_message = NULL;
    executor->has_error = false;
    executor->functions = NULL;
    initialize_job_control(executor);

    return executor;
}

// Free executor
void executor_free(executor_t *executor) {
    if (executor) {
        // Don't free global symtable - it's managed globally

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
void executor_set_debug(executor_t *executor, bool debug) {
    if (executor) {
        executor->debug = debug;
        if (executor->symtable) {
            symtable_manager_set_debug(executor->symtable, debug);
        }
    }
}

// Set interactive mode
void executor_set_interactive(executor_t *executor, bool interactive) {
    if (executor) {
        executor->interactive = interactive;
    }
}

// Set external symtable
void executor_set_symtable(executor_t *executor, symtable_manager_t *symtable) {
    if (executor) {
        // Don't free the old symtable if it exists - it might be external
        executor->symtable = symtable;
    }
}

// Check for errors
bool executor_has_error(executor_t *executor) {
    return executor && executor->has_error;
}

const char *executor_error(executor_t *executor) {
    return executor ? executor->error_message : "Invalid executor";
}

// Set error
static void set_executor_error(executor_t *executor, const char *message) {
    if (executor) {
        executor->error_message = message;
        executor->has_error = true;
    }
}

// Main execution entry point
int executor_execute(executor_t *executor, node_t *ast) {
    if (!executor || !ast) {
        return 1;
    }

    executor->has_error = false;
    executor->error_message = NULL;

    // Check if this is a command sequence (has siblings) or a single command
    if (ast->next_sibling) {
        // This is a command sequence, execute all siblings
        int result = execute_command_list(executor, ast);
        executor->exit_status = result;
        return result;
    } else {
        // Single command, execute normally
        int result = execute_node(executor, ast);
        executor->exit_status = result;
        return result;
    }
}

// Execute command line (parse and execute)
int executor_execute_command_line(executor_t *executor, const char *input) {
    if (!executor || !input) {
        return 1;
    }

    // Parse the input
    parser_t *parser = parser_new(input);
    if (!parser) {
        set_executor_error(executor, "Failed to create parser");
        return 1;
    }

    node_t *ast = parser_parse(parser);

    // Check syntax check mode (set -n) - parse but don't execute
    if (shell_opts.syntax_check) {
        if (parser_has_error(parser)) {
            set_executor_error(executor, parser_error(parser));
            parser_free(parser);
            return 2; // Syntax error
        }
        parser_free(parser);
        return 0; // Syntax check successful
    }

    if (parser_has_error(parser)) {
        set_executor_error(executor, parser_error(parser));
        parser_free(parser);
        return 1;
    }

    if (!ast) {
        parser_free(parser);
        return 0; // Empty command
    }

    int result = executor_execute(executor, ast);

    free_node_tree(ast);
    parser_free(parser);

    return result;
}

// Core node execution dispatcher
static int execute_node(executor_t *executor, node_t *node) {
    if (!node) {
        return 0;
    }

    // Enhanced debug tracing
    if (executor->debug) {
        printf("DEBUG: Executing node type %d\n", node->type);
        if (node->val.str) {
            printf("DEBUG: Node value: '%s'\n", node->val.str);
        }
    }

    // Advanced debug system integration
    DEBUG_TRACE_NODE(node, __FILE__, __LINE__);

    // Check for breakpoints
    DEBUG_BREAKPOINT_CHECK(__FILE__, __LINE__);

    switch (node->type) {
    case NODE_COMMAND:
        return execute_command(executor, node);
    case NODE_PIPE:
        return execute_pipeline(executor, node);
    case NODE_IF:
        return execute_if(executor, node);
    case NODE_WHILE:
        return execute_while(executor, node);
    case NODE_UNTIL:
        return execute_until(executor, node);
    case NODE_FOR:
        return execute_for(executor, node);
    case NODE_CASE:
        return execute_case(executor, node);
    case NODE_LOGICAL_AND:
        return execute_logical_and(executor, node);
    case NODE_LOGICAL_OR:
        return execute_logical_or(executor, node);
    case NODE_FUNCTION:
        return execute_function_definition(executor, node);
    case NODE_BRACE_GROUP:
        return execute_brace_group(executor, node);
    case NODE_SUBSHELL:
        return execute_subshell(executor, node);
    case NODE_COMMAND_LIST:
        return execute_command_list(executor, node);
    case NODE_BACKGROUND:
        return executor_execute_background(executor, node);
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
static int execute_command_list(executor_t *executor, node_t *list) {
    if (!list) {
        return 0;
    }

    int last_result = 0;
    node_t *current;

    // Handle NODE_COMMAND_LIST with children
    if (list->type == NODE_COMMAND_LIST) {
        current = list->first_child;
    } else {
        // Handle legacy case where list is the first command in a sibling chain
        current = list;
    }

    while (current) {
        // Check syntax check mode (set -n) - don't execute commands
        if (shell_opts.syntax_check) {
            return 0; // Syntax check mode - don't execute
        }

        last_result = execute_node(executor, current);

        // Update exit status after each command in the sequence
        set_exit_status(last_result);

        if (executor->debug) {
            printf("DEBUG: Command result: %d\n", last_result);
        }

        // Handle set -e (exit_on_error): exit if command failed
        if (shell_opts.exit_on_error && last_result != 0) {
            executor->exit_status = last_result;
            return last_result;
        }

        current = current->next_sibling;
    }

    return last_result;
}

// Execute a simple command
static int execute_command(executor_t *executor, node_t *command) {
    if (!command || command->type != NODE_COMMAND) {
        return 1;
    }

    // Check for assignment
    if (command->val.str && is_assignment(command->val.str)) {
        return execute_assignment(executor, command->val.str);
    }

    // Check for standalone parameter expansion ${...}
    if (command->val.str && command->val.str[0] == '$' &&
        command->val.str[1] == '{') {

        // Expand the command parameter expansion
        char *result = expand_variable(executor, command->val.str);
        if (result) {
            free(result);
        }

        // Also process any arguments that are parameter expansions
        node_t *child = command->first_child;
        while (child) {
            if (child->val.str && child->val.str[0] == '$' &&
                child->val.str[1] == '{') {

                char *arg_result = expand_variable(executor, child->val.str);
                if (arg_result) {
                    free(arg_result);
                }
            }
            child = child->next_sibling;
        }

        return 0; // Success
    }

    // Check if command has redirections
    bool has_redirections = count_redirections(command) > 0;

    // Build argument vector (excluding redirection nodes)
    int argc;
    char **argv = build_argv_from_ast(executor, command, &argc);
    if (!argv || argc == 0) {
        return 1;
    }

    // Check if all arguments are parameter expansions
    bool all_param_expansions = true;
    for (int i = 0; i < argc; i++) {
        if (!argv[i] || argv[i][0] != '$' || argv[i][1] != '{') {
            all_param_expansions = false;
            break;
        }
    }

    if (all_param_expansions && argc > 0) {

        // Execute all parameter expansions
        for (int i = 0; i < argc; i++) {

            char *result = expand_variable(executor, argv[i]);
            if (result) {
                free(result);
            }
        }
        // Free argv and return success
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
        free(argv);
        return 0;
    }

    // Process parameter expansion arguments for any command
    for (int i = 1; i < argc; i++) {
        if (argv[i] && argv[i][0] == '$' && argv[i][1] == '{') {
            char *result = expand_variable(executor, argv[i]);
            if (result) {
                free(result);
            }
        }
    }

    // Check for stderr redirection pattern (2>/dev/null or 2> /dev/null)
    bool redirect_stderr = false;
    char **filtered_argv = NULL;
    int filtered_argc = 0;

    // Look for 2>/dev/null pattern in arguments
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "2>/dev/null") == 0) {
            redirect_stderr = true;
            break;
        } else if (i + 2 < argc && strcmp(argv[i], "2") == 0 &&
                   strcmp(argv[i + 1], ">") == 0 &&
                   strcmp(argv[i + 2], "/dev/null") == 0) {
            redirect_stderr = true;
            break;
        }
    }

    if (redirect_stderr) {
        // Create filtered argv without redirection tokens
        filtered_argv = malloc((argc + 1) * sizeof(char *));
        if (!filtered_argv) {
            // Free original argv and return error
            for (int i = 0; i < argc; i++) {
                free(argv[i]);
            }
            free(argv);
            return 1;
        }

        int j = 0;
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "2>/dev/null") == 0) {
                // Skip this token
                continue;
            } else if (i + 2 < argc && strcmp(argv[i], "2") == 0 &&
                       strcmp(argv[i + 1], ">") == 0 &&
                       strcmp(argv[i + 2], "/dev/null") == 0) {
                // Skip these three tokens
                i += 2;
                continue;
            } else {
                filtered_argv[j] = strdup(argv[i]);
                j++;
            }
        }
        filtered_argv[j] = NULL;
        filtered_argc = j;
    } else {
        filtered_argv = argv;
        filtered_argc = argc;
    }

    if (executor->debug) {
        printf("DEBUG: Executing command: %s with %d args\n", filtered_argv[0],
               filtered_argc - 1);
        for (int i = 0; i < filtered_argc; i++) {
            printf("DEBUG: argv[%d] = '%s'\n", i, filtered_argv[i]);
        }
        if (redirect_stderr) {
            printf("DEBUG: stderr redirection enabled\n");
        }
    }

    // Check for alias expansion and rebuild argv if needed
    char *alias_expanded = lookup_alias(filtered_argv[0]);
    if (alias_expanded) {
        // Reconstruct original command for expansion
        char *original_command = NULL;
        size_t total_len = 1; // for null terminator

        for (int i = 0; i < filtered_argc; i++) {
            total_len += strlen(filtered_argv[i]) + (i > 0 ? 1 : 0);
        }

        original_command = malloc(total_len);
        if (original_command) {
            strcpy(original_command, filtered_argv[0]);
            for (int i = 1; i < filtered_argc; i++) {
                strcat(original_command, " ");
                strcat(original_command, filtered_argv[i]);
            }

            // Expand the full command line with recursive expansion
            char *recursive_expanded =
                expand_aliases_recursive(filtered_argv[0], 10); // max depth 10
            char *expanded_command = NULL;

            if (recursive_expanded) {
                // If recursive expansion succeeded, use it to build full
                // command
                if (filtered_argc > 1) {
                    // Add original arguments to recursively expanded command
                    size_t len = strlen(recursive_expanded) + 1;
                    for (int i = 1; i < filtered_argc; i++) {
                        len += strlen(filtered_argv[i]) + 1;
                    }
                    expanded_command = malloc(len);
                    if (expanded_command) {
                        strcpy(expanded_command, recursive_expanded);
                        for (int i = 1; i < filtered_argc; i++) {
                            strcat(expanded_command, " ");
                            strcat(expanded_command, filtered_argv[i]);
                        }
                    }
                } else {
                    expanded_command = strdup(recursive_expanded);
                }
                free(recursive_expanded);
            } else {
                // Fall back to simple first-word expansion
                expanded_command = expand_first_word_alias(original_command);
            }
            if (expanded_command &&
                strcmp(expanded_command, original_command) != 0) {
                // Create new argv array for expanded command
                char **new_argv =
                    malloc(256 * sizeof(char *)); // reasonable limit
                if (new_argv) {
                    // Tokenize expanded command into new argv
                    char *expanded_copy = strdup(expanded_command);
                    char *token = strtok(expanded_copy, " ");
                    int new_argc = 0;

                    while (token && new_argc < 255) {
                        new_argv[new_argc] = strdup(token);
                        new_argc++;
                        token = strtok(NULL, " ");
                    }
                    new_argv[new_argc] = NULL;

                    // Only replace if we successfully created the new argv
                    if (new_argc > 0) {
                        // Free old argv only if it's not the same as original
                        // argv
                        if (filtered_argv != argv) {
                            for (int i = 0; i < filtered_argc; i++) {
                                free(filtered_argv[i]);
                            }
                            free(filtered_argv);
                        }

                        filtered_argv = new_argv;
                        filtered_argc = new_argc;
                    } else {
                        // Failed to create new argv, clean up
                        free(new_argv);
                    }

                    free(expanded_copy);
                }
            }

            free(expanded_command);
            free(original_command);
        }
    }

    int result;
    if (is_function_defined(executor, filtered_argv[0])) {
        result = execute_function_call(executor, filtered_argv[0],
                                       filtered_argv, filtered_argc);
    } else if (is_builtin_command(filtered_argv[0])) {
        // For builtin commands with stdout redirections, check if stdout is
        // captured
        if (has_redirections && has_stdout_redirections(command) &&
            is_stdout_captured()) {
            // When stdout is captured externally and command has stdout
            // redirections, use child process to avoid file descriptor
            // interference
            result = execute_builtin_with_captured_stdout(
                executor, filtered_argv, command);
        } else {
            // Normal case: handle redirections in parent process
            redirection_state_t redir_state;
            if (has_redirections) {
                save_file_descriptors(&redir_state);
                int redir_result = setup_redirections(executor, command);
                if (redir_result != 0) {
                    restore_file_descriptors(&redir_state);
                    return redir_result;
                }
            }

            result = execute_builtin_command(executor, filtered_argv);

            // Restore file descriptors after builtin execution
            if (has_redirections) {
                // Flush output streams before restoring file descriptors
                fflush(stdout);
                fflush(stderr);
                restore_file_descriptors(&redir_state);
            }
        }
    } else {
        // Check auto_cd before attempting external command execution
        int auto_cd_enabled = symtable_get_global_int("AUTO_CD", 0);
        if (auto_cd_enabled && argc > 0 && argv[0]) {
            struct stat st;
            // Check if the command is actually a directory
            if (stat(argv[0], &st) == 0 && S_ISDIR(st.st_mode)) {
                // Auto-cd to the directory
                if (chdir(argv[0]) == 0) {
                    // Successfully changed directory, update PWD
                    char *new_pwd = getcwd(NULL, 0);
                    if (new_pwd) {
                        symtable_set_global("PWD", new_pwd);
                        free(new_pwd);
                    }
                    result = 0; // Success
                } else {
                    // Failed to change directory, show error
                    perror("cd");
                    result = 1;
                }
            } else {
                // Not a directory, proceed with normal command execution
                goto normal_execution;
            }
        } else {
            // Auto-cd disabled, proceed with normal command execution
            normal_execution:
            // Check if command exists first, offer auto-correction if not
            if (config.spell_correction && autocorrect_is_enabled()) {
            // First, check if the command actually exists
            if (!autocorrect_command_exists(executor, filtered_argv[0])) {
                // Command doesn't exist, try auto-correction
                correction_results_t correction_results;
                int suggestions = autocorrect_find_suggestions(
                    executor, filtered_argv[0], &correction_results);

                if (suggestions > 0 && config.autocorrect_interactive) {
                    char selected_command[MAX_COMMAND_LENGTH];
                    if (autocorrect_prompt_user(&correction_results,
                                                selected_command)) {
                        // User selected a correction, replace the command
                        free(filtered_argv[0]);
                        filtered_argv[0] = strdup(selected_command);

                        // Learn the corrected command
                        autocorrect_learn_command(selected_command);

                        // Re-check if it's a builtin or function after
                        // correction
                        if (is_builtin_command(filtered_argv[0])) {
                            result = execute_builtin_command(executor,
                                                             filtered_argv);
                        } else if (is_function_defined(executor,
                                                       filtered_argv[0])) {
                            result = execute_function_call(
                                executor, filtered_argv[0], filtered_argv,
                                filtered_argc);
                        } else {
                            // Execute the corrected external command
                            result = execute_external_command_with_setup(
                                executor, filtered_argv, redirect_stderr,
                                command);
                        }
                    } else {
                        // User declined correction, show original error
                        result = 127; // Command not found
                    }
                } else {
                    // No suggestions or interactive prompts disabled
                    result = execute_external_command_with_setup(
                        executor, filtered_argv, redirect_stderr, command);
                }

                // Clean up correction results
                autocorrect_free_results(&correction_results);
            } else {
                // Command exists, execute normally
                result = execute_external_command_with_setup(
                    executor, filtered_argv, redirect_stderr, command);
            }
            } else {
                // Auto-correction disabled, execute normally
                result = execute_external_command_with_setup(
                    executor, filtered_argv, redirect_stderr, command);
            }
        }
    }

    // Free argv
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);

    // Free filtered argv if it was separately allocated
    if (redirect_stderr && filtered_argv) {
        for (int i = 0; i < filtered_argc; i++) {
            free(filtered_argv[i]);
        }
        free(filtered_argv);
    }

    // Update exit status for $? variable
    set_exit_status(result);

    // Profile function exit
    DEBUG_PROFILE_EXIT("execute_command");

    return result;
}

// Execute pipeline
static int execute_pipeline(executor_t *executor, node_t *pipeline) {
    if (!pipeline || pipeline->type != NODE_PIPE) {
        return 1;
    }

    // For now, implement simple two-command pipeline
    // A more complete implementation would handle N-command pipelines

    node_t *left = pipeline->first_child;
    node_t *right = left ? left->next_sibling : NULL;

    if (!left || !right) {
        set_executor_error(executor, "Malformed pipeline");
        return 1;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        set_executor_error(executor, "Failed to create pipe");
        return 1;
    }

    pid_t left_pid = fork();
    if (left_pid == -1) {
        set_executor_error(executor, "Failed to fork for pipeline");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return 1;
    }

    if (left_pid == 0) {
        // Left command: write to pipe
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        int result = execute_node(executor, left);
        exit(result);
    }

    pid_t right_pid = fork();
    if (right_pid == -1) {
        set_executor_error(executor, "Failed to fork for pipeline");
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

        int result = execute_node(executor, right);
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

// Execute a chain of commands connected by next_sibling
static int execute_command_chain(executor_t *executor, node_t *first_command) {
    if (!first_command) {
        return 0;
    }

    int last_result = 0;
    node_t *current = first_command;

    while (current) {
        last_result = execute_node(executor, current);

        // Handle set -e (exit_on_error): exit if command failed and not part of
        // conditional
        if (shell_opts.exit_on_error && last_result != 0) {
            // Don't exit on error for certain contexts (conditionals,
            // pipelines, etc.) For now, implement basic exit-on-error behavior
            executor->exit_status = last_result;
            return last_result;
        }

        current = current->next_sibling;
    }

    return last_result;
}

// Execute if statement
static int execute_if(executor_t *executor, node_t *if_node) {
    if (!if_node || if_node->type != NODE_IF) {
        return 1;
    }

    // Traverse through all children of the if statement
    node_t *current = if_node->first_child;
    if (!current) {
        set_executor_error(executor, "Malformed if statement");
        return 1;
    }

    // First child is always the if condition
    node_t *condition = current;
    current = current->next_sibling;

    if (!current) {
        set_executor_error(executor,
                           "Malformed if statement - missing then body");
        return 1;
    }

    // Execute the initial if condition
    int condition_result = execute_node(executor, condition);

    if (condition_result == 0) { // Success in shell terms
        // Execute the then body (second child)
        return execute_node(executor, current);
    }

    // Move to next child (elif condition or else body)
    current = current->next_sibling;

    // Process elif clauses - they come in pairs (condition, body)
    while (current && current->next_sibling) {
        // Execute elif condition
        condition_result = execute_node(executor, current);

        if (condition_result == 0) { // Success in shell terms
            // Execute elif body (next sibling)
            return execute_node(executor, current->next_sibling);
        }

        // Move past the elif body to the next elif condition or else body
        current = current->next_sibling->next_sibling;
    }

    // Handle final else clause if present (no condition, just body)
    if (current) {
        return execute_node(executor, current);
    }

    return 0;
}

// Execute while loop
static int execute_while(executor_t *executor, node_t *while_node) {
    if (!while_node || while_node->type != NODE_WHILE) {
        return 1;
    }

    node_t *condition = while_node->first_child;
    node_t *body = condition ? condition->next_sibling : NULL;

    if (!condition || !body) {
        set_executor_error(executor, "Malformed while loop");
        return 1;
    }

    int last_result = 0;
    int iteration = 0;
    const int max_iterations = 10000; // Safety limit

    while (iteration < max_iterations) {
        // Execute condition
        int condition_result = execute_node(executor, condition);

        if (executor->debug) {
            printf("DEBUG: WHILE iteration %d, condition result: %d\n",
                   iteration, condition_result);
        }

        // If condition fails, exit loop
        if (condition_result != 0) {
            break;
        }

        // Execute body
        last_result = execute_command_chain(executor, body);

        iteration++;
    }

    if (iteration >= max_iterations) {
        set_executor_error(executor, "While loop exceeded maximum iterations");
        return 1;
    }

    return last_result;
}

// Execute until loop
static int execute_until(executor_t *executor, node_t *until_node) {
    if (!until_node || until_node->type != NODE_UNTIL) {
        return 1;
    }

    node_t *condition = until_node->first_child;
    node_t *body = condition ? condition->next_sibling : NULL;

    if (!condition || !body) {
        set_executor_error(executor, "Malformed until loop");
        return 1;
    }

    int last_result = 0;
    int iteration = 0;
    const int max_iterations = 10000; // Safety limit

    while (iteration < max_iterations) {
        // Execute condition
        int condition_result = execute_node(executor, condition);

        if (executor->debug) {
            printf("DEBUG: UNTIL iteration %d, condition result: %d\n",
                   iteration, condition_result);
        }

        // If condition succeeds (returns 0), exit loop
        // This is the key difference from while loop
        if (condition_result == 0) {
            break;
        }

        // Execute body
        last_result = execute_command_chain(executor, body);

        iteration++;
    }

    if (iteration >= max_iterations) {
        set_executor_error(executor, "Until loop exceeded maximum iterations");
        return 1;
    }

    return last_result;
}

// Execute for loop
static int execute_for(executor_t *executor, node_t *for_node) {
    if (!for_node || for_node->type != NODE_FOR) {
        return 1;
    }

    const char *var_name = for_node->val.str;
    if (!var_name) {
        set_executor_error(executor, "For loop missing variable name");
        return 1;
    }

    node_t *word_list = for_node->first_child;
    node_t *body = word_list ? word_list->next_sibling : NULL;

    if (!body) {
        set_executor_error(executor, "For loop missing body");
        return 1;
    }

    // Push loop scope
    if (symtable_push_scope(executor->symtable, SCOPE_LOOP, "for-loop") != 0) {
        set_executor_error(executor, "Failed to create loop scope");
        return 1;
    }

    int last_result = 0;

    // Build expanded word list for iteration
    char **expanded_words = NULL;
    int word_count = 0;

    // Process each word in the word list, expanding and splitting
    if (word_list && word_list->first_child) {
        node_t *word = word_list->first_child;
        while (word) {
            if (word->val.str) {
                // Special handling for "$@" to preserve word boundaries
                if (strcmp(word->val.str, "\"$@\"") == 0 ||
                    strcmp(word->val.str, "$@") == 0) {
                    // Handle quoted "$@" - preserve word boundaries
                    extern int shell_argc;
                    extern char **shell_argv;

                    for (int i = 1; i < shell_argc; i++) {
                        if (shell_argv[i]) {
                            // Resize array if needed
                            expanded_words =
                                realloc(expanded_words,
                                        (word_count + 1) * sizeof(char *));
                            if (!expanded_words) {
                                set_executor_error(
                                    executor,
                                    "Memory allocation failed in for loop");
                                symtable_pop_scope(executor->symtable);
                                return 1;
                            }

                            expanded_words[word_count] = strdup(shell_argv[i]);
                            word_count++;
                        }
                    }
                } else {
                    // Normal expansion and splitting for other words
                    char *expanded = expand_if_needed(executor, word->val.str);
                    if (expanded) {
                        // Get IFS for field splitting (default to
                        // space/tab/newline)
                        const char *ifs =
                            symtable_get(executor->symtable, "IFS");
                        if (!ifs) {
                            ifs = " \t\n"; // Default IFS
                        }

                        // Split the expanded string into individual words
                        char *expanded_copy = strdup(expanded);
                        char *token = strtok(expanded_copy, ifs);

                        while (token) {
                            // Resize array if needed
                            expanded_words =
                                realloc(expanded_words,
                                        (word_count + 1) * sizeof(char *));
                            if (!expanded_words) {
                                set_executor_error(
                                    executor,
                                    "Memory allocation failed in for loop");
                                free(expanded);
                                free(expanded_copy);
                                symtable_pop_scope(executor->symtable);
                                return 1;
                            }

                            expanded_words[word_count] = strdup(token);
                            word_count++;
                            token = strtok(NULL, ifs);
                        }

                        free(expanded_copy);
                        free(expanded);
                    }
                }
            }
            word = word->next_sibling;
        }
    }

    // Iterate over expanded words
    for (int i = 0; i < word_count; i++) {
        if (expanded_words[i]) {
            // Set loop variable in current (loop) scope
            if (symtable_set_local_var(executor->symtable, var_name,
                                       expanded_words[i]) != 0) {
                set_executor_error(executor, "Failed to set loop variable");
                // Cleanup expanded words
                for (int j = 0; j < word_count; j++) {
                    free(expanded_words[j]);
                }
                free(expanded_words);
                symtable_pop_scope(executor->symtable);
                return 1;
            }

            if (executor->debug) {
                printf("DEBUG: FOR loop setting %s=%s\n", var_name,
                       expanded_words[i]);
            }

            // Execute body
            last_result = execute_command_chain(executor, body);
        }
    }

    // Cleanup expanded words
    for (int i = 0; i < word_count; i++) {
        free(expanded_words[i]);
    }
    free(expanded_words);

    // Pop loop scope
    symtable_pop_scope(executor->symtable);

    return last_result;
}

// Execute logical AND operator (&&)
static int execute_logical_and(executor_t *executor, node_t *and_node) {
    if (!and_node || and_node->type != NODE_LOGICAL_AND) {
        return 1;
    }

    node_t *left = and_node->first_child;
    node_t *right = left ? left->next_sibling : NULL;

    if (!left || !right) {
        set_executor_error(executor, "Logical AND missing operands");
        return 1;
    }

    // Execute left command
    int left_result = execute_node(executor, left);

    // Only execute right command if left succeeded (exit code 0)
    if (left_result == 0) {
        return execute_node(executor, right);
    }

    // Left failed, return its exit code without executing right
    return left_result;
}

// Execute logical OR operator (||)
static int execute_logical_or(executor_t *executor, node_t *or_node) {
    if (!or_node || or_node->type != NODE_LOGICAL_OR) {
        return 1;
    }

    node_t *left = or_node->first_child;
    node_t *right = left ? left->next_sibling : NULL;

    if (!left || !right) {
        set_executor_error(executor, "Logical OR missing operands");
        return 1;
    }

    // Execute left command
    int left_result = execute_node(executor, left);

    // Only execute right command if left failed (non-zero exit code)
    if (left_result != 0) {
        return execute_node(executor, right);
    }

    // Left succeeded, return its exit code without executing right
    return left_result;
}

// Helper function to add arguments to dynamic list (C99 compliant)
static int add_to_argv_list(char ***argv_list, int *argv_count,
                            int *argv_capacity, char *arg) {
    if (*argv_count >= *argv_capacity) {
        *argv_capacity = *argv_capacity ? *argv_capacity * 2 : 8;
        char **new_list = realloc(*argv_list, *argv_capacity * sizeof(char *));
        if (!new_list) {
            return 0; // Failed to expand
        }
        *argv_list = new_list;
    }
    (*argv_list)[(*argv_count)++] = arg;
    return 1;
}

// Build argv from AST
static char **build_argv_from_ast(executor_t *executor, node_t *command,
                                  int *argc) {
    if (!executor || !command || !argc) {
        return NULL;
    }

    // Dynamic argument list to handle glob expansion
    char **argv_list = NULL;
    int argv_count = 0;
    int argv_capacity = 0;

    // Find here document delimiters to exclude
    char *heredoc_delimiters[10] = {0};
    int delimiter_count = 0;

    node_t *child = command->first_child;
    while (child && delimiter_count < 10) {
        if (child->type == NODE_REDIR_HEREDOC ||
            child->type == NODE_REDIR_HEREDOC_STRIP) {
            if (child->val.str) {
                heredoc_delimiters[delimiter_count] = strdup(child->val.str);
                delimiter_count++;
            }
        }
        child = child->next_sibling;
    }

    // Add command name (no glob expansion for command names)
    if (command->val.str) {
        char *expanded_cmd = expand_if_needed(executor, command->val.str);
        if (!add_to_argv_list(&argv_list, &argv_count, &argv_capacity,
                              expanded_cmd)) {
            free(expanded_cmd);
            goto cleanup_and_fail;
        }
    }

    // Process arguments with glob expansion
    child = command->first_child;
    while (child) {
        // Skip redirection nodes
        if (!is_redirection_node(child)) {
            if (child->val.str) {
                // Check if this is a here document delimiter
                bool is_delimiter = false;
                for (int i = 0; i < delimiter_count; i++) {
                    if (heredoc_delimiters[i] &&
                        strcmp(child->val.str, heredoc_delimiters[i]) == 0) {
                        is_delimiter = true;
                        break;
                    }
                }

                if (!is_delimiter) {
                    char *expanded_arg;

                    // Handle different node types appropriately
                    if (child->type == NODE_STRING_LITERAL) {
                        // Single-quoted strings: no expansion at all
                        expanded_arg = strdup(child->val.str);
                    } else if (child->type == NODE_STRING_EXPANDABLE) {
                        // Double-quoted strings: expand variables but not globs
                        expanded_arg =
                            expand_quoted_string(executor, child->val.str);
                    } else if (child->type == NODE_ARITH_EXP) {
                        // Arithmetic expansion: $((expr))
                        expanded_arg =
                            expand_arithmetic(executor, child->val.str);
                    } else if (child->type == NODE_COMMAND_SUB) {
                        // Command substitution: $(cmd) or `cmd`
                        expanded_arg = expand_command_substitution(
                            executor, child->val.str);
                    } else {
                        // Regular variables and other expandable content
                        expanded_arg =
                            expand_if_needed(executor, child->val.str);
                    }

                    if (getenv("NEW_PARSER_DEBUG")) {
                        fprintf(stderr,
                                "DEBUG: Processing argument: '%s' -> '%s'\n",
                                child->val.str, expanded_arg);
                    }

                    // Check if argument needs brace expansion first
                    if (needs_brace_expansion(expanded_arg)) {
                        int brace_count;
                        char **brace_results =
                            expand_brace_pattern(expanded_arg, &brace_count);

                        if (brace_results) {
                            // Process each brace expansion result for potential
                            // glob expansion
                            for (int j = 0; j < brace_count; j++) {
                                if (needs_glob_expansion(brace_results[j])) {
                                    int glob_count;
                                    char **glob_results = expand_glob_pattern(
                                        brace_results[j], &glob_count);

                                    if (glob_results) {
                                        // Add all glob results, free brace
                                        // result since we won't use it
                                        free(brace_results[j]);
                                        for (int k = 0; k < glob_count; k++) {
                                            if (!add_to_argv_list(
                                                    &argv_list, &argv_count,
                                                    &argv_capacity,
                                                    glob_results[k])) {
                                                // Cleanup remaining strings on
                                                // failure
                                                for (int l = k; l < glob_count;
                                                     l++) {
                                                    free(glob_results[l]);
                                                }
                                                free(glob_results);
                                                for (int l = j + 1;
                                                     l < brace_count; l++) {
                                                    free(brace_results[l]);
                                                }
                                                free(brace_results);
                                                free(expanded_arg);
                                                goto cleanup_and_fail;
                                            }
                                        }
                                        free(glob_results);
                                    } else {
                                        // Glob expansion failed, use brace
                                        // result
                                        if (!add_to_argv_list(
                                                &argv_list, &argv_count,
                                                &argv_capacity,
                                                brace_results[j])) {
                                            for (int l = j + 1; l < brace_count;
                                                 l++) {
                                                free(brace_results[l]);
                                            }
                                            free(brace_results);
                                            free(expanded_arg);
                                            goto cleanup_and_fail;
                                        }
                                    }
                                } else {
                                    // No glob expansion needed, use brace
                                    // result directly
                                    if (!add_to_argv_list(
                                            &argv_list, &argv_count,
                                            &argv_capacity, brace_results[j])) {
                                        for (int l = j + 1; l < brace_count;
                                             l++) {
                                            free(brace_results[l]);
                                        }
                                        free(brace_results);
                                        free(expanded_arg);
                                        goto cleanup_and_fail;
                                    }
                                }
                            }
                            free(brace_results);
                        } else {
                            // Brace expansion failed, fall back to normal
                            // expansion
                            if (needs_glob_expansion(expanded_arg)) {
                                int glob_count;
                                char **glob_results = expand_glob_pattern(
                                    expanded_arg, &glob_count);

                                if (glob_results) {
                                    for (int j = 0; j < glob_count; j++) {
                                        if (!add_to_argv_list(
                                                &argv_list, &argv_count,
                                                &argv_capacity,
                                                glob_results[j])) {
                                            for (int k = j; k < glob_count;
                                                 k++) {
                                                free(glob_results[k]);
                                            }
                                            free(glob_results);
                                            free(expanded_arg);
                                            goto cleanup_and_fail;
                                        }
                                    }
                                    free(glob_results);
                                } else {
                                    if (!add_to_argv_list(
                                            &argv_list, &argv_count,
                                            &argv_capacity, expanded_arg)) {
                                        free(expanded_arg);
                                        goto cleanup_and_fail;
                                    }
                                    expanded_arg =
                                        NULL; // Ownership transferred
                                }
                            } else {
                                if (!add_to_argv_list(&argv_list, &argv_count,
                                                      &argv_capacity,
                                                      expanded_arg)) {
                                    free(expanded_arg);
                                    goto cleanup_and_fail;
                                }
                                expanded_arg = NULL; // Ownership transferred
                            }
                        }
                        if (expanded_arg) {
                            free(expanded_arg);
                        }
                    } else if (needs_glob_expansion(expanded_arg)) {
                        // No brace expansion, check for glob expansion
                        int glob_count;
                        char **glob_results =
                            expand_glob_pattern(expanded_arg, &glob_count);

                        if (glob_results) {
                            // Add all glob results
                            for (int j = 0; j < glob_count; j++) {
                                if (!add_to_argv_list(&argv_list, &argv_count,
                                                      &argv_capacity,
                                                      glob_results[j])) {
                                    // Cleanup on failure
                                    for (int k = j; k < glob_count; k++) {
                                        free(glob_results[k]);
                                    }
                                    free(glob_results);
                                    free(expanded_arg);
                                    goto cleanup_and_fail;
                                }
                            }
                            free(glob_results); // Free the array but not the
                                                // strings
                        } else {
                            // Glob expansion failed, use original
                            if (!add_to_argv_list(&argv_list, &argv_count,
                                                  &argv_capacity,
                                                  expanded_arg)) {
                                free(expanded_arg);
                                goto cleanup_and_fail;
                            }
                        }
                        free(
                            expanded_arg); // We copied the strings or used them
                    } else {
                        // No expansion needed
                        if (!add_to_argv_list(&argv_list, &argv_count,
                                              &argv_capacity, expanded_arg)) {
                            free(expanded_arg);
                            goto cleanup_and_fail;
                        }
                        // expanded_arg ownership transferred to argv_list,
                        // don't free
                    }
                }
            }
        }
        child = child->next_sibling;
    }

    if (argv_count == 0) {
        *argc = 0;
        free(argv_list);
        goto cleanup_delimiters;
    }

    // Convert to final argv array
    char **argv = malloc((argv_count + 1) * sizeof(char *));
    if (!argv) {
        goto cleanup_and_fail;
    }

    for (int i = 0; i < argv_count; i++) {
        argv[i] = argv_list[i];
    }
    argv[argv_count] = NULL;

    *argc = argv_count;
    free(argv_list);

    // Clean up here document delimiters
    for (int k = 0; k < delimiter_count; k++) {
        free(heredoc_delimiters[k]);
    }

    return argv;

cleanup_and_fail:
    // Free all allocated arguments
    for (int i = 0; i < argv_count; i++) {
        free(argv_list[i]);
    }
    free(argv_list);

cleanup_delimiters:
    // Clean up here document delimiters
    for (int k = 0; k < delimiter_count; k++) {
        free(heredoc_delimiters[k]);
    }

    *argc = 0;
    return NULL;
}

// Expand variable/arithmetic/command substitution if needed
char *expand_if_needed(executor_t *executor, const char *text) {
    if (!executor || !text) {
        return NULL;
    }

    // Check for tilde expansion first
    if (text[0] == '~') {
        char *tilde_expanded = expand_tilde(text);
        if (tilde_expanded && strcmp(tilde_expanded, text) != 0) {
            // Tilde was expanded, now check if result needs variable expansion
            const char *first_dollar = strchr(tilde_expanded, '$');
            if (first_dollar) {
                char *final_result =
                    expand_quoted_string(executor, tilde_expanded);
                free(tilde_expanded);
                return final_result;
            }
            return tilde_expanded;
        }
        if (tilde_expanded) {
            free(tilde_expanded);
        }
    }

    // Check if this looks like it contains variables (has $)
    // This is a heuristic for expandable strings
    const char *first_dollar = strchr(text, '$');
    if (first_dollar) {
        // Count dollar signs to determine if we have multiple variables
        int dollar_count = 0;
        for (const char *p = text; *p; p++) {
            if (*p == '$') {
                dollar_count++;
            }
        }

        // If we have multiple dollar signs or the first dollar is not at
        // position 0, treat as quoted string with multiple expansions
        if (dollar_count > 1 || first_dollar != text) {
            return expand_quoted_string(executor, text);
        }

        // Single expansion starting at position 0
        if (strncmp(text, "$((", 3) == 0) {
            return expand_arithmetic(executor, text);
        } else if (strncmp(text, "$(", 2) == 0) {
            return expand_command_substitution(executor, text);
        } else {
            return expand_variable(executor, text);
        }
    }

    // Check for backtick command substitution
    if (text[0] == '`') {
        return expand_command_substitution(executor, text);
    }

    // Regular text - just duplicate
    return strdup(text);
}

// Execute external command
static int execute_external_command(executor_t *executor, char **argv) {
    return execute_external_command_with_redirection(executor, argv, false);
}

static int execute_external_command_with_redirection(executor_t *executor,
                                                     char **argv,
                                                     bool redirect_stderr) {
    if (!argv || !argv[0]) {
        return 1;
    }

    // Reset terminal state before forking for external commands
    // This ensures git and other commands get proper TTY behavior
    if (is_interactive_shell()) {
        fflush(stdout);
        fflush(stderr);
    }

    pid_t pid = fork();
    if (pid == -1) {
        set_executor_error(executor, "Failed to fork");
        return 1;
    }

    if (pid == 0) {
        // Child process
        if (redirect_stderr) {
            // Redirect stderr to /dev/null
            int null_fd = open("/dev/null", O_WRONLY);
            if (null_fd != -1) {
                dup2(null_fd, STDERR_FILENO);
                close(null_fd);
            }
        }

        execvp(argv[0], argv);
        // Check errno to determine appropriate exit code
        int exit_code = 127; // Default: command not found
        if (errno == EACCES) {
            exit_code = 126; // Permission denied
        } else if (errno == ENOENT) {
            exit_code = 127; // Command not found
        }
        if (!redirect_stderr) {
            perror(argv[0]);
        }
        exit(exit_code);
    } else {
        // Parent process
        set_current_child_pid(pid);

        // Enhanced debug tracing for external commands
        DEBUG_TRACE_COMMAND(argv[0], argv, 0);
        DEBUG_PROFILE_ENTER(argv[0]);

        int status;
        waitpid(pid, &status, 0);
        clear_current_child_pid();

        DEBUG_PROFILE_EXIT(argv[0]);

        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}

// Execute brace group { commands; }
static int execute_brace_group(executor_t *executor, node_t *group) {
    if (!group || group->type != NODE_BRACE_GROUP) {
        return 1;
    }

    int last_result = 0;
    node_t *command = group->first_child;

    while (command) {
        last_result = execute_node(executor, command);

        if (executor->debug) {
            printf("DEBUG: Brace group command result: %d\n", last_result);
        }

        command = command->next_sibling;
    }

    return last_result;
}

// Execute subshell ( commands )
static int execute_subshell(executor_t *executor, node_t *subshell) {
    if (!subshell || subshell->type != NODE_SUBSHELL) {
        return 1;
    }

    // Fork a new process for the subshell
    pid_t pid = fork();
    if (pid == -1) {
        set_executor_error(executor, "Failed to fork for subshell");
        return 1;
    }

    if (pid == 0) {
        // Child process - execute commands in subshell environment
        int last_result = 0;
        node_t *command = subshell->first_child;

        while (command) {
            last_result = execute_node(executor, command);
            command = command->next_sibling;
        }

        // Exit with the last command's result
        exit(last_result);
    } else {
        // Parent process - wait for subshell to complete
        int status;
        waitpid(pid, &status, 0);

        int result;
        if (WIFEXITED(status)) {
            result = WEXITSTATUS(status);
        } else {
            result = 1; // Abnormal termination
        }

        return result;
    }
}

// Expand glob pattern using system glob() function
static char **expand_glob_pattern(const char *pattern, int *expanded_count) {
    if (!pattern || !expanded_count) {
        *expanded_count = 0;
        return NULL;
    }

    // Check if globbing is disabled (set -f)
    if (shell_opts.no_globbing) {
        // Return the original pattern without expansion
        char **result = malloc(sizeof(char *));
        if (result) {
            result[0] = strdup(pattern);
            *expanded_count = 1;
            return result;
        }
        *expanded_count = 0;
        return NULL;
    }

    glob_t globbuf;
    int glob_result = glob(pattern, GLOB_NOSORT, NULL, &globbuf);

    if (glob_result == GLOB_NOMATCH) {
        // No matches - return original pattern (POSIX behavior)
        char **result = malloc(2 * sizeof(char *));
        if (result) {
            result[0] = strdup(pattern);
            result[1] = NULL;
            *expanded_count = 1;
        } else {
            *expanded_count = 0;
        }
        return result;
    } else if (glob_result != 0) {
        // Error in globbing
        *expanded_count = 0;
        return NULL;
    }

    // Success - copy results
    *expanded_count = globbuf.gl_pathc;
    char **result = malloc((globbuf.gl_pathc + 1) * sizeof(char *));
    if (!result) {
        globfree(&globbuf);
        *expanded_count = 0;
        return NULL;
    }

    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        result[i] = strdup(globbuf.gl_pathv[i]);

        if (!result[i]) {
            // Cleanup on allocation failure
            for (size_t j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            globfree(&globbuf);
            *expanded_count = 0;
            return NULL;
        }
    }
    result[globbuf.gl_pathc] = NULL;

    globfree(&globbuf);
    return result;
}

// Check if a string contains glob patterns that need expansion
static bool needs_glob_expansion(const char *str) {
    if (!str) {
        return false;
    }

    // Check for glob metacharacters: *, ?, and character classes [...]
    while (*str) {
        if (*str == '*' || *str == '?' || *str == '[') {
            return true;
        }
        str++;
    }
    return false;
}

// Check if a string contains brace patterns that need expansion
static bool needs_brace_expansion(const char *str) {
    if (!str) {
        return false;
    }

    // Check for brace expansion patterns: {a,b,c}
    const char *p = str;
    while (*p) {
        if (*p == '{') {
            // Look for comma and closing brace
            const char *comma = strchr(p + 1, ',');
            const char *close = strchr(p + 1, '}');
            if (comma && close && comma < close) {
                return true;
            }
        }
        p++;
    }
    return false;
}

// Expand brace patterns like {a,b,c} into multiple strings
static char **expand_brace_pattern(const char *pattern, int *expanded_count) {
    if (!pattern || !expanded_count) {
        *expanded_count = 0;
        return NULL;
    }

    // Find the first brace pattern
    const char *open = strchr(pattern, '{');
    if (!open) {
        // No braces - return original pattern
        char **result = malloc(2 * sizeof(char *));
        if (result) {
            result[0] = strdup(pattern);
            result[1] = NULL;
            *expanded_count = 1;
        } else {
            *expanded_count = 0;
        }
        return result;
    }

    const char *close = strchr(open + 1, '}');
    if (!close) {
        // Malformed brace - return original pattern
        char **result = malloc(2 * sizeof(char *));
        if (result) {
            result[0] = strdup(pattern);
            result[1] = NULL;
            *expanded_count = 1;
        } else {
            *expanded_count = 0;
        }
        return result;
    }

    // Extract prefix, brace content, and suffix
    size_t prefix_len = open - pattern;
    size_t content_len = close - open - 1;
    const char *suffix = close + 1;

    char *prefix = malloc(prefix_len + 1);
    char *content = malloc(content_len + 1);
    if (!prefix || !content) {
        free(prefix);
        free(content);
        *expanded_count = 0;
        return NULL;
    }

    strncpy(prefix, pattern, prefix_len);
    prefix[prefix_len] = '\0';
    strncpy(content, open + 1, content_len);
    content[content_len] = '\0';

    // Count comma-separated items
    int item_count = 1;
    for (const char *p = content; *p; p++) {
        if (*p == ',') {
            item_count++;
        }
    }

    // Allocate result array
    char **result = malloc((item_count + 1) * sizeof(char *));
    if (!result) {
        free(prefix);
        free(content);
        *expanded_count = 0;
        return NULL;
    }

    // Split content by commas and build result strings
    int result_index = 0;
    char *item_start = content;
    char *comma_pos = content;

    while (result_index < item_count) {
        // Find next comma or end of string
        while (*comma_pos && *comma_pos != ',') {
            comma_pos++;
        }

        // Extract current item
        size_t item_len = comma_pos - item_start;
        char *item = malloc(item_len + 1);
        if (!item) {
            // Cleanup on failure
            for (int i = 0; i < result_index; i++) {
                free(result[i]);
            }
            free(result);
            free(prefix);
            free(content);
            *expanded_count = 0;
            return NULL;
        }
        strncpy(item, item_start, item_len);
        item[item_len] = '\0';

        // Build full result string: prefix + item + suffix
        size_t full_len = strlen(prefix) + strlen(item) + strlen(suffix);
        result[result_index] = malloc(full_len + 1);
        if (!result[result_index]) {
            // Cleanup on failure
            free(item);
            for (int i = 0; i < result_index; i++) {
                free(result[i]);
            }
            free(result);
            free(prefix);
            free(content);
            *expanded_count = 0;
            return NULL;
        }

        strcpy(result[result_index], prefix);
        strcat(result[result_index], item);
        strcat(result[result_index], suffix);

        free(item);
        result_index++;

        // Move to next item
        if (*comma_pos == ',') {
            comma_pos++;
            item_start = comma_pos;
        }
    }

    result[item_count] = NULL;
    *expanded_count = item_count;

    free(prefix);
    free(content);
    return result;
}

// Execute external command with full redirection setup in child process
static int execute_external_command_with_setup(executor_t *executor,
                                               char **argv,
                                               bool redirect_stderr,
                                               node_t *command) {
    if (!argv || !argv[0]) {
        return 1;
    }

    // Reset terminal state before forking for external commands
    // This ensures git and other commands get proper TTY behavior
    if (is_interactive_shell()) {
        fflush(stdout);
        fflush(stderr);
    }

    pid_t pid = fork();
    if (pid == -1) {
        set_executor_error(executor, "Failed to fork");
        return 1;
    }

    if (pid == 0) {
        // Child process - setup redirections here
        int redir_result = setup_redirections(executor, command);
        if (redir_result != 0) {
            exit(1);
        }

        if (redirect_stderr) {
            // Redirect stderr to /dev/null
            int null_fd = open("/dev/null", O_WRONLY);
            if (null_fd != -1) {
                dup2(null_fd, STDERR_FILENO);
                close(null_fd);
            }
        }

        execvp(argv[0], argv);
        // Check errno to determine appropriate exit code
        int exit_code = 127; // Default: command not found
        if (errno == EACCES) {
            exit_code = 126; // Permission denied
        } else if (errno == ENOENT) {
            exit_code = 127; // Command not found
        }
        if (!redirect_stderr) {
            perror(argv[0]);
        }
        exit(exit_code);
    } else {
        // Parent process
        set_current_child_pid(pid);

        // Enhanced debug tracing for external commands with setup
        DEBUG_TRACE_COMMAND(argv[0], argv, 0);
        DEBUG_PROFILE_ENTER(argv[0]);

        int status;
        waitpid(pid, &status, 0);
        clear_current_child_pid();

        DEBUG_PROFILE_EXIT(argv[0]);

        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}

// Execute builtin command
static int execute_test_builtin(executor_t *executor, char **argv);

static int execute_builtin_command(executor_t *executor, char **argv) {
    if (!argv || !argv[0]) {
        return 1;
    }

    // Set global executor for job control builtins
    current_executor = executor;

    // Find the builtin function in the builtin table
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0) {
            // Count arguments
            int argc = 0;
            while (argv[argc]) {
                argc++;
            }

            int result = builtins[i].func(argc, argv);

            // Clear global executor
            current_executor = NULL;

            return result;
        }
    }

    // Clear global executor
    current_executor = NULL;

    return 1; // Command not found
}

// Check if command is builtin
static bool is_builtin_command(const char *cmd) { return is_builtin(cmd); }

// Execute test builtin command
static int execute_test_builtin(executor_t *executor, char **argv) {
    if (!argv || !argv[0]) {
        return 1;
    }

    // Count arguments
    int argc = 0;
    while (argv[argc]) {
        argc++;
    }

    // Handle [ command - must end with ]
    if (strcmp(argv[0], "[") == 0) {
        if (argc < 2 || strcmp(argv[argc - 1], "]") != 0) {
            return 1; // Missing closing ]
        }
        argc--; // Don't count the closing ]
    }

    // Handle different test cases
    if (argc == 1) {
        // test with no arguments - false
        return 1;
    }

    if (argc == 2) {
        // test STRING - true if STRING is non-empty
        return (argv[1] && strlen(argv[1]) > 0) ? 0 : 1;
    }

    if (argc == 3) {
        // Unary operators
        if (strcmp(argv[1], "-z") == 0) {
            // -z STRING - true if STRING is empty
            return (argv[2] && strlen(argv[2]) == 0) ? 0 : 1;
        }
        if (strcmp(argv[1], "-n") == 0) {
            // -n STRING - true if STRING is non-empty
            return (argv[2] && strlen(argv[2]) > 0) ? 0 : 1;
        }
        // Add more unary operators as needed
        return 1;
    }

    if (argc == 4) {
        // Binary operators: STRING1 OP STRING2
        char *str1 = argv[1];
        char *op = argv[2];
        char *str2 = argv[3];

        if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0) {
            // String equality
            return strcmp(str1, str2) == 0 ? 0 : 1;
        }

        if (strcmp(op, "!=") == 0) {
            // String inequality
            return strcmp(str1, str2) != 0 ? 0 : 1;
        }

        if (strcmp(op, "-eq") == 0) {
            // Numeric equality
            int num1 = atoi(str1);
            int num2 = atoi(str2);
            return (num1 == num2) ? 0 : 1;
        }

        if (strcmp(op, "-ne") == 0) {
            // Numeric inequality
            int num1 = atoi(str1);
            int num2 = atoi(str2);
            return (num1 != num2) ? 0 : 1;
        }

        if (strcmp(op, "-lt") == 0) {
            // Numeric less than
            int num1 = atoi(str1);
            int num2 = atoi(str2);
            return (num1 < num2) ? 0 : 1;
        }

        if (strcmp(op, "-le") == 0) {
            // Numeric less than or equal
            int num1 = atoi(str1);
            int num2 = atoi(str2);
            return (num1 <= num2) ? 0 : 1;
        }

        if (strcmp(op, "-gt") == 0) {
            // Numeric greater than
            int num1 = atoi(str1);
            int num2 = atoi(str2);
            return (num1 > num2) ? 0 : 1;
        }

        if (strcmp(op, "-ge") == 0) {
            // Numeric greater than or equal
            int num1 = atoi(str1);
            int num2 = atoi(str2);
            return (num1 >= num2) ? 0 : 1;
        }
    }

    // Default: false
    return 1;
}

// Check if text is an assignment
static bool is_assignment(const char *text) {
    if (!text) {
        return false;
    }

    // Don't treat parameter expansion ${...} as assignment
    if (text[0] == '$' && text[1] == '{') {
        return false;
    }

    // Look for '=' not at the beginning
    const char *eq = strchr(text, '=');
    return eq && eq != text;
}

// Execute assignment
// Execute assignment using modern symbol table
static int execute_assignment(executor_t *executor, const char *assignment) {
    if (!executor || !assignment) {
        return 1;
    }

    char *eq = strchr(assignment, '=');
    if (!eq) {
        return 1;
    }

    // Split into variable and value
    size_t var_len = eq - assignment;
    char *var_name = malloc(var_len + 1);
    if (!var_name) {
        return 1;
    }

    strncpy(var_name, assignment, var_len);
    var_name[var_len] = '\0';

    // Validate variable name
    if (!var_name[0] || (!isalpha(var_name[0]) && var_name[0] != '_')) {
        free(var_name);
        return 1;
    }

    for (size_t i = 1; i < var_len; i++) {
        if (!isalnum(var_name[i]) && var_name[i] != '_') {
            free(var_name);
            return 1;
        }
    }

    // Expand the value using modern expansion
    char *value = expand_if_needed(executor, eq + 1);

    // Set the variable in the global scope by default (shell behavior)
    int result = symtable_set_global_var(executor->symtable, var_name,
                                         value ? value : "");

    if (executor->debug) {
        printf("DEBUG: Assignment %s=%s (result: %d)\n", var_name,
               value ? value : "", result);
    }

    free(var_name);
    free(value);

    return result == 0 ? 0 : 1;
}

// Execute case statement
static int execute_case(executor_t *executor, node_t *node) {
    if (!executor || !node || node->type != NODE_CASE) {
        return 1;
    }

    // Get the test word and expand variables in it
    char *test_word = expand_if_needed(executor, node->val.str);
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
            char *expanded_pattern = expand_if_needed(executor, pattern);
            if (expanded_pattern) {
                if (match_pattern(test_word, expanded_pattern)) {
                    matched = true;

                    // Execute commands for this case item
                    node_t *commands = case_item->first_child;
                    while (commands) {
                        result = execute_node(executor, commands);
                        if (result != 0) {
                            break;
                        }
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
static int execute_function_definition(executor_t *executor, node_t *node) {
    if (!executor || !node || node->type != NODE_FUNCTION) {
        return 1;
    }

    char *function_name = node->val.str;
    if (!function_name) {
        set_executor_error(executor, "Function definition missing name");
        return 1;
    }

    // Get function body (can be NULL for empty function bodies)
    node_t *body = node->first_child;

    // Extract parameter information from function name if encoded
    function_param_t *params = NULL;
    int param_count = 0;
    char *actual_function_name = function_name;
    
    // Check if function name contains parameter encoding
    char *param_separator = strchr(function_name, '|');
    if (param_separator) {
        // Extract actual function name
        size_t name_len = param_separator - function_name;
        actual_function_name = malloc(name_len + 1);
        strncpy(actual_function_name, function_name, name_len);
        actual_function_name[name_len] = '\0';
        
        // Parse parameter information
        char *param_info = param_separator + 1;
        if (strncmp(param_info, "PARAMS{", 7) == 0) {
            char *param_list = param_info + 7;
            char *end_brace = strchr(param_list, '}');
            if (end_brace) {
                *end_brace = '\0'; // Temporarily null-terminate
                
                // Parse parameter list
                char *param_copy = strdup(param_list);
                char *token = strtok(param_copy, ",");
                function_param_t *last_param = NULL;
                
                while (token) {
                    char *equals = strchr(token, '=');
                    char *param_name = token;
                    char *default_value = NULL;
                    
                    if (equals) {
                        *equals = '\0';
                        default_value = equals + 1;
                    }
                    
                    function_param_t *param = create_function_param(param_name, default_value);
                    if (param) {
                        if (!params) {
                            params = param;
                        } else {
                            last_param->next = param;
                        }
                        last_param = param;
                        param_count++;
                    }
                    
                    token = strtok(NULL, ",");
                }
                
                free(param_copy);
                *end_brace = '}'; // Restore original string
            }
        }
    }

    // Store function in function table
    if (store_function(executor, actual_function_name, body, params, param_count) != 0) {
        set_executor_error(executor, "Failed to define function");
        if (actual_function_name != function_name) {
            free(actual_function_name);
        }
        return 1;
    }

    if (executor->debug) {
        printf("DEBUG: Defined function '%s' with %d parameters\n", actual_function_name, param_count);
    }

    // Clean up allocated function name if we created one
    if (actual_function_name != function_name) {
        free(actual_function_name);
    }

    return 0;
}

// Check if a function is defined
static bool is_function_defined(executor_t *executor,
                                const char *function_name) {
    return find_function(executor, function_name) != NULL;
}

// Execute function call
static int execute_function_call(executor_t *executor,
                                 const char *function_name, char **argv,
                                 int argc) {
    if (!executor || !function_name) {
        return 1;
    }

    function_def_t *func = find_function(executor, function_name);
    if (!func) {
        set_executor_error(executor, "Function not found");
        return 1;
    }

    // Validate function parameters
    if (validate_function_parameters(func, argv, argc) != 0) {
        set_executor_error(executor, "Function parameter validation failed");
        return 1;
    }

    if (executor->debug) {
        printf("DEBUG: Calling function '%s' with %d args\n", function_name,
               argc - 1);
    }

    // Create new scope for function
    if (symtable_push_scope(executor->symtable, SCOPE_FUNCTION,
                            function_name) != 0) {
        set_executor_error(executor, "Failed to create function scope");
        return 1;
    }

    // Set parameters (both positional and named)
    if (func->params) {
        // Set named parameters with defaults
        int arg_index = 1; // Skip function name at argv[0]
        function_param_t *param = func->params;
        
        while (param) {
            const char *value;
            if (arg_index < argc) {
                // Use provided argument
                value = argv[arg_index++];
            } else {
                // Use default value (already validated that required params are present)
                value = param->default_value ? param->default_value : "";
            }
            
            // Set named parameter
            if (symtable_set_local_var(executor->symtable, param->name, value) != 0) {
                symtable_pop_scope(executor->symtable);
                set_executor_error(executor, "Failed to set function parameter");
                return 1;
            }
            param = param->next;
        }
    }
    
    // Set positional parameters ($1, $2, etc.) for backward compatibility
    for (int i = 1; i < argc; i++) {
        char param_name[16];
        snprintf(param_name, sizeof(param_name), "%d", i);
        if (symtable_set_local_var(executor->symtable, param_name, argv[i]) !=
            0) {
            symtable_pop_scope(executor->symtable);
            set_executor_error(executor, "Failed to set function parameter");
            return 1;
        }
    }

    // Set $# (argument count)
    char argc_str[16];
    snprintf(argc_str, sizeof(argc_str), "%d", argc - 1);
    symtable_set_local_var(executor->symtable, "#", argc_str);

    // Execute function body (handle multiple commands)
    int result = 0;
    node_t *command = func->body;
    while (command) {
        result = execute_node(executor, command);

        // Check if this is a function return (special code 200-255)
        if (result >= 200 && result <= 255) {
            // Extract the actual return value from the special code
            int actual_return = result - 200;

            // Restore previous scope before returning
            symtable_pop_scope(executor->symtable);

            return actual_return;
        }

        if (result != 0) {
            break; // Stop on first error
        }
        command = command->next_sibling;
    }

    // Restore previous scope
    symtable_pop_scope(executor->symtable);

    return result;
}

// Create a new function parameter
function_param_t *create_function_param(const char *name, const char *default_value) {
    if (!name) {
        return NULL;
    }
    
    function_param_t *param = malloc(sizeof(function_param_t));
    if (!param) {
        return NULL;
    }
    
    param->name = strdup(name);
    if (!param->name) {
        free(param);
        return NULL;
    }
    
    param->default_value = default_value ? strdup(default_value) : NULL;
    param->is_required = (default_value == NULL);
    param->next = NULL;
    
    return param;
}

// Free function parameter list
void free_function_params(function_param_t *params) {
    while (params) {
        function_param_t *next = params->next;
        free(params->name);
        free(params->default_value);
        free(params);
        params = next;
    }
}

// Validate function parameters against call arguments
static int validate_function_parameters(function_def_t *func, char **argv, int argc) {
    if (!func) {
        return 1;
    }
    
    // If no parameters defined, allow any arguments (backward compatibility)
    if (!func->params) {
        return 0;
    }
    
    int arg_index = 1; // Skip function name at argv[0]
    function_param_t *param = func->params;
    
    while (param) {
        if (arg_index < argc) {
            // Argument provided for this parameter
            arg_index++;
        } else if (param->is_required) {
            // Required parameter missing
            fprintf(stderr, "Error: Function '%s' requires parameter '%s'\n", 
                    func->name, param->name);
            return 1;
        }
        // Optional parameter without argument - will use default
        param = param->next;
    }
    
    // Check for too many arguments
    if (arg_index < argc) {
        fprintf(stderr, "Error: Function '%s' called with %d arguments but only accepts %d\n",
                func->name, argc - 1, func->param_count);
        return 1;
    }
    
    return 0;
}

// Find function in function table
static function_def_t *find_function(executor_t *executor,
                                     const char *function_name) {
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
static int store_function(executor_t *executor, const char *function_name,
                          node_t *body, function_param_t *params, int param_count) {
    if (!executor || !function_name) {
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
            free_function_params(to_remove->params);
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

    // Create a deep copy of the body AST (including sibling chain)
    // Allow NULL bodies for empty functions
    new_func->body = copy_ast_chain(body);
    if (!new_func->body && body != NULL) {
        // Only fail if body was non-NULL but copy failed
        free(new_func->name);
        free(new_func);
        return 1;
    }

    // Store parameter information
    new_func->params = params;
    new_func->param_count = param_count;

    // Add to front of function list
    new_func->next = executor->functions;
    executor->functions = new_func;

    return 0;
}

// Copy AST node recursively
static node_t *copy_ast_node(node_t *node) {
    if (!node) {
        return NULL;
    }

    node_t *copy = new_node(node->type);
    if (!copy) {
        return NULL;
    }

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

// Copy AST node chain (including siblings)
static node_t *copy_ast_chain(node_t *node) {
    if (!node) {
        return NULL;
    }

    node_t *first_copy = copy_ast_node(node);
    if (!first_copy) {
        return NULL;
    }

    node_t *current_copy = first_copy;
    node_t *current_orig = node->next_sibling;

    while (current_orig) {
        node_t *sibling_copy = copy_ast_node(current_orig);
        if (!sibling_copy) {
            free_node_tree(first_copy);
            return NULL;
        }

        current_copy->next_sibling = sibling_copy;
        sibling_copy->prev_sibling = current_copy;

        current_copy = sibling_copy;
        current_orig = current_orig->next_sibling;
    }

    return first_copy;
}

// Helper function to check if a string is empty or null
static bool is_empty_or_null(const char *str) { return !str || str[0] == '\0'; }

// Helper function to extract substring
static char *extract_substring(const char *str, int offset, int length) {
    if (!str) {
        return strdup("");
    }

    int str_len = strlen(str);

    // Handle negative offset (from end)
    if (offset < 0) {
        offset = str_len + offset;
        if (offset < 0) {
            offset = 0;
        }
    }

    // Bounds check
    if (offset >= str_len) {
        return strdup("");
    }

    // Calculate actual length
    int remaining = str_len - offset;
    if (length < 0 || length > remaining) {
        length = remaining;
    }

    char *result = malloc(length + 1);
    if (!result) {
        return strdup("");
    }

    strncpy(result, str + offset, length);
    result[length] = '\0';

    return result;
}

// Enhanced glob pattern matching for parameter expansion with special character
// support
static bool match_pattern(const char *str, const char *pattern) {
    if (!str || !pattern) {
        return false;
    }

    const char *s = str;
    const char *p = pattern;

    while (*p) {
        if (*p == '*') {
            // Handle wildcard
            p++; // Skip the *

            // If * is at the end, it matches everything remaining
            if (*p == '\0') {
                return true;
            }

            // Try to match the rest of the pattern at each position in the
            // string
            while (*s) {
                if (match_pattern(s, p)) {
                    return true;
                }
                s++;
            }

            // Try matching the pattern with empty string (for cases like
            // "*suffix")
            return match_pattern(s, p);
        } else if (*p == '?') {
            // Wildcard matches any single character
            if (*s == '\0') {
                return false; // ? can't match empty
            }
            s++;
            p++;
        } else if (*p == '[') {
            // Character class pattern [abc] or [a-z]
            if (*s == '\0') {
                return false; // Character class can't match empty
            }

            p++; // Skip opening [
            bool matched = false;
            bool negated = false;

            // Check for negation [!abc] or [^abc]
            if (*p == '!' || *p == '^') {
                negated = true;
                p++;
            }

            while (*p && *p != ']') {
                if (p[1] == '-' && p[2] != ']' && p[2] != '\0') {
                    // Range pattern like a-z
                    if (*s >= *p && *s <= p[2]) {
                        matched = true;
                    }
                    p += 3; // Skip a-z
                } else {
                    // Single character
                    if (*s == *p) {
                        matched = true;
                    }
                    p++;
                }
            }

            if (*p == ']') {
                p++; // Skip closing ]
            }

            // Apply negation if needed
            if (negated) {
                matched = !matched;
            }

            if (!matched) {
                return false;
            }

            s++;
        } else {
            // Literal character match (including special chars like : @ / etc.)
            if (*s != *p) {
                return false;
            }
            s++;
            p++;
        }
    }

    // Pattern is exhausted, string should be too for a complete match
    return *s == '\0';
}

// Find shortest match from beginning (for # operator)
static int find_prefix_match(const char *str, const char *pattern,
                             bool longest) {
    if (!str || !pattern) {
        return 0;
    }

    int str_len = strlen(str);
    int match_len = 0;

    for (int i = 0; i <= str_len; i++) {
        char *substr = malloc(i + 1);
        if (!substr) {
            break;
        }

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
static int find_suffix_match(const char *str, const char *pattern,
                             bool longest) {
    if (!str || !pattern) {
        return 0;
    }

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
    if (!str) {
        return strdup("");
    }

    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) {
        return strdup("");
    }

    strcpy(result, str);
    if (len > 0 && islower(result[0])) {
        result[0] = toupper(result[0]);
    }

    return result;
}

static char *convert_case_first_lower(const char *str) {
    if (!str) {
        return strdup("");
    }

    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) {
        return strdup("");
    }

    strcpy(result, str);
    if (len > 0 && isupper(result[0])) {
        result[0] = tolower(result[0]);
    }

    return result;
}

static char *convert_case_all_upper(const char *str) {
    if (!str) {
        return strdup("");
    }

    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) {
        return strdup("");
    }

    for (size_t i = 0; i < len; i++) {
        result[i] = toupper(str[i]);
    }
    result[len] = '\0';

    return result;
}

static char *convert_case_all_lower(const char *str) {
    if (!str) {
        return strdup("");
    }

    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) {
        return strdup("");
    }

    for (size_t i = 0; i < len; i++) {
        result[i] = tolower(str[i]);
    }
    result[len] = '\0';

    return result;
}

// Recursively expand variables within a string (for parameter expansion
// defaults)
static char *expand_variables_in_string(executor_t *executor, const char *str) {
    if (!str || !executor) {
        return strdup("");
    }

    size_t len = strlen(str);
    char *result = malloc(len * 2 + 1); // Start with double size
    if (!result) {
        return strdup("");
    }

    size_t result_pos = 0;
    size_t result_size = len * 2 + 1;

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '$') {
            // Check for arithmetic expansion $((...)
            if (i + 2 < len && str[i + 1] == '(' && str[i + 2] == '(') {
                // This is arithmetic expansion $((expr))
                size_t arith_start = i;
                size_t arith_end = i + 3;
                int paren_depth = 2;

                while (arith_end < len && paren_depth > 0) {
                    if (str[arith_end] == '(') {
                        paren_depth++;
                    } else if (str[arith_end] == ')') {
                        paren_depth--;
                    }
                    arith_end++;
                }

                if (paren_depth == 0) {
                    // Extract arithmetic expression including $(( and ))
                    size_t full_arith_len = arith_end - arith_start;
                    char *full_arith_expr = malloc(full_arith_len + 1);
                    if (full_arith_expr) {
                        strncpy(full_arith_expr, &str[arith_start],
                                full_arith_len);
                        full_arith_expr[full_arith_len] = '\0';

                        // Expand arithmetic expression
                        char *arith_result =
                            expand_arithmetic(executor, full_arith_expr);
                        if (arith_result) {
                            size_t result_len = strlen(arith_result);

                            // Ensure buffer is large enough
                            while (result_pos + result_len >= result_size) {
                                result_size *= 2;
                                char *new_result = realloc(result, result_size);
                                if (!new_result) {
                                    free(result);
                                    free(arith_result);
                                    free(full_arith_expr);
                                    return strdup("");
                                }
                                result = new_result;
                            }

                            // Copy arithmetic result
                            strcpy(&result[result_pos], arith_result);
                            result_pos += result_len;
                            free(arith_result);
                        }
                        free(full_arith_expr);
                    }

                    i = arith_end - 1; // Skip past the entire $((...)
                    continue;
                }
            }
            // Check for command substitution $(...)
            else if (i + 1 < len && str[i + 1] == '(') {
                // Find matching closing parenthesis using find_closing_brace
                char *temp_str =
                    (char *)&str[i + 1]; // Start from the opening parenthesis
                size_t brace_offset = find_closing_brace(temp_str);

                if (brace_offset > 0) {
                    // Extract command from $(...)
                    size_t cmd_len =
                        brace_offset - 1; // Exclude the closing paren
                    char *command = malloc(cmd_len + 1);
                    if (command) {
                        strncpy(command, &str[i + 2], cmd_len); // Skip $(
                        command[cmd_len] = '\0';

                        // Execute command substitution - need to wrap in $()
                        // format
                        char *wrapped_cmd =
                            malloc(cmd_len + 4); // +3 for $() +1 for null
                        if (wrapped_cmd) {
                            snprintf(wrapped_cmd, cmd_len + 4, "$(%s)",
                                     command);
                            char *cmd_result = expand_command_substitution(
                                executor, wrapped_cmd);
                            free(wrapped_cmd);
                            if (cmd_result) {
                                size_t value_len = strlen(cmd_result);

                                // Ensure buffer is large enough
                                while (result_pos + value_len >= result_size) {
                                    result_size *= 2;
                                    char *new_result =
                                        realloc(result, result_size);
                                    if (!new_result) {
                                        free(result);
                                        free(cmd_result);
                                        free(command);
                                        return strdup("");
                                    }
                                    result = new_result;
                                }

                                strcpy(&result[result_pos], cmd_result);
                                result_pos += value_len;
                                free(cmd_result);
                            }
                        }

                        free(command);
                        i = i + 1 + brace_offset; // Skip past the entire $(...)
                        continue;
                    }
                }
            }

            // Find variable name
            size_t var_start = i + 1;
            size_t var_end = var_start;

            // Handle ${var} format
            if (var_start < len && str[var_start] == '{') {
                // Use proper brace matching for nested expressions
                char *brace_str = (char *)&str[var_start];
                size_t brace_len = find_closing_brace(brace_str);

                if (brace_len > 0) {
                    // brace_len is the index of the closing brace
                    var_end = var_start + brace_len +
                              1; // Point to after closing brace
                } else {
                    // Fallback: find closing brace manually with nesting
                    // support
                    int brace_count = 1;
                    var_end = var_start + 1; // Start after opening {

                    while (var_end < len && brace_count > 0) {
                        if (str[var_end] == '{') {
                            brace_count++;
                        } else if (str[var_end] == '}') {
                            brace_count--;
                        }
                        var_end++;
                    }
                }
            } else {
                // Handle $var format
                while (var_end < len &&
                       (isalnum(str[var_end]) || str[var_end] == '_')) {
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

                    char *var_value = expand_variable(executor, var_expr);
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
static char *parse_parameter_expansion(executor_t *executor,
                                       const char *expansion) {
    if (!expansion) {
        return strdup("");
    }

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
    const char *operators[] = {":-", ":+", "##", "%%", "^^", ",,", "#", "%",
                               "^",  ",",  "-",  "+",  ":=", "=",  ":", NULL};
    int op_type = -1;

    // Find the first valid operator - prioritize longer operators first
    for (int i = 0; operators[i]; i++) {
        const char *found = strstr(expansion, operators[i]);
        if (found) {
            // Skip single-character operators that are part of longer ones
            if (strlen(operators[i]) == 1) {
                // Check if this single char is part of a longer operator
                bool part_of_longer = false;

                // Check for :- and :+ before processing single :
                if (strcmp(operators[i], ":") == 0) {
                    if ((found > expansion &&
                         (found[-1] == '-' || found[-1] == '+')) ||
                        (found[1] == '-' || found[1] == '+' ||
                         found[1] == '=')) {
                        part_of_longer = true;
                    }
                }

                // Check for ## and %% before processing single # or %
                if (strcmp(operators[i], "#") == 0 && found[1] == '#') {
                    part_of_longer = true;
                }
                if (strcmp(operators[i], "%") == 0 && found[1] == '%') {
                    part_of_longer = true;
                }

                if (part_of_longer) {
                    continue;
                }
            }

            // If we haven't found an operator yet, or this one comes first, use
            // it
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
        if (!var_name) {
            return strdup("");
        }

        strncpy(var_name, expansion, var_len);
        var_name[var_len] = '\0';

        // Get variable value
        char *var_value = symtable_get_var(executor->symtable, var_name);
        const char *default_value = op_pos + strlen(operators[op_type]);

        // Expand variables in default value
        char *expanded_default =
            expand_variables_in_string(executor, default_value);

        char *result = NULL;

        switch (op_type) {
        case 0: // ${var:-default} - use default if var is unset or empty
            if (is_empty_or_null(var_value)) {
                result = strdup(expanded_default);
            } else {
                result = strdup(var_value);
            }
            break;

        case 1: // ${var:+alternative} - use alternative if var is set and
                // non-empty
            if (!is_empty_or_null(var_value)) {
                result = strdup(expanded_default);
            } else {
                result = strdup("");
            }
            break;

        case 2: // ${var##pattern} - remove longest match of pattern from
                // beginning
            if (var_value) {
                int match_len =
                    find_prefix_match(var_value, expanded_default, true);
                result = strdup(var_value + match_len);
            } else {
                result = strdup("");
            }
            break;

        case 3: // ${var%%pattern} - remove longest match of pattern from end
            if (var_value) {
                int str_len = strlen(var_value);
                int match_len =
                    find_suffix_match(var_value, expanded_default, true);
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

        case 6: // ${var#pattern} - remove shortest match of pattern from
                // beginning
            if (var_value) {
                int match_len =
                    find_prefix_match(var_value, expanded_default, false);
                result = strdup(var_value + match_len);
            } else {
                result = strdup("");
            }
            break;

        case 7: // ${var%pattern} - remove shortest match of pattern from end
            if (var_value) {
                int str_len = strlen(var_value);
                int match_len =
                    find_suffix_match(var_value, expanded_default, false);
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

        case 8: // ${var^} - convert first character to uppercase
            if (var_value) {
                result = convert_case_first_upper(var_value);
            } else {
                result = strdup("");
            }
            break;

        case 9: // ${var,} - convert first character to lowercase
            if (var_value) {
                result = convert_case_first_lower(var_value);
            } else {
                result = strdup("");
            }
            break;

        case 10: // ${var-default} - use default if var is unset (but not if
                 // empty)
            if (!var_value) {
                result = strdup(expanded_default);
            } else {
                result = strdup(var_value);
            }
            break;

        case 11: // ${var+alternative} - use alternative if var is set (even if
                 // empty)
            if (var_value) {
                result = strdup(expanded_default);
            } else {
                result = strdup("");
            }
            break;

        case 12: // ${var:=default} - assign default if var is unset or empty
                 // and return it
            if (is_empty_or_null(var_value)) {
                symtable_set_var(executor->symtable, var_name, expanded_default,
                                 SYMVAR_NONE);
                result = strdup(expanded_default);
            } else {
                result = strdup(var_value);
            }
            break;

        case 13: // ${var=default} - assign default if var is unset and return
                 // it
            if (!var_value) {
                symtable_set_var(executor->symtable, var_name, expanded_default,
                                 SYMVAR_NONE);
                result = strdup(expanded_default);
            } else {
                result = strdup(var_value);
            }
            break;

        case 14: // ${var:offset:length} - substring expansion
            if (var_value) {
                // Parse offset and optional length (with variable expansion)
                char *expanded_offset_str =
                    expand_variables_in_string(executor, expanded_default);
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
        }

        free(var_name);
        free(expanded_default);
        return result ? result : strdup("");
    }

    // No operator found, just get the variable value
    // First check for special variables that aren't in the symbol table
    if (strlen(expansion) == 1) {
        extern int shell_argc;
        extern char **shell_argv;
        extern int last_exit_status;
        extern pid_t shell_pid;
        extern pid_t last_background_pid;

        char buffer[1024];

        switch (expansion[0]) {
        case '?': // Exit status of last command
            snprintf(buffer, sizeof(buffer), "%d", last_exit_status);
            return strdup(buffer);

        case '$': // Shell process ID
            snprintf(buffer, sizeof(buffer), "%d", (int)shell_pid);
            return strdup(buffer);

        case '#': // Number of positional parameters
            snprintf(buffer, sizeof(buffer), "%d",
                     shell_argc > 1 ? shell_argc - 1 : 0);
            return strdup(buffer);

        case '!': // Process ID of last background command
            if (last_background_pid > 0) {
                snprintf(buffer, sizeof(buffer), "%d",
                         (int)last_background_pid);
                return strdup(buffer);
            } else {
                return strdup("");
            }

        case '*': // All positional parameters as single word
            if (shell_argc > 1) {
                size_t total_len = 0;
                for (int i = 1; i < shell_argc; i++) {
                    if (shell_argv[i]) {
                        total_len += strlen(shell_argv[i]) + 1; // +1 for space
                    }
                }
                if (total_len > 0) {
                    char *result = malloc(total_len);
                    if (result) {
                        result[0] = '\0';
                        for (int i = 1; i < shell_argc; i++) {
                            if (shell_argv[i]) {
                                if (i > 1) {
                                    strcat(result, " ");
                                }
                                strcat(result, shell_argv[i]);
                            }
                        }
                        return result;
                    }
                }
            }
            return strdup("");

        case '@': // All positional parameters as separate words
            if (shell_argc > 1) {
                size_t total_len = 0;
                for (int i = 1; i < shell_argc; i++) {
                    if (shell_argv[i]) {
                        total_len += strlen(shell_argv[i]) + 1; // +1 for space
                    }
                }
                if (total_len > 0) {
                    char *result = malloc(total_len);
                    if (result) {
                        result[0] = '\0';
                        for (int i = 1; i < shell_argc; i++) {
                            if (shell_argv[i]) {
                                if (i > 1) {
                                    strcat(result, " ");
                                }
                                strcat(result, shell_argv[i]);
                            }
                        }
                        return result;
                    }
                }
            }
            return strdup("");

        default:
            if (expansion[0] >= '0' && expansion[0] <= '9') {
                // Handle positional parameters $0, $1, $2, etc.
                int pos = expansion[0] - '0';

                if (pos == 0) {
                    // $0 is the script/shell name
                    return strdup((shell_argc > 0 && shell_argv[0])
                                      ? shell_argv[0]
                                      : "lusush");
                } else if (pos > 0 && pos < shell_argc && shell_argv[pos]) {
                    // $1, $2, etc. are script arguments
                    return strdup(shell_argv[pos]);
                } else {
                    // Parameter doesn't exist, return empty string
                    return strdup("");
                }
            }
            break;
        }
    }

    // Fall back to symbol table lookup for regular variables
    char *value = symtable_get_var(executor->symtable, expansion);
    return value ? strdup(value) : strdup("");
}

// Expand variable reference using modern symbol table with advanced parameter
// expansion
static char *expand_variable(executor_t *executor, const char *var_text) {
    if (!executor || !var_text || var_text[0] != '$') {
        return strdup(var_text ? var_text : "");
    }

    // Special case: if var_text is exactly "$$", treat it as shell PID
    if (strcmp(var_text, "$$") == 0) {
        extern pid_t shell_pid;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", (int)shell_pid);
        return strdup(buffer);
    }

    // Special case: if var_text is exactly "$", treat it as shell PID
    if (strcmp(var_text, "$") == 0) {
        extern pid_t shell_pid;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", (int)shell_pid);
        return strdup(buffer);
    }

    // Special case: if var_text is exactly "$?", treat it as exit status
    if (strcmp(var_text, "$?") == 0) {
        extern int last_exit_status;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", last_exit_status);
        return strdup(buffer);
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
        // Simple $var format - handle special variables and regular variables
        size_t name_len = 0;

        // Check for special single-character variables first
        if (var_name[0] == '?' || var_name[0] == '$' || var_name[0] == '#' ||
            var_name[0] == '*' || var_name[0] == '@' || var_name[0] == '!' ||
            (var_name[0] >= '0' && var_name[0] <= '9')) {
            name_len = 1;
        } else {
            // Regular variable names (alphanumeric + underscore)
            while (var_name[name_len] &&
                   (isalnum(var_name[name_len]) || var_name[name_len] == '_')) {
                name_len++;
            }
        }

        if (name_len > 0) {
            char *name = malloc(name_len + 1);
            if (name) {
                strncpy(name, var_name, name_len);
                name[name_len] = '\0';

                // Look up in modern symbol table
                char *value = symtable_get_var(executor->symtable, name);

                // Check for unset variable error (set -u)
                if (!value && shell_opts.unset_error && name_len > 0) {
                    // Don't error on special variables that have default
                    // behavior
                    if (name_len != 1 ||
                        (name[0] != '?' && name[0] != '$' && name[0] != '#' &&
                         name[0] != '0' && name[0] != '@' && name[0] != '*')) {
                        fprintf(stderr, "%s: %s: unbound variable\n", "lusush",
                                name);
                        free(name);
                        exit(1); // POSIX requires shell to exit on unbound
                                 // variable
                    }
                }

                // If not found in symbol table and it's a special variable,
                // handle it directly
                if (!value && name_len == 1) {
                    extern int shell_argc;
                    extern char **shell_argv;
                    extern int last_exit_status;
                    extern pid_t shell_pid;
                    extern pid_t last_background_pid;

                    char buffer[1024];

                    switch (name[0]) {
                    case '?': // Exit status of last command
                        snprintf(buffer, sizeof(buffer), "%d",
                                 last_exit_status);
                        free(name);
                        return strdup(buffer);

                    case '$': // Shell process ID
                        snprintf(buffer, sizeof(buffer), "%d", (int)shell_pid);
                        free(name);
                        return strdup(buffer);

                    case '#': // Number of positional parameters
                        snprintf(buffer, sizeof(buffer), "%d",
                                 shell_argc > 1 ? shell_argc - 1 : 0);
                        free(name);
                        return strdup(buffer);

                    case '!': // Process ID of last background command
                        if (last_background_pid > 0) {
                            snprintf(buffer, sizeof(buffer), "%d",
                                     (int)last_background_pid);
                            free(name);
                            return strdup(buffer);
                        } else {
                            free(name);
                            return strdup("");
                        }

                    case '*': // All positional parameters as single word
                        {
                            // Check if we're in function scope - try to get $# from local scope
                            char *func_argc_str = symtable_get_var(executor->symtable, "#");
                            if (func_argc_str && executor->symtable) {
                                // We're in a function scope - use function parameters
                                int func_argc = atoi(func_argc_str);
                                if (func_argc > 0) {
                                    size_t total_len = 0;
                                    // Calculate total length needed
                                    for (int i = 1; i <= func_argc; i++) {
                                        char param_name[16];
                                        snprintf(param_name, sizeof(param_name), "%d", i);
                                        char *param_value = symtable_get_var(executor->symtable, param_name);
                                        if (param_value) {
                                            total_len += strlen(param_value) + 1; // +1 for space
                                        }
                                    }
                                    if (total_len > 0) {
                                        char *result = malloc(total_len);
                                        if (result) {
                                            result[0] = '\0';
                                            for (int i = 1; i <= func_argc; i++) {
                                                char param_name[16];
                                                snprintf(param_name, sizeof(param_name), "%d", i);
                                                char *param_value = symtable_get_var(executor->symtable, param_name);
                                                if (param_value) {
                                                    if (i > 1) {
                                                        strcat(result, " ");
                                                    }
                                                    strcat(result, param_value);
                                                }
                                            }
                                            free(name);
                                            return result;
                                        }
                                    }
                                }
                                free(name);
                                return strdup("");
                            } else {
                                // Use global shell parameters
                                if (shell_argc > 1) {
                                    size_t total_len = 0;
                                    for (int i = 1; i < shell_argc; i++) {
                                        if (shell_argv[i]) {
                                            total_len += strlen(shell_argv[i]) +
                                                         1; // +1 for space
                                        }
                                    }
                                    if (total_len > 0) {
                                        char *result = malloc(total_len);
                                        if (result) {
                                            result[0] = '\0';
                                            for (int i = 1; i < shell_argc; i++) {
                                                if (shell_argv[i]) {
                                                    if (i > 1) {
                                                        strcat(result, " ");
                                                    }
                                                    strcat(result, shell_argv[i]);
                                                }
                                            }
                                            free(name);
                                            return result;
                                        }
                                    }
                                }
                                free(name);
                                return strdup("");
                            }
                        }

                    case '@': // All positional parameters as separate words
                        {
                            // Check if we're in function scope - try to get $# from local scope
                            char *func_argc_str = symtable_get_var(executor->symtable, "#");
                            if (func_argc_str && executor->symtable) {
                                // We're in a function scope - use function parameters
                                int func_argc = atoi(func_argc_str);
                                if (func_argc > 0) {
                                    size_t total_len = 0;
                                    // Calculate total length needed
                                    for (int i = 1; i <= func_argc; i++) {
                                        char param_name[16];
                                        snprintf(param_name, sizeof(param_name), "%d", i);
                                        char *param_value = symtable_get_var(executor->symtable, param_name);
                                        if (param_value) {
                                            total_len += strlen(param_value) + 1; // +1 for space
                                        }
                                    }
                                    if (total_len > 0) {
                                        char *result = malloc(total_len);
                                        if (result) {
                                            result[0] = '\0';
                                            for (int i = 1; i <= func_argc; i++) {
                                                char param_name[16];
                                                snprintf(param_name, sizeof(param_name), "%d", i);
                                                char *param_value = symtable_get_var(executor->symtable, param_name);
                                                if (param_value) {
                                                    if (i > 1) {
                                                        strcat(result, " ");
                                                    }
                                                    strcat(result, param_value);
                                                }
                                            }
                                            free(name);
                                            return result;
                                        }
                                    }
                                }
                                free(name);
                                return strdup("");
                            } else {
                                // Use global shell parameters  
                                // Note: This should ideally preserve word boundaries,
                                // but for now we'll implement it similarly to $* for
                                // compatibility
                                if (shell_argc > 1) {
                                    size_t total_len = 0;
                                    for (int i = 1; i < shell_argc; i++) {
                                        if (shell_argv[i]) {
                                            total_len += strlen(shell_argv[i]) +
                                                         1; // +1 for space
                                        }
                                    }
                                    if (total_len > 0) {
                                        char *result = malloc(total_len);
                                        if (result) {
                                            result[0] = '\0';
                                            for (int i = 1; i < shell_argc; i++) {
                                                if (shell_argv[i]) {
                                                    if (i > 1) {
                                                        strcat(result, " ");
                                                    }
                                                    strcat(result, shell_argv[i]);
                                                }
                                            }
                                            free(name);
                                            return result;
                                        }
                                    }
                                }
                                free(name);
                                return strdup("");
                            }
                        }

                    default:
                        if (name[0] >= '0' && name[0] <= '9') {
                            // Handle positional parameters $0, $1, $2, etc.
                            int pos = name[0] - '0';

                            if (pos == 0) {
                                // $0 is the script/shell name
                                free(name);
                                return strdup((shell_argc > 0 && shell_argv[0])
                                                  ? shell_argv[0]
                                                  : "lusush");
                            } else if (pos > 0 && pos < shell_argc &&
                                       shell_argv[pos]) {
                                // $1, $2, etc. are script arguments
                                free(name);
                                return strdup(shell_argv[pos]);
                            } else {
                                // Parameter doesn't exist, return empty string
                                free(name);
                                return strdup("");
                            }
                        }
                        break;
                    }
                }

                free(name);
                return value ? strdup(value) : strdup("");
            }
        }
    }

    return strdup("");
}

// Expand tilde (~) to home directory
static char *expand_tilde(const char *text) {
    if (!text || text[0] != '~') {
        return strdup(text ? text : "");
    }

    // Find the end of the tilde expression (until '/' or end of string)
    const char *slash = strchr(text, '/');
    const char *rest = slash ? slash : "";
    size_t tilde_len = slash ? (size_t)(slash - text) : strlen(text);

    if (tilde_len == 1) {
        // Simple ~ expansion to $HOME
        const char *home = getenv("HOME");
        if (!home) {
            // Fallback if HOME is not set
            struct passwd *pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : "/";
        }

        if (strlen(rest) == 0) {
            return strdup(home);
        } else {
            size_t result_len = strlen(home) + strlen(rest) + 1;
            char *result = malloc(result_len);
            if (result) {
                strcpy(result, home);
                strcat(result, rest);
            }
            return result;
        }
    } else {
        // ~user expansion to user's home directory
        char *username = malloc(tilde_len);
        if (!username) {
            return strdup(text);
        }

        strncpy(username, text + 1, tilde_len - 1);
        username[tilde_len - 1] = '\0';

        struct passwd *pw = getpwnam(username);
        free(username);

        if (!pw) {
            // User not found, return original text
            return strdup(text);
        }

        if (strlen(rest) == 0) {
            return strdup(pw->pw_dir);
        } else {
            size_t result_len = strlen(pw->pw_dir) + strlen(rest) + 1;
            char *result = malloc(result_len);
            if (result) {
                strcpy(result, pw->pw_dir);
                strcat(result, rest);
            }
            return result;
        }
    }
}

// Modern arithmetic expansion using extracted and modernized shunting yard
// algorithm
extern char *arithm_expand(const char *orig_expr);

// Expand arithmetic expression using modern implementation
static char *expand_arithmetic(executor_t *executor, const char *arith_text) {
    if (!executor || !arith_text) {
        return strdup("0");
    }

    // Use the modern arithmetic evaluator with executor context for scoped
    // variables
    char *result = arithm_expand_with_executor(executor, arith_text);
    if (result) {
        return result;
    }

    // If arithm_expand returns NULL, there was an error (like division by zero)
    // Print error message and set exit status to indicate error
    extern bool arithm_error_flag;
    extern char *arithm_error_message;

    if (arithm_error_flag && arithm_error_message) {
        fprintf(stderr, "lusush: arithmetic: %s\n", arithm_error_message);
    } else {
        fprintf(stderr, "lusush: arithmetic: evaluation error\n");
    }

    set_exit_status(1);
    return strdup("");
}

static char *expand_command_substitution(executor_t *executor,
                                         const char *cmd_text) {
    if (!executor || !cmd_text) {
        return strdup("");
    }

    // Extract command from $(command) or `command` format
    char *command = NULL;
    if (strncmp(cmd_text, "$(", 2) == 0 &&
        cmd_text[strlen(cmd_text) - 1] == ')') {
        // Extract from $(command)
        size_t len = strlen(cmd_text) - 3; // Remove $( and )
        command = malloc(len + 1);
        if (!command) {
            return strdup("");
        }
        strncpy(command, cmd_text + 2, len);
        command[len] = '\0';
    } else if (cmd_text[0] == '`' && cmd_text[strlen(cmd_text) - 1] == '`') {
        // Extract from `command`
        size_t len = strlen(cmd_text) - 2; // Remove backticks
        command = malloc(len + 1);
        if (!command) {
            return strdup("");
        }
        strncpy(command, cmd_text + 1, len);
        command[len] = '\0';
    } else {
        // Already extracted command
        command = strdup(cmd_text);
        if (!command) {
            return strdup("");
        }
    }

    // Expand variables in the command before executing it
    char *expanded_command = expand_variables_in_string(executor, command);
    free(command);
    if (!expanded_command) {
        return strdup("");
    }
    command = expanded_command;

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
        // Child process - execute command using lusush's own parser/executor
        close(pipefd[0]);               // Close read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[1]);

        // Parse and execute command using lusush's own parser/executor
        // This preserves all function definitions and variables in the child
        parser_t *parser = parser_new(command);
        int result = 127;

        if (parser) {
            node_t *ast = parser_parse(parser);
            if (!parser_has_error(parser) && ast) {
                // Execute in current context (functions are inherited via fork)
                result = execute_node(executor, ast);
                free_node_tree(ast);
            }
            parser_free(parser);
        }

        // Ensure all output is flushed before exit
        fflush(stdout);
        _exit(result);
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

        // Wait for child process to complete first
        int status;
        waitpid(pid, &status, 0);

        // Then read all available output
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            if (output_len + bytes_read >= output_size) {
                output_size *= 2;
                char *new_output = realloc(output, output_size);
                if (!new_output) {
                    free(output);
                    close(pipefd[0]);
                    return strdup("");
                }
                output = new_output;
            }
            memcpy(output + output_len, buffer, bytes_read);
            output_len += bytes_read;
        }

        close(pipefd[0]);

        // Null terminate and remove trailing newlines
        output[output_len] = '\0';
        while (output_len > 0 && (output[output_len - 1] == '\n' ||
                                  output[output_len - 1] == '\r')) {
            output[--output_len] = '\0';
        }

        return output;
    }
}

// Copy function definitions from source executor to destination executor
static void copy_function_definitions(executor_t *dest, executor_t *src) {
    if (!dest || !src) {
        return;
    }

    function_def_t *src_func = src->functions;
    while (src_func) {
        // Create a copy of the function definition
        function_def_t *new_func = malloc(sizeof(function_def_t));
        if (!new_func) {
            break;
        }

        new_func->name = strdup(src_func->name);
        if (!new_func->name) {
            free(new_func);
            break;
        }

        // Create a simple copy of the function body AST
        new_func->body = copy_node_simple(src_func->body);
        if (!new_func->body) {
            free(new_func->name);
            free(new_func);
            break;
        }

        // Add to destination's function list
        new_func->next = dest->functions;
        dest->functions = new_func;

        src_func = src_func->next;
    }
}

// Simple node copying function for function definitions
static node_t *copy_node_simple(node_t *original) {
    if (!original) {
        return NULL;
    }

    node_t *copy = new_node(original->type);
    if (!copy) {
        return NULL;
    }

    copy->val_type = original->val_type;
    copy->val = original->val;

    // If the node has a string value, copy it
    if (original->val_type == VAL_STR && original->val.str) {
        copy->val.str = strdup(original->val.str);
        if (!copy->val.str) {
            free_node_tree(copy);
            return NULL;
        }
    }

    // Copy children recursively
    node_t *child = original->first_child;
    while (child) {
        node_t *child_copy = copy_node_simple(child);
        if (!child_copy) {
            free_node_tree(copy);
            return NULL;
        }
        add_child_node(copy, child_copy);
        child = child->next_sibling;
    }

    return copy;
}

// Expand variables within double-quoted strings
static char *expand_quoted_string(executor_t *executor, const char *str) {
    if (!executor || !str) {
        return strdup("");
    }

    size_t len = strlen(str);
    if (len == 0) {
        return strdup("");
    }

    // Allocate a buffer for expansion (estimate double the original size)
    size_t buffer_size = len * 2 + 256;
    char *result = malloc(buffer_size);
    if (!result) {
        return strdup("");
    }

    size_t result_pos = 0;
    size_t i = 0;

    while (i < len) {
        if (str[i] == '$' && i + 1 < len) {
            // Check for arithmetic expansion $((...))
            if (str[i + 1] == '(' && i + 2 < len && str[i + 2] == '(') {
                // This is arithmetic expansion $((expr))
                size_t arith_start = i;
                size_t arith_end = i + 3;
                int paren_depth = 2;

                while (arith_end < len && paren_depth > 0) {
                    if (str[arith_end] == '(') {
                        paren_depth++;
                    } else if (str[arith_end] == ')') {
                        paren_depth--;
                    }
                    arith_end++;
                }

                if (paren_depth == 0) {
                    // Extract arithmetic expression including $(( and ))
                    size_t full_arith_len = arith_end - arith_start;
                    char *full_arith_expr = malloc(full_arith_len + 1);
                    if (full_arith_expr) {
                        strncpy(full_arith_expr, &str[arith_start],
                                full_arith_len);
                        full_arith_expr[full_arith_len] = '\0';

                        // Expand arithmetic expression
                        char *arith_result =
                            expand_arithmetic(executor, full_arith_expr);
                        if (arith_result) {
                            size_t result_len = strlen(arith_result);
                            // Ensure buffer is large enough
                            while (result_pos + result_len >= buffer_size) {
                                buffer_size *= 2;
                                char *new_result = realloc(result, buffer_size);
                                if (!new_result) {
                                    free(result);
                                    free(arith_result);
                                    free(full_arith_expr);
                                    return strdup("");
                                }
                                result = new_result;
                            }

                            // Copy arithmetic result
                            strcpy(&result[result_pos], arith_result);
                            result_pos += result_len;
                            free(arith_result);
                        }

                        free(full_arith_expr);
                        i = arith_end; // Skip past the closing ))
                        continue;
                    }
                }
            }
            // Check for command substitution $(...)
            else if (str[i + 1] == '(') {
                // Use the robust find_closing_brace function to handle nested
                // quotes
                size_t cmd_start = i;

                // Create a temporary string starting from the '(' to use with
                // find_closing_brace
                char *temp_str =
                    (char *)&str[i + 1]; // Start from the opening parenthesis
                size_t brace_offset = find_closing_brace(temp_str);

                if (brace_offset > 0) {
                    // Found matching closing parenthesis
                    size_t cmd_end =
                        i + 1 + brace_offset; // Points to the closing paren

                    // Extract command substitution including $( and )
                    size_t full_cmd_len = cmd_end - cmd_start + 1;
                    char *full_cmd_expr = malloc(full_cmd_len + 1);
                    if (full_cmd_expr) {
                        strncpy(full_cmd_expr, &str[cmd_start], full_cmd_len);
                        full_cmd_expr[full_cmd_len] = '\0';

                        // Expand command substitution
                        char *cmd_result = expand_command_substitution(
                            executor, full_cmd_expr);
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
                        i = cmd_end + 1; // Skip past the closing )
                        continue;
                    }
                }
            }

            // Variable expansion needed
            size_t var_start = i + 1;
            size_t var_end = var_start;

            // Handle ${var} format
            if (str[var_start] == '{') {

                // Use proper brace matching for nested expressions
                int brace_count = 1;
                var_end = var_start + 1; // Start after opening {

                while (var_end < len && brace_count > 0) {
                    if (str[var_end] == '{') {
                        brace_count++;
                    } else if (str[var_end] == '}') {
                        brace_count--;
                    }
                    var_end++;
                }

                if (brace_count == 0) {
                    var_start++; // Skip opening brace for variable name
                                 // extraction
                    var_end--;   // Point to closing brace
                    // Extract variable name
                    size_t var_name_len = var_end - var_start;
                    char *var_name = malloc(var_name_len + 1);
                    if (var_name) {
                        strncpy(var_name, &str[var_start], var_name_len);
                        var_name[var_name_len] = '\0';
                        // Use parameter expansion to handle operators like =,
                        // :-, etc.
                        char *var_value =
                            parse_parameter_expansion(executor, var_name);

                        if (var_value) {
                            size_t value_len = strlen(var_value);
                            // Ensure buffer is large enough
                            while (result_pos + value_len >= buffer_size) {
                                buffer_size *= 2;
                                result = realloc(result, buffer_size);
                                if (!result) {
                                    free(var_name);
                                    return strdup("");
                                }
                            }
                            strcpy(&result[result_pos], var_value);
                            result_pos += value_len;
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
                // Simple $var format - handle special variables and regular
                // variables
                size_t var_name_len = 0;

                // Check for special single-character variables first
                if (str[var_start] == '?' || str[var_start] == '$' ||
                    str[var_start] == '#' || str[var_start] == '*' ||
                    str[var_start] == '@' || str[var_start] == '!' ||
                    (str[var_start] >= '0' && str[var_start] <= '9')) {
                    var_name_len = 1;
                } else {
                    // Regular variable names (alphanumeric + underscore)
                    while (var_start + var_name_len < len &&
                           (isalnum(str[var_start + var_name_len]) ||
                            str[var_start + var_name_len] == '_')) {
                        var_name_len++;
                    }
                }

                if (var_name_len > 0) {
                    // Create variable expression for expansion
                    char *var_expr = malloc(
                        var_name_len + 2); // +2 for '$' and null terminator
                    if (var_expr) {
                        var_expr[0] = '$';
                        strncpy(&var_expr[1], &str[var_start], var_name_len);
                        var_expr[var_name_len + 1] = '\0';

                        // Use the main variable expansion function
                        char *var_value = expand_variable(executor, var_expr);
                        if (var_value) {
                            size_t value_len = strlen(var_value);
                            // Ensure buffer is large enough
                            while (result_pos + value_len >= buffer_size) {
                                buffer_size *= 2;
                                char *new_result = realloc(result, buffer_size);
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
                        i = var_start + var_name_len;
                    } else {
                        result[result_pos++] = str[i++];
                    }
                } else {
                    result[result_pos++] = str[i++];
                }
            }
        } else if (str[i] == '`') {
            // Handle backtick command substitution
            size_t cmd_start = i;
            size_t cmd_end = i + 1;

            // Find closing backtick
            while (cmd_end < len && str[cmd_end] != '`') {
                if (str[cmd_end] == '\\' && cmd_end + 1 < len) {
                    cmd_end += 2; // Skip escaped character
                } else {
                    cmd_end++;
                }
            }

            if (cmd_end < len && str[cmd_end] == '`') {
                // Found matching closing backtick
                size_t full_cmd_len = cmd_end - cmd_start + 1;
                char *full_cmd_expr = malloc(full_cmd_len + 1);
                if (full_cmd_expr) {
                    strncpy(full_cmd_expr, &str[cmd_start], full_cmd_len);
                    full_cmd_expr[full_cmd_len] = '\0';

                    // Expand command substitution
                    char *cmd_result =
                        expand_command_substitution(executor, full_cmd_expr);
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
                    i = cmd_end + 1; // Skip past the closing backtick
                    continue;
                }
            }

            // If we get here, no matching backtick found, treat as literal
            result[result_pos++] = str[i++];
        } else if (str[i] == '\\' && i + 1 < len) {
            // Handle escape sequences
            char next_char = str[i + 1];
            char escape_char;

            switch (next_char) {
            case 'n':
                escape_char = '\n';
                break;
            case 't':
                escape_char = '\t';
                break;
            case 'r':
                escape_char = '\r';
                break;
            case 'b':
                escape_char = '\b';
                break;
            case 'f':
                escape_char = '\f';
                break;
            case 'v':
                escape_char = '\v';
                break;
            case 'a':
                escape_char = '\a';
                break;
            case '\\':
                escape_char = '\\';
                break;
            case '"':
                escape_char = '"';
                break;
            case '$':
                escape_char = '$';
                break;
            default:
                // Not a recognized escape sequence, keep backslash
                escape_char = '\\';
                break;
            }

            if (next_char == 'n' || next_char == 't' || next_char == 'r' ||
                next_char == 'b' || next_char == 'f' || next_char == 'v' ||
                next_char == 'a' || next_char == '\\' || next_char == '"' ||
                next_char == '$') {
                // Valid escape sequence, skip both backslash and next char
                if (result_pos >= buffer_size - 1) {
                    buffer_size *= 2;
                    result = realloc(result, buffer_size);
                    if (!result) {
                        return strdup("");
                    }
                }
                result[result_pos++] = escape_char;
                i += 2; // Skip both backslash and next character
            } else {
                // Invalid escape sequence, include backslash and next char
                // literally
                if (result_pos >= buffer_size - 2) {
                    buffer_size *= 2;
                    result = realloc(result, buffer_size);
                    if (!result) {
                        return strdup("");
                    }
                }
                result[result_pos++] = '\\';       // Add backslash
                result[result_pos++] = str[i + 1]; // Add following character
                i += 2; // Skip both backslash and next character
            }
        } else {
            // Regular character
            if (result_pos >= buffer_size - 1) {
                buffer_size *= 2;
                result = realloc(result, buffer_size);
                if (!result) {
                    return strdup("");
                }
            }
            result[result_pos++] = str[i++];
        }
    }

    result[result_pos] = '\0';
    return result;
}
// ========== JOB CONTROL IMPLEMENTATION ==========

#include "../include/executor.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Initialize job control in executor
static void initialize_job_control(executor_t *executor) {
    if (!executor) {
        return;
    }

    executor->jobs = NULL;
    executor->next_job_id = 1;
    executor->shell_pgid = getpgrp();
}

// Create a new process structure
static process_t *create_process(pid_t pid, const char *command) {
    process_t *proc = malloc(sizeof(process_t));
    if (!proc) {
        return NULL;
    }

    proc->pid = pid;
    proc->command = command ? strdup(command) : NULL;
    proc->status = 0;
    proc->next = NULL;

    return proc;
}

// Free process list
static void free_process_list(process_t *processes) {
    while (processes) {
        process_t *next = processes->next;
        free(processes->command);
        free(processes);
        processes = next;
    }
}

// Add a new job to the job list
job_t *executor_add_job(executor_t *executor, pid_t pgid,
                        const char *command_line) {
    if (!executor) {
        return NULL;
    }

    job_t *job = malloc(sizeof(job_t));
    if (!job) {
        return NULL;
    }

    job->job_id = executor->next_job_id++;
    job->pgid = pgid;
    job->state = JOB_RUNNING;
    job->foreground = false;
    job->processes = NULL;
    job->command_line = command_line ? strdup(command_line) : NULL;
    job->next = executor->jobs;

    executor->jobs = job;
    return job;
}

// Find job by ID
job_t *executor_find_job(executor_t *executor, int job_id) {
    if (!executor) {
        return NULL;
    }

    job_t *job = executor->jobs;
    while (job) {
        if (job->job_id == job_id) {
            return job;
        }
        job = job->next;
    }
    return NULL;
}

// Remove job from job list
void executor_remove_job(executor_t *executor, int job_id) {
    if (!executor || !executor->jobs) {
        return;
    }

    job_t *job = executor->jobs;
    job_t *prev = NULL;

    while (job) {
        if (job->job_id == job_id) {
            if (prev) {
                prev->next = job->next;
            } else {
                executor->jobs = job->next;
            }

            free_process_list(job->processes);
            free(job->command_line);
            free(job);
            return;
        }
        prev = job;
        job = job->next;
    }
}

// Update job status by checking all processes
void executor_update_job_status(executor_t *executor) {
    if (!executor) {
        return;
    }

    job_t *job = executor->jobs;
    while (job) {
        job_t *next_job = job->next;

        if (job->state == JOB_RUNNING) {
            int status;
            pid_t result = waitpid(-job->pgid, &status, WNOHANG | WUNTRACED);

            if (result > 0) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    job->state = JOB_DONE;
                    printf("[%d]+ Done                    %s\n", job->job_id,
                           job->command_line ? job->command_line : "unknown");
                    executor_remove_job(executor, job->job_id);
                } else if (WIFSTOPPED(status)) {
                    job->state = JOB_STOPPED;
                    printf("[%d]+ Stopped                 %s\n", job->job_id,
                           job->command_line ? job->command_line : "unknown");
                }
            }
        }

        job = next_job;
    }
}

// Execute command in background
int executor_execute_background(executor_t *executor, node_t *command) {
    if (!executor || !command) {
        return 1;
    }

    // Build command line for display
    char *command_line = NULL;
    if (command->first_child && command->first_child->type == NODE_COMMAND) {
        command_line = command->first_child->val.str;
    }

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Failed to fork for background job\n");
        return 1;
    }

    if (pid == 0) {
        // Child process - create new process group
        setpgid(0, 0);

        // Execute the command
        int result = execute_node(executor, command->first_child);
        exit(result);
    } else {
        // Parent process - add to job list
        setpgid(pid, pid); // Set child's process group

        // Store the background PID for $! variable
        extern pid_t last_background_pid;
        last_background_pid = pid;

        job_t *job = executor_add_job(executor, pid, command_line);
        if (job) {
            printf("[%d] %d\n", job->job_id, pid);
        }

        return 0; // Background job started successfully
    }
}

// Built-in jobs command
int executor_builtin_jobs(executor_t *executor, char **argv) {
    if (!executor) {
        return 1;
    }

    // Update job statuses first
    executor_update_job_status(executor);

    job_t *job = executor->jobs;
    while (job) {
        const char *state_str;
        switch (job->state) {
        case JOB_RUNNING:
            state_str = "Running";
            break;
        case JOB_STOPPED:
            state_str = "Stopped";
            break;
        case JOB_DONE:
            state_str = "Done";
            break;
        default:
            state_str = "Unknown";
            break;
        }

        printf("[%d]%c %-20s %s\n", job->job_id, job->foreground ? '+' : '-',
               state_str, job->command_line ? job->command_line : "unknown");

        job = job->next;
    }

    return 0;
}

// Built-in fg command
int executor_builtin_fg(executor_t *executor, char **argv) {
    if (!executor) {
        return 1;
    }

    int job_id = 1; // Default to job 1
    if (argv[1]) {
        job_id = atoi(argv[1]);
    }

    job_t *job = executor_find_job(executor, job_id);
    if (!job) {
        fprintf(stderr, "fg: %d: no such job\n", job_id);
        return 1;
    }

    if (job->state == JOB_DONE) {
        fprintf(stderr, "fg: %d: job has terminated\n", job_id);
        return 1;
    }

    // Continue the job if it was stopped
    if (job->state == JOB_STOPPED) {
        kill(-job->pgid, SIGCONT);
    }

    job->foreground = true;
    job->state = JOB_RUNNING;

    // Wait for the job to complete or stop
    int status;
    waitpid(-job->pgid, &status, WUNTRACED);

    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        executor_remove_job(executor, job_id);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    } else if (WIFSTOPPED(status)) {
        job->state = JOB_STOPPED;
        job->foreground = false;
        printf("[%d]+ Stopped                 %s\n", job_id,
               job->command_line ? job->command_line : "unknown");
    }

    return 0;
}

// Built-in bg command
int executor_builtin_bg(executor_t *executor, char **argv) {
    if (!executor) {
        return 1;
    }

    int job_id = 1; // Default to job 1
    if (argv[1]) {
        job_id = atoi(argv[1]);
    }

    job_t *job = executor_find_job(executor, job_id);
    if (!job) {
        fprintf(stderr, "bg: %d: no such job\n", job_id);
        return 1;
    }

    if (job->state != JOB_STOPPED) {
        fprintf(stderr, "bg: %d: job already in background\n", job_id);
        return 1;
    }

    // Continue the job in background
    job->state = JOB_RUNNING;
    job->foreground = false;
    kill(-job->pgid, SIGCONT);

    printf("[%d]+ %s &\n", job_id,
           job->command_line ? job->command_line : "unknown");

    return 0;
}

// Check if stdout is being captured (piped or redirected to file)
static bool is_stdout_captured(void) {
    struct stat stat_buf;
    if (fstat(STDOUT_FILENO, &stat_buf) == -1) {
        return false;
    }

    // If stdout is not a terminal (tty), it's likely being captured
    return !isatty(STDOUT_FILENO);
}

// Check if command has redirections that affect stdout
static bool has_stdout_redirections(node_t *command) {
    if (!command) {
        return false;
    }

    node_t *child = command->first_child;
    while (child) {
        // Check for stdout-affecting redirections
        if (child->type == NODE_REDIR_OUT ||     // >
            child->type == NODE_REDIR_APPEND ||  // >>
            child->type == NODE_REDIR_BOTH ||    // &>
            child->type == NODE_REDIR_CLOBBER) { // >|
            return true;
        }
        child = child->next_sibling;
    }
    return false;
}

// Execute builtin command with redirections in child process when stdout is
// captured This prevents redirection setup from interfering with the shell's
// captured stdout
static int execute_builtin_with_captured_stdout(executor_t *executor,
                                                char **argv, node_t *command) {
    if (!argv || !argv[0]) {
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        set_executor_error(executor,
                           "Failed to fork for builtin with captured stdout");
        return 1;
    }

    if (pid == 0) {
        // Child process - setup redirections and execute builtin
        int redir_result = setup_redirections(executor, command);
        if (redir_result != 0) {
            exit(1);
        }

        // Execute the builtin command
        int result = execute_builtin_command(executor, argv);
        exit(result);
    } else {
        // Parent process - wait for child
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            set_executor_error(executor,
                               "Failed to wait for builtin child process");
            return 1;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        return 1;
    }
}
