/**
 * @file cursor_math.c
 * @brief Lusush Line Editor - Cursor Mathematics Implementation
 *
 * Mathematical framework for cursor positioning and calculations with support
 * for multiline prompts, terminal geometry, and precise cursor positioning.
 * This module provides the core mathematical functions for accurate cursor
 * positioning across different terminal sizes and prompt configurations.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "cursor_math.h"
#include "text_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Calculate the display width of text considering ANSI escape sequences
 *
 * Helper function to calculate the actual display width of text, which may
 * contain ANSI escape sequences that don't contribute to visual width.
 * For now, this is a simplified implementation that assumes no ANSI codes.
 *
 * @param text Pointer to text string
 * @param length Length of text in bytes
 * @return Display width in characters
 *
 * @note Future enhancement: Full ANSI escape sequence parsing
 */
static size_t lle_calculate_display_width(const char *text, size_t length) {
    if (!text || length == 0) {
        return 0;
    }
    
    // For ASCII text, byte length equals character length
    // TODO: Enhanced implementation for UTF-8 and ANSI escape sequences
    return length;
}

/**
 * @brief Calculate cursor position within the terminal
 *
 * Calculates both absolute and relative cursor positions based on the current
 * text buffer state, terminal geometry, and prompt configuration. This function
 * implements the core mathematical algorithms for cursor positioning.
 *
 * @param buffer Pointer to text buffer containing current text and cursor
 * @param geometry Pointer to terminal geometry information
 * @param prompt_width Width of the prompt in characters
 * @return Cursor position structure with calculated coordinates
 *
 * Mathematical approach:
 * 1. Calculate total text width including prompt
 * 2. Determine line wrapping based on terminal width
 * 3. Calculate absolute position from terminal origin
 * 4. Calculate relative position from prompt start
 * 5. Detect boundary conditions for line wrapping
 */
lle_cursor_position_t lle_calculate_cursor_position(
    const lle_text_buffer_t *buffer,
    const lle_terminal_geometry_t *geometry,
    size_t prompt_width
) {
    lle_cursor_position_t result = {0};
    
    // Input validation
    if (!buffer || !geometry || !lle_validate_terminal_geometry(geometry)) {
        result.valid = false;
        return result;
    }
    
    // Validate buffer state
    if (!lle_text_buffer_is_valid(buffer)) {
        result.valid = false;
        return result;
    }
    
    // Validate prompt width doesn't exceed terminal width
    if (prompt_width > geometry->width) {
        result.valid = false;
        return result;
    }
    
    // Calculate display width of text up to cursor position
    size_t text_width = lle_calculate_display_width(buffer->buffer, buffer->cursor_pos);
    
    // Total display width including prompt
    size_t total_width = prompt_width + text_width;
    
    // Calculate relative position (within the prompt/input area)
    if (geometry->width == 0) {
        result.valid = false;
        return result;
    }
    
    result.relative_row = total_width / geometry->width;
    result.relative_col = total_width % geometry->width;
    
    // Calculate absolute position (from terminal origin)
    // Assume prompt starts at top of terminal for now
    // TODO: Enhanced implementation will consider prompt_height offset
    result.absolute_row = result.relative_row;
    result.absolute_col = result.relative_col;
    
    // Detect boundary conditions - at boundary when we're at column 0 due to wrapping
    result.at_boundary = (result.relative_col == 0 && result.relative_row > 0);
    
    // Validate calculated position is within terminal bounds
    if (result.absolute_row >= geometry->height || 
        result.absolute_col >= geometry->width) {
        result.valid = false;
        return result;
    }
    
    result.valid = true;
    return result;
}

/**
 * @brief Validate cursor position structure
 *
 * Validates that a cursor position structure contains mathematically
 * consistent and reasonable values. Checks invariants and constraints.
 *
 * @param pos Pointer to cursor position structure to validate
 * @return true if position is valid, false otherwise
 *
 * Validation checks:
 * - Structure pointer is not NULL
 * - Absolute coordinates are reasonable (< 10000)
 * - Relative coordinates are reasonable (< 10000)
 * - Mathematical invariants hold (absolute >= relative)
 * - Valid flag consistency
 */
bool lle_validate_cursor_position(const lle_cursor_position_t *pos) {
    if (!pos) {
        return false;
    }
    
    // If marked as invalid, should be considered invalid
    if (!pos->valid) {
        return false;
    }
    
    // Check for reasonable bounds (prevent overflow/corruption)
    const size_t MAX_REASONABLE_COORD = 10000;
    if (pos->absolute_row >= MAX_REASONABLE_COORD ||
        pos->absolute_col >= MAX_REASONABLE_COORD ||
        pos->relative_row >= MAX_REASONABLE_COORD ||
        pos->relative_col >= MAX_REASONABLE_COORD) {
        return false;
    }
    
    // Check mathematical invariant: absolute >= relative
    // (absolute coordinates include any offset from prompt positioning)
    if (pos->absolute_row < pos->relative_row) {
        return false;
    }
    
    return true;
}

/**
 * @brief Calculate cursor position for a specific byte offset
 *
 * Helper function to calculate cursor position for any byte offset within
 * the text buffer, not just the current cursor position.
 *
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
) {
    lle_cursor_position_t result = {0};
    
    // Input validation
    if (!buffer || !geometry || !lle_validate_terminal_geometry(geometry)) {
        result.valid = false;
        return result;
    }
    
    // Validate offset is within buffer bounds
    if (byte_offset > buffer->length) {
        result.valid = false;
        return result;
    }
    
    // Calculate display width of text up to specified offset
    size_t text_width = lle_calculate_display_width(buffer->buffer, byte_offset);
    
    // Total display width including prompt
    size_t total_width = prompt_width + text_width;
    
    // Calculate position using same logic as main function
    if (geometry->width == 0) {
        result.valid = false;
        return result;
    }
    
    result.relative_row = total_width / geometry->width;
    result.relative_col = total_width % geometry->width;
    result.absolute_row = result.relative_row;
    result.absolute_col = result.relative_col;
    result.at_boundary = (result.relative_col == 0 && total_width > 0);
    
    // Validate calculated position
    if (result.absolute_row >= geometry->height || 
        result.absolute_col >= geometry->width) {
        result.valid = false;
        return result;
    }
    
    result.valid = true;
    return result;
}

/**
 * @brief Calculate the byte offset that would place cursor at given position
 *
 * Reverse calculation: given a desired cursor position, calculate what
 * byte offset in the text buffer would result in that position.
 *
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
) {
    if (!buffer || !geometry || !target_pos || !target_pos->valid) {
        return SIZE_MAX;
    }
    
    if (!lle_validate_terminal_geometry(geometry)) {
        return SIZE_MAX;
    }
    
    // Calculate total character position from row/column
    size_t total_char_pos = target_pos->relative_row * geometry->width + target_pos->relative_col;
    
    // Subtract prompt width to get text position
    if (total_char_pos < prompt_width) {
        return 0; // Position is within prompt area
    }
    
    size_t text_pos = total_char_pos - prompt_width;
    
    // For ASCII text, character position equals byte position
    // TODO: Enhanced for UTF-8 where this conversion is more complex
    if (text_pos > buffer->length) {
        return buffer->length; // Clamp to end of buffer
    }
    
    return text_pos;
}

/**
 * @brief Check if cursor position requires line wrapping
 *
 * Determines if the current cursor position is at a location where
 * line wrapping would occur, which affects display and navigation.
 *
 * @param pos Pointer to cursor position structure
 * @param geometry Pointer to terminal geometry
 * @return true if position requires wrapping, false otherwise
 */
bool lle_cursor_position_requires_wrap(
    const lle_cursor_position_t *pos,
    const lle_terminal_geometry_t *geometry
) {
    if (!pos || !geometry || !pos->valid) {
        return false;
    }
    
    // Wrapping occurs when cursor is at the rightmost column
    return (pos->relative_col == geometry->width - 1);
}

/**
 * @brief Calculate the number of lines occupied by text
 *
 * Calculates how many terminal lines the current text (including prompt)
 * occupies, which is useful for display and scrolling calculations.
 *
 * @param buffer Pointer to text buffer
 * @param geometry Pointer to terminal geometry
 * @param prompt_width Width of the prompt in characters
 * @return Number of lines occupied (minimum 1)
 */
size_t lle_calculate_text_lines(
    const lle_text_buffer_t *buffer,
    const lle_terminal_geometry_t *geometry,
    size_t prompt_width
) {
    if (!buffer || !geometry || geometry->width == 0) {
        return 1;
    }
    
    // Calculate total display width
    size_t text_width = lle_calculate_display_width(buffer->buffer, buffer->length);
    size_t total_width = prompt_width + text_width;
    
    // Calculate lines needed (minimum 1)
    if (total_width == 0) {
        return 1;
    }
    
    return (total_width + geometry->width - 1) / geometry->width;
}

/**
 * @brief Calculate display width of text with ANSI escape sequences
 *
 * Calculates the actual display width of text that may contain ANSI escape
 * sequences. ANSI sequences don't contribute to visual width and are filtered out.
 *
 * @param text Pointer to text string
 * @param length Length of text in bytes
 * @return Display width in characters (excluding ANSI sequences)
 *
 * @note Enhanced version of lle_calculate_display_width with ANSI support
 */
size_t lle_calculate_display_width_ansi(const char *text, size_t length) {
    if (!text || length == 0) {
        return 0;
    }
    
    size_t display_width = 0;
    size_t i = 0;
    
    while (i < length) {
        // Check for ANSI escape sequence
        if (text[i] == '\033' && i + 1 < length && text[i + 1] == '[') {
            // Skip ANSI escape sequence
            i += 2; // Skip '\033['
            
            // Find the end of the sequence (marked by a letter)
            while (i < length && !((text[i] >= 'A' && text[i] <= 'Z') || 
                                   (text[i] >= 'a' && text[i] <= 'z'))) {
                i++;
            }
            
            if (i < length) {
                i++; // Skip the terminating letter
            }
        } else {
            // Regular character contributes to display width
            display_width++;
            i++;
        }
    }
    
    return display_width;
}

/**
 * @brief Calculate geometry for a prompt string
 *
 * Calculates the geometry of a prompt string, handling multiline prompts
 * and ANSI escape sequences. Determines the width, height, and last line
 * width needed for proper cursor positioning.
 *
 * @param prompt Pointer to null-terminated prompt string
 * @param terminal Pointer to terminal geometry information
 * @return Prompt geometry structure with calculated dimensions
 *
 * Mathematical approach:
 * 1. Split prompt into lines (by newline characters)
 * 2. Calculate display width of each line (handling ANSI)
 * 3. Determine wrapping based on terminal width
 * 4. Calculate total height including wrapped lines
 * 5. Return geometry with width (max), height, and last line width
 */
lle_prompt_geometry_t lle_calculate_prompt_geometry(
    const char *prompt,
    const lle_terminal_geometry_t *terminal
) {
    lle_prompt_geometry_t result = {0, 1, 0}; // Default: height 1
    
    // Input validation
    if (!prompt || !terminal || !lle_validate_terminal_geometry(terminal)) {
        return result;
    }
    
    size_t prompt_len = strlen(prompt);
    if (prompt_len == 0) {
        return result; // Empty prompt: width=0, height=1, last_line_width=0
    }
    
    size_t max_width = 0;
    size_t current_line_start = 0;
    size_t line_count = 0;
    size_t last_line_width = 0;
    
    // Process prompt character by character
    for (size_t i = 0; i <= prompt_len; i++) {
        // End of line or end of string
        if (i == prompt_len || prompt[i] == '\n') {
            size_t line_length = i - current_line_start;
            
            if (line_length > 0) {
                // Calculate display width of this line
                size_t line_display_width = lle_calculate_display_width_ansi(
                    prompt + current_line_start, line_length);
                
                // Handle line wrapping
                if (terminal->width > 0) {
                    size_t lines_for_this_text = line_display_width == 0 ? 1 : 
                        (line_display_width + terminal->width - 1) / terminal->width;
                    line_count += lines_for_this_text;
                    
                    // Track maximum width (before wrapping)
                    if (line_display_width > max_width) {
                        max_width = line_display_width;
                    }
                    
                    // Last line width is the remainder after wrapping
                    last_line_width = line_display_width % terminal->width;
                    if (last_line_width == 0 && line_display_width > 0) {
                        last_line_width = terminal->width;
                    }
                } else {
                    // Zero width terminal - shouldn't happen but handle gracefully
                    line_count++;
                    if (line_length > max_width) {
                        max_width = line_length;
                    }
                    last_line_width = line_length;
                }
            } else {
                // Empty line
                line_count++;
                last_line_width = 0;
            }
            
            current_line_start = i + 1;
        }
    }
    
    // Ensure at least one line
    if (line_count == 0) {
        line_count = 1;
    }
    
    result.width = max_width;
    result.height = line_count;
    result.last_line_width = last_line_width;
    
    return result;
}

/**
 * @brief Validate prompt geometry structure
 *
 * Validates that a prompt geometry structure contains reasonable values
 * and maintains mathematical consistency.
 *
 * @param geometry Pointer to prompt geometry structure
 * @return true if geometry is valid, false otherwise
 */
bool lle_validate_prompt_geometry(const lle_prompt_geometry_t *geometry) {
    if (!geometry) {
        return false;
    }
    
    // Check for reasonable bounds (prevent overflow/corruption)
    const size_t MAX_REASONABLE_PROMPT_SIZE = 10000;
    if (geometry->width >= MAX_REASONABLE_PROMPT_SIZE ||
        geometry->height >= MAX_REASONABLE_PROMPT_SIZE ||
        geometry->last_line_width >= MAX_REASONABLE_PROMPT_SIZE) {
        return false;
    }
    
    // Height must be at least 1
    if (geometry->height == 0) {
        return false;
    }
    
    // last_line_width should not exceed total width (unless width is 0)
    if (geometry->width > 0 && geometry->last_line_width > geometry->width) {
        return false;
    }
    
    return true;
}