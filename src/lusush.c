/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"

#include "../include/executor.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/linenoise/linenoise.h"

#include "../include/symtable.h"
#include "../include/signals.h"


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

        // Flush output buffers before exit to ensure all output is displayed
        fflush(stdout);
        fflush(stderr);

        // Clean up and exit
        free(shell_opts.command_string);

        // Execute EXIT traps before terminating
        execute_exit_traps();
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

        // Execute using unified modern parser and store exit status
        int exit_status = parse_and_execute(line);
        last_exit_status = exit_status;
        set_exit_status(exit_status);

        if (is_interactive_shell()) {
            linenoiseFree(line);
        } else {
            free_input_buffers();
        }
    }

    if (in) {
        fclose(in);
    }

    // Execute EXIT traps before shell terminates normally
    execute_exit_traps();
}

int parse_and_execute(const char *command) {
    // Use unified executor for all commands
    executor_t *executor = executor_new();
    if (!executor) {
        return 1;
    }

    int exit_status = executor_execute_command_line(executor, command);

    executor_free(executor);

    return exit_status;
}
