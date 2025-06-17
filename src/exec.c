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
            return 1;
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
 * parse_pipeline_simple:
 *      Simple pipeline parser that works with token lists
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
    while (*p) {
        if (*p == '|') pipe_count++;
        p++;
    }
    
    int cmd_count = pipe_count + 1;
    
    // Split command line on pipes
    char **commands = malloc(cmd_count * sizeof(char*));
    if (!commands) {
        free(expanded_line);
        return 0;
    }
    
    char *cmdline_copy = strdup(cmdline);
    char *cmd_start = cmdline_copy;
    int cmd_idx = 0;
    
    p = cmdline_copy;
    while (*p && cmd_idx < cmd_count) {
        if (*p == '|') {
            *p = '\0';
            char *trimmed = str_strip_whitespace(cmd_start);
            commands[cmd_idx] = strdup(trimmed);
            cmd_idx++;
            cmd_start = p + 1;
        }
        p++;
    }
    // Add the last command
    if (cmd_idx < cmd_count) {
        char *trimmed = str_strip_whitespace(cmd_start);
        commands[cmd_idx] = strdup(trimmed);
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
 *      Execute a simple pipeline of commands
 */
int execute_simple_pipeline(char **commands, int cmd_count) {
    if (!commands || cmd_count <= 0) {
        return 0;
    }
    
    // Parse each command into argc/argv
    char ***cmd_args = calloc(cmd_count, sizeof(char**));
    int *cmd_argc = calloc(cmd_count, sizeof(int));
    if (!cmd_args || !cmd_argc) {
        free(cmd_args);
        free(cmd_argc);
        return 0;
    }
    
    for (int i = 0; i < cmd_count; i++) {
        // Simple tokenization by whitespace
        size_t argc = 0, targc = 0;
        char **argv = NULL;
        
        char *cmd_copy = strdup(commands[i]);
        char *token = strtok(cmd_copy, " \t\n");
        
        while (token) {
            if (check_buffer_bounds(&argc, &targc, &argv)) {
                argv[argc] = strdup(token);
                argc++;
            }
            token = strtok(NULL, " \t\n");
        }
        
        if (check_buffer_bounds(&argc, &targc, &argv)) {
            argv[argc] = NULL;
        }
        
        cmd_args[i] = argv;
        cmd_argc[i] = argc;
        free(cmd_copy);
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
 * execute_pipeline_commands:
 *      Execute the actual pipeline
 */
int execute_pipeline_commands(char ***cmd_args, int *cmd_argc, int cmd_count) {
    if (cmd_count <= 0) return 0;
    
    // Create pipes
    int pipes[cmd_count - 1][2];
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 0;
        }
    }
    
    // Execute commands
    pid_t *pids = calloc(cmd_count, sizeof(pid_t));
    if (!pids) {
        return 0;
    }
    
    for (int i = 0; i < cmd_count; i++) {
        if (cmd_argc[i] == 0) continue;
        
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            continue;
        } else if (pid == 0) {
            // Child process
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe fds
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute command
            do_exec_cmd(cmd_argc[i], cmd_args[i]);
            exit(127);
        } else {
            pids[i] = pid;
        }
    }
    
    // Close pipes in parent
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for children
    for (int i = 0; i < cmd_count; i++) {
        if (pids[i] > 0) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    }
    
    free(pids);
    return 1;
}
