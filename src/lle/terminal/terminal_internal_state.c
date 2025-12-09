/*
 * terminal_internal_state.c - Internal State Authority Model (Spec 02 Subsystem 2)
 * 
 * CRITICAL DESIGN PRINCIPLE:
 * The internal state is the SINGLE SOURCE OF TRUTH for all editing operations.
 * NEVER query terminal state - internal model is authoritative.
 * 
 * This module maintains:
 * - Command buffer (authoritative text storage)
 * - Logical cursor position in buffer
 * - Display state model (what we believe terminal contains)
 * - Display geometry and scroll offsets
 * 
 * Spec 02: Terminal Abstraction - Subsystem 2
 */

#include "lle/terminal_abstraction.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ============================================================================
 * COMMAND BUFFER OPERATIONS
 * ============================================================================ */

/*
 * Initialize command buffer with initial capacity
 */
lle_result_t lle_command_buffer_init(lle_command_buffer_t **buffer,
                                     size_t initial_capacity) {
    if (!buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (initial_capacity < 64) {
        initial_capacity = 64;  /* Minimum capacity */
    }
    
    lle_command_buffer_t *buf = calloc(1, sizeof(lle_command_buffer_t));
    if (!buf) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Allocate buffer data */
    buf->data = calloc(initial_capacity, 1);
    if (!buf->data) {
        free(buf);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    buf->capacity = initial_capacity;
    buf->allocated_size = initial_capacity;
    buf->length = 0;
    buf->last_change_offset = 0;
    buf->last_change_length = 0;
    buf->needs_full_refresh = false;
    
    *buffer = buf;
    return LLE_SUCCESS;
}

/*
 * Destroy command buffer
 */
void lle_command_buffer_destroy(lle_command_buffer_t *buffer) {
    if (!buffer) {
        return;
    }
    
    if (buffer->data) {
        free(buffer->data);
    }
    
    free(buffer);
}

/*
 * Ensure buffer has sufficient capacity for new content
 */
static lle_result_t ensure_capacity(lle_command_buffer_t *buffer, size_t required) {
    if (buffer->capacity >= required) {
        return LLE_SUCCESS;  /* Already have enough capacity */
    }
    
    /* Calculate new capacity (grow by 1.5x) */
    size_t new_capacity = buffer->capacity;
    while (new_capacity < required) {
        new_capacity = new_capacity + (new_capacity / 2);
    }
    
    /* Reallocate buffer */
    char *new_data = realloc(buffer->data, new_capacity);
    if (!new_data) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    buffer->data = new_data;
    buffer->capacity = new_capacity;
    buffer->allocated_size = new_capacity;
    
    return LLE_SUCCESS;
}

/*
 * Insert text into command buffer at specified position
 */
lle_result_t lle_command_buffer_insert(lle_command_buffer_t *buffer,
                                       size_t position,
                                       const char *text,
                                       size_t length) {
    if (!buffer || !text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (position > buffer->length) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (length == 0) {
        return LLE_SUCCESS;  /* Nothing to insert */
    }
    
    /* Ensure we have capacity for new text */
    lle_result_t result = ensure_capacity(buffer, buffer->length + length + 1);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Move existing content after insertion point to make room */
    if (position < buffer->length) {
        memmove(buffer->data + position + length,
                buffer->data + position,
                buffer->length - position);
    }
    
    /* Copy new text into buffer */
    memcpy(buffer->data + position, text, length);
    
    /* Update buffer state */
    buffer->length += length;
    buffer->data[buffer->length] = '\0';  /* Null terminate */
    
    /* Track change for optimization */
    buffer->last_change_offset = position;
    buffer->last_change_length = length;
    buffer->needs_full_refresh = false;
    
    return LLE_SUCCESS;
}

/*
 * Delete text from command buffer at specified position
 */
lle_result_t lle_command_buffer_delete(lle_command_buffer_t *buffer,
                                       size_t position,
                                       size_t length) {
    if (!buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (position >= buffer->length) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (length == 0) {
        return LLE_SUCCESS;  /* Nothing to delete */
    }
    
    /* Clamp length to available content */
    if (position + length > buffer->length) {
        length = buffer->length - position;
    }
    
    /* Move content after deletion point backward */
    if (position + length < buffer->length) {
        memmove(buffer->data + position,
                buffer->data + position + length,
                buffer->length - (position + length));
    }
    
    /* Update buffer state */
    buffer->length -= length;
    buffer->data[buffer->length] = '\0';  /* Null terminate */
    
    /* Track change for optimization */
    buffer->last_change_offset = position;
    buffer->last_change_length = length;
    buffer->needs_full_refresh = false;
    
    return LLE_SUCCESS;
}

/*
 * Clear command buffer
 */
void lle_command_buffer_clear(lle_command_buffer_t *buffer) {
    if (!buffer) {
        return;
    }
    
    buffer->length = 0;
    if (buffer->data) {
        buffer->data[0] = '\0';
    }
    
    buffer->last_change_offset = 0;
    buffer->last_change_length = 0;
    buffer->needs_full_refresh = true;
}

/* ============================================================================
 * INTERNAL STATE OPERATIONS
 * ============================================================================ */

/*
 * Initialize internal state with terminal capabilities
 */
lle_result_t lle_internal_state_init(lle_internal_state_t **state,
                                     lle_terminal_capabilities_t *caps) {
    if (!state || !caps) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_internal_state_t *internal_state = calloc(1, sizeof(lle_internal_state_t));
    if (!internal_state) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize command buffer */
    lle_result_t result = lle_command_buffer_init(&internal_state->command_buffer, 1024);
    if (result != LLE_SUCCESS) {
        free(internal_state);
        return result;
    }
    
    /* Initialize display lines array */
    internal_state->display_capacity = 10;  /* Initial capacity */
    internal_state->display_lines = calloc(internal_state->display_capacity, 
                                          sizeof(lle_display_line_t));
    if (!internal_state->display_lines) {
        lle_command_buffer_destroy(internal_state->command_buffer);
        free(internal_state);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Allocate content for each display line */
    for (size_t i = 0; i < internal_state->display_capacity; i++) {
        internal_state->display_lines[i].capacity = 256;
        internal_state->display_lines[i].content = calloc(256, 1);
        if (!internal_state->display_lines[i].content) {
            /* Clean up previously allocated lines */
            for (size_t j = 0; j < i; j++) {
                free(internal_state->display_lines[j].content);
            }
            free(internal_state->display_lines);
            lle_command_buffer_destroy(internal_state->command_buffer);
            free(internal_state);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        internal_state->display_lines[i].length = 0;
        internal_state->display_lines[i].contains_cursor = false;
    }
    
    /* Set initial terminal geometry from capabilities */
    internal_state->terminal_width = caps->terminal_width > 0 ? caps->terminal_width : 80;
    internal_state->terminal_height = caps->terminal_height > 0 ? caps->terminal_height : 24;
    
    /* Initialize state */
    internal_state->cursor_position = 0;
    internal_state->display_line_count = 0;
    internal_state->display_offset = 0;
    internal_state->vertical_offset = 0;
    internal_state->prompt_width = 0;
    internal_state->selection_start = 0;
    internal_state->selection_end = 0;
    internal_state->has_selection = false;
    internal_state->buffer_modified = false;
    internal_state->modification_count = 0;
    internal_state->last_update_time = lle_get_current_time_microseconds();
    
    *state = internal_state;
    return LLE_SUCCESS;
}

/*
 * Destroy internal state
 */
void lle_internal_state_destroy(lle_internal_state_t *state) {
    if (!state) {
        return;
    }
    
    /* Destroy command buffer */
    if (state->command_buffer) {
        lle_command_buffer_destroy(state->command_buffer);
    }
    
    /* Free display lines */
    if (state->display_lines) {
        for (size_t i = 0; i < state->display_capacity; i++) {
            if (state->display_lines[i].content) {
                free(state->display_lines[i].content);
            }
        }
        free(state->display_lines);
    }
    
    free(state);
}

/*
 * Insert text into internal state at current cursor position
 */
lle_result_t lle_internal_state_insert_text(lle_internal_state_t *state,
                                            size_t position,
                                            const char *text,
                                            size_t text_length) {
    if (!state || !text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Insert into command buffer (authoritative) */
    lle_result_t result = lle_command_buffer_insert(state->command_buffer,
                                                   position,
                                                   text,
                                                   text_length);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Update cursor position */
    state->cursor_position = position + text_length;
    
    /* Mark buffer as modified */
    state->buffer_modified = true;
    state->modification_count++;
    state->last_update_time = lle_get_current_time_microseconds();
    
    return LLE_SUCCESS;
}

/*
 * Delete text from internal state
 */
lle_result_t lle_internal_state_delete_text(lle_internal_state_t *state,
                                            size_t position,
                                            size_t length) {
    if (!state) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Delete from command buffer (authoritative) */
    lle_result_t result = lle_command_buffer_delete(state->command_buffer,
                                                   position,
                                                   length);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Update cursor position */
    if (state->cursor_position > position) {
        if (state->cursor_position <= position + length) {
            state->cursor_position = position;
        } else {
            state->cursor_position -= length;
        }
    }
    
    /* Mark buffer as modified */
    state->buffer_modified = true;
    state->modification_count++;
    state->last_update_time = lle_get_current_time_microseconds();
    
    return LLE_SUCCESS;
}

/*
 * Calculate cursor display position from internal state
 * 
 * CRITICAL: This function calculates where cursor SHOULD be based on
 * internal buffer state. We NEVER query terminal for cursor position.
 */
lle_result_t lle_internal_state_calculate_cursor_display_position(
                                    lle_internal_state_t *state,
                                    size_t *display_line,
                                    size_t *display_column) {
    if (!state || !display_line || !display_column) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Calculate visual column accounting for prompt width and line wrapping */
    size_t visual_column = state->prompt_width + state->cursor_position;
    
    /* Account for horizontal scroll offset */
    if (visual_column < state->display_offset) {
        visual_column = 0;
    } else {
        visual_column -= state->display_offset;
    }
    
    /* Calculate line wrapping */
    *display_line = visual_column / state->terminal_width;
    *display_column = visual_column % state->terminal_width;
    
    /* Account for vertical offset */
    if (*display_line >= state->vertical_offset) {
        *display_line -= state->vertical_offset;
    } else {
        *display_line = 0;
    }
    
    return LLE_SUCCESS;
}

/*
 * Update internal state geometry after window resize
 */
lle_result_t lle_internal_state_update_geometry(lle_internal_state_t *state,
                                               size_t width,
                                               size_t height) {
    if (!state) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Update terminal dimensions */
    state->terminal_width = width > 0 ? width : 80;
    state->terminal_height = height > 0 ? height : 24;
    
    /* Force full refresh on next display generation */
    if (state->command_buffer) {
        state->command_buffer->needs_full_refresh = true;
    }
    
    state->buffer_modified = true;
    state->last_update_time = lle_get_current_time_microseconds();
    
    return LLE_SUCCESS;
}
