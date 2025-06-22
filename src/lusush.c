/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/linenoise/linenoise.h"
#include "../include/node.h"
#include "../include/parser.h"
#include "../include/parser_modern.h"
#include "../include/executor_modern.h"
#include "../include/scanner_old.h"

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





        // Execute using unified modern parser
        parse_and_execute(&src);

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
    // Use unified modern executor for all commands
    executor_modern_t *executor = executor_modern_new();
    if (!executor) {
        return 1;
    }
    
    int exit_status = executor_modern_execute_command_line(executor, src->buf);
    
    executor_modern_free(executor);
    
    return exit_status;
}


