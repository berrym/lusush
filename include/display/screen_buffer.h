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
 * Represents a line prefix (e.g., continuation prompt)
 * 
 * Prefixes are rendered before line content and tracked separately for
 * efficient updates. Used for continuation prompts and future features
 * like autosuggestions.
 */
typedef struct {
    char *text;              // Prefix text (e.g., "> ", "loop> ")
    size_t length;           // Length in bytes
    size_t visual_width;     // Visual width in columns (excluding ANSI codes)
    bool contains_ansi;      // True if prefix contains ANSI escape codes
    bool dirty;              // True if prefix changed since last render
} screen_line_prefix_t;

/**
 * Represents one line in the virtual screen
 */
typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];
    int length;              // Number of characters in this line
    bool dirty;              // True if line content changed since last render
    
    screen_line_prefix_t *prefix;  // Optional prefix (NULL if none)
    bool prefix_dirty;       // True if prefix changed since last render
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
 * Note: This does NOT free line prefixes. Prefixes persist across clears.
 * Use screen_buffer_cleanup() to free all resources.
 * 
 * @param buffer Buffer to clear
 */
void screen_buffer_clear(screen_buffer_t *buffer);

/**
 * Cleanup screen buffer and free all resources
 * 
 * Frees all line prefixes and resets the buffer to empty state.
 * The buffer can be reused after calling screen_buffer_init() again.
 * 
 * @param buffer Buffer to cleanup
 */
void screen_buffer_cleanup(screen_buffer_t *buffer);

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

// ============================================================================
// PREFIX SUPPORT FUNCTIONS (Phase 2: Continuation Prompts)
// ============================================================================

/**
 * Set prefix for a line (e.g., continuation prompt)
 * 
 * The prefix is rendered before the line content. Prefixes are tracked
 * separately from content for efficient updates (independent dirty tracking).
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @param prefix_text Prefix text (will be copied, can be freed after call)
 * @return true on success, false on error (invalid line, allocation failure)
 */
bool screen_buffer_set_line_prefix(
    screen_buffer_t *buffer,
    int line_num,
    const char *prefix_text
);

/**
 * Clear prefix for a line
 * 
 * Removes and frees the prefix for the specified line.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @return true on success, false on error (invalid line)
 */
bool screen_buffer_clear_line_prefix(screen_buffer_t *buffer, int line_num);

/**
 * Get prefix text for a line
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @return Prefix text (NULL if no prefix or invalid line)
 */
const char *screen_buffer_get_line_prefix(
    const screen_buffer_t *buffer,
    int line_num
);

/**
 * Get visual width of line prefix
 * 
 * Returns the visual width of the prefix in columns, accounting for
 * ANSI escape sequences, UTF-8, wide characters, and tabs.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @return Visual width in columns (0 if no prefix or invalid line)
 */
size_t screen_buffer_get_line_prefix_visual_width(
    const screen_buffer_t *buffer,
    int line_num
);

/**
 * Check if line prefix is dirty
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @return true if prefix changed since last render, false otherwise
 */
bool screen_buffer_is_line_prefix_dirty(
    const screen_buffer_t *buffer,
    int line_num
);

/**
 * Clear line prefix dirty flag
 * 
 * Marks the prefix as clean (rendered). Note: This does NOT clear the
 * content dirty flag - they are tracked independently.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 */
void screen_buffer_clear_line_prefix_dirty(screen_buffer_t *buffer, int line_num);

/**
 * Translate buffer column to display column
 * 
 * Translates a column position in the line content (buffer space) to
 * the corresponding column position on the display (display space),
 * accounting for the prefix width.
 * 
 * Example: If prefix is "loop> " (6 columns), buffer column 5 maps to
 *          display column 11.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @param buffer_col Column in buffer space (0-based)
 * @return Column in display space, or -1 on error
 */
int screen_buffer_translate_buffer_to_display_col(
    const screen_buffer_t *buffer,
    int line_num,
    int buffer_col
);

/**
 * Translate display column to buffer column
 * 
 * Translates a column position on the display (display space) to the
 * corresponding column position in the line content (buffer space),
 * accounting for the prefix width.
 * 
 * If the display column is within the prefix area, returns 0 (start of content).
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @param display_col Column in display space (0-based)
 * @return Column in buffer space, or -1 on error
 */
int screen_buffer_translate_display_to_buffer_col(
    const screen_buffer_t *buffer,
    int line_num,
    int display_col
);

/**
 * Render a single line with prefix into a string
 * 
 * Renders the prefix (if present) followed by the line content into
 * the provided output buffer.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return true on success, false on error (buffer too small, invalid line)
 */
bool screen_buffer_render_line_with_prefix(
    const screen_buffer_t *buffer,
    int line_num,
    char *output,
    size_t output_size
);

/**
 * Render multiple lines with prefixes into a string
 * 
 * Renders a range of lines (each with prefix if present) into the
 * provided output buffer, separated by newlines.
 * 
 * @param buffer Screen buffer
 * @param start_line First line to render (0-based, inclusive)
 * @param num_lines Number of lines to render
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return true on success, false on error (buffer too small, invalid range)
 */
bool screen_buffer_render_multiline_with_prefixes(
    const screen_buffer_t *buffer,
    int start_line,
    int num_lines,
    char *output,
    size_t output_size
);

/**
 * Calculate visual width of text with ANSI, UTF-8, wide chars, and tabs
 * 
 * This is an enhanced version of screen_buffer_visual_width() that also
 * handles tab expansion.
 * 
 * @param text Text to measure
 * @param start_col Starting column position (for tab expansion)
 * @return Visual width in columns
 */
size_t screen_buffer_calculate_visual_width(
    const char *text,
    size_t start_col
);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_BUFFER_H */
