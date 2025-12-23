/**
 * I/O Redirection Implementation for Lusush Shell
 *
 * Comprehensive implementation of POSIX shell I/O redirection including:
 * - Basic output redirection (>)
 * - Append redirection (>>)
 * - Input redirection (<)
 * - Error redirection (2>, 2>>)
 * - Combined redirection (&>)
 * - Here strings (<<<)
 * - Here documents (<<, <<-)
 */

#include "../include/redirection.h"

#include "../include/executor.h"
#include "../include/lusush.h"
#include "../include/node.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Forward declarations
static int handle_redirection_node(executor_t *executor, node_t *redir_node);
static int setup_here_document(const char *delimiter, bool strip_tabs);
static int setup_here_document_with_content(const char *content);

// External function from executor.c
extern bool is_privileged_redirection_allowed(const char *target);
static int setup_here_document_with_processing(executor_t *executor,
                                               const char *content,
                                               bool strip_tabs,
                                               bool expand_vars);
static int setup_here_string(executor_t *executor, const char *content);
static char *expand_redirection_target(executor_t *executor,
                                       const char *target);

// External function from executor_modern.c
extern char *expand_if_needed(executor_t *executor, const char *text);

// Forward declaration for file descriptor redirection
static int setup_fd_redirection(const char *redir_text);

// Setup redirections for a command
int setup_redirections(executor_t *executor, node_t *command) {
    if (!command) {
        return 0; // No redirections to setup
    }

    // POSIX compliant: Process redirections left-to-right in order they appear
    node_t *child = command->first_child;
    while (child) {
        if (child->type >= NODE_REDIR_IN && child->type <= NODE_REDIR_CLOBBER) {
            int result = handle_redirection_node(executor, child);
            if (result != 0) {
                return result;
            }
        }
        child = child->next_sibling;
    }

    return 0;
}

// Handle individual redirection node
static int handle_redirection_node(executor_t *executor, node_t *redir_node) {
    if (!redir_node) {
        return 1;
    }

    if (getenv("LUSUSH_DEBUG_REDIR")) {
        printf("DEBUG: handle_redirection_node called with type %d\n",
               redir_node->type);
    }

    // For here documents, check if we have pre-collected content
    if (redir_node->type == NODE_REDIR_HEREDOC ||
        redir_node->type == NODE_REDIR_HEREDOC_STRIP) {
        // The delimiter is stored in the redirection node value
        // The content is stored in the first child node
        // The expand variables flag is stored in the second child node
        node_t *content_node = redir_node->first_child;
        node_t *expand_flag_node =
            content_node ? content_node->next_sibling : NULL;

        if (content_node && content_node->val.str) {
            bool strip_tabs = (redir_node->type == NODE_REDIR_HEREDOC_STRIP);
            bool expand_vars = true; // Default to expand

            // Check expand variables flag from parser
            if (expand_flag_node && expand_flag_node->val.str) {
                expand_vars = (strcmp(expand_flag_node->val.str, "1") == 0);
            }

            return setup_here_document_with_processing(
                executor, content_node->val.str, strip_tabs, expand_vars);
        }
        // If no content node, fall back to interactive here document
        if (redir_node->val.str) {
            return setup_here_document(redir_node->val.str,
                                       redir_node->type ==
                                           NODE_REDIR_HEREDOC_STRIP);
        }
        return 1; // No delimiter found
    }

    // Handle NODE_REDIR_FD first since it doesn't need a target child
    if (redir_node->type == NODE_REDIR_FD) {
        // File descriptor redirection: >&2, 2>&1, etc.
        return setup_fd_redirection(redir_node->val.str);
    }

    // Get the target (filename) from the first child for other redirections
    node_t *target_node = redir_node->first_child;
    if (!target_node || !target_node->val.str) {
        return 1; // No target specified
    }

    // Expand variables in the target
    char *target = expand_redirection_target(executor, target_node->val.str);
    if (!target) {
        return 1;
    }

    // Privileged mode security check for redirection target
    if (!is_privileged_redirection_allowed(target)) {
        fprintf(stderr, "lusush: %s: restricted redirection target in privileged mode\n", target);
        free(target);
        return 1;
    }

    int result = 0;

    switch (redir_node->type) {
    case NODE_REDIR_OUT: {
        // Standard output redirection: command > file
        // Check for noclobber: prevent overwriting existing files
        if (is_noclobber_enabled()) {
            struct stat st;
            if (stat(target, &st) == 0) {
                // File exists and noclobber is enabled
                fprintf(stderr, "lusush: %s: cannot overwrite existing file (noclobber)\n", target);
                result = 1;
                break;
            }
        }
        
        int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror(target);
            result = 1;
            break;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            result = 1;
        }
        close(fd);
        break;
    }

    case NODE_REDIR_APPEND: {
        // Append output redirection: command >> file
        int fd = open(target, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror(target);
            result = 1;
            break;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            result = 1;
            break;
        }
        close(fd);
        break;
    }

    case NODE_REDIR_IN: {
        // Input redirection: command < file
        int fd = open(target, O_RDONLY);
        if (fd == -1) {
            perror(target);
            result = 1;
            break;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(fd);
            result = 1;
            break;
        }
        close(fd);
        break;
    }

    case NODE_REDIR_ERR: {
        // Error redirection: command 2> file
        int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror(target);
            result = 1;
            break;
        }
        if (dup2(fd, STDERR_FILENO) == -1) {
            perror("dup2");
            close(fd);
            result = 1;
            break;
        }
        close(fd);
        break;
    }

    case NODE_REDIR_ERR_APPEND: {
        // Error append redirection: command 2>> file
        int fd = open(target, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror(target);
            result = 1;
            break;
        }
        if (dup2(fd, STDERR_FILENO) == -1) {
            perror("dup2");
            close(fd);
            result = 1;
            break;
        }
        close(fd);
        break;
    }

    case NODE_REDIR_BOTH: {
        // Combined stdout/stderr redirection: command &> file
        int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror(target);
            result = 1;
            break;
        }
        if (dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {
            perror("dup2");
            close(fd);
            result = 1;
            break;
        }
        close(fd);
        break;
    }

    case NODE_REDIR_HEREDOC: {
        // Here document: command << DELIMITER
        result = setup_here_document(target, false);
        break;
    }

    case NODE_REDIR_HEREDOC_STRIP: {
        // Here document with tab stripping: command <<- DELIMITER
        result = setup_here_document(target, true);
        break;
    }

    case NODE_REDIR_HERESTRING: {
        // Here string: command <<< string
        result = setup_here_string(executor, target);
        break;
    }

    case NODE_REDIR_CLOBBER: {
        // Force output redirection: command >| file
        // Override noclobber setting - always allow overwriting
        int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror(target);
            result = 1;
            break;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            result = 1;
        }
        close(fd);
        break;
    }

    default:
        // Unknown redirection type
        result = 1;
        break;
    }

    free(target);
    return result;
}

// Setup here document redirection
static int setup_here_document(const char *delimiter, bool strip_tabs) {
    // Create a temporary pipe for the here document content
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        // Child process: read here document lines and write to pipe
        close(pipefd[0]); // Close read end

        char *line = NULL;
        size_t line_cap = 0;
        ssize_t line_len;

        // Read lines until we find the delimiter
        while ((line_len = getline(&line, &line_cap, stdin)) != -1) {
            // Remove trailing newline for comparison
            if (line_len > 0 && line[line_len - 1] == '\n') {
                line[line_len - 1] = '\0';
                line_len--;
            }

            // Strip leading tabs if requested (<<- variant)
            char *content = line;
            if (strip_tabs) {
                while (*content == '\t') {
                    content++;
                }
            }

            // Check if this line matches the delimiter
            if (strcmp(content, delimiter) == 0) {
                break; // End of here document
            }

            // Write the original line (with newline) to the pipe
            write(pipefd[1], line, strlen(line));
            write(pipefd[1], "\n", 1);
        }

        if (line) {
            free(line);
        }
        close(pipefd[1]);
        _exit(0);
    } else {
        // Parent process: redirect stdin to read from pipe
        close(pipefd[1]); // Close write end

        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            close(pipefd[0]);
            return 1;
        }
        close(pipefd[0]);

        // Wait for child to finish writing the here document
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}

// Setup here document redirection with pre-collected content
static int setup_here_document_with_content(const char *content) {
    if (!content) {
        return 1;
    }

    // Create a temporary pipe for the here document content
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        // Child process: write pre-collected content to pipe
        close(pipefd[0]); // Close read end

        // Write the content to the pipe
        size_t content_len = strlen(content);
        if (content_len > 0) {
            ssize_t written = write(pipefd[1], content, content_len);
            if (written == -1) {
                perror("write");
            }
        }

        close(pipefd[1]);
        _exit(0);
    } else {
        // Parent process: redirect stdin to read from pipe
        close(pipefd[1]); // Close write end

        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            close(pipefd[0]);
            return 1;
        }
        close(pipefd[0]);

        // Wait for child to finish writing the here document
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}

// Setup here document with variable expansion and tab stripping
static int setup_here_document_with_processing(executor_t *executor,
                                               const char *content,
                                               bool strip_tabs,
                                               bool expand_vars) {
    if (!content) {
        return 1;
    }

    // Process the content line by line
    size_t processed_size = strlen(content) * 2 + 1; // Allow for expansion
    char *processed_content = malloc(processed_size);
    if (!processed_content) {
        return 1;
    }
    processed_content[0] = '\0';

    char *line_start = (char *)content;
    char *line_end;

    while (*line_start) {
        // Find end of current line
        line_end = strchr(line_start, '\n');
        if (!line_end) {
            line_end = line_start + strlen(line_start);
        }

        // Extract line
        size_t line_len = line_end - line_start;
        char *line = malloc(line_len + 1);
        if (!line) {
            free(processed_content);
            return 1;
        }
        strncpy(line, line_start, line_len);
        line[line_len] = '\0';

        // Strip leading tabs if requested
        char *processed_line = line;
        if (strip_tabs) {
            while (*processed_line == '\t') {
                processed_line++;
            }
        }

        // Variable expansion if requested and executor available
        char *final_line = processed_line;
        if (expand_vars && executor) {
            // Use the executor's variable expansion function
            char *expanded_line = expand_if_needed(executor, processed_line);
            if (expanded_line) {
                final_line = expanded_line;
            }
        }

        // Ensure buffer is large enough
        size_t needed = strlen(processed_content) + strlen(final_line) + 2;
        if (needed > processed_size) {
            processed_size = needed * 2;
            char *new_content = realloc(processed_content, processed_size);
            if (!new_content) {
                free(line);
                if (final_line != processed_line) {
                    free(final_line);
                }
                free(processed_content);
                return 1;
            }
            processed_content = new_content;
        }

        // Add line to processed content
        strcat(processed_content, final_line);
        if (*line_end == '\n') {
            strcat(processed_content, "\n");
        }

        // Clean up memory
        if (final_line != processed_line) {
            free(final_line);
        }
        free(line);

        // Move to next line
        if (*line_end == '\n') {
            line_start = line_end + 1;
        } else {
            break;
        }
    }

    // Now use the regular setup function with processed content
    int result = setup_here_document_with_content(processed_content);
    free(processed_content);
    return result;
}

// Setup here string redirection
static int setup_here_string(executor_t *executor, const char *content) {

    if (getenv("LUSUSH_DEBUG_REDIR")) {
        printf("DEBUG: setup_here_string called with: '%s'\n", content);
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    // Expand variables in the content first
    char *expanded_content = expand_redirection_target(executor, content);
    if (!expanded_content) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    // Write the expanded string to the pipe
    ssize_t written =
        write(pipefd[1], expanded_content, strlen(expanded_content));
    if (written == -1) {
        perror("write");
        free(expanded_content);
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    // Add a newline at the end
    write(pipefd[1], "\n", 1);
    close(pipefd[1]);
    free(expanded_content);

    // Redirect stdin to read from the pipe
    if (dup2(pipefd[0], STDIN_FILENO) == -1) {
        perror("dup2");
        close(pipefd[0]);
        return 1;
    }
    close(pipefd[0]);

    return 0;
}

// Expand variables in redirection target
static char *expand_redirection_target(executor_t *executor,
                                       const char *target) {
    if (!target) {
        return NULL;
    }

    if (getenv("LUSUSH_DEBUG_REDIR")) {
        printf("DEBUG: expand_redirection_target called with: '%s'\n", target);
    }

    // Use the comprehensive expansion function that handles all variable types,
    // including special variables like $$, $?, $#, etc.
    char *result = expand_if_needed(executor, target);

    // If expansion failed, fall back to a copy of the original
    if (!result) {
        result = strdup(target);
    }

    if (getenv("LUSUSH_DEBUG_REDIR")) {
        printf("DEBUG: expand_redirection_target result: '%s'\n",
               result ? result : "NULL");
    }

    return result;
}

// Setup file descriptor redirection (>&2, 2>&1, etc.)
static int setup_fd_redirection(const char *redir_text) {
    if (!redir_text) {
        return 1;
    }

    // Parse patterns like ">&2", "2>&1", etc.
    if (redir_text[0] == '>' && redir_text[1] == '&' &&
        isdigit(redir_text[2])) {
        // >&N - redirect stdout to file descriptor N
        int target_fd = redir_text[2] - '0';

        if (dup2(target_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            return 1;
        }
        return 0;
    }

    if (isdigit(redir_text[0]) && redir_text[1] == '>' &&
        redir_text[2] == '&' && isdigit(redir_text[3])) {
        // N>&M - redirect file descriptor N to file descriptor M
        int source_fd = redir_text[0] - '0';
        int target_fd = redir_text[3] - '0';

        if (dup2(target_fd, source_fd) == -1) {
            perror("dup2");
            return 1;
        }
        return 0;
    }

    return 1; // Unknown pattern
}

// Save current file descriptors for later restoration
int save_file_descriptors(redirection_state_t *state) {
    if (!state) {
        return 1;
    }

    // Initialize state
    state->stdin_saved = false;
    state->stdout_saved = false;
    state->stderr_saved = false;

    // Save stdin
    state->saved_stdin = dup(STDIN_FILENO);
    if (state->saved_stdin != -1) {
        state->stdin_saved = true;
    }

    // Save stdout
    state->saved_stdout = dup(STDOUT_FILENO);
    if (state->saved_stdout != -1) {
        state->stdout_saved = true;
    }

    // Save stderr
    state->saved_stderr = dup(STDERR_FILENO);
    if (state->saved_stderr != -1) {
        state->stderr_saved = true;
    }

    return 0;
}

// Restore file descriptors after command execution
int restore_file_descriptors(redirection_state_t *state) {
    if (!state) {
        return 1;
    }

    int result = 0;

    // Restore stdin
    if (state->stdin_saved) {
        if (dup2(state->saved_stdin, STDIN_FILENO) == -1) {
            perror("Failed to restore stdin");
            result = 1;
        }
        close(state->saved_stdin);
    }

    // Restore stdout
    if (state->stdout_saved) {
        if (dup2(state->saved_stdout, STDOUT_FILENO) == -1) {
            perror("Failed to restore stdout");
            result = 1;
        }
        close(state->saved_stdout);
    }

    // Restore stderr
    if (state->stderr_saved) {
        if (dup2(state->saved_stderr, STDERR_FILENO) == -1) {
            perror("Failed to restore stderr");
            result = 1;
        }
        close(state->saved_stderr);
    }

    return result;
}

// Print redirection error message
void redirection_error(const char *message) {
    if (message) {
        fprintf(stderr, "redirection: %s\n", message);
    }
}

// Check if a node represents a redirection
bool is_redirection_node(node_t *node) {
    if (!node) {
        return false;
    }

    return (node->type >= NODE_REDIR_IN && node->type <= NODE_REDIR_CLOBBER);
}

// Count redirection nodes in a command
int count_redirections(node_t *command) {
    if (!command) {
        return 0;
    }

    int count = 0;
    node_t *child = command->first_child;
    while (child) {

        if (is_redirection_node(child)) {
            count++;
        }
        child = child->next_sibling;
    }

    return count;
}
