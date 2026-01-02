/**
 * @file history.h
 * @brief Basic shell command history interface
 *
 * Provides core history functionality including initialization, adding
 * commands, saving, and lookup operations.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef HISTORY_H
#define HISTORY_H

/**
 * @brief Initialize the command history system
 *
 * Sets up history data structures and loads history from
 * the history file if it exists.
 */
void init_history(void);

/**
 * @brief Add a command to history
 *
 * Adds the specified command to the in-memory history list.
 * Duplicate and whitespace-only commands may be filtered.
 *
 * @param cmd Command string to add
 */
void history_add(const char *cmd);

/**
 * @brief Save history to file
 *
 * Writes the current history list to the history file
 * for persistence across sessions.
 */
void history_save(void);

/**
 * @brief Print all history entries
 *
 * Displays the history list with line numbers to stdout.
 */
void history_print(void);

/**
 * @brief Look up a command by pattern
 *
 * Searches history for a command matching the specified pattern.
 * Supports history expansion patterns like "!string" and "!n".
 *
 * @param s Pattern to search for
 * @return Matching command string, or NULL if not found
 */
char *history_lookup(const char *s);

/**
 * @brief Print usage information for the history builtin
 *
 * Displays help text for the history command.
 */
void history_usage(void);

#endif
