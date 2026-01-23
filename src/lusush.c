/**
 * @file lusush.c
 * @brief Lusush - A modern Unix shell with native line editing
 *
 * Main entry point and REPL (Read-Eval-Print Loop) for the Lusush shell.
 * Handles command line parsing, interactive mode, and script execution.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 * @license MIT
 *
 */

#include "lusush.h"

#include "config.h"
#include "display_integration.h"
#include "executor.h"
#include "init.h"
#include "input.h"
#include "lle/lle_shell_event_hub.h"
#include "lle/lle_shell_integration.h"
#include "posix_history.h"
#include "signals.h"
#include "symtable.h"

#include <time.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Detect if command line ends with background operator
 *
 * Checks if command ends with '&' but not '&&' (logical AND).
 * Used by Spec 26 shell event hub to set is_background flag correctly.
 *
 * @param command The command line to check
 * @return true if command will run in background, false otherwise
 */
static bool is_background_command(const char *command) {
    if (!command) {
        return false;
    }

    size_t len = strlen(command);
    if (len == 0) {
        return false;
    }

    /* Find last non-whitespace character */
    size_t i = len;
    while (i > 0 && isspace((unsigned char)command[i - 1])) {
        i--;
    }

    if (i == 0) {
        return false;
    }

    /* Check if ends with '&' */
    if (command[i - 1] != '&') {
        return false;
    }

    /* Make sure it's not '&&' (logical AND) */
    if (i >= 2 && command[i - 2] == '&') {
        return false;
    }

    return true;
}

// Forward declarations
extern posix_history_manager_t *global_posix_history;
void executor_update_job_status(executor_t *executor);

// Global executor for persistent function definitions across commands
static executor_t *global_executor = NULL;

/**
 * @brief Main entry point for the Lusush shell
 *
 * Initializes the shell environment, handles command-line options,
 * and runs the main read-eval-print loop (REPL) for interactive sessions
 * or executes scripts for non-interactive sessions.
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return Exit status of the last command executed (POSIX requirement)
 */
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

        /**
         * @brief Fire pre-command event for command mode (Spec 26)
         *
         * Command mode (-c) must also fire shell events for consistency
         * with the event system. This enables proper tracking even for
         * single-command invocations.
         */
        uint64_t cmd_start_us = 0;
        if (g_lle_integration) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            cmd_start_us =
                (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
            bool is_bg = is_background_command(shell_opts.command_string);
            lle_fire_pre_command(shell_opts.command_string, is_bg);
        }

        // Execute the command string and exit
        int exit_status = parse_and_execute(shell_opts.command_string);

        /**
         * @brief Fire post-command event for command mode (Spec 26)
         */
        if (g_lle_integration) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            uint64_t cmd_end_us =
                (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
            lle_fire_post_command(shell_opts.command_string, exit_status,
                                  cmd_end_us - cmd_start_us);
        }

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
            // Check if this was due to SIGINT (Ctrl+C) rather than real EOF
            if (check_and_clear_sigint_flag()) {
                // SIGINT received - just continue to show new prompt
                // The signal handler already printed newline
                continue;
            }
            // Real EOF encountered - print newline to prevent shell prompt
            // artifacts
            if (is_interactive_shell()) {
                printf("\n");
                fflush(stdout);
            }
            exit_flag = true;
            continue;
        }

        // Add command to history if in interactive mode (handled by readline)
        // History is automatically managed by the readline integration

        /**
         * @brief Fire pre-command event for LLE shell integration (Spec 26)
         *
         * Records command start time and notifies handlers before execution.
         * Detects background commands (ending with &) to set is_background
         * flag.
         */
        uint64_t cmd_start_us = 0;
        if (g_lle_integration) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            cmd_start_us =
                (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
            bool is_bg = is_background_command(line);
            lle_fire_pre_command(line, is_bg);
        }

        // Execute using unified modern parser and store exit status
        int exit_status = parse_and_execute(line);
        last_exit_status = exit_status;
        set_exit_status(exit_status);

        /* Fire post-command event for LLE shell integration (Spec 26)
         * Provides exit code and execution duration for prompt and history.
         */
        if (g_lle_integration) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            uint64_t cmd_end_us =
                (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
            lle_fire_post_command(line, exit_status, cmd_end_us - cmd_start_us);
        }

        // Post-command display integration for layered display caching
        // Enables the layered display system to handle post-command prompt
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

        // Free the line buffer (returned by get_unified_input)
        free(line);
        if (!is_interactive_shell()) {
            // Also cleanup global input state for non-interactive mode
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

/**
 * @brief Parse and execute a shell command string
 *
 * Uses the global persistent executor to parse and execute the given
 * command string. The global executor maintains function definitions
 * across multiple command invocations.
 *
 * @param command The command string to parse and execute
 * @return Exit status of the executed command (0 for success, non-zero for failure)
 */
int parse_and_execute(const char *command) {
    // Use global persistent executor for all commands to maintain function
    // definitions
    if (!global_executor) {
        global_executor = executor_new();
        if (!global_executor) {
            return 1;
        }
        
        // Set script context if running a script (not interactive)
        // $0 contains the script name when running a script
        if (!is_interactive_shell()) {
            char *script_name = symtable_get_global("0");
            if (script_name) {
                executor_set_script_context(global_executor, script_name, 1);
                free(script_name);
            }
        }
    }

    int exit_status = executor_execute_command_line(global_executor, command);

    // Flush output streams after command execution
    // This ensures output appears immediately, especially under valgrind/piping
    fflush(stdout);
    fflush(stderr);

    // Print error messages to stderr if there were any errors
    // (Skip if error_message is NULL - means it was already displayed via structured system)
    if (executor_has_error(global_executor) && executor_error(global_executor)) {
        fprintf(stderr, "lusush: %s\n", executor_error(global_executor));
        fflush(stderr);
    }

    return exit_status;
}

/**
 * @brief Get the global executor instance
 *
 * Returns a pointer to the global executor used for command execution.
 * This allows builtins (e.g., source command) to access the executor
 * for debugging or execution purposes.
 *
 * @return Pointer to the global executor instance, or NULL if not initialized
 */
executor_t *get_global_executor(void) { return global_executor; }
