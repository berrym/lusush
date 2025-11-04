/*
 * Lusush Shell - Screen Buffer Management
 * 
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 * 
 * SCREEN BUFFER SYSTEM
 * 
 * Implements virtual screen buffer management for differential terminal updates.
 * This is the proven approach used by ZLE, Fish, and Replxx to handle line
 * wrapping reliably.
 * 
 * Architecture:
 * - Maintains virtual representation of terminal screen state
 * - Renders LLE buffer into virtual screen (prompt + command with wrapping)
 * - Compares old vs new virtual screens to find differences
 * - Generates minimal terminal escape sequences to apply changes
 * 
 * Key Principle: LLE has zero terminal knowledge. Display system handles ALL
 * terminal interaction through screen buffer abstraction.
 */

#ifndef SCREEN_BUFFER_H
#define SCREEN_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS
// ============================================================================

#define SCREEN_BUFFER_MAX_ROWS 100
#define SCREEN_BUFFER_MAX_COLS 512

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

/**
 * Represents a single character cell in the virtual screen
 */
typedef struct {
    char ch;              // Character (UTF-8 byte)
    bool is_prompt;       // True if this cell is part of the prompt
} screen_cell_t;

/**
 * Represents one line in the virtual screen
 */
typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];
    int length;           // Number of characters in this line
    bool dirty;           // True if this line changed since last render
} screen_line_t;

/**
 * Virtual screen buffer
 */
typedef struct {
    screen_line_t lines[SCREEN_BUFFER_MAX_ROWS];
    int num_rows;         // Number of rows currently used
    int terminal_width;   // Terminal width in columns
    int cursor_row;       // Cursor row position (0-based)
    int cursor_col;       // Cursor column position (0-based)
} screen_buffer_t;

/**
 * Represents a change operation for differential update
 */
typedef enum {
    SCREEN_CHANGE_NONE,
    SCREEN_CHANGE_WRITE_TEXT,     // Write text at position
    SCREEN_CHANGE_CLEAR_TO_EOL,   // Clear from position to end of line
    SCREEN_CHANGE_CLEAR_TO_EOS,   // Clear from position to end of screen
    SCREEN_CHANGE_MOVE_CURSOR     // Move cursor to position
} screen_change_type_t;

typedef struct {
    screen_change_type_t type;
    int row;
    int col;
    char text[SCREEN_BUFFER_MAX_COLS];  // Text to write (for WRITE_TEXT)
    int text_len;
} screen_change_t;

/**
 * List of changes to transform one screen state to another
 */
typedef struct {
    screen_change_t changes[SCREEN_BUFFER_MAX_ROWS * 2];  // Max changes
    int num_changes;
} screen_diff_t;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Initialize a screen buffer
 * 
 * @param buffer Buffer to initialize
 * @param terminal_width Terminal width in columns
 */
void screen_buffer_init(screen_buffer_t *buffer, int terminal_width);

/**
 * Clear screen buffer (reset to empty state)
 * 
 * @param buffer Buffer to clear
 */
void screen_buffer_clear(screen_buffer_t *buffer);

/**
 * Render prompt and command into screen buffer
 * 
 * This takes the abstract prompt string and command string and renders them
 * into the screen buffer, handling line wrapping automatically.
 * 
 * @param buffer Buffer to render into
 * @param prompt_text Prompt string (may contain ANSI codes)
 * @param command_text Command string (may contain ANSI codes)
 * @param cursor_byte_offset Cursor position in command_text (byte offset)
 */
void screen_buffer_render(
    screen_buffer_t *buffer,
    const char *prompt_text,
    const char *command_text,
    size_t cursor_byte_offset
);

/**
 * Calculate visual width of text, handling ANSI codes and UTF-8
 * 
 * @param text Text to measure
 * @param byte_length Length in bytes
 * @return Visual width in columns
 */
size_t screen_buffer_visual_width(const char *text, size_t byte_length);

/**
 * Compare two screen buffers and generate diff
 * 
 * @param old_buffer Previous screen state
 * @param new_buffer Desired screen state
 * @param diff Output diff structure
 */
void screen_buffer_diff(
    const screen_buffer_t *old_buffer,
    const screen_buffer_t *new_buffer,
    screen_diff_t *diff
);

/**
 * Apply diff to terminal (write escape sequences)
 * 
 * @param diff Diff to apply
 * @param fd File descriptor to write to (usually STDOUT_FILENO)
 */
void screen_buffer_apply_diff(const screen_diff_t *diff, int fd);

/**
 * Copy screen buffer (for saving old state)
 * 
 * @param dest Destination buffer
 * @param src Source buffer
 */
void screen_buffer_copy(screen_buffer_t *dest, const screen_buffer_t *src);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_BUFFER_H */
