/*
 * Enhanced command substitution implementation
 */

#include "../include/lusush.h"
#include "../include/errors.h"
#include "../include/expand.h"
#include "../include/strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

/*
 * command_substitute_ctx:
 *     Perform command substitution (both `...` and $(...) forms)
 *     with expansion context support.
 */
char *command_substitute_ctx(char *orig_cmd, expand_ctx_t *ctx) {
    if (!orig_cmd || !*orig_cmd) {
        return strdup("");
    }
    
    // If command substitution is disabled in this context, return as-is
    if (ctx && expand_ctx_check(ctx, EXPAND_NOCMD)) {
        return strdup(orig_cmd);
    }
    
    // Check if this is a $(command) or `command` substitution
    char *cmd = orig_cmd;
    size_t cmd_len = 0;
    
    // Handle $(command) form
    if (orig_cmd[0] == '$' && orig_cmd[1] == '(') {
        cmd = orig_cmd + 2;  // Skip the $( prefix
        
        // Find the closing parenthesis, handling nested parentheses
        int depth = 1;
        char *p = cmd;
        while (*p && depth > 0) {
            if (*p == '(') {
                depth++;
            } else if (*p == ')') {
                depth--;
            }
            p++;
        }
        
        if (depth != 0) {
            error_message("error: unmatched parentheses in command substitution");
            return strdup("");
        }
        
        cmd_len = p - cmd - 1;  // Exclude the closing parenthesis
    } 
    // Handle `command` form
    else if (orig_cmd[0] == '`') {
        cmd = orig_cmd + 1;  // Skip the backtick prefix
        
        // Find the closing backtick
        char *p = cmd;
        while (*p && *p != '`') {
            // Handle escaped backticks
            if (*p == '\\' && *(p+1) == '`') {
                p += 2;
            } else {
                p++;
            }
        }
        
        if (!*p) {
            error_message("error: unmatched backtick in command substitution");
            return strdup("");
        }
        
        cmd_len = p - cmd;
    } else {
        // Not a command substitution
        return strdup(orig_cmd);
    }
    
    // Extract the command
    char *command = strndup(cmd, cmd_len);
    if (!command) {
        error_syscall("error: `command_substitute`");
        return strdup("");
    }
    
    // Create pipes for capturing the command output
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        error_syscall("error: `command_substitute` - pipe");
        free(command);
        return strdup("");
    }
    
    // Fork a child process to execute the command
    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed
        error_syscall("error: `command_substitute` - fork");
        free(command);
        close(pipefd[0]);
        close(pipefd[1]);
        return strdup("");
    } else if (pid == 0) {
        // Child process
        
        // Redirect stdout to the pipe
        close(pipefd[0]);  // Close the read end
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);  // Close the original write end
        
        // Create a temporary source_t structure for the command
        source_t src;
        src.buf = command;
        src.bufsize = strlen(command);
        src.pos = INIT_SRC_POS;
        
        // Parse and execute the command
        parse_and_execute(&src);
        
        // Exit the child process
        exit(0);
    } else {
        // Parent process
        close(pipefd[1]);  // Close the write end
        
        // Read the command output from the pipe
        char buffer[MAXLINE];
        ssize_t bytes_read;
        size_t total_read = 0;
        char *output = NULL;
        
        while ((bytes_read = read(pipefd[0], buffer, MAXLINE - 1)) > 0) {
            buffer[bytes_read] = '\0';
            
            // Append to the output buffer
            if (!output) {
                output = strdup(buffer);
                if (!output) {
                    error_syscall("error: `command_substitute` - strdup");
                    break;
                }
                total_read = bytes_read;
            } else {
                char *new_output = realloc(output, total_read + bytes_read + 1);
                if (!new_output) {
                    error_syscall("error: `command_substitute` - realloc");
                    free(output);
                    output = NULL;
                    break;
                }
                output = new_output;
                memcpy(output + total_read, buffer, bytes_read);
                total_read += bytes_read;
                output[total_read] = '\0';
            }
        }
        
        close(pipefd[0]);  // Close the read end
        
        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);
        
        // Clean up
        free(command);
        
        // Trim trailing newlines
        if (output) {
            size_t len = strlen(output);
            while (len > 0 && output[len - 1] == '\n') {
                output[--len] = '\0';
            }
            
            return output;
        }
    }
    
    return strdup("");
}

/**
 * command_substitute:
 *     Backward compatibility wrapper for the original function
 */
char *command_substitute(char *orig_cmd) {
    expand_ctx_t ctx;
    expand_ctx_init(&ctx, EXPAND_NORMAL);
    return command_substitute_ctx(orig_cmd, &ctx);
}
