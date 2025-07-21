/**
 * @file display.c
 * @brief Lusush Line Editor - Display Management Implementation
 *
 * This module implements display management for prompts and input text,
 * handling multiline scenarios, cursor positioning, and efficient screen updates.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "display.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize display state structure
 */
bool lle_display_init(lle_display_state_t *state) {
    if (!state) {
        return false;
    }
    
    // Initialize all fields to safe defaults
    state->prompt = NULL;
    state->buffer = NULL;
    state->terminal = NULL;
    
    // Initialize cursor position
    state->cursor_pos.absolute_row = 0;
    state->cursor_pos.absolute_col = 0;
    state->cursor_pos.relative_row = 0;
    state->cursor_pos.relative_col = 0;
    state->cursor_pos.at_boundary = false;
    state->cursor_pos.valid = false;
    
    // Initialize geometry
    state->geometry.width = 80;
    state->geometry.height = 24;
    state->geometry.prompt_width = 0;
    
    // Initialize display state
    state->display_start_offset = 0;
    state->display_start_line = 0;
    state->last_rendered_length = 0;
    state->last_rendered_lines = 0;
    state->needs_refresh = true;
    state->cursor_visible = true;
    state->initialized = true;
    state->display_flags = LLE_DISPLAY_FLAG_NONE;
    
    return true;
}

/**
 * @brief Create a new display state with associated components
 */
lle_display_state_t *lle_display_create(
    lle_prompt_t *prompt,
    lle_text_buffer_t *buffer,
    lle_terminal_manager_t *terminal
) {
    if (!prompt || !buffer || !terminal) {
        return NULL;
    }
    
    lle_display_state_t *state = malloc(sizeof(lle_display_state_t));
    if (!state) {
        return NULL;
    }
    
    if (!lle_display_init(state)) {
        free(state);
        return NULL;
    }
    
    // Associate components
    state->prompt = prompt;
    state->buffer = buffer;
    state->terminal = terminal;
    
    // Update geometry from terminal
    lle_display_update_geometry(state);
    
    return state;
}

/**
 * @brief Clean up display state
 */
bool lle_display_cleanup(lle_display_state_t *state) {
    if (!state) {
        return false;
    }
    
    // Reset fields but don't free associated structures
    state->prompt = NULL;
    state->buffer = NULL;
    state->terminal = NULL;
    state->initialized = false;
    
    return true;
}

/**
 * @brief Destroy display state and free memory
 */
void lle_display_destroy(lle_display_state_t *state) {
    if (!state) {
        return;
    }
    
    lle_display_cleanup(state);
    free(state);
}

/**
 * @brief Validate display state structure
 */
bool lle_display_validate(const lle_display_state_t *state) {
    if (!state || !state->initialized) {
        return false;
    }
    
    // Check that required components are present
    if (!state->prompt || !state->buffer || !state->terminal) {
        return false;
    }
    
    // Validate individual components
    if (!lle_prompt_validate(state->prompt)) {
        return false;
    }
    
    if (!lle_text_buffer_is_valid(state->buffer)) {
        return false;
    }
    
    if (!lle_terminal_manager_is_valid(state->terminal)) {
        return false;
    }
    
    return true;
}

/**
 * @brief Render complete display (prompt + input text)
 */
bool lle_display_render(lle_display_state_t *state) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // Clear display if flag is set
    if (state->display_flags & LLE_DISPLAY_FLAG_CLEAR_FIRST) {
        if (!lle_display_clear(state)) {
            return false;
        }
    }
    
    // Render the prompt
    bool clear_previous = (state->display_flags & LLE_DISPLAY_FLAG_FORCE_REFRESH) != 0;
    if (!lle_prompt_render(state->terminal, state->prompt, clear_previous)) {
        return false;
    }
    
    // Get text from buffer
    const char *text = state->buffer->buffer;
    size_t text_length = state->buffer->length;
    
    if (text && text_length > 0) {
        // Calculate how text should be displayed
        size_t prompt_last_line_width = lle_prompt_get_last_line_width(state->prompt);
        size_t terminal_width = state->geometry.width;
        
        // Write text character by character, handling line wrapping
        size_t current_col = prompt_last_line_width;
        size_t current_line = 0;
        
        for (size_t i = state->display_start_offset; i < text_length; i++) {
            char c = text[i];
            
            // Handle newlines in input text
            if (c == '\n') {
                if (!lle_terminal_write(state->terminal, "\n", 1)) {
                    return false;
                }
                current_col = 0;
                current_line++;
                continue;
            }
            
            // Handle line wrapping
            if (current_col >= terminal_width) {
                if (!lle_terminal_write(state->terminal, "\n", 1)) {
                    return false;
                }
                current_col = 0;
                current_line++;
            }
            
            // Write the character
            if (!lle_terminal_write(state->terminal, &c, 1)) {
                return false;
            }
            current_col++;
        }
        
        state->last_rendered_lines = current_line + 1;
    } else {
        state->last_rendered_lines = lle_prompt_get_height(state->prompt);
    }
    
    state->last_rendered_length = text_length;
    
    // Update cursor position if not cursor-only mode
    if (!(state->display_flags & LLE_DISPLAY_FLAG_CURSOR_ONLY)) {
        if (!lle_display_update_cursor(state)) {
            return false;
        }
    }
    
    state->needs_refresh = false;
    return true;
}

/**
 * @brief Update cursor position only
 */
bool lle_display_update_cursor(lle_display_state_t *state) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // Calculate cursor position based on current buffer offset
    if (!lle_display_calculate_cursor_position(state, &state->cursor_pos)) {
        return false;
    }
    
    // Position cursor if visible
    if (state->cursor_visible && !(state->display_flags & LLE_DISPLAY_FLAG_NO_CURSOR)) {
        if (!lle_prompt_position_cursor(state->terminal, state->prompt, &state->cursor_pos)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Clear the display
 */
bool lle_display_clear(lle_display_state_t *state) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // Clear prompt from terminal
    if (!lle_prompt_clear_from_terminal(state->terminal, state->prompt)) {
        return false;
    }
    
    // Clear any additional lines that were rendered
    if (state->last_rendered_lines > lle_prompt_get_height(state->prompt)) {
        size_t extra_lines = state->last_rendered_lines - lle_prompt_get_height(state->prompt);
        for (size_t i = 0; i < extra_lines; i++) {
            if (!lle_terminal_clear_line(state->terminal)) {
                return false;
            }
            if (i < extra_lines - 1) {
                if (!lle_terminal_move_cursor_down(state->terminal, 1)) {
                    return false;
                }
            }
        }
    }
    
    state->last_rendered_length = 0;
    state->last_rendered_lines = 0;
    state->needs_refresh = true;
    
    return true;
}

/**
 * @brief Refresh display with current buffer content
 */
bool lle_display_refresh(lle_display_state_t *state) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    state->needs_refresh = true;
    state->display_flags |= LLE_DISPLAY_FLAG_FORCE_REFRESH;
    
    bool result = lle_display_render(state);
    
    // Clear the force refresh flag
    state->display_flags &= ~LLE_DISPLAY_FLAG_FORCE_REFRESH;
    
    return result;
}

/**
 * @brief Update display after text insertion
 */
bool lle_display_update_after_insert(
    lle_display_state_t *state,
    size_t insert_offset,
    size_t insert_length
) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // For simplicity, do a full refresh for now
    // TODO: Optimize for incremental updates in future
    (void)insert_offset;   // Suppress unused parameter warning
    (void)insert_length;   // Suppress unused parameter warning
    
    return lle_display_refresh(state);
}

/**
 * @brief Update display after text deletion
 */
bool lle_display_update_after_delete(
    lle_display_state_t *state,
    size_t delete_offset,
    size_t delete_length
) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // For simplicity, do a full refresh for now
    // TODO: Optimize for incremental updates in future
    (void)delete_offset;   // Suppress unused parameter warning
    (void)delete_length;   // Suppress unused parameter warning
    
    return lle_display_refresh(state);
}

/**
 * @brief Set cursor position and update display
 */
bool lle_display_set_cursor_offset(lle_display_state_t *state, size_t offset) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // Set cursor in text buffer
    if (!lle_text_set_cursor(state->buffer, offset)) {
        return false;
    }
    
    // Update cursor display
    return lle_display_update_cursor(state);
}

/**
 * @brief Calculate cursor position for current buffer offset
 */
bool lle_display_calculate_cursor_position(
    const lle_display_state_t *state,
    lle_cursor_position_t *cursor_pos
) {
    if (!state || !cursor_pos || !lle_display_validate(state)) {
        return false;
    }
    
    // Get current cursor offset from buffer
    size_t cursor_offset = state->buffer->cursor_pos;
    
    // Use cursor math to calculate position
    *cursor_pos = lle_calculate_cursor_position_at_offset(
        state->buffer,
        &state->geometry,
        lle_prompt_get_last_line_width(state->prompt),
        cursor_offset
    );
    
    return cursor_pos->valid;
}

/**
 * @brief Set display flags for controlling behavior
 */
bool lle_display_set_flags(lle_display_state_t *state, uint32_t flags) {
    if (!state) {
        return false;
    }
    
    state->display_flags = flags;
    return true;
}

/**
 * @brief Get current display flags
 */
uint32_t lle_display_get_flags(const lle_display_state_t *state) {
    if (!state) {
        return 0;
    }
    
    return state->display_flags;
}

/**
 * @brief Check if display needs refresh
 */
bool lle_display_needs_refresh(const lle_display_state_t *state) {
    if (!state) {
        return false;
    }
    
    return state->needs_refresh;
}

/**
 * @brief Update terminal geometry cache
 */
bool lle_display_update_geometry(lle_display_state_t *state) {
    if (!state || !state->terminal) {
        return false;
    }
    
    // Get current terminal size
    if (!lle_terminal_get_size(state->terminal)) {
        return false;
    }
    
    // Update cached geometry from terminal manager
    if (state->terminal->geometry_valid) {
        size_t old_width = state->geometry.width;
        size_t old_height = state->geometry.height;
        
        state->geometry = state->terminal->geometry;
        
        // Check if size changed and refresh is needed
        if (old_width != state->geometry.width || old_height != state->geometry.height) {
            state->needs_refresh = true;
        }
    }
    
    return true;
}

/**
 * @brief Get display statistics
 */
bool lle_display_get_statistics(
    const lle_display_state_t *state,
    size_t *lines_rendered,
    size_t *chars_rendered,
    size_t *cursor_line,
    size_t *cursor_col
) {
    if (!state || !lines_rendered || !chars_rendered || !cursor_line || !cursor_col) {
        return false;
    }
    
    *lines_rendered = state->last_rendered_lines;
    *chars_rendered = state->last_rendered_length;
    
    if (state->cursor_pos.valid) {
        *cursor_line = state->cursor_pos.relative_row;
        *cursor_col = state->cursor_pos.relative_col;
    } else {
        *cursor_line = 0;
        *cursor_col = 0;
    }
    
    return true;
}