#include "../include/alias.h"
#include "../include/alias_expand.h"
#include "../include/builtins.h"
#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/node.h"
#include "../include/scanner.h"
#include "../include/strings.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Forward declarations
int execute_pipeline_from_node(node_t *node);
int execute_new_parser_command(node_t *cmd);
int execute_new_parser_pipeline(node_t *pipe_node);
static int count_pipeline_commands(node_t *node);
static int extract_pipeline_commands(node_t *node, node_t **commands, int max_commands);
static int extract_pipeline_commands_recursive(node_t *node, node_t **commands, int max_commands, int current_index);
static int execute_new_parser_pipeline_commands(node_t **commands, int cmd_count);
static int execute_compound_list(node_t *list_node);
int execute_new_parser_control_structure(node_t *control_node);

/**
 * execute_new_parser_command: Adapter for new parser's AST structure
 * 
 * The new parser creates a different AST structure than the old parser.
 * This function adapts the new parser's output to work with the existing execution logic.
 * 
 * New parser structure:
 *   NODE_COMMAND (value: "command_name", children: arguments as separate nodes)
 * 
 * Old parser structure expected by execute_command:
 *   NODE_COMMAND (children: command and args as NODE_VAR nodes)
 */
int execute_new_parser_command(node_t *cmd) {
    if (!cmd || cmd->type != NODE_COMMAND) {
        return 0;
    }
    
    // Check if we're in syntax check mode - if so, don't execute
    if (is_syntax_check_mode()) {
        return 0;
    }
    
    // Build argv array from the command node and its children
    size_t argc = 0;
    size_t targc = 32;
    char **argv = calloc(targc, sizeof(char*));
    if (!argv) {
        return 1;
    }
    
    // Add the command name (from the node's value)
    if (cmd->val.str && *cmd->val.str) {
        argv[argc++] = cmd->val.str;
    }
    
    // Add arguments from child nodes
    node_t *child = cmd->first_child;
    while (child) {
        if (child->val.str && *child->val.str) {
            if (argc >= targc - 1) {
                targc *= 2;
                char **new_argv = realloc(argv, targc * sizeof(char*));
                if (!new_argv) {
                    free(argv);
                    return 1;
                }
                argv = new_argv;
            }
            argv[argc++] = child->val.str;
        }
        child = child->next_sibling;
    }
    
    argv[argc] = NULL;
    
    // Check if we have a valid command to execute
    if (argc == 0 || !argv[0] || !*argv[0]) {
        free(argv);
        return 0;
    }
    
    // Trace command execution if -x flag is set
    if (should_trace_execution()) {
        fprintf(stderr, "+");
        for (int i = 0; i < argc; i++) {
            fprintf(stderr, " %s", argv[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    
    // Execute builtin commands
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0) {
            if (setup_redirections(cmd) == -1) {
                free(argv);
                return 0;
            }
            int exit_code = builtins[i].func(argc, argv);
            last_exit_status = exit_code;
            
            // Check if we should exit on error (-e flag)
            if (should_exit_on_error() && exit_code != 0) {
                fprintf(stderr, "lusush: exiting due to error (exit code %d)\n", exit_code);
                exit(exit_code);
            }
            
            free(argv);
            return exit_code;
        }
    }
    
    // Execute external commands
    pid_t child_pid = fork();
    int status = 0;

    if (child_pid == -1) {
        error_return("error: `execute_new_parser_command`");
        free(argv);
        return 0;
    } else if (child_pid == 0) {
        if (setup_redirections(cmd) == -1) {
            exit(EXIT_FAILURE);
        }
        
        do_exec_cmd(argc, argv);
        error_return("error: `execute_new_parser_command`");

        switch (errno) {
        case ENOEXEC:
            exit(126);
            break;
        case ENOENT:
            exit(127);
            break;
        default:
            exit(EXIT_FAILURE);
            break;
        }
    } else {
        waitpid(child_pid, &status, 0);
        int exit_code = WEXITSTATUS(status);
        last_exit_status = exit_code;
        
        // Check if we should exit on error (-e flag)
        if (should_exit_on_error() && exit_code != 0) {
            fprintf(stderr, "lusush: exiting due to error (exit code %d)\n", exit_code);
            exit(exit_code);
        }
        
        free(argv);
        return exit_code;
    }
}

/**
 * search_path:
 *      Search for an executable file in the directories listed in the PATH environment variable
 */
char *search_path(char *fn) {
    char *PATH = getenv("PATH"), *p = PATH, *p2 = NULL;

    while (p && *p) {
        p2 = p;
        while (*p2 && *p2 != ':') {
            p2++;
        }

        size_t plen = p2 - p;

        if (!plen) {
            plen = 1;
        }

        size_t alen = strlen(fn);
        char path[plen + 1 + alen + 1];

        strncpy(path, p, p2 - p);
        path[p2 - p] = '\0';

        if (p2[-1] != '/') {
            strcat(path, "/");
        }

        strcat(path, fn);

        struct stat st;

        if (stat(path, &st) == 0) {
            if (!S_ISREG(st.st_mode)) {
                errno = ENOENT;
                p = p2;
                if (*p2 == ':') {
                    p++;
                }
                continue;
            }

            if (!(p = alloc_str(strlen(path) + 1, false))) {
                return NULL;
            }

            strcpy(p, path);
            return p;
        } else {
            p = p2;
            if (*p2 == ':') {
                p++;
            }
        }
    }

    errno = ENOENT;
    return NULL;
}

/**
 * do_exec_cmd:
 *      Execute a command with arguments
 */
int do_exec_cmd(int argc __attribute__((unused)), char **argv) {
    // Validate input
    if (!argv || !argv[0] || !*argv[0]) {
        error_message("do_exec_cmd: invalid or empty command");
        return 1;
    }

    if (strchr(*argv, '/')) {
        execv(*argv, argv);
    } else {
        char *path = search_path(*argv);

        if (path == NULL) {
            return 1;
        }

        execv(path, argv);
        error_return("error: `do_exec_cmd`");
    }

    return 0;
}

// Removed old do_basic_pipe_list function - replaced with simpler pipeline implementation

/**
 * NEW EXECUTION ARCHITECTURE
 * execute_command: Handle command execution with assignments, pipelines, and arguments
 * This is the main command execution dispatcher that handles:
 * - Variable assignments
 * - Pipeline detection and routing
 * - Multi-token command processing
 * - Both builtin and external command execution
 */
int execute_command(node_t *cmd) {
    if (!cmd || cmd->type != NODE_COMMAND || !cmd->first_child) {
        return 0;
    }
    
    // Check if we're in syntax check mode - if so, don't execute
    if (is_syntax_check_mode()) {
        return 0;
    }
    
    // First check if this command contains pipes - if so, handle as pipeline
    node_t *child = cmd->first_child;
    bool has_pipe = false;
    while (child) {
        if (child->type == NODE_PIPE) {
            has_pipe = true;
            break;
        }
        child = child->next_sibling;
    }
    
    if (has_pipe) {
        // This is a pipeline, not a simple command
        return execute_pipeline_from_node(cmd);
    }
    
    child = cmd->first_child;
    
    // Process variable assignments at the beginning
    while (child && child->val.str && strchr(child->val.str, '=')) {
        char *eq = strchr(child->val.str, '=');
        if (eq > child->val.str) {
            // Check if everything before '=' is a valid identifier
            bool valid_name = true;
            for (char *p = child->val.str; p < eq; p++) {
                if (!isalnum(*p) && *p != '_') {
                    valid_name = false;
                    break;
                }
            }
            if (valid_name && isalpha(child->val.str[0])) {
                // Perform the assignment
                *eq = '\0';  // Split the string
                char *name = child->val.str;
                char *value = eq + 1;
                
                // Process the value through word expansion (including quote removal)
                char *processed_value = word_expand_to_str(value);
                if (!processed_value) {
                    processed_value = strdup(value); // Fallback to original value
                }
                
                symtable_entry_t *entry = add_to_symtable(name);
                if (entry) {
                    symtable_entry_setval(entry, processed_value);
                }
                
                free(processed_value);
                *eq = '=';  // Restore the string
                
                // Move to next child (skip this assignment)
                child = child->next_sibling;
                continue;
            }
        }
        // Not a valid assignment, treat as regular command
        break;
    }
    
    // If we only had assignments and no command, we're done
    if (!child) {
        return 0;
    }
    
    // Build argv array from all remaining children
    size_t argc = 0;
    size_t targc = 32;
    char **argv = calloc(targc, sizeof(char*));
    if (!argv) {
        return 1;
    }
    
    while (child) {
        if (child->type == NODE_VAR && child->val.str) {
            word_t *w = word_expand(child->val.str);
            
            if (w) {
                const word_t *w2 = w;
                while (w2) {
                    if (argc >= targc - 1) {
                        targc *= 2;
                        char **new_argv = realloc(argv, targc * sizeof(char*));
                        if (!new_argv) {
                            free_argv(argc, argv);
                            free_all_words(w);
                            return 1;
                        }
                        argv = new_argv;
                    }
                    
                    char *str = alloc_str(strlen(w2->data) + 1, false);
                    if (str) {
                        strcpy(str, w2->data);
                        argv[argc++] = str;
                    }
                    w2 = w2->next;
                }
                free_all_words(w);
            }
            // Note: NULL return from word_expand is valid for empty unquoted expansions
            // Only return error if there was an actual expansion error (e.g., -u flag violations)
            // The word_expand function handles those errors internally
        }
        child = child->next_sibling;
    }
    
    argv[argc] = NULL;
    
    // Check if we have a valid command to execute
    if (argc == 0 || !argv[0] || !*argv[0]) {
        free_argv(argc, argv);
        return 0;
    }
    
    // Trace command execution if -x flag is set
    if (should_trace_execution()) {
        fprintf(stderr, "+");
        for (int i = 0; i < argc; i++) {
            fprintf(stderr, " %s", argv[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    
    // Execute builtin commands
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0) {
            if (setup_redirections(cmd) == -1) {
                free_argv(argc, argv);
                return 0;
            }
            int exit_code = builtins[i].func(argc, argv);
            last_exit_status = exit_code;
            
            // Check if we should exit on error (-e flag)
            if (should_exit_on_error() && exit_code != 0) {
                fprintf(stderr, "lusush: exiting due to error (exit code %d)\n", exit_code);
                exit(exit_code);
            }
            
            free_argv(argc, argv);
            return exit_code;
        }
    }
    
    // Execute external commands
    pid_t child_pid = fork();
    int status = 0;

    if (child_pid == -1) {
        error_return("error: `execute_command`");
        free_argv(argc, argv);
        return 0;
    } else if (child_pid == 0) {
        if (setup_redirections(cmd) == -1) {
            exit(EXIT_FAILURE);
        }
        
        do_exec_cmd(argc, argv);
        error_return("error: `execute_command`");

        switch (errno) {
        case ENOEXEC:
            exit(126);
            break;
        case ENOENT:
            exit(127);
            break;
        default:
            exit(EXIT_FAILURE);
            break;
        }
    } else {
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status)) {
            last_exit_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            last_exit_status = 128 + WTERMSIG(status);
        } else {
            last_exit_status = 1;
        }
        
        // Check if we should exit on error (-e flag)
        if (should_exit_on_error() && last_exit_status != 0) {
            fprintf(stderr, "lusush: exiting due to error (exit code %d)\n", last_exit_status);
            exit(last_exit_status);
        }
    }

    free_argv(argc, argv);
    return last_exit_status;
}

/**
 * setup_redirection:
 *      Set up a single redirection
 */
int setup_redirection(node_t *redir) {
    if (redir == NULL || redir->first_child == NULL) {
        return -1;
    }

    char *target = redir->first_child->val.str;
    int fd = -1;
    int flags = 0;
    mode_t mode = 0644;

    switch (redir->type) {
        case NODE_REDIR_IN:
            flags = O_RDONLY;
            fd = open(target, flags);
            if (fd >= 0) {
                if (dup2(fd, STDIN_FILENO) == -1) {
                    close(fd);
                    return -1;
                }
                close(fd);
            }
            break;
            
        case NODE_REDIR_OUT:
        case NODE_REDIR_CLOBBER:
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            fd = open(target, flags, mode);
            if (fd >= 0) {
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    close(fd);
                    return -1;
                }
                close(fd);
            }
            break;
            
        case NODE_REDIR_APPEND:
            flags = O_WRONLY | O_CREAT | O_APPEND;
            fd = open(target, flags, mode);
            if (fd >= 0) {
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    close(fd);
                    return -1;
                }
                close(fd);
            }
            break;
            
        case NODE_REDIR_ERR:
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            fd = open(target, flags, mode);
            if (fd >= 0) {
                if (dup2(fd, STDERR_FILENO) == -1) {
                    close(fd);
                    return -1;
                }
                close(fd);
            }
            break;
            
        case NODE_REDIR_ERR_APPEND:
            flags = O_WRONLY | O_CREAT | O_APPEND;
            fd = open(target, flags, mode);
            if (fd >= 0) {
                if (dup2(fd, STDERR_FILENO) == -1) {
                    close(fd);
                    return -1;
                }
                close(fd);
            }
            break;
            
        case NODE_REDIR_HEREDOC:
            // TODO: Implement heredoc support
            fprintf(stderr, "Heredoc redirection not yet implemented\n");
            return -1;
            
        default:
            return -1;
    }

    if (fd == -1) {
        perror(target);
        return -1;
    }

    return 0;
}

/**
 * setup_redirections:
 *      Set up all redirections for a command
 */
int setup_redirections(node_t *cmd) {
    if (cmd == NULL) {
        return 0;
    }

    for (node_t *child = cmd->first_child; child; child = child->next_sibling) {
        if (child->type >= NODE_REDIR_IN && child->type <= NODE_REDIR_CLOBBER) {
            if (setup_redirection(child) == -1) {
                return -1;
            }
        }
    }

    return 0;
}

/**
 * execute_pipeline_simple:
 *      Parse and execute a pipeline from a command line string
 */
int execute_pipeline_simple(char *cmdline) {
    if (!cmdline || !*cmdline) {
        return 0;
    }

    // First, expand any aliases in the first word
    char *expanded_line = NULL;
    char *first_word_end = cmdline;
    while (*first_word_end && !isspace(*first_word_end) && *first_word_end != '|') {
        first_word_end++;
    }
    
    if (first_word_end > cmdline) {
        char *first_word = strndup(cmdline, first_word_end - cmdline);
        if (first_word) {
            char *alias_expansion = expand_first_word_alias(first_word);
            if (alias_expansion && strcmp(alias_expansion, first_word) != 0) {
                // Alias was expanded - create new command line
                size_t new_len = strlen(alias_expansion) + strlen(first_word_end) + 2;
                expanded_line = malloc(new_len);
                if (expanded_line) {
                    snprintf(expanded_line, new_len, "%s%s", alias_expansion, first_word_end);
                    cmdline = expanded_line;
                }
            }
            free(alias_expansion);
            free(first_word);
        }
    }

    // Count pipes to determine number of commands
    int pipe_count = 0;
    char *p = cmdline;
    bool in_quotes = false;
    char quote_char = '\0';
    
    while (*p) {
        if (!in_quotes && (*p == '"' || *p == '\'')) {
            in_quotes = true;
            quote_char = *p;
        } else if (in_quotes && *p == quote_char && (p == cmdline || p[-1] != '\\')) {
            in_quotes = false;
            quote_char = '\0';
        } else if (!in_quotes && *p == '|') {
            pipe_count++;
        }
        p++;
    }
    
    int cmd_count = pipe_count + 1;
    if (cmd_count > 100) { // Reasonable limit
        error_message("Pipeline too long (max 100 commands)");
        free(expanded_line);
        return 0;
    }
    
    // Split command line on pipes
    char **commands = malloc(cmd_count * sizeof(char*));
    if (!commands) {
        free(expanded_line);
        return 0;
    }
    
    char *cmdline_copy = strdup(cmdline);
    if (!cmdline_copy) {
        free(commands);
        free(expanded_line);
        return 0;
    }
    
    char *cmd_start = cmdline_copy;
    int cmd_idx = 0;
    in_quotes = false;
    quote_char = '\0';
    
    p = cmdline_copy;
    while (*p && cmd_idx < cmd_count) {
        if (!in_quotes && (*p == '"' || *p == '\'')) {
            in_quotes = true;
            quote_char = *p;
        } else if (in_quotes && *p == quote_char && (p == cmdline_copy || p[-1] != '\\')) {
            in_quotes = false;
            quote_char = '\0';
        } else if (!in_quotes && *p == '|') {
            *p = '\0';
            char *trimmed = str_strip_whitespace(cmd_start);
            commands[cmd_idx] = strdup(trimmed ? trimmed : "");
            cmd_idx++;
            cmd_start = p + 1;
        }
        p++;
    }
    // Add the last command
    if (cmd_idx < cmd_count) {
        char *trimmed = str_strip_whitespace(cmd_start);
        commands[cmd_idx] = strdup(trimmed ? trimmed : "");
    }
    
    // Execute pipeline
    int result = execute_simple_pipeline(commands, cmd_count);
    
    // Cleanup
    for (int i = 0; i < cmd_count; i++) {
        free(commands[i]);
    }
    free(commands);
    free(cmdline_copy);
    free(expanded_line);
    
    return result;
}

/**
 * execute_simple_pipeline:
 *      Execute a simple pipeline of commands with proper word expansion
 */
int execute_simple_pipeline(char **commands, int cmd_count) {
    if (!commands || cmd_count <= 0) {
        return 0;
    }
    
    // Special case: single command (no pipeline)
    if (cmd_count == 1) {
        return execute_single_command(commands[0]);
    }
    
    // Parse each command into argc/argv with word expansion
    char ***cmd_args = calloc(cmd_count, sizeof(char**));
    int *cmd_argc = calloc(cmd_count, sizeof(int));
    if (!cmd_args || !cmd_argc) {
        free(cmd_args);
        free(cmd_argc);
        return 0;
    }
    
    for (int i = 0; i < cmd_count; i++) {
        if (!commands[i] || !*commands[i]) {
            cmd_argc[i] = 0;
            cmd_args[i] = NULL;
            continue;
        }
        
        // Use word expansion to properly parse the command
        word_t *words = word_expand(commands[i]);
        if (!words) {
            cmd_argc[i] = 0;
            cmd_args[i] = NULL;
            continue;
        }
        
        // Count words and build argv
        size_t argc = 0, targc = 0;
        char **argv = NULL;
        
        const word_t *w = words;
        while (w) {
            if (w->data && *w->data) { // Skip empty words
                if (check_buffer_bounds(&argc, &targc, &argv)) {
                    argv[argc] = strdup(w->data);
                    argc++;
                }
            }
            w = w->next;
        }
        
        if (check_buffer_bounds(&argc, &targc, &argv)) {
            argv[argc] = NULL;
        }
        
        cmd_args[i] = argv;
        cmd_argc[i] = argc;
        free_all_words(words);
    }
    
    // Execute the pipeline
    int result = execute_pipeline_commands(cmd_args, cmd_argc, cmd_count);
    
    // Cleanup
    for (int i = 0; i < cmd_count; i++) {
        if (cmd_args[i]) {
            free_argv(cmd_argc[i], cmd_args[i]);
        }
    }
    free(cmd_args);
    free(cmd_argc);
    
    return result;
}

/**
 * execute_single_command:
 *      Execute a single command (no pipeline) with full word expansion
 */
int execute_single_command(char *command) {
    if (!command || !*command) {
        return 0;
    }
    
    // Expand the command using our word expansion system
    word_t *words = word_expand(command);
    if (!words) {
        return 0;
    }
    
    // Build argc/argv
    size_t argc = 0, targc = 0;
    char **argv = NULL;
    
    const word_t *w = words;
    while (w) {
        if (w->data && *w->data) { // Skip empty words
            if (check_buffer_bounds(&argc, &targc, &argv)) {
                argv[argc] = strdup(w->data);
                argc++;
            }
        }
        w = w->next;
    }
    
    if (check_buffer_bounds(&argc, &targc, &argv)) {
        argv[argc] = NULL;
    }
    
    free_all_words(words);
    
    if (argc == 0) {
        free_argv(argc, argv);
        return 0;
    }
    
    // Check for builtin command
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0) {
            int exit_code = builtins[i].func(argc, argv);
            last_exit_status = exit_code;
            free_argv(argc, argv);
            return exit_code;
        }
    }
    
    // Execute external command
    pid_t child_pid = fork();
    int status = 0;

    if (child_pid == -1) {
        error_return("error: fork failed");
        free_argv(argc, argv);
        return 0;
    } else if (child_pid == 0) {
        // Child process
        do_exec_cmd(argc, argv);
        error_return("error: exec failed");

        switch (errno) {
        case ENOEXEC:
            exit(126);
            break;
        case ENOENT:
            exit(127);
            break;
        default:
            exit(EXIT_FAILURE);
            break;
        }
    } else {
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status)) {
            last_exit_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            last_exit_status = 128 + WTERMSIG(status);
        } else {
            last_exit_status = 1;
        }
    }

    free_argv(argc, argv);
    return 1;
}

/**
 * execute_pipeline_commands:
 *      Execute the actual pipeline with proper process management
 */
int execute_pipeline_commands(char ***cmd_args, int *cmd_argc, int cmd_count) {
    if (cmd_count <= 0) return 0;
    
    // Check for empty commands
    for (int i = 0; i < cmd_count; i++) {
        if (cmd_argc[i] == 0 || !cmd_args[i] || !cmd_args[i][0]) {
            error_message("Empty command in pipeline");
            // In syntax check mode, return proper error code for syntax errors
            return is_syntax_check_mode() ? 2 : 1;
        }
    }
    
    // Special case: pipeline with builtins is tricky, only allow if single command
    if (cmd_count == 1) {
        // Check if it's a builtin
        for (size_t i = 0; i < builtins_count; i++) {
            if (strcmp(cmd_args[0][0], builtins[i].name) == 0) {
                int exit_code = builtins[i].func(cmd_argc[0], cmd_args[0]);
                last_exit_status = exit_code;
                return exit_code;
            }
        }
    } else {
        // For multi-command pipelines, warn about builtins
        for (int i = 0; i < cmd_count; i++) {
            for (size_t j = 0; j < builtins_count; j++) {
                if (strcmp(cmd_args[i][0], builtins[j].name) == 0) {
                    error_message("Warning: builtin '%s' in pipeline may not work as expected", 
                                cmd_args[i][0]);
                }
            }
        }
    }
    
    // Create pipes
    int pipes[cmd_count - 1][2];
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            // Close any pipes we already created
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return 0;
        }
    }
    
    // Execute commands
    pid_t *pids = calloc(cmd_count, sizeof(pid_t));
    if (!pids) {
        // Close all pipes
        for (int i = 0; i < cmd_count - 1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        return 0;
    }
    
    for (int i = 0; i < cmd_count; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            pids[i] = -1; // Mark as failed
            continue;
        } else if (pid == 0) {
            // Child process
            
            // Set up input redirection
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2 input");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Set up output redirection
            if (i < cmd_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 output");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Close all pipe fds in child
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute command
            do_exec_cmd(cmd_argc[i], cmd_args[i]);
            
            // If we reach here, exec failed
            switch (errno) {
            case ENOEXEC:
                exit(126);
                break;
            case ENOENT:
                exit(127);
                break;
            default:
                exit(EXIT_FAILURE);
                break;
            }
        } else {
            pids[i] = pid;
        }
    }
    
    // Close pipes in parent
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for children and collect exit status from last command
    int final_status = 0;
    for (int i = 0; i < cmd_count; i++) {
        if (pids[i] > 0) {
            int status;
            if (waitpid(pids[i], &status, 0) == -1) {
                perror("waitpid");
                continue;
            }
            
            // Use exit status of last command in pipeline
            if (i == cmd_count - 1) {
                if (WIFEXITED(status)) {
                    final_status = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    final_status = 128 + WTERMSIG(status);
                } else {
                    final_status = 1;
                }
            }
        }
    }
    
    last_exit_status = final_status;
    free(pids);
    return last_exit_status;
}

/**
 * execute_pipeline_from_node: Execute pipeline from old parser node structure
 * 
 * This is a compatibility function for the old parser's pipeline nodes.
 * For the migration, this delegates to the new parser pipeline execution.
 */
int execute_pipeline_from_node(node_t *node) {
    if (!node || node->type != NODE_PIPE) {
        return 0;
    }
    
    // Delegate to new parser pipeline execution
    return execute_new_parser_pipeline(node);
}

// ============================================================================
// PIPELINE HELPER FUNCTIONS FOR NEW PARSER
// ============================================================================

/**
 * Count the number of commands in a pipeline AST
 */
static int count_pipeline_commands(node_t *node) {
    if (!node) {
        return 0;
    }
    
    if (node->type == NODE_PIPE) {
        // Pipeline node has two children: left and right commands
        node_t *left = node->first_child;
        node_t *right = left ? left->next_sibling : NULL;
        
        return count_pipeline_commands(left) + count_pipeline_commands(right);
    } else {
        // This is a command node, count as 1
        return 1;
    }
}

/**
 * Extract commands from pipeline AST into a linear array
 */
static int extract_pipeline_commands(node_t *node, node_t **commands, int max_commands) {
    return extract_pipeline_commands_recursive(node, commands, max_commands, 0);
}

/**
 * Recursively extract commands from pipeline AST
 */
static int extract_pipeline_commands_recursive(node_t *node, node_t **commands, int max_commands, int current_index) {
    if (!node || current_index >= max_commands) {
        return current_index;
    }
    
    if (node->type == NODE_PIPE) {
        // Pipeline node has two children: left and right commands
        node_t *left = node->first_child;
        node_t *right = left ? left->next_sibling : NULL;
        
        // Extract from left side first, then right side
        current_index = extract_pipeline_commands_recursive(left, commands, max_commands, current_index);
        current_index = extract_pipeline_commands_recursive(right, commands, max_commands, current_index);
    } else {
        // This is a command node, add it to the array
        commands[current_index] = node;
        current_index++;
    }
    
    return current_index;
}

/**
 * Execute a linear array of pipeline commands
 */
static int execute_new_parser_pipeline_commands(node_t **commands, int cmd_count) {
    if (!commands || cmd_count <= 0) {
        return 0;
    }
    
    // For a single command, just execute it directly
    if (cmd_count == 1) {
        return execute_new_parser_command(commands[0]);
    }
    
    // For multiple commands, set up pipes and execute
    int pipes[cmd_count - 1][2];  // One pipe between each pair of commands
    pid_t pids[cmd_count];
    int last_exit_status = 0;
    
    // Create all pipes
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    
    // Fork and execute each command
    for (int i = 0; i < cmd_count; i++) {
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("fork");
            return 1;
        } else if (pids[i] == 0) {
            // Child process
            
            // Set up input redirection (except for first command)
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Set up output redirection (except for last command)
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe file descriptors
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            // Convert node to argv format for execution
            if (commands[i] && commands[i]->type == NODE_COMMAND) {
                // Build argv array
                size_t argc = 0;
                size_t targc = 32;
                char **argv = calloc(targc, sizeof(char*));
                if (!argv) {
                    exit(1);
                }
                
                // Add command name
                if (commands[i]->val.str && *commands[i]->val.str) {
                    argv[argc++] = commands[i]->val.str;
                }
                
                // Add arguments
                node_t *child = commands[i]->first_child;
                while (child && argc < targc - 1) {
                    if (child->val.str && *child->val.str) {
                        argv[argc++] = child->val.str;
                    }
                    child = child->next_sibling;
                }
                argv[argc] = NULL;
                
                // Execute
                if (argc > 0) {
                    execvp(argv[0], argv);
                    perror(argv[0]);
                }
                free(argv);
            }
            exit(1);
        }
    }
    
    // Parent process: close all pipes and wait for children
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children and get exit status of last command
    for (int i = 0; i < cmd_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == cmd_count - 1) {  // Last command determines exit status
            last_exit_status = WEXITSTATUS(status);
        }
    }
    
    return last_exit_status;
}

/**
 * execute_new_parser_pipeline: Execute pipeline from new parser AST
 * 
 * The new parser creates pipeline nodes with the structure:
 *   NODE_PIPE (left_child: cmd1, right_child: cmd2)
 * 
 * For multi-command pipelines, they are left-associative:
 *   cmd1 | cmd2 | cmd3 becomes:
 *   NODE_PIPE(NODE_PIPE(cmd1, cmd2), cmd3)
 */
int execute_new_parser_pipeline(node_t *pipe_node) {
    if (!pipe_node || pipe_node->type != NODE_PIPE) {
        return 0;
    }
    
    // Check if we're in syntax check mode - if so, don't execute
    if (is_syntax_check_mode()) {
        return 0;
    }
    
    // Convert the pipeline AST to a linear array of commands
    // First, count the commands in the pipeline
    int cmd_count = count_pipeline_commands(pipe_node);
    if (cmd_count <= 0) {
        return 0;
    }
    
    // Allocate array for commands
    node_t **commands = malloc(cmd_count * sizeof(node_t*));
    if (!commands) {
        return 1;
    }
    
    // Extract commands from pipeline AST into linear array
    extract_pipeline_commands(pipe_node, commands, cmd_count);
    
    // Execute the pipeline
    int result = execute_new_parser_pipeline_commands(commands, cmd_count);
    
    free(commands);
    return result;
}

// ============================================================================
// NEW PARSER EXECUTION ADAPTERS
// ============================================================================
/**
 * execute_new_parser_control_structure: Main adapter for control structure execution
 */
int execute_new_parser_control_structure(node_t *control_node) {
    if (!control_node) {
        return 0;
    }
    
    // Check if we're in syntax check mode - if so, don't execute
    if (is_syntax_check_mode()) {
        return 0;
    }
    
    switch (control_node->type) {
        case NODE_IF:
            return execute_new_parser_if(control_node);
        case NODE_WHILE:
            return execute_new_parser_while(control_node);
        case NODE_FOR:
            return execute_new_parser_for(control_node);
        case NODE_UNTIL:
            // Until is essentially a while with negated condition
            // For now, fall back to old parser execution
            return execute_node(control_node);
        case NODE_CASE:
            // Case is complex, fall back to old parser execution for now
            return execute_node(control_node);
        default:
            // Unknown control structure, try old execution
            return execute_node(control_node);
    }
}

/**
 * execute_new_parser_if: Execute if statement from new parser AST
 * 
 * AST structure:
 *   NODE_IF (child[0]: condition, child[1]: then_body, child[2]: else_body)
 */
int execute_new_parser_if(node_t *if_node) {
    if (!if_node || if_node->type != NODE_IF) {
        return 0;
    }
    
    // Get children
    node_t *condition = if_node->first_child;
    node_t *then_body = condition ? condition->next_sibling : NULL;
    node_t *else_body = then_body ? then_body->next_sibling : NULL;
    
    if (!condition || !then_body) {
        return 1; // Malformed if statement
    }
    
    // Execute condition
    int condition_result = execute_compound_list(condition);
    
    if (getenv("NEW_PARSER_DEBUG")) {
        fprintf(stderr, "DEBUG: if condition result: %d\n", condition_result);
    }
    
    // Execute appropriate body based on condition result
    if (condition_result == 0) { // Success (true in shell logic)
        if (getenv("NEW_PARSER_DEBUG")) {
            fprintf(stderr, "DEBUG: executing then body\n");
        }
        return execute_compound_list(then_body);
    } else if (else_body) {
        if (getenv("NEW_PARSER_DEBUG")) {
            fprintf(stderr, "DEBUG: executing else body\n");
        }
        return execute_compound_list(else_body);
    }
    
    return 0; // No else clause, successful completion
}

/**
 * execute_new_parser_while: Execute while loop from new parser AST
 * 
 * AST structure:
 *   NODE_WHILE (child[0]: condition, child[1]: body)
 */
int execute_new_parser_while(node_t *while_node) {
    if (!while_node || while_node->type != NODE_WHILE) {
        return 0;
    }
    
    // Get children
    node_t *condition = while_node->first_child;
    node_t *body = condition ? condition->next_sibling : NULL;
    
    if (!condition || !body) {
        return 1; // Malformed while statement
    }
    
    int last_result = 0;
    
    // Execute loop
    while (true) {
        // Execute condition
        int condition_result = execute_compound_list(condition);
        
        // If condition fails (non-zero), exit loop
        if (condition_result != 0) {
            break;
        }
        
        // Execute body
        last_result = execute_compound_list(body);
        
        // Check for break/continue signals (these would be handled by signal handling)
        // For now, just continue the loop
    }
    
    return last_result;
}

/**
 * execute_new_parser_for: Execute for loop from new parser AST
 * 
 * AST structure:
 *   NODE_FOR (val.str: variable_name, child[0]: word_list or NULL, child[1]: body)
 */
int execute_new_parser_for(node_t *for_node) {
    if (!for_node || for_node->type != NODE_FOR) {
        return 0;
    }
    
    // Get variable name from node value
    const char *var_name = for_node->val.str;
    if (!var_name) {
        return 1; // No variable name
    }
    
    // Get children
    node_t *word_list = for_node->first_child;
    node_t *body = NULL;
    
    // Determine if we have a word list or just a body
    if (word_list && word_list->next_sibling) {
        // We have both word_list and body
        body = word_list->next_sibling;
    } else if (word_list) {
        // Only one child - could be word_list or body
        // If it has children, it's likely a word_list, otherwise it's the body
        if (word_list->first_child) {
            body = word_list->next_sibling; // This might be NULL
        } else {
            // Single child with no children - this is the body
            body = word_list;
            word_list = NULL;
        }
    }
    
    if (!body) {
        return 1; // No body to execute
    }
    
    int last_result = 0;
    
    if (word_list && word_list->first_child) {
        // Iterate over word list
        node_t *word_node = word_list->first_child;
        while (word_node) {
            if (word_node->val.str) {
                // Set loop variable
                setenv(var_name, word_node->val.str, 1);
                
                // Execute body
                last_result = execute_compound_list(body);
            }
            word_node = word_node->next_sibling;
        }
    } else {
        // No word list provided - iterate over positional parameters ($@)
        // For now, this is a simplified implementation
        // A full implementation would need to access $@ from the shell context
        char *argv_str = getenv("LUSUSH_ARGV");
        if (argv_str) {
            // Simple implementation: split on spaces and iterate
            char *argv_copy = strdup(argv_str);
            if (argv_copy) {
                char *token = strtok(argv_copy, " ");
                while (token) {
                    setenv(var_name, token, 1);
                    last_result = execute_compound_list(body);
                    token = strtok(NULL, " ");
                }
                free(argv_copy);
            }
        }
    }
    
    return last_result;
}

/**
 * execute_node: General node execution dispatcher
 * 
 * This function dispatches execution to the appropriate handler based on node type.
 * This is the main execution entry point for the old parser compatibility.
 */
int execute_node(node_t *node) {
    if (!node) {
        return 0;
    }
    
    switch (node->type) {
        case NODE_COMMAND:
            return execute_command(node);
        case NODE_PIPE:
            return execute_pipeline_from_node(node);
        case NODE_IF:
        case NODE_WHILE:
        case NODE_FOR:
        case NODE_UNTIL:
        case NODE_CASE:
            // Control structures - for now, return success
            // In a full implementation, these would be handled by specific functions
            // For the migration, these should go through the new parser
            return 0;
        case NODE_VAR:
            // Individual variable nodes might need special handling
            return 0;
        default:
            // For unknown node types, try the command execution as fallback
            return execute_command(node);
    }
}

/**
 * execute_compound_list: Execute a compound list (sequence of commands)
 * 
 * A compound list can be a single command or a sequence of commands
 * separated by semicolons, &&, ||, etc.
 */
static int execute_compound_list(node_t *list_node) {
    if (!list_node) {
        return 0;
    }
    
    // Debug: Show what type of node we're trying to execute
    if (getenv("NEW_PARSER_DEBUG")) {
        fprintf(stderr, "DEBUG: execute_compound_list - node type: %d\n", list_node->type);
        if (list_node->val.str) {
            fprintf(stderr, "DEBUG: execute_compound_list - node value: '%s'\n", list_node->val.str);
        }
    }
    
    // If it's a simple command, execute it directly
    if (list_node->type == NODE_COMMAND) {
        // Check if this is actually a command or a separator
        if (list_node->val.str && 
            (strcmp(list_node->val.str, ";") == 0 || 
             strcmp(list_node->val.str, "&&") == 0 || 
             strcmp(list_node->val.str, "||") == 0)) {
            // This is a separator node, execute its children
            if (list_node->first_child) {
                int last_result = 0;
                node_t *child = list_node->first_child;
                
                while (child) {
                    // Only execute actual command nodes, not separators
                    if (child->type == NODE_COMMAND && child->val.str && 
                        !(strcmp(child->val.str, ";") == 0 || 
                          strcmp(child->val.str, "&&") == 0 || 
                          strcmp(child->val.str, "||") == 0)) {
                        last_result = execute_compound_list(child);
                    } else {
                        // Recursively handle nested structures
                        last_result = execute_compound_list(child);
                    }
                    child = child->next_sibling;
                }
                
                return last_result;
            }
            return 0;
        } else {
            // This is a real command
            return execute_new_parser_command(list_node);
        }
    }
    
    // If it's a pipeline, execute it
    if (list_node->type == NODE_PIPE) {
        return execute_new_parser_pipeline(list_node);
    }
    
    // For compound lists that contain multiple commands, 
    // execute each child command in sequence
    if (list_node->first_child) {
        int last_result = 0;
        node_t *child = list_node->first_child;
        
        while (child) {
            // Recursively execute each child command
            if (getenv("NEW_PARSER_DEBUG")) {
                fprintf(stderr, "DEBUG: execute_compound_list - executing child type: %d\n", child->type);
            }
            last_result = execute_compound_list(child);
            child = child->next_sibling;
        }
        
        return last_result;
    }
    
    // For other types, fall back to the old execution system
    if (getenv("NEW_PARSER_DEBUG")) {
        fprintf(stderr, "DEBUG: execute_compound_list - no children, returning 0\n");
    }
    return 0;
}

// ============================================================================