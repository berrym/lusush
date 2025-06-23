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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

// Forward declarations
static int handle_redirection_node(node_t *redir_node);
static int setup_here_document(const char *delimiter, bool strip_tabs);
static int setup_here_string(const char *content);
static char *expand_redirection_target(const char *target);

// Setup redirections for a command
int setup_redirections(node_t *command) {
    if (!command) {
        return 0; // No redirections to setup
    }
    
    // Look for redirection nodes among the command's children
    node_t *child = command->first_child;
    while (child) {
        if (child->type >= NODE_REDIR_IN && child->type <= NODE_REDIR_FD) {
            int result = handle_redirection_node(child);
            if (result != 0) {
                return result;
            }
        }
        child = child->next_sibling;
    }
    
    return 0;
}

// Handle individual redirection node
static int handle_redirection_node(node_t *redir_node) {
    if (!redir_node) {
        return 1;
    }
    
    // Get the target (filename or delimiter) from the first child
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