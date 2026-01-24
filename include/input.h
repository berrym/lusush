/**
 * @file input.h
 * @brief Shell input handling and line reading
 *
 * Provides core input functions for reading commands from various sources
 * including interactive terminals, files, and pipes. Supports multiline
 * input with continuation prompts.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Free all input-related buffers
 *
 * Releases memory allocated for input processing buffers.
 * Should be called during shell cleanup.
 */
void free_input_buffers(void);

/**
 * @brief Read a line of input from a file stream
 *
 * Reads a single line of input from the specified file stream.
 * Does not handle multiline continuation.
 *
 * @param in Input file stream to read from
 * @return Allocated string containing the input line, or NULL on EOF/error
 */
char *get_input(FILE *in);

/**
 * @brief Read a line of input using the line editor
 *
 * Reads input interactively using LLE (Lush Line Editor) with
 * command-line editing, history, and completion support.
 *
 * @return Allocated string containing the input line, or NULL on EOF/error
 */
char *ln_gets(void);

/**
 * @brief Read complete input with multiline continuation handling
 *
 * Reads input from the specified file stream, handling multiline
 * continuation for incomplete shell constructs (quotes, brackets,
 * control structures).
 *
 * @param in Input file stream to read from
 * @return Allocated string containing the complete input, or NULL on EOF/error
 */
char *get_input_complete(FILE *in);

/**
 * @brief Read unified input from file stream
 *
 * Primary input function that handles both interactive and non-interactive
 * input with proper multiline continuation detection.
 *
 * @param in Input file stream to read from
 * @return Allocated string containing the input, or NULL on EOF/error
 */
char *get_unified_input(FILE *in);

/**
 * @brief Read complete input using line editor with continuation
 *
 * @deprecated Use get_unified_input() instead
 *
 * Legacy function that reads input interactively with multiline
 * continuation support.
 *
 * @return Allocated string containing the complete input, or NULL on EOF/error
 */
char *ln_gets_complete(void);

/**
 * @brief Get the current continuation prompt string
 *
 * Returns the appropriate prompt to display for multiline input
 * continuation based on the current parsing context.
 *
 * @return Continuation prompt string (e.g., "> ", "quote> ")
 */
const char *lush_get_current_continuation_prompt(void);

#endif
