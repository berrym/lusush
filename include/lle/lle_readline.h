/**
 * @file lle_readline.h
 * @brief LLE Readline Function - Public API
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 *
 * This header declares lle_readline(), the main entry point for interactive
 * line editing in LLE. This function replaces GNU readline's readline() when
 * LLE is enabled.
 *
 * Usage:
 *   char *line = lle_readline("prompt> ");
 *   if (line != NULL) {
 *       // Process line
 *       free(line);  // Caller must free
 *   }
 */

#ifndef LLE_READLINE_H
#define LLE_READLINE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read a line of input from the user with line editing
 *
 * This function orchestrates all LLE subsystems to provide interactive line
 * editing similar to GNU readline. It:
 * - Displays the prompt
 * - Reads user input character by character
 * - Supports line editing operations (insert, delete, cursor movement)
 * - Handles special keys (arrows, Home, End, etc.)
 * - Supports multiline editing for incomplete commands
 * - Returns the completed line when user presses Enter
 *
 * The function enters raw terminal mode during execution and restores the
 * terminal state before returning.
 *
 * @param prompt The prompt string to display (e.g., "$ ", "> ")
 *               May be NULL for no prompt.
 *
 * @return Newly allocated string containing the user's input line.
 *         The caller is responsible for freeing this memory with free().
 *         Returns NULL on:
 *         - EOF (Ctrl-D on empty line)
 *         - Interrupt (Ctrl-C)
 *         - Error (LLE system not initialized, terminal error, etc.)
 *
 * @note This function is NOT reentrant. Only one readline operation
 *       should be active at a time.
 *
 * @note The LLE system must be initialized with lle_system_initialize()
 *       before calling this function.
 */
char *lle_readline(const char *prompt);

#ifdef __cplusplus
}
#endif

#endif /* LLE_READLINE_H */
