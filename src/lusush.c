/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/linenoise/linenoise.h"
#include "../include/node.h"
#include "../include/parser.h"
#include "../include/scanner.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void skip_conditional_commands(source_t *src);

int main(int argc, char **argv) {
    FILE *in = NULL;   // input file stream pointer
    char *line = NULL; // pointer to a line of input read

    // Initialize special shell variables
    shell_pid = getpid();
    shell_argc = argc;
    shell_argv = argv;
    last_exit_status = 0;

    // Perform startup tasks
    init(argc, argv, &in);

    // Handle command mode (-c option)
    if (shell_opts.command_mode && shell_opts.command_string) {
        // Print the command if verbose mode is enabled
        if (shell_opts.verbose) {
            fprintf(stderr, "%s\n", shell_opts.command_string);
        }
        
        // Execute the command string and exit
        source_t src;
        src.buf = shell_opts.command_string;
        src.bufsize = strlen(shell_opts.command_string);
        src.pos = INIT_SRC_POS;
        
        int exit_status = parse_and_execute(&src);
        
        // Clean up and exit
        free(shell_opts.command_string);
        exit(exit_status);
    }

    // Read input (buffering complete syntactic units) until user exits
    // or EOF is read from either stdin or input file
    while (!exit_flag) {
        // Read complete command(s) using unified input system
        // This ensures consistent parsing behavior between interactive and non-interactive modes
        line = get_unified_input(in);

        if (line == NULL) {
            exit_flag = true;
            continue;
        }

        // Create a source structure from input
        source_t src;
        src.buf = line;
        src.bufsize = strlen(line);
        src.pos = INIT_SRC_POS;

        // Check if this line contains ONLY simple pipes (no logical operators)
        // If there are logical operators mixed with pipes, use full parser
        bool has_single_pipe = false;
        bool has_logical_operators = false;
        
        // Check for logical operators first
        if (strstr(line, "&&") || strstr(line, "||") || strchr(line, ';')) {
            has_logical_operators = true;
        }
        
        // Check for single pipes (not || or |&)  
        char *pipe_pos = strchr(line, '|');
        while (pipe_pos != NULL) {
            // Check if it's a single pipe (not || or |&)
            if ((pipe_pos == line || pipe_pos[-1] != '|') && 
                (pipe_pos[1] != '|' && pipe_pos[1] != '&')) {
                has_single_pipe = true;
                break;
            }
            pipe_pos = strchr(pipe_pos + 1, '|');
        }
        
        // Use simple pipeline execution ONLY if we have pipes but NO logical operators
        if (has_single_pipe && !has_logical_operators) {
            execute_pipeline_simple(line);
        } else {
            // Parse then execute a command normally (handles logical operators and mixed expressions)
            parse_and_execute(&src);
        }

        if (shell_type() != NORMAL_SHELL) {
            linenoiseFree(line);
        } else {
            free_input_buffers();
        }
    }

    if (in) {
        fclose(in);
    }
}

int parse_and_execute(source_t *src) {
    int last_exit_status = 0;  // Initialize to 0 (success)
    
    while (true) {
        skip_whitespace(src);
        
        // Use new parser architecture to get complete commands
        node_t *cmd = parse_complete_command(src);
        
        if (cmd == NULL) {
            break;  // EOF or error
        }

        // Execute the parsed command using the appropriate handler
        last_exit_status = execute_node(cmd);
        
        free_node_tree(cmd);
        
        // Check for more commands (semicolon or newline separated)
        skip_whitespace(src);
        token_t *delimiter = tokenize(src);
        
        if (delimiter == &eof_token) {
            break;
        }
        
        // Handle conditional execution for && and ||
        if (delimiter->type == TOKEN_AND_IF) {
            // && : continue only if last command succeeded (exit code 0)
            free_token(delimiter);
            if (last_exit_status == 0) {
                continue;  // Success: execute next command
            } else {
                // Failure: skip remaining commands until next unconditional delimiter
                skip_conditional_commands(src);
                continue;
            }
        } else if (delimiter->type == TOKEN_OR_IF) {
            // || : continue only if last command failed (exit code != 0)  
            free_token(delimiter);
            if (last_exit_status != 0) {
                continue;  // Failure: execute next command
            } else {
                // Success: skip remaining commands until next unconditional delimiter
                skip_conditional_commands(src);
                continue;
            }
        } else if (delimiter->type == TOKEN_SEMI || delimiter->type == TOKEN_NEWLINE) {
            // Unconditional continuation
            free_token(delimiter);
            continue;  // More commands to process
        } else {
            // Push back non-delimiter token
            unget_token(delimiter);
            break;
        }
    }

    return last_exit_status;  // Return the exit status of the last command
}

/**
 * skip_conditional_commands:
 *      Skip commands in a conditional chain until an unconditional delimiter
 *      Used when && or || conditions are not met
 */
static void skip_conditional_commands(source_t *src) {
    while (true) {
        skip_whitespace(src);
        
        // Try to parse the next command (but don't execute it)
        node_t *cmd = parse_complete_command(src);
        if (cmd == NULL) {
            break;  // EOF or error
        }
        free_node_tree(cmd);  // Discard the parsed command
        
        // Check what delimiter follows
        skip_whitespace(src);
        token_t *delimiter = tokenize(src);
        if (delimiter == &eof_token) {
            break;
        }
        
        if (delimiter->type == TOKEN_SEMI || delimiter->type == TOKEN_NEWLINE) {
            // Found unconditional delimiter - stop skipping
            unget_token(delimiter);  // Put it back for main loop
            break;
        } else if (delimiter->type == TOKEN_AND_IF || delimiter->type == TOKEN_OR_IF) {
            // Continue skipping conditional commands
            free_token(delimiter);
            continue;
        } else {
            // Other token - put it back and stop
            unget_token(delimiter);
            break;
        }
    }
}
