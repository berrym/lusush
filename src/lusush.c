/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"

#include "../include/config.h"
#include "../include/executor.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/readline_integration.h"
#include "../include/posix_history.h"
#include "../include/signals.h"
#include "../include/symtable.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
extern posix_history_manager_t *global_posix_history;

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
        // Debug: Check shell mode
        const char *debug_env = getenv("LLE_DEBUG");
        bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
        
        if (debug_mode) {
            fprintf(stderr, "[LUSUSH_MAIN] Starting input read, interactive=%s\n", 
                    is_interactive_shell() ? "true" : "false");
        }
        
        // Read complete command(s) using unified input system
        // This ensures consistent parsing behavior between interactive and
        // non-interactive modes
        line = get_unified_input(in);

        if (debug_mode) {
            if (line) {
                fprintf(stderr, "[LUSUSH_MAIN] get_unified_input returned: '%s'\n", line);
            } else {
                fprintf(stderr, "[LUSUSH_MAIN] get_unified_input returned NULL, setting exit_flag\n");
            }
        }

        if (line == NULL) {
            exit_flag = true;
            continue;
        }

        // Add command to enhanced history if in interactive mode
        if (is_interactive_shell() && global_posix_history && line && *line) {
            // Skip commands that are just whitespace
            const char *trimmed = line;
            while (*trimmed && isspace(*trimmed)) trimmed++;
            if (*trimmed) {
                enhanced_history_add(line);
            }
        }

        // Execute using unified modern parser and store exit status
        int exit_status = parse_and_execute(line);
        last_exit_status = exit_status;
        set_exit_status(exit_status);

        if (is_interactive_shell()) {
            free(line);
        } else {
            free_input_buffers();
        }
    }

    if (in) {
        fclose(in);
    }

    // Execute logout scripts if this is a login shell
    if (is_login_shell()) {
        config_execute_logout_scripts();
    }

    // Save enhanced history before exit
    if (is_interactive_shell() && global_posix_history) {
        enhanced_history_save();
    }

    // Execute EXIT traps before shell terminates normally
    execute_exit_traps();

    // Exit with the status of the last command executed (POSIX requirement)
    exit(last_exit_status);
}

int parse_and_execute(const char *command) {
    // Use unified executor for all commands
    executor_t *executor = executor_new();
    if (!executor) {
        return 1;
    }

    int exit_status = executor_execute_command_line(executor, command);

    // Print error messages to stderr if there were any errors
    if (executor_has_error(executor)) {
        fprintf(stderr, "lusush: %s\n", executor_error(executor));
    }

    executor_free(executor);

    return exit_status;
}
