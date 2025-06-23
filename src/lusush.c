/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/input_modern.h"
#include "../include/linenoise/linenoise.h"
#include "../include/node.h"
#include "../include/parser_modern.h"
#include "../include/executor_modern.h"


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Forward declarations

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
        int exit_status = parse_and_execute(shell_opts.command_string);
        
        // Clean up and exit
        free(shell_opts.command_string);
        exit(exit_status);
    }

    // Create modern input context
    bool is_interactive = (shell_type() == INTERACTIVE_SHELL);
    input_context_t *input_context = input_context_new(in, is_interactive);
    if (!input_context) {
        fprintf(stderr, "Failed to initialize input system\n");
        exit(1);
    }

    // Read input (buffering complete syntactic units) until user exits
    // or EOF is read from either stdin or input file
    while (!exit_flag) {
        // Read complete command(s) using modern input system
        // This ensures consistent parsing behavior between interactive and non-interactive modes
        line = input_get_command_line(input_context);

        if (line == NULL) {
            exit_flag = true;
            continue;
        }

        // Execute using unified modern parser
        parse_and_execute(line);

        // Free the line - modern input system manages its own memory
        free(line);
    }

    // Clean up modern input context
    input_context_free(input_context);

    if (in) {
        fclose(in);
    }
}

int parse_and_execute(const char *command) {
    // Use unified modern executor for all commands
    executor_modern_t *executor = executor_modern_new();
    if (!executor) {
        return 1;
    }
    
    int exit_status = executor_modern_execute_command_line(executor, command);
    
    executor_modern_free(executor);
    
    return exit_status;
}


