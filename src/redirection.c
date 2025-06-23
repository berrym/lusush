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

#include "../include/node.h"
#include "../include/redirection.h"
#include "../include/executor_modern.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

// Forward declarations
static int handle_redirection_node(executor_modern_t *executor, node_t *redir_node);
static int setup_here_document(const char *delimiter, bool strip_tabs);
static int setup_here_document_with_content(const char *content);
static int setup_here_document_with_processing(executor_modern_t *executor, const char *content, bool strip_tabs, bool expand_vars);
static int setup_here_string(const char *content);
static char *expand_redirection_target(const char *target);

// External function from executor_modern.c
extern char *expand_if_needed_modern(executor_modern_t *executor, const char *text);

// Setup redirections for a command
int setup_redirections(executor_modern_t *executor, node_t *command) {
    if (!command) {
        return 0; // No redirections to setup
    }
    
    // Look for redirection nodes among the command's children
    node_t *child = command->first_child;
    while (child) {
        if (child->type >= NODE_REDIR_IN && child->type <= NODE_REDIR_FD) {
            int result = handle_redirection_node(executor, child);
            if (result != 0) {
                return result;
            }
        }
        child = child->next_sibling;
    }
    
    return 0; // All redirections set up successfully
}

// Handle individual redirection node
static int handle_redirection_node(executor_modern_t *executor, node_t *redir_node) {
    if (!redir_node) {
        return 1;
    }
    
    // For here documents, check if we have pre-collected content
    if (redir_node->type == NODE_REDIR_HEREDOC || redir_node->type == NODE_REDIR_HEREDOC_STRIP) {
        // The delimiter is stored in the redirection node value
        // The content is stored in the first child node
        // The expand variables flag is stored in the second child node
        node_t *content_node = redir_node->first_child;
        node_t *expand_flag_node = content_node ? content_node->next_sibling : NULL;
        
        if (content_node && content_node->val.str) {
            bool strip_tabs = (redir_node->type == NODE_REDIR_HEREDOC_STRIP);
            bool expand_vars = true; // Default to expand
            
            // Check expand variables flag from parser
            if (expand_flag_node && expand_flag_node->val.str) {
                expand_vars = (strcmp(expand_flag_node->val.str, "1") == 0);
            }
            
            return setup_here_document_with_processing(executor, content_node->val.str, strip_tabs, expand_vars);
        }
        // If no content node, fall back to interactive here document
        if (redir_node->val.str) {
            return setup_here_document(redir_node->val.str, 
                                     redir_node->type == NODE_REDIR_HEREDOC_STRIP);
        }
        return 1; // No delimiter found
    }
    
    // Get the target (filename) from the first child for other redirections
    node_t *target_node = redir_node->first_child;
    if (!target_node || !target_node->val.str) {
        return 1; // No target specified
    }
    
    // Expand variables in the target
    char *target = expand_redirection_target(target_node->val.str);
    if (!target) {
        return 1;
    }
    
    int result = 0;
    
    switch (redir_node->type) {
        case NODE_REDIR_OUT: {
            // Standard output redirection: command > file
            int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
            result = setup_here_string(target);
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
static int setup_here_document_with_processing(executor_modern_t *executor, const char *content, bool strip_tabs, bool expand_vars) {
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
            char *expanded_line = expand_if_needed_modern(executor, processed_line);
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
static int setup_here_string(const char *content) {
    // Create a temporary pipe to provide the string as input
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }
    
    // Write the string to the pipe
    ssize_t written = write(pipefd[1], content, strlen(content));
    if (written == -1) {
        perror("write");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }
    
    // Add a newline at the end
    write(pipefd[1], "\n", 1);
    close(pipefd[1]);
    
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
static char *expand_redirection_target(const char *target) {
    if (!target) {
        return NULL;
    }
    
    // For now, just return a copy of the target
    // In a full implementation, this would expand variables like $HOME, etc.
    return strdup(target);
}

// Save current file descriptors before redirection
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
    
    return (node->type >= NODE_REDIR_IN && node->type <= NODE_REDIR_FD);
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