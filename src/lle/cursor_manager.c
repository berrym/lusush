/**
 * @file cursor_manager.c
 * @brief LLE Cursor Position Management Implementation
 * 
 * Specification: Spec 03 - Buffer Management, Section 6
 * 
 * Complete implementation of cursor position tracking and movement operations.
 * Manages logical and visual cursor positions with full UTF-8 awareness.
 * 
 * Key Features:
 * - Multi-dimensional position tracking (byte, codepoint, grapheme, line, visual)
 * - Efficient position calculation
 * - Sticky column support for vertical movement
 * - Complete validation and correction
 */

#include "lle/buffer_management.h"
#include "lle/utf8_support.h"
#include "lle/unicode_grapheme.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Calculate line and column positions from byte offset
 * 
 * Scans through buffer to find which line the byte offset is in.
 */
static lle_result_t calculate_line_column(lle_cursor_manager_t *manager) {
    if (!manager || !manager->buffer || !manager->buffer->data) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_buffer_t *buffer = manager->buffer;
    size_t byte_offset = manager->position.byte_offset;
    
    /* Scan through buffer to find line containing byte_offset */
    size_t line_number = 0;
    size_t line_start = 0;
    
    for (size_t i = 0; i < byte_offset && i < buffer->length; i++) {
        if (buffer->data[i] == '\n') {
            line_number++;
            line_start = i + 1;
        }
    }
    
    /* Calculate column offsets */
    size_t column_offset = byte_offset - line_start;
    
    /* Calculate column in codepoints */
    size_t column_codepoint = 0;
    if (column_offset > 0) {
        column_codepoint = lle_utf8_count_codepoints(
            buffer->data + line_start, column_offset);
    }
    
    /* Calculate column in graphemes */
    size_t column_grapheme = 0;
    if (column_offset > 0) {
        column_grapheme = lle_utf8_count_graphemes(
            buffer->data + line_start, column_offset);
    }
    
    /* Update position */
    manager->position.line_number = line_number;
    manager->position.column_offset = column_offset;
    manager->position.column_codepoint = column_codepoint;
    manager->position.column_grapheme = column_grapheme;
    
    return LLE_SUCCESS;
}

/**
 * @brief Calculate visual position from logical position
 * 
 * For now, visual position equals logical position (no wrapping).
 * Full implementation would handle line wrapping.
 */
static lle_result_t calculate_visual_position(lle_cursor_manager_t *manager) {
    if (!manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* For now, visual position equals logical position */
    /* Full implementation would calculate wrapping based on terminal width */
    manager->position.visual_line = manager->position.line_number;
    manager->position.visual_column = manager->position.column_grapheme;
    
    return LLE_SUCCESS;
}

/**
 * @brief Convert grapheme index to byte offset
 */
static lle_result_t grapheme_index_to_byte_offset(lle_buffer_t *buffer,
                                                  size_t grapheme_index,
                                                  size_t *byte_offset) {
    if (!buffer || !buffer->data || !byte_offset) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (grapheme_index == 0) {
        *byte_offset = 0;
        return LLE_SUCCESS;
    }
    
    /* Scan through buffer counting graphemes */
    size_t current_grapheme = 0;
    size_t offset = 0;
    const char *data = buffer->data;
    const char *end = data + buffer->length;
    
    while (offset < buffer->length && current_grapheme < grapheme_index) {
        const char *ptr = data + offset;
        
        /* Find next grapheme boundary */
        const char *next = ptr + 1;
        while (next < end && !lle_is_grapheme_boundary(next, data, end)) {
            next++;
        }
        
        offset = next - data;
        current_grapheme++;
    }
    
    *byte_offset = offset;
    return LLE_SUCCESS;
}

/**
 * @brief Convert codepoint index to byte offset
 */
static lle_result_t codepoint_index_to_byte_offset(lle_buffer_t *buffer,
                                                   size_t codepoint_index,
                                                   size_t *byte_offset) {
    if (!buffer || !buffer->data || !byte_offset) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (codepoint_index == 0) {
        *byte_offset = 0;
        return LLE_SUCCESS;
    }
    
    /* Scan through buffer counting codepoints */
    size_t current_codepoint = 0;
    size_t offset = 0;
    
    while (offset < buffer->length && current_codepoint < codepoint_index) {
        int seq_len = lle_utf8_sequence_length((unsigned char)buffer->data[offset]);
        if (seq_len <= 0) {
            return LLE_ERROR_INVALID_ENCODING;
        }
        
        offset += seq_len;
        current_codepoint++;
    }
    
    *byte_offset = offset;
    return LLE_SUCCESS;
}

/* ============================================================================
 * CURSOR MANAGER LIFECYCLE
 * ============================================================================
 */

lle_result_t lle_cursor_manager_init(lle_cursor_manager_t **manager,
                                     lle_buffer_t *buffer) {
    if (!manager || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate cursor manager */
    lle_cursor_manager_t *mgr = (lle_cursor_manager_t *)lle_pool_alloc(
        sizeof(lle_cursor_manager_t));
    if (!mgr) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize all fields */
    memset(mgr, 0, sizeof(lle_cursor_manager_t));
    
    mgr->buffer = buffer;
    mgr->sticky_column = true;
    mgr->preferred_visual_column = 0;
    
    /* Initialize position to start of buffer */
    mgr->position.byte_offset = 0;
    mgr->position.codepoint_index = 0;
    mgr->position.grapheme_index = 0;
    mgr->position.line_number = 0;
    mgr->position.column_offset = 0;
    mgr->position.column_codepoint = 0;
    mgr->position.column_grapheme = 0;
    mgr->position.visual_line = 0;
    mgr->position.visual_column = 0;
    mgr->position.position_valid = true;
    mgr->position.buffer_version = buffer->modification_count;
    
    *manager = mgr;
    return LLE_SUCCESS;
}

lle_result_t lle_cursor_manager_destroy(lle_cursor_manager_t *manager) {
    if (!manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free position cache if allocated */
    if (manager->position_cache) {
        lle_pool_free(manager->position_cache);
    }
    
    /* Free manager */
    lle_pool_free(manager);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * CURSOR MOVEMENT OPERATIONS
 * ============================================================================
 */

lle_result_t lle_cursor_manager_move_to_byte_offset(lle_cursor_manager_t *manager,
                                                    size_t byte_offset) {
    if (!manager || !manager->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (byte_offset > manager->buffer->length) {
        return LLE_ERROR_INVALID_RANGE;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    /* Step 1: Set byte offset */
    manager->position.byte_offset = byte_offset;
    
    /* Step 2: Calculate codepoint index */
    if (byte_offset == 0) {
        manager->position.codepoint_index = 0;
    } else {
        manager->position.codepoint_index = lle_utf8_count_codepoints(
            manager->buffer->data, byte_offset);
    }
    
    /* Step 3: Calculate grapheme index */
    if (byte_offset == 0) {
        manager->position.grapheme_index = 0;
    } else {
        manager->position.grapheme_index = lle_utf8_count_graphemes(
            manager->buffer->data, byte_offset);
    }
    
    /* Step 4: Calculate line and column positions */
    result = calculate_line_column(manager);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Step 5: Calculate visual position */
    result = calculate_visual_position(manager);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Step 6: Update position validity */
    manager->position.position_valid = true;
    manager->position.buffer_version = manager->buffer->modification_count;
    
    /* Step 7: Update preferred visual column if sticky */
    if (manager->sticky_column) {
        manager->preferred_visual_column = manager->position.visual_column;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_cursor_manager_move_by_graphemes(lle_cursor_manager_t *manager,
                                                  int grapheme_delta) {
    if (!manager || !manager->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Calculate target grapheme index */
    int target_grapheme = (int)manager->position.grapheme_index + grapheme_delta;
    
    /* Bounds checking */
    if (target_grapheme < 0) {
        target_grapheme = 0;
    } else if (target_grapheme > (int)manager->buffer->grapheme_count) {
        target_grapheme = (int)manager->buffer->grapheme_count;
    }
    
    /* Convert back to byte offset */
    size_t target_byte_offset;
    lle_result_t result = grapheme_index_to_byte_offset(
        manager->buffer,
        (size_t)target_grapheme,
        &target_byte_offset
    );
    
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    return lle_cursor_manager_move_to_byte_offset(manager, target_byte_offset);
}

lle_result_t lle_cursor_manager_move_by_codepoints(lle_cursor_manager_t *manager,
                                                   int codepoint_delta) {
    if (!manager || !manager->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Calculate target codepoint index */
    int target_codepoint = (int)manager->position.codepoint_index + codepoint_delta;
    
    /* Bounds checking */
    if (target_codepoint < 0) {
        target_codepoint = 0;
    } else if (target_codepoint > (int)manager->buffer->codepoint_count) {
        target_codepoint = (int)manager->buffer->codepoint_count;
    }
    
    /* Convert back to byte offset */
    size_t target_byte_offset;
    lle_result_t result = codepoint_index_to_byte_offset(
        manager->buffer,
        (size_t)target_codepoint,
        &target_byte_offset
    );
    
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    return lle_cursor_manager_move_to_byte_offset(manager, target_byte_offset);
}

lle_result_t lle_cursor_manager_move_to_line_start(lle_cursor_manager_t *manager) {
    if (!manager || !manager->buffer || !manager->buffer->data) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Find start of current line */
    size_t byte_offset = manager->position.byte_offset;
    const char *data = manager->buffer->data;
    
    /* Scan backwards to find newline or start of buffer */
    while (byte_offset > 0 && data[byte_offset - 1] != '\n') {
        byte_offset--;
    }
    
    return lle_cursor_manager_move_to_byte_offset(manager, byte_offset);
}

lle_result_t lle_cursor_manager_move_to_line_end(lle_cursor_manager_t *manager) {
    if (!manager || !manager->buffer || !manager->buffer->data) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Find end of current line */
    size_t byte_offset = manager->position.byte_offset;
    const char *data = manager->buffer->data;
    size_t length = manager->buffer->length;
    
    /* Scan forwards to find newline or end of buffer */
    while (byte_offset < length && data[byte_offset] != '\n') {
        byte_offset++;
    }
    
    return lle_cursor_manager_move_to_byte_offset(manager, byte_offset);
}

lle_result_t lle_cursor_manager_move_by_lines(lle_cursor_manager_t *manager,
                                              int line_delta) {
    if (!manager || !manager->buffer || !manager->buffer->data) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (line_delta == 0) {
        return LLE_SUCCESS;
    }
    
    /* Calculate target line number */
    int target_line = (int)manager->position.line_number + line_delta;
    if (target_line < 0) {
        target_line = 0;
    }
    
    /* Find start of target line */
    const char *data = manager->buffer->data;
    size_t length = manager->buffer->length;
    size_t byte_offset = 0;
    int current_line = 0;
    
    while (byte_offset < length && current_line < target_line) {
        if (data[byte_offset] == '\n') {
            current_line++;
        }
        byte_offset++;
    }
    
    /* Try to restore preferred visual column */
    if (manager->sticky_column) {
        size_t line_start = byte_offset;
        size_t column_graphemes = 0;
        
        /* Move to preferred column on this line */
        while (byte_offset < length && 
               data[byte_offset] != '\n' &&
               column_graphemes < manager->preferred_visual_column) {
            
            /* Find next grapheme boundary */
            const char *ptr = data + byte_offset;
            const char *end = data + length;
            const char *next = ptr + 1;
            
            while (next < end && !lle_is_grapheme_boundary(next, data, end)) {
                next++;
            }
            
            byte_offset = next - data;
            column_graphemes++;
        }
    }
    
    return lle_cursor_manager_move_to_byte_offset(manager, byte_offset);
}

/* ============================================================================
 * CURSOR VALIDATION
 * ============================================================================
 */

lle_result_t lle_cursor_manager_validate_and_correct(lle_cursor_manager_t *manager) {
    if (!manager || !manager->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if position is beyond buffer end */
    if (manager->position.byte_offset > manager->buffer->length) {
        manager->position.byte_offset = manager->buffer->length;
    }
    
    /* Ensure cursor is on a valid UTF-8 boundary */
    const char *data = manager->buffer->data;
    size_t offset = manager->position.byte_offset;
    
    /* Move back to valid UTF-8 start byte if needed */
    while (offset > 0 && offset < manager->buffer->length) {
        unsigned char byte = (unsigned char)data[offset];
        
        /* Check if this is a valid UTF-8 start byte or ASCII */
        if ((byte & 0x80) == 0 ||        /* ASCII */
            (byte & 0xC0) == 0xC0) {     /* UTF-8 start byte */
            break;
        }
        
        /* This is a continuation byte, move back */
        offset--;
    }
    
    manager->position.byte_offset = offset;
    
    /* Recalculate all position fields */
    return lle_cursor_manager_move_to_byte_offset(manager, offset);
}

/* ============================================================================
 * CURSOR QUERY FUNCTIONS
 * ============================================================================
 */

lle_result_t lle_cursor_manager_get_position(const lle_cursor_manager_t *manager,
                                             lle_cursor_position_t *position) {
    if (!manager || !position) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Always return buffer's current cursor position, not cached position */
    /* This ensures cursor manager sees updates from direct buffer operations */
    if (manager->buffer) {
        *position = manager->buffer->cursor;
    } else {
        *position = manager->position;
    }
    return LLE_SUCCESS;
}
