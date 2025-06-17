#include "../include/alias.h"
#include "../include/alias_expand.h"
#include "../include/builtins.h"
#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/node.h"
#include "../include/scanner.h"
#include "../include/strings.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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

int do_exec_cmd(int argc __attribute__((unused)), char **argv) {
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

int do_basic_command(node_t *n) {
    size_t argc = 0, targc = 0;
    char **argv = NULL, *str = NULL;

    if (n == NULL) {
        return 0;
    }

    node_t *child = n->first_child;

    if (child == NULL) {
        return 0;
    }

    // Set a flag if performing a variable assignment word expansion first
    bool var_assignment = false;
    if (child->val.str[0] == '$' && strchr(child->val.str, '=')) {
        var_assignment = true;
    }

    // Build argv array from variable nodes only
    while (child) {
        if (child->type == NODE_VAR) {
            str = child->val.str;

            word_t *w = word_expand(str);

            if (w == NULL) {
                child = child->next_sibling;
                continue;
            }

            const word_t *w2 = w;
            while (w2) {
                if (check_buffer_bounds(&argc, &targc, &argv)) {
                    str = alloc_str(strlen(w2->data) + 1, false);
                    if (str) {
                        strcpy(str, w2->data);
                        argv[argc++] = str;
                    }
                }
                w2 = w2->next;
            }

            free_all_words(w);
        }
        child = child->next_sibling;
    }

    if (check_buffer_bounds(&argc, &targc, &argv)) {
        argv[argc] = NULL;
    }

    // If var_assignment flag is set don't execute the command
    if (var_assignment) {
        free_argv(argc, argv);
        return 1;
    }

    // Execute a builtin command
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(*argv, builtins[i].name) == 0) {
            // Set up redirections for builtins too
            if (setup_redirections(n) == -1) {
                free_argv(argc, argv);
                return 0;
            }
            int exit_code = builtins[i].func(argc, argv);
            last_exit_status = exit_code;
            free_argv(argc, argv);
            return exit_code;
        }
    }

    pid_t child_pid = fork();
    int status = 0;

    if (child_pid == -1) {
        error_return("error: `do_basic_command`");
        free_argv(argc, argv);
        return 0;
    } else if (child_pid == 0) {
        // Set up redirections in child process
        if (setup_redirections(n) == -1) {
            exit(EXIT_FAILURE);
        }
        
        do_exec_cmd(argc, argv);
        error_return("error: `do_basic_command`");

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
            return 0;
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
    return 1;
}

/**
 * do_if_clause:
 *      Execute an if statement
 */
/**
 * do_if_clause:
 *      Execute an if/elif/else/fi statement
 *      
 *      The if node structure is:
 *      - condition1, then_body1, [condition2, then_body2, ...], [else_body]
 *      - Each condition/then pair represents if/elif clauses
 *      - The last child (if odd number of children) is the else body
 */
int do_if_clause(node_t *node) {
    if (!node || node->type != NODE_IF) {
        return 0;
    }
    
    node_t *child = node->first_child;
    if (!child) {
        return 0;
    }
    
    // Count children to determine structure
    int child_count = 0;
    for (node_t *c = child; c; c = c->next_sibling) {
        child_count++;
    }
    
    // Process condition/then pairs
    node_t *current = child;
    while (current && current->next_sibling) {
        // Execute condition
        node_t *condition = current;
        node_t *then_body = current->next_sibling;
        
        int condition_result = 0;
        if (condition->type == NODE_COMMAND) {
            condition_result = do_basic_command(condition);
        } else {
            condition_result = do_basic_command(condition);
        }
        
        // If condition succeeded, execute then body and exit
        if (condition_result == 0) {
            int result = 0;
            if (then_body->type == NODE_COMMAND) {
                result = do_basic_command(then_body);
            } else {
                result = do_basic_command(then_body);
            }
            return result;
        }
        
        // Move to next condition/then pair (skip both condition and then)
        current = then_body->next_sibling;
        if (current && current->next_sibling) {
            // This is another condition/then pair
            continue;
        } else if (current) {
            // This is the else body (odd number of children)
            int result = 0;
            if (current->type == NODE_COMMAND) {
                result = do_basic_command(current);
            } else {
                result = do_basic_command(current);
            }
            return result;
        }
        
        // No more conditions and no else body
        break;
    }
    
    // No condition succeeded and no else clause
    return 0;
}

/**
 * Stub implementations for control structures (to be implemented later)
 */
int do_for_loop(node_t *node) {
    (void)node;
    error_message("for loops not yet implemented");
    return 0;
}

int do_while_loop(node_t *node) {
    (void)node;
    error_message("while loops not yet implemented");
    return 0;
}

int do_until_loop(node_t *node) {
    (void)node;
    error_message("until loops not yet implemented");
    return 0;
}

int do_case_clause(node_t *node) {
    (void)node;
    error_message("case statements not yet implemented");
    return 0;
}

int do_function_def(node_t *node) {
    (void)node;
    error_message("function definitions not yet implemented");
    return 0;
}

/**
 * execute_node:
 *      Main execution dispatcher for different node types
 */
int execute_node(node_t *node) {
    if (!node) {
        return 0;
    }
    
    switch (node->type) {
        case NODE_IF:
            return do_if_clause(node);
        case NODE_FOR:
            return do_for_loop(node);
        case NODE_WHILE:
            return do_while_loop(node);
        case NODE_UNTIL:
            return do_until_loop(node);
        case NODE_CASE:
            return do_case_clause(node);
        case NODE_FUNCTION:
            return do_function_def(node);
        case NODE_COMMAND:
        case NODE_VAR:
        default:
            return do_basic_command(node);
    }
}
