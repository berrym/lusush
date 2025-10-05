/*
 * Lusush - A modern shell with GNU Readline integration
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "../include/display_integration.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
extern posix_history_manager_t *global_posix_history;
void executor_update_job_status(executor_t *executor);

// Global executor for persistent function definitions across commands
static executor_t *global_executor = NULL;

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
        // This ensures consistent parsing behavior between interactive and
        // non-interactive modes
        line = get_unified_input(in);

        if (line == NULL) {
            // EOF encountered - print newline to prevent shell prompt artifacts
            if (is_interactive_shell()) {
                printf("\n");
                fflush(stdout);
            }
            exit_flag = true;
            continue;
        }

        // Add command to history if in interactive mode (handled by readline)
        // History is automatically managed by the readline integration

        // Execute using unified modern parser and store exit status
        int exit_status = parse_and_execute(line);
        last_exit_status = exit_status;
        set_exit_status(exit_status);

        // Phase 1: Post-command display integration for layered display caching
        // This enables the layered display system to handle post-command prompt
        // rendering and achieve >75% cache hit rate targets
        display_integration_post_command_update(line);

        // Check notify option (-b): asynchronous background job notification
        if (shell_opts.notify && global_executor) {
            executor_update_job_status(global_executor);
        }

        // Check onecmd option (-t): exit after executing one command
        if (shell_opts.onecmd) {
            exit_flag = true;
        }

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

    // Cleanup is handled by atexit() handlers registered in init.c
    // This prevents double cleanup when exit() command is used

    // Cleanup global executor before exit
    if (global_executor) {
        executor_free(global_executor);
        global_executor = NULL;
    }

    // Execute EXIT traps before shell terminates normally
    execute_exit_traps();

    // Exit with the status of the last command executed (POSIX requirement)
    exit(last_exit_status);
}

int parse_and_execute(const char *command) {
    // Use global persistent executor for all commands to maintain function definitions
    if (!global_executor) {
        global_executor = executor_new();
        if (!global_executor) {
            return 1;
        }
    }

    int exit_status = executor_execute_command_line(global_executor, command);

    // Print error messages to stderr if there were any errors
    if (executor_has_error(global_executor)) {
        fprintf(stderr, "lusush: %s\n", executor_error(global_executor));
    }

    return exit_status;
}

// Get global executor for use by builtins (e.g., source command for debugging)
executor_t *get_global_executor(void) {
    return global_executor;
}
