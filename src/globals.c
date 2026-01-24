/**
 * @file globals.c
 * @brief Global shell state variables
 *
 * Defines global variables that track shell state including:
 * - Exit flag and status
 * - Process IDs (shell, background jobs)
 * - Positional parameters (argc/argv)
 * - POSIX history manager instance
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#include "lush.h"
#include "posix_history.h"

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

/** @brief Flag indicating shell should exit */
bool exit_flag = false;

/** @brief Exit status of the last executed command ($?) */
int last_exit_status = 0;

/** @brief PID of the current shell process ($$) */
pid_t shell_pid = 0;

/** @brief Argument vector for positional parameters ($1, $2, etc.) */
char **shell_argv = NULL;

/** @brief Argument count for positional parameters ($#) */
int shell_argc = 0;

/** @brief PID of the last background process ($!) */
pid_t last_background_pid = 0;

/** @brief Track if shell_argv is dynamically allocated (needs freeing) */
bool shell_argv_is_dynamic = false;

/** @brief Global POSIX history manager instance */
posix_history_manager_t *global_posix_history = NULL;
