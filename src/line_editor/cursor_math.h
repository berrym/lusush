#ifndef LLE_CURSOR_MATH_H
#define LLE_CURSOR_MATH_H

#include <stddef.h>
#include <stdbool.h>
#include "text_buffer.h"

/**
 * @file cursor_math.h
 * @brief Lusush Line Editor - Cursor Mathematics
 *
 * Mathematical framework for cursor positioning and calculations with support
 * for multiline prompts, terminal geometry, and precise cursor positioning.
 * This module provides the mathematical foundation for accurate cursor
 * positioning across different terminal sizes and prompt configurations.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

/**
 * @brief Maximum reasonable terminal width (characters)
 */
#define LLE_MAX_TERMINAL_WIDTH 500

/**
 * @brief Maximum reasonable terminal height (rows)
 */
#define LLE_MAX_TERMINAL_HEIGHT 200

/**
 * @brief Minimum practical terminal width (characters)
 */
#define LLE_MIN_TERMINAL_WIDTH 20

/**
 * @brief Minimum practical terminal height (rows)
 */
#define LLE_MIN_TERMINAL_HEIGHT 5

/**
 * @brief Default terminal width if detection fails
 */
#define LLE_DEFAULT_TERMINAL_WIDTH 80

/**
 * @brief Default terminal height if detection fails
 */
#define LLE_DEFAULT_TERMINAL_HEIGHT 24

/**
 * @brief Cursor position information with both absolute and relative coordinates
 *
 * This structure contains comprehensive cursor position data including both
 * absolute terminal coordinates and relative positioning within the prompt/input
 * area. The distinction between absolute and relative positioning is critical
 * for multiline prompt support and proper cursor display.
 *
 * Mathematical invariants:
 * - absolute_row >= relative_row (absolute includes prompt offset)
 * - relative_col < terminal_width (wraps at terminal boundaries)
 * - at_boundary is true when cursor is at end of terminal line
 */
typedef struct {
    size_t absolute_row;    /**< Terminal row (0-based from top of terminal) */
    size_t absolute_col;    /**< Terminal column (0-based from left edge) */
    size_t relative_row;    /**< Row relative to prompt start (0-based) */
    size_t relative_col;    /**< Column within current logical line (0-based) */
    bool at_boundary;       /**< True if cursor is at line wrap boundary */
    bool valid;             /**< True if position calculation is valid */
} lle_cursor_position_t;

/**
 * @brief Absolute terminal coordinates for multi-line positioning
 *
 * This structure represents absolute coordinates within the terminal screen,
 * used for converting between relative cursor positions and actual terminal
 * positioning commands. Critical for the architectural rewrite to support
 * proper multi-line cursor positioning.
 *
 * Coordinate system:
 * - terminal_row: 0-based absolute row from top of terminal screen
 * - terminal_col: 0-based absolute column from left edge of terminal
 * - Both coordinates are ready for use with lle_terminal_move_cursor()
 */
typedef struct {
    size_t terminal_row;          /**< Absolute terminal row (0-based) */
    size_t terminal_col;          /**< Absolute terminal column (0-based) */
    bool valid;                   /**< True if coordinates are valid */
} lle_terminal_coordinates_t;

/**
 * @brief Terminal geometry and prompt layout information
 *
 * This structure contains all geometric information needed for cursor
 * position calculations, including terminal dimensions and prompt layout.
 * The prompt dimensions are critical for calculating relative positioning
 * and handling multiline prompts correctly.
 *
 * Mathematical constraints:
 * - width >= LLE_MIN_TERMINAL_WIDTH
 * - height >= LLE_MIN_TERMINAL_HEIGHT
 * - prompt_width <= width (prompt cannot exceed terminal width)
 * - prompt_height <= height (prompt cannot exceed terminal height)
 */
typedef struct {
    size_t width;           /**< Terminal width in characters */
    size_t height;          /**< Terminal height in rows */
    size_t prompt_width;    /**< Display width of current prompt (last line) */
    size_t prompt_height;   /**< Number of rows occupied by prompt */
} lle_terminal_geometry_t;

/**
 * @brief Prompt geometry information for multiline prompt layout
 *
 * This structure contains calculated geometry information for prompts,
 * including multiline prompts and prompts with ANSI escape sequences.
 * Used to determine proper positioning and layout for display.
 */
typedef struct {
    size_t width;           /**< Display width of prompt (widest line) */
    size_t height;          /**< Number of lines occupied by prompt */
    size_t last_line_width; /**< Display width of last prompt line */
} lle_prompt_geometry_t;

// Function declarations will be added as tasks are implemented

// Task LLE-007: Basic cursor position calculation functions
/**
 * @brief Calculate cursor position within the terminal
 * @param buffer Pointer to text buffer containing current text and cursor
 * @param geometry Pointer to terminal geometry information
 * @param prompt_width Width of the prompt in characters
 * @return Cursor position structure with calculated coordinates
 */
lle_cursor_position_t lle_calculate_cursor_position(
    const lle_text_buffer_t *buffer,
    const lle_terminal_geometry_t *geometry,
    size_t prompt_width
);

/**
 * @brief Validate cursor position structure
 * @param pos Pointer to cursor position structure to validate
 * @return true if position is valid, false otherwise
 */
bool lle_validate_cursor_position(const lle_cursor_position_t *pos);

/**
 * @brief Calculate cursor position for a specific byte offset
 * @param buffer Pointer to text buffer
 * @param geometry Pointer to terminal geometry
 * @param prompt_width Width of the prompt in characters
 * @param byte_offset Byte offset within the buffer (0-based)
 * @return Cursor position structure for the specified offset
 */
lle_cursor_position_t lle_calculate_cursor_position_at_offset(
    const lle_text_buffer_t *buffer,
    const lle_terminal_geometry_t *geometry,
    size_t prompt_width,
    size_t byte_offset
);

/**
 * @brief Calculate the byte offset that would place cursor at given position
 * @param buffer Pointer to text buffer
 * @param geometry Pointer to terminal geometry
 * @param prompt_width Width of the prompt in characters
 * @param target_pos Desired cursor position
 * @return Byte offset that would result in target position, or SIZE_MAX if impossible
 */
size_t lle_calculate_offset_for_position(
    const lle_text_buffer_t *buffer,
    const lle_terminal_geometry_t *geometry,
    size_t prompt_width,
    const lle_cursor_position_t *target_pos
);

/**
 * @brief Check if cursor position requires line wrapping
 * @param pos Pointer to cursor position structure
 * @param geometry Pointer to terminal geometry
 * @return true if position requires wrapping, false otherwise
 */
bool lle_cursor_position_requires_wrap(
    const lle_cursor_position_t *pos,
    const lle_terminal_geometry_t *geometry
);

/**
 * @brief Calculate the number of lines occupied by text
 * @param buffer Pointer to text buffer
 * @param geometry Pointer to terminal geometry
 * @param prompt_width Width of the prompt in characters
 * @return Number of lines occupied (minimum 1)
 */
size_t lle_calculate_text_lines(
    const lle_text_buffer_t *buffer,
    const lle_terminal_geometry_t *geometry,
    size_t prompt_width
);

// Task LLE-008: Prompt geometry calculation functions
/**
 * @brief Calculate geometry for a prompt string
 * @param prompt Pointer to null-terminated prompt string
 * @param terminal Pointer to terminal geometry information
 * @return Prompt geometry structure with calculated dimensions
 */
lle_prompt_geometry_t lle_calculate_prompt_geometry(
    const char *prompt,
    const lle_terminal_geometry_t *terminal
);

/**
 * @brief Calculate display width of text with ANSI escape sequences
 * @param text Pointer to text string
 * @param length Length of text in bytes
 * @return Display width in characters (excluding ANSI sequences)
 */
size_t lle_calculate_display_width_ansi(const char *text, size_t length);

/**
 * @brief Validate prompt geometry structure
 * @param geometry Pointer to prompt geometry structure
 * @return true if geometry is valid, false otherwise
 */
bool lle_validate_prompt_geometry(const lle_prompt_geometry_t *geometry);


/**
 * @brief Validate terminal geometry structure
 *
 * Helper function to check if terminal geometry values are within
 * reasonable bounds and mathematically consistent.
 *
 * @param geometry Pointer to terminal geometry structure
 * @return true if geometry is valid, false otherwise
 */
static inline bool lle_validate_terminal_geometry(const lle_terminal_geometry_t *geometry) {
    if (!geometry) {
        return false;
    }
    
    // Check basic bounds
    if (geometry->width < LLE_MIN_TERMINAL_WIDTH || 
        geometry->width > LLE_MAX_TERMINAL_WIDTH ||
        geometry->height < LLE_MIN_TERMINAL_HEIGHT || 
        geometry->height > LLE_MAX_TERMINAL_HEIGHT) {
        return false;
    }
    
    // Check prompt constraints
    if (geometry->prompt_width > geometry->width ||
        geometry->prompt_height > geometry->height) {
        return false;
    }
    
    return true;
}

/**
 * @brief Initialize terminal geometry with default values
 *
 * Helper function to initialize a terminal geometry structure with
 * safe default values.
 *
 * @param geometry Pointer to terminal geometry structure to initialize
 */
static inline void lle_init_terminal_geometry(lle_terminal_geometry_t *geometry) {
    if (!geometry) {
        return;
    }
    
    geometry->width = LLE_DEFAULT_TERMINAL_WIDTH;
    geometry->height = LLE_DEFAULT_TERMINAL_HEIGHT;
    geometry->prompt_width = 0;
    geometry->prompt_height = 1;
}

// Phase 1A: Coordinate Conversion Functions (Architecture Rewrite)

/**
 * @brief Convert relative cursor position to absolute terminal coordinates
 *
 * Converts a cursor position relative to the prompt/input area into absolute
 * terminal coordinates that can be used with lle_terminal_move_cursor().
 * This is the core function for the architectural rewrite to support proper
 * multi-line cursor positioning.
 *
 * @param relative_pos Relative cursor position from cursor math calculations
 * @param prompt_start_row Absolute row where prompt starts (0-based)
 * @param prompt_start_col Absolute column where prompt starts (0-based)
 * @return Absolute terminal coordinates ready for terminal positioning
 */
lle_terminal_coordinates_t lle_convert_to_terminal_coordinates(
    const lle_cursor_position_t *relative_pos,
    size_t prompt_start_row,
    size_t prompt_start_col);

/**
 * @brief Convert absolute terminal coordinates to relative cursor position
 *
 * Converts absolute terminal coordinates back to relative cursor position
 * within the prompt/input area. Used for converting terminal cursor queries
 * back to internal cursor representation.
 *
 * @param terminal_coords Absolute terminal coordinates
 * @param prompt_start_row Absolute row where prompt starts (0-based)
 * @param prompt_start_col Absolute column where prompt starts (0-based)
 * @param geometry Terminal geometry for boundary checking
 * @return Relative cursor position within prompt/input area
 */
lle_cursor_position_t lle_convert_from_terminal_coordinates(
    const lle_terminal_coordinates_t *terminal_coords,
    size_t prompt_start_row,
    size_t prompt_start_col,
    const lle_terminal_geometry_t *geometry);

/**
 * @brief Calculate absolute terminal coordinates for text content start
 *
 * Calculates where text content begins in absolute terminal coordinates,
 * accounting for prompt positioning and multi-line prompts. This is used
 * to establish the reference point for all text positioning operations.
 *
 * @param prompt_start_row Absolute row where prompt starts (0-based)
 * @param prompt_start_col Absolute column where prompt starts (0-based)
 * @param prompt_geometry Prompt geometry information
 * @return Absolute coordinates where text content should start
 */
lle_terminal_coordinates_t lle_calculate_content_start_coordinates(
    size_t prompt_start_row,
    size_t prompt_start_col,
    const lle_prompt_geometry_t *prompt_geometry);

/**
 * @brief Validate terminal coordinates structure
 *
 * Validates that terminal coordinates are within reasonable bounds
 * and ready for use with terminal positioning functions.
 *
 * @param coords Terminal coordinates to validate
 * @param geometry Terminal geometry for boundary checking
 * @return true if coordinates are valid, false otherwise
 */
bool lle_validate_terminal_coordinates(
    const lle_terminal_coordinates_t *coords,
    const lle_terminal_geometry_t *geometry);

#endif // LLE_CURSOR_MATH_H
