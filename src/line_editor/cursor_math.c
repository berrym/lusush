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
    
    size_t display_width = 0;
    size_t i = 0;
    
    while (i < length) {
        // Handle ANSI escape sequences (zero display width)
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
            continue; // ANSI sequences have zero display width
        }
        
        // Handle UTF-8 multi-byte characters
        unsigned char byte = (unsigned char)text[i];
        size_t char_bytes = 1;
        uint32_t codepoint = 0;
        
        // Determine UTF-8 character byte length and decode codepoint
        if (byte < 0x80) {
            // ASCII (0xxxxxxx)
            codepoint = byte;
            char_bytes = 1;
        } else if ((byte & 0xE0) == 0xC0) {
            // 2-byte UTF-8 (110xxxxx 10xxxxxx)
            if (i + 1 < length && (text[i + 1] & 0xC0) == 0x80) {
                codepoint = ((byte & 0x1F) << 6) | (text[i + 1] & 0x3F);
                char_bytes = 2;
            } else {
                // Invalid UTF-8, treat as single byte
                char_bytes = 1;
                codepoint = byte;
            }
        } else if ((byte & 0xF0) == 0xE0) {
            // 3-byte UTF-8 (1110xxxx 10xxxxxx 10xxxxxx)
            if (i + 2 < length && 
                (text[i + 1] & 0xC0) == 0x80 && 
                (text[i + 2] & 0xC0) == 0x80) {
                codepoint = ((byte & 0x0F) << 12) | 
                           ((text[i + 1] & 0x3F) << 6) | 
                           (text[i + 2] & 0x3F);
                char_bytes = 3;
            } else {
                // Invalid UTF-8, treat as single byte
                char_bytes = 1;
                codepoint = byte;
            }
        } else if ((byte & 0xF8) == 0xF0) {
            // 4-byte UTF-8 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            if (i + 3 < length && 
                (text[i + 1] & 0xC0) == 0x80 && 
                (text[i + 2] & 0xC0) == 0x80 && 
                (text[i + 3] & 0xC0) == 0x80) {
                codepoint = ((byte & 0x07) << 18) | 
                           ((text[i + 1] & 0x3F) << 12) | 
                           ((text[i + 2] & 0x3F) << 6) | 
                           (text[i + 3] & 0x3F);
                char_bytes = 4;
            } else {
                // Invalid UTF-8, treat as single byte
                char_bytes = 1;
                codepoint = byte;
            }
        } else {
            // Invalid UTF-8 start byte, treat as single byte
            char_bytes = 1;
            codepoint = byte;
        }
        
        // Determine display width based on Unicode codepoint
        size_t char_display_width = 1; // Default for most characters
        
        if (codepoint < 0x20) {
            // Control characters (0x00-0x1F) - zero width except tab
            if (codepoint == '\t') {
                char_display_width = 8; // Tab width (could be configurable)
            } else {
                char_display_width = 0; // Other control characters are zero-width
            }
        } else if (codepoint == 0x7F) {
            // DEL character - zero width
            char_display_width = 0;
        } else if (codepoint >= 0x80 && codepoint <= 0x9F) {
            // C1 control characters - zero width
            char_display_width = 0;
        } else if ((codepoint >= 0x0300 && codepoint <= 0x036F) ||  // Combining Diacritical Marks
                   (codepoint >= 0x1AB0 && codepoint <= 0x1AFF) ||  // Combining Diacritical Marks Extended
                   (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) ||  // Combining Diacritical Marks Supplement
                   (codepoint >= 0x20D0 && codepoint <= 0x20FF) ||  // Combining Diacritical Marks for Symbols
                   (codepoint >= 0xFE20 && codepoint <= 0xFE2F)) {  // Combining Half Marks
            // Combining characters - zero width
            char_display_width = 0;
        } else if ((codepoint >= 0x1100 && codepoint <= 0x115F) ||  // Hangul Jamo
                   (codepoint >= 0x2E80 && codepoint <= 0x2EFF) ||  // CJK Radicals Supplement
                   (codepoint >= 0x2F00 && codepoint <= 0x2FDF) ||  // Kangxi Radicals
                   (codepoint >= 0x3000 && codepoint <= 0x303F) ||  // CJK Symbols and Punctuation
                   (codepoint >= 0x3040 && codepoint <= 0x309F) ||  // Hiragana
                   (codepoint >= 0x30A0 && codepoint <= 0x30FF) ||  // Katakana
                   (codepoint >= 0x3100 && codepoint <= 0x312F) ||  // Bopomofo
                   (codepoint >= 0x3130 && codepoint <= 0x318F) ||  // Hangul Compatibility Jamo
                   (codepoint >= 0x3190 && codepoint <= 0x319F) ||  // Kanbun
                   (codepoint >= 0x31A0 && codepoint <= 0x31BF) ||  // Bopomofo Extended
                   (codepoint >= 0x31C0 && codepoint <= 0x31EF) ||  // CJK Strokes
                   (codepoint >= 0x31F0 && codepoint <= 0x31FF) ||  // Katakana Phonetic Extensions
                   (codepoint >= 0x3200 && codepoint <= 0x32FF) ||  // Enclosed CJK Letters and Months
                   (codepoint >= 0x3300 && codepoint <= 0x33FF) ||  // CJK Compatibility
                   (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||  // CJK Unified Ideographs Extension A
                   (codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||  // CJK Unified Ideographs
                   (codepoint >= 0xA000 && codepoint <= 0xA48F) ||  // Yi Syllables
                   (codepoint >= 0xA490 && codepoint <= 0xA4CF) ||  // Yi Radicals
                   (codepoint >= 0xAC00 && codepoint <= 0xD7AF) ||  // Hangul Syllables
                   (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||  // CJK Compatibility Ideographs
                   (codepoint >= 0xFE10 && codepoint <= 0xFE19) ||  // Vertical Forms
                   (codepoint >= 0xFE30 && codepoint <= 0xFE4F) ||  // CJK Compatibility Forms
                   (codepoint >= 0xFE50 && codepoint <= 0xFE6F) ||  // Small Form Variants
                   (codepoint >= 0xFF00 && codepoint <= 0xFF60) ||  // Fullwidth Forms
                   (codepoint >= 0xFFE0 && codepoint <= 0xFFE6) ||  // Fullwidth Forms
                   (codepoint >= 0x20000 && codepoint <= 0x2FFFD) || // CJK Unified Ideographs Extension B-E
                   (codepoint >= 0x30000 && codepoint <= 0x3FFFD)) { // CJK Unified Ideographs Extension F
            // Double-width characters (CJK, fullwidth)
            char_display_width = 2;
        } else if (codepoint >= 0x1F600 && codepoint <= 0x1F64F) {
            // Emoticons - typically double-width in terminals
            char_display_width = 2;
        } else if (codepoint >= 0x1F300 && codepoint <= 0x1F5FF) {
            // Miscellaneous Symbols and Pictographs - typically double-width
            char_display_width = 2;
        } else if (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) {
            // Transport and Map Symbols - typically double-width
            char_display_width = 2;
        } else {
            // Regular single-width character
            char_display_width = 1;
        }
        
        display_width += char_display_width;
        i += char_bytes;
    }
    
    return display_width;
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
    
    // Comprehensive debug output for cursor math calculation
    fprintf(stderr, "[CURSOR_MATH] INPUTS: buffer->length=%zu, buffer->cursor_pos=%zu, prompt_width=%zu, terminal_width=%zu\n",
           buffer->length, buffer->cursor_pos, prompt_width, geometry->width);
    fprintf(stderr, "[CURSOR_MATH] TEXT_ANALYSIS: bytes_to_cursor=%zu, display_width=%zu\n",
           buffer->cursor_pos, text_width);
    
    // Total display width including prompt
    size_t total_width = prompt_width + text_width;
    
    fprintf(stderr, "[CURSOR_MATH] CALCULATION: prompt_width=%zu + text_width=%zu = total_width=%zu\n", 
           prompt_width, text_width, total_width);
    
    // Calculate relative position (within the prompt/input area)
    if (geometry->width == 0) {
        result.valid = false;
        return result;
    }
    
    // Handle special case where total_width is exactly at terminal width boundary
    if (total_width > 0 && total_width % geometry->width == 0) {
        // Cursor wraps to start of next line
        result.relative_row = total_width / geometry->width;
        result.relative_col = 0;
        result.at_boundary = true;
    } else {
        // Normal case: calculate position within line
        result.relative_row = total_width / geometry->width;
        result.relative_col = total_width % geometry->width;
        result.at_boundary = false;
    }
    
    fprintf(stderr, "[CURSOR_MATH] POSITION_CALC: relative_row=%zu, relative_col=%zu, at_boundary=%s\n",
           result.relative_row, result.relative_col, result.at_boundary ? "true" : "false");
    
    // For line editor, absolute position is same as relative position
    // The display system handles positioning relative to prompt location
    result.absolute_row = result.relative_row;
    result.absolute_col = result.relative_col;
    
    fprintf(stderr, "[CURSOR_MATH] RELATIVE_MODE: absolute_row=%zu, absolute_col=%zu (relative to prompt, not terminal origin)\n",
           result.absolute_row, result.absolute_col);
    
    // Additional boundary condition validation
    if (result.at_boundary && result.relative_col != 0) {
        fprintf(stderr, "[CURSOR_MATH] BOUNDARY_ERROR: at_boundary=true but col=%zu != 0 (should be at start of next line)\n",
               result.relative_col);
        result.valid = false;
        return result;
    }
    
    // Validate calculated position is reasonable for text input area
    // Allow reasonable number of wrapped lines for text input
    size_t max_text_lines = geometry->height - 2; // Leave space for prompt and status
    if (result.absolute_row >= max_text_lines || 
        result.absolute_col >= geometry->width) {
        fprintf(stderr, "[CURSOR_MATH] VALIDATION_FAILED: position out of bounds (row=%zu >= max_lines=%zu OR col=%zu >= width=%zu)\n",
               result.absolute_row, max_text_lines, result.absolute_col, geometry->width);
        result.valid = false;
        return result;
    }
    
    fprintf(stderr, "[CURSOR_MATH] Position calculated successfully: row=%zu, col=%zu\n", 
           result.absolute_row, result.absolute_col);
    result.valid = true;
    return result;
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
    size_t prompt_start_col)
{
    lle_terminal_coordinates_t result = {0};
    
    // Input validation
    if (!relative_pos || !relative_pos->valid) {
        result.valid = false;
        return result;
    }
    
    // Convert relative coordinates to absolute terminal coordinates
    result.terminal_row = prompt_start_row + relative_pos->absolute_row;
    result.terminal_col = prompt_start_col + relative_pos->absolute_col;
    result.valid = true;
    
    return result;
}

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
    const lle_terminal_geometry_t *geometry)
{
    lle_cursor_position_t result = {0};
    
    // Input validation
    if (!terminal_coords || !terminal_coords->valid || !geometry) {
        result.valid = false;
        return result;
    }
    
    // Validate terminal coordinates are within bounds
    if (terminal_coords->terminal_row >= geometry->height ||
        terminal_coords->terminal_col >= geometry->width) {
        result.valid = false;
        return result;
    }
    
    // Convert absolute coordinates back to relative coordinates
    if (terminal_coords->terminal_row >= prompt_start_row &&
        terminal_coords->terminal_col >= prompt_start_col) {
        
        result.absolute_row = terminal_coords->terminal_row - prompt_start_row;
        result.absolute_col = terminal_coords->terminal_col - prompt_start_col;
        result.relative_row = result.absolute_row;
        result.relative_col = result.absolute_col;
        result.at_boundary = false; // Would need more context to determine this
        result.valid = true;
    } else {
        // Coordinates are before prompt start - invalid
        result.valid = false;
    }
    
    return result;
}

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
    const lle_prompt_geometry_t *prompt_geometry)
{
    lle_terminal_coordinates_t result = {0};
    
    // Input validation
    if (!prompt_geometry || !lle_validate_prompt_geometry(prompt_geometry)) {
        result.valid = false;
        return result;
    }
    
    // For single-line prompts, content starts immediately after prompt
    if (prompt_geometry->height == 1) {
        result.terminal_row = prompt_start_row;
        result.terminal_col = prompt_start_col + prompt_geometry->last_line_width;
        result.valid = true;
    }
    // For multi-line prompts, content starts after the last line of prompt
    else {
        result.terminal_row = prompt_start_row + prompt_geometry->height - 1;
        result.terminal_col = prompt_start_col + prompt_geometry->last_line_width;
        result.valid = true;
    }
    
    return result;
}

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
    const lle_terminal_geometry_t *geometry)
{
    // Input validation
    if (!coords || !geometry) {
        return false;
    }
    
    // Check if coordinates are marked as valid
    if (!coords->valid) {
        return false;
    }
    
    // Check bounds against terminal geometry
    if (coords->terminal_row >= geometry->height ||
        coords->terminal_col >= geometry->width) {
        return false;
    }
    
    return true;
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
    
    // Use same robust boundary detection as main function
    if (total_width > 0 && total_width % geometry->width == 0) {
        result.relative_row = total_width / geometry->width;
        result.relative_col = 0;
        result.at_boundary = true;
    } else {
        result.relative_row = total_width / geometry->width;
        result.relative_col = total_width % geometry->width;
        result.at_boundary = false;
    }
    
    // Use relative positioning - absolute same as relative for line editor
    result.absolute_row = result.relative_row;
    result.absolute_col = result.relative_col;
    
    // Validate calculated position for text input area
    size_t max_text_lines = geometry->height - 2;
    if (result.absolute_row >= max_text_lines || 
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
    
    // Handle boundary positions correctly - boundary positions represent end of line
    
    // Calculate total character position from row/column
    size_t total_char_pos;
    if (target_pos->at_boundary && target_pos->relative_col == 0) {
        // Special case: boundary position at start of line represents end of previous line
        total_char_pos = target_pos->relative_row * geometry->width;
    } else {
        total_char_pos = target_pos->relative_row * geometry->width + target_pos->relative_col;
    }
    
    // Subtract prompt width to get text position
    if (total_char_pos < prompt_width) {
        return 0; // Position is within prompt area
    }
    
    size_t text_pos = total_char_pos - prompt_width;
    
    // Convert character position to byte position for UTF-8 text
    if (text_pos == 0) {
        return 0;
    }
    
    // For UTF-8 text, we need to iterate through characters to find byte position
    size_t char_count = 0;
    size_t byte_pos = 0;
    
    while (byte_pos < buffer->length && char_count < text_pos) {
        unsigned char byte = (unsigned char)buffer->buffer[byte_pos];
        size_t char_bytes = 1;
        
        // Determine UTF-8 character byte length
        if (byte < 0x80) {
            // ASCII (0xxxxxxx)
            char_bytes = 1;
        } else if ((byte & 0xE0) == 0xC0) {
            // 2-byte UTF-8 (110xxxxx 10xxxxxx)
            char_bytes = 2;
        } else if ((byte & 0xF0) == 0xE0) {
            // 3-byte UTF-8 (1110xxxx 10xxxxxx 10xxxxxx)
            char_bytes = 3;
        } else if ((byte & 0xF8) == 0xF0) {
            // 4-byte UTF-8 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            char_bytes = 4;
        } else {
            // Invalid UTF-8 start byte, treat as single byte
            char_bytes = 1;
        }
        
        // Ensure we don't go beyond buffer bounds
        if (byte_pos + char_bytes > buffer->length) {
            break;
        }
        
        byte_pos += char_bytes;
        char_count++;
    }
    
    return byte_pos;
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
    
    // Position requires wrap if:
    // 1. It's at the rightmost column (would wrap on next character), OR
    // 2. It's already at a boundary (wrapped position)
    return (pos->relative_col >= geometry->width - 1) || pos->at_boundary;
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
    // Delegate to the main robust implementation
    // This function is kept for API compatibility
    return lle_calculate_display_width(text, length);
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