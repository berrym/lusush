/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/linenoise/linenoise.h"
#include "../include/node.h"
#include "../include/parser.h"
#include "../include/parser_new_simple.h"
#include "../include/scanner.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void skip_conditional_commands(source_t *src);
static int parse_and_execute_simple(source_t *src);

/**
 * Analyze command complexity to determine which parser to use
 */
typedef enum {
    CMD_SIMPLE,      // Simple command: echo hello, ls -la
    CMD_PIPELINE,    // Simple pipeline: ls | grep test  
    CMD_COMPLEX      // Complex: logical operators, control structures, etc.
} command_complexity_t;

static command_complexity_t analyze_command_complexity(const char *line) {
    if (!line || strlen(line) == 0) {
        return CMD_SIMPLE;
    }
    
    // Check for control structure keywords
    const char *control_keywords[] = {
        "if ", "then ", "else ", "elif ", "fi ",
        "while ", "for ", "do ", "done ",
        "case ", "esac ", "until ",
        "function ", "{", "}"
    };
    
    for (size_t i = 0; i < sizeof(control_keywords) / sizeof(control_keywords[0]); i++) {
        if (strstr(line, control_keywords[i])) {
            return CMD_COMPLEX;
        }
    }
    
    // Check for logical operators (&&, ||, ;)
    if (strstr(line, "&&") || strstr(line, "||") || strchr(line, ';')) {
        return CMD_COMPLEX;
    }
    
    // Check for simple pipes
    char *pipe_pos = strchr(line, '|');
    if (pipe_pos != NULL) {
        // Make sure it's not || or |&
        if ((pipe_pos == line || pipe_pos[-1] != '|') && 
            (pipe_pos[1] != '|' && pipe_pos[1] != '&')) {
            return CMD_PIPELINE;
        }
    }
    
    return CMD_SIMPLE;
}

// Optional: Bridge function to test new parser (can be enabled via environment variable)
static node_t *parse_complete_command_new_bridge(source_t *src) {
    parser_t *parser = parser_create(src, NULL);
    if (!parser) {
        return NULL;
    }
    
    node_t *result = parser_parse(parser);
    parser_destroy(parser);
    return result;
}

/**
 * parse_and_execute_simple:
 *   Use new parser for simple commands only (Phase 1 of gradual migration)
 *   This function bypasses the old parser's complex logic and uses the new
 *   POSIX-compliant parser directly for simple command execution.
 */
static int parse_and_execute_simple(source_t *src) {
    // Debug: Show what we're trying to parse
    fprintf(stderr, "DEBUG: Parsing simple command: '%.50s'\n", src->buf);
    
    // Create new parser instance
    parser_t *parser = parser_create(src, NULL);
    if (!parser) {
        fprintf(stderr, "Error: Failed to create parser for simple command\n");
        return 1;
    }
    
    // Parse the simple command using new parser
    node_t *cmd = parser_parse(parser);
    if (!cmd) {
        fprintf(stderr, "DEBUG: New parser returned NULL\n");
        parser_destroy(parser);
        return 0;  // Empty command or EOF
    }
    
    // Debug: Show parser routing (optional)
    if (getenv("NEW_PARSER_DEBUG")) {
        fprintf(stderr, "DEBUG: Parsing simple command: '%s'\n", src->buf);
        fprintf(stderr, "DEBUG: Parsed node type: %d\n", cmd->type);
        if (cmd->val.str) {
            fprintf(stderr, "DEBUG: Node value: '%s'\n", cmd->val.str);
        }
    }
    
    // Execute the command using the new parser adapter
    int exit_status = execute_new_parser_command(cmd);
    
    // Clean up
    free_node_tree(cmd);
    parser_destroy(parser);
    
    return exit_status;
}

/**
 * parse_and_execute_pipeline: Parse and execute pipeline using new POSIX parser
 */
int parse_and_execute_pipeline(source_t *src) {
    if (!src) {
        return 0;
    }
    
    // Create new parser instance
    parser_t *parser = parser_create(src, NULL);
    if (!parser) {
        fprintf(stderr, "DEBUG: Failed to create new parser\n");
        return 0;
    }
    
    // Parse the command (should be a pipeline)
    node_t *cmd = parser_parse(parser);
    if (!cmd) {
        if (getenv("NEW_PARSER_DEBUG")) {
            fprintf(stderr, "DEBUG: New parser returned NULL\n");
        }
        parser_destroy(parser);
        return 0;  // Empty command or EOF
    }
    
    // Debug: Show parser routing (optional)
    if (getenv("NEW_PARSER_DEBUG")) {
        fprintf(stderr, "DEBUG: Parsing pipeline command\n");
        fprintf(stderr, "DEBUG: Parsed node type: %d\n", cmd->type);
        if (cmd->val.str) {
            fprintf(stderr, "DEBUG: Node value: '%s'\n", cmd->val.str);
        }
    }
    
    // Execute the command - check if it's actually a pipeline
    int exit_status;
    if (cmd->type == NODE_PIPE) {
        exit_status = execute_new_parser_pipeline(cmd);
    } else {
        // Not a pipeline, execute as simple command
        exit_status = execute_new_parser_command(cmd);
    }
    
    // Clean up
    free_node_tree(cmd);
    parser_destroy(parser);
    
    return exit_status;
}

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

        // Analyze command complexity to choose appropriate parser
        command_complexity_t complexity = analyze_command_complexity(line);
        
        switch (complexity) {
            case CMD_SIMPLE:
                // Use new parser for simple commands (Phase 1 complete)
                parse_and_execute_simple(&src);
                break;
                
            case CMD_PIPELINE:
                // Use new parser for pipelines (Phase 2)
                parse_and_execute_pipeline(&src);
                break;
                
            case CMD_COMPLEX:
                // Use old parser for complex commands (fallback during transition)
                parse_and_execute(&src);
                break;
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
    
    // Option to use new parser via environment variable (for testing/comparison)
    const char *use_new_parser = getenv("LUSUSH_NEW_PARSER");
    
    while (true) {
        skip_whitespace(src);
        
        // Choose parser based on environment variable
        node_t *cmd = NULL;
        if (use_new_parser && strcmp(use_new_parser, "1") == 0) {
            cmd = parse_complete_command_new_bridge(src);  // Use new parser
        } else {
            cmd = parse_complete_command(src);  // Use old parser (default for stability)
        }
        
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
