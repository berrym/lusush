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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for helper functions
static bool lle_display_render_plain_text(lle_display_state_t *state,
                                         const char *text,
                                         size_t text_length,
                                         size_t start_col);

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
    
    // Initialize syntax highlighting integration
    state->syntax_highlighter = NULL;
    state->theme_integration = NULL;
    state->syntax_highlighting_enabled = false;
    state->last_applied_color[0] = '\0';
    
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
    
    // Clear syntax highlighting references (don't destroy - we don't own them)
    state->syntax_highlighter = NULL;
    state->theme_integration = NULL;
    state->syntax_highlighting_enabled = false;
    state->last_applied_color[0] = '\0';
    
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
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY] Validating display state: %p\n", (void*)state);
    }
    
    if (!state || !state->initialized) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY] Validation failed: state=%p, initialized=%s\n", 
                    (void*)state, state ? (state->initialized ? "true" : "false") : "N/A");
        }
        return false;
    }
    
    // Check that required components are present
    if (!state->prompt || !state->buffer || !state->terminal) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY] Validation failed: prompt=%p, buffer=%p, terminal=%p\n", 
                    (void*)state->prompt, (void*)state->buffer, (void*)state->terminal);
        }
        return false;
    }
    
    // Validate individual components
    if (!lle_prompt_validate(state->prompt)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY] Validation failed: prompt validation failed\n");
        }
        return false;
    }
    
    if (!lle_text_buffer_is_valid(state->buffer)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY] Validation failed: text buffer validation failed\n");
        }
        return false;
    }
    
    if (!lle_terminal_manager_is_valid(state->terminal)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY] Validation failed: terminal manager validation failed\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY] Validation successful\n");
    }
    
    return true;
}

/**
 * @brief Render complete display (prompt + input text)
 */
bool lle_display_render(lle_display_state_t *state) {
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_RENDER] Starting display render\n");
    }
    
    if (!lle_display_validate(state)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Display validation failed\n");
        }
        return false;
    }
    
    // Clear display if flag is set
    if (state->display_flags & LLE_DISPLAY_FLAG_CLEAR_FIRST) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Clearing display first\n");
        }
        if (!lle_display_clear(state)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_RENDER] Display clear failed\n");
            }
            return false;
        }
    }
    
    // Render the prompt
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_RENDER] Rendering prompt\n");
    }
    bool clear_previous = (state->display_flags & LLE_DISPLAY_FLAG_FORCE_REFRESH) != 0;
    if (!lle_prompt_render(state->terminal, state->prompt, clear_previous)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Prompt render failed\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_RENDER] Prompt render completed, proceeding to text rendering\n");
    }
    
    // Get text from buffer
    const char *text = state->buffer->buffer;
    size_t text_length = state->buffer->length;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_RENDER] Text buffer: length=%zu, text=%p\n", text_length, (void*)text);
    }
    
    if (text && text_length > 0) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Rendering non-empty text\n");
        }
        size_t prompt_last_line_width = lle_prompt_get_last_line_width(state->prompt);
        
        // Use syntax highlighting if enabled and available
        if (lle_display_is_syntax_highlighting_enabled(state)) {
            if (!lle_display_render_with_syntax_highlighting(state, text, text_length, prompt_last_line_width)) {
                return false;
            }
        } else {
            // Fallback to plain text rendering
            if (!lle_display_render_plain_text(state, text, text_length, prompt_last_line_width)) {
                return false;
            }
        }
        
        // Calculate rendered lines based on actual newlines and wrapping
        size_t terminal_width = state->geometry.width;
        
        // Count actual lines by counting newlines and wrapping
        size_t line_count = 1; // Start with 1 line
        size_t current_col = prompt_last_line_width;
        
        for (size_t i = 0; i < text_length; i++) {
            if (text[i] == '\n') {
                line_count++;
                current_col = 0;
            } else {
                current_col++;
                if (current_col >= terminal_width) {
                    line_count++;
                    current_col = 0;
                }
            }
        }
        
        state->last_rendered_lines = line_count;
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Text rendering completed, lines=%zu\n", line_count);
        }
    } else {
        state->last_rendered_lines = lle_prompt_get_height(state->prompt);
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] No text to render, using prompt height=%zu\n", state->last_rendered_lines);
        }
    }
    
    state->last_rendered_length = text_length;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_RENDER] About to update cursor position\n");
    }
    
    // Update cursor position if not cursor-only mode
    if (!(state->display_flags & LLE_DISPLAY_FLAG_CURSOR_ONLY)) {
        if (!lle_display_update_cursor(state)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_RENDER] Cursor update failed\n");
            }
            return false;
        }
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Cursor update completed\n");
        }
    } else {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Skipping cursor update (cursor-only mode)\n");
        }
    }
    
    state->needs_refresh = false;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_RENDER] Display render completed successfully\n");
    }
    
    return true;
}

/**
 * @brief Update cursor position only
 */
bool lle_display_update_cursor(lle_display_state_t *state) {
    // Get debug flag from environment
    static int debug_mode = -1;
    if (debug_mode == -1) {
        debug_mode = getenv("LLE_DEBUG") ? 1 : 0;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_CURSOR_UPDATE] Starting cursor update\n");
    }
    
    if (!lle_display_validate(state)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_CURSOR_UPDATE] Display validation failed\n");
        }
        return false;
    }

    if (debug_mode) {
        fprintf(stderr, "[LLE_CURSOR_UPDATE] Display validated, calculating cursor position\n");
    }

    // Calculate cursor position based on current buffer offset
    if (!lle_display_calculate_cursor_position(state, &state->cursor_pos)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_CURSOR_UPDATE] Cursor position calculation failed\n");
        }
        return false;
    }

    if (debug_mode) {
        fprintf(stderr, "[LLE_CURSOR_UPDATE] Cursor position calculated, checking visibility\n");
    }

    // Position cursor if visible
    if (state->cursor_visible && !(state->display_flags & LLE_DISPLAY_FLAG_NO_CURSOR)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_CURSOR_UPDATE] Positioning cursor\n");
        }
        if (!lle_prompt_position_cursor(state->terminal, state->prompt, &state->cursor_pos)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_CURSOR_UPDATE] Cursor positioning failed\n");
            }
            return false;
        }
        if (debug_mode) {
            fprintf(stderr, "[LLE_CURSOR_UPDATE] Cursor positioned successfully\n");
        }
    } else {
        if (debug_mode) {
            fprintf(stderr, "[LLE_CURSOR_UPDATE] Cursor not visible or cursor disabled\n");
        }
    }

    if (debug_mode) {
        fprintf(stderr, "[LLE_CURSOR_UPDATE] Cursor update completed successfully\n");
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
 * @brief Force a full refresh of the display
 *
 * @param state Display state to refresh
 * @return true on success, false on failure
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
 * @brief Update display incrementally without redrawing prompt
 *
 * This function updates only the text content and cursor position without
 * redrawing the entire prompt. This prevents the visual chaos caused by
 * constant prompt redraws during character-by-character input.
 *
 * @param state Display state to update
 * @return true on success, false on failure
 */
bool lle_display_update_incremental(lle_display_state_t *state) {
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Starting incremental display update\n");
    }

    if (!lle_display_validate(state)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Display validation failed\n");
        }
        return false;
    }

    // Get text from buffer
    const char *text = state->buffer->buffer;
    size_t text_length = state->buffer->length;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Text buffer: length=%zu\n", text_length);
    }

    // Get prompt geometry for positioning
    size_t prompt_last_line_width = lle_prompt_get_last_line_width(state->prompt);
    size_t terminal_width = state->geometry.width;
    
    // Handle multiline text by using text-only rendering instead of full render
    if (text && text_length > 0) {
        // Check for actual newlines - handle with proper multiline rendering
        if (memchr(text, '\n', text_length)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Multiline text detected, handling with text-only rendering\n");
            }
            
            // Move to start of text area after prompt
            if (!lle_terminal_move_cursor_to_column(state->terminal, prompt_last_line_width)) {
                return false;
            }
            
            // Clear from cursor to end of screen to remove old content
            if (!lle_terminal_clear_to_eos(state->terminal)) {
                return false;
            }
            
            // Render text with syntax highlighting if enabled
            if (lle_display_is_syntax_highlighting_enabled(state)) {
                if (!lle_display_render_with_syntax_highlighting(state, text, text_length, prompt_last_line_width)) {
                    return false;
                }
            } else {
                if (!lle_display_render_plain_text(state, text, text_length, prompt_last_line_width)) {
                    return false;
                }
            }
            
            // Update cursor position
            return lle_display_update_cursor(state);
        }
        
        // Handle line wrapping with proper text-only rendering
        if ((prompt_last_line_width + text_length) > terminal_width) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Line wrapping detected (prompt=%zu + text=%zu > width=%zu), handling with text-only rendering\n", 
                       prompt_last_line_width, text_length, terminal_width);
            }
            
            // Move to start of text area after prompt
            if (!lle_terminal_move_cursor_to_column(state->terminal, prompt_last_line_width)) {
                return false;
            }
            
            // Clear from cursor to end of screen to handle wrapped content
            if (!lle_terminal_clear_to_eos(state->terminal)) {
                return false;
            }
            
            // Render text with syntax highlighting if enabled
            if (lle_display_is_syntax_highlighting_enabled(state)) {
                if (!lle_display_render_with_syntax_highlighting(state, text, text_length, prompt_last_line_width)) {
                    return false;
                }
            } else {
                if (!lle_display_render_plain_text(state, text, text_length, prompt_last_line_width)) {
                    return false;
                }
            }
            
            // Update cursor position
            return lle_display_update_cursor(state);
        }
    }

    // Simple single-line case: move cursor to end of prompt and write text
    if (!lle_terminal_move_cursor_to_column(state->terminal, prompt_last_line_width)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to move cursor to text start\n");
        }
        return false;
    }
    
    // Clear from cursor to end of line to remove old text
    if (!lle_terminal_clear_to_eol(state->terminal)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to clear to end of line\n");
        }
        return false;
    }

    // Write the current text content
    if (text && text_length > 0) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Writing text: '%.*s'\n", (int)text_length, text);
        }
        
        if (!lle_terminal_write(state->terminal, text, text_length)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to write text\n");
            }
            return false;
        }
    }

    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Incremental update completed successfully\n");
    }

    return true;
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
    
    // Use incremental update instead of full refresh
    (void)insert_offset;   // Suppress unused parameter warning
    (void)insert_length;   // Suppress unused parameter warning
    
    return lle_display_update_incremental(state);
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
    
    // Use incremental update instead of full refresh
    (void)delete_offset;   // Suppress unused parameter warning
    (void)delete_length;   // Suppress unused parameter warning
    
    return lle_display_update_incremental(state);
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

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Render plain text without syntax highlighting
 */
static bool lle_display_render_plain_text(lle_display_state_t *state,
                                         const char *text,
                                         size_t text_length,
                                         size_t start_col) {
    if (!state || !text) {
        return false;
    }
    
    size_t terminal_width = state->geometry.width;
    size_t current_col = start_col;
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
    
    return true;
}

// ============================================================================
// Syntax Highlighting Integration Functions
// ============================================================================

/**
 * @brief Set syntax highlighter for display
 */
bool lle_display_set_syntax_highlighter(lle_display_state_t *state,
                                        lle_syntax_highlighter_t *highlighter) {
    if (!state) {
        return false;
    }
    
    state->syntax_highlighter = highlighter;
    
    // Update syntax highlighting if both highlighter and theme are available
    if (state->syntax_highlighter && state->theme_integration) {
        lle_display_update_syntax_highlighting(state);
    }
    
    return true;
}

/**
 * @brief Set theme integration for display
 */
bool lle_display_set_theme_integration(lle_display_state_t *state,
                                       lle_theme_integration_t *theme_integration) {
    if (!state) {
        return false;
    }
    
    state->theme_integration = theme_integration;
    return true;
}

/**
 * @brief Enable or disable syntax highlighting in display
 */
bool lle_display_enable_syntax_highlighting(lle_display_state_t *state, bool enable) {
    if (!state) {
        return false;
    }
    
    // Can only enable if both highlighter and theme integration are available
    if (enable && (!state->syntax_highlighter || !state->theme_integration)) {
        return false;
    }
    
    state->syntax_highlighting_enabled = enable;
    return true;
}

/**
 * @brief Check if syntax highlighting is enabled
 */
bool lle_display_is_syntax_highlighting_enabled(const lle_display_state_t *state) {
    if (!state) {
        return false;
    }
    
    return state->syntax_highlighting_enabled && 
           state->syntax_highlighter && 
           state->theme_integration;
}

/**
 * @brief Update syntax highlighting for current buffer content
 */
bool lle_display_update_syntax_highlighting(lle_display_state_t *state) {
    if (!lle_display_is_syntax_highlighting_enabled(state)) {
        return false;
    }
    
    const char *text = state->buffer->buffer;
    size_t text_length = state->buffer->length;
    
    if (!text || text_length == 0) {
        return true; // No text to highlight
    }
    
    return lle_syntax_highlight_text(state->syntax_highlighter, text, text_length);
}

/**
 * @brief Map syntax type to theme element
 */
static lle_theme_element_t lle_display_syntax_type_to_theme_element(lle_syntax_type_t type) {
    switch (type) {
        case LLE_SYNTAX_KEYWORD:
        case LLE_SYNTAX_COMMAND:
            return LLE_THEME_SYNTAX_KEYWORD;
        case LLE_SYNTAX_STRING:
            return LLE_THEME_SYNTAX_STRING;
        case LLE_SYNTAX_COMMENT:
            return LLE_THEME_SYNTAX_COMMENT;
        case LLE_SYNTAX_OPERATOR:
            return LLE_THEME_SYNTAX_OPERATOR;
        case LLE_SYNTAX_VARIABLE:
            return LLE_THEME_SYNTAX_VARIABLE;
        case LLE_SYNTAX_ERROR:
            return LLE_THEME_ERROR_HIGHLIGHT;
        case LLE_SYNTAX_NUMBER:
        case LLE_SYNTAX_PATH:
        case LLE_SYNTAX_NORMAL:
        default:
            return LLE_THEME_INPUT_TEXT;
    }
}

/**
 * @brief Apply color for syntax type
 */
static bool lle_display_apply_syntax_color(lle_display_state_t *state, lle_syntax_type_t type) {
    if (!state || !state->theme_integration) {
        return false;
    }
    
    lle_theme_element_t theme_element = lle_display_syntax_type_to_theme_element(type);
    const char *color_code = lle_theme_get_color(state->theme_integration, theme_element);
    
    if (!color_code || color_code[0] == '\0') {
        return true; // No color to apply
    }
    
    // Optimization: only apply color if it's different from last applied
    if (strcmp(state->last_applied_color, color_code) == 0) {
        return true;
    }
    
    // Apply the color
    if (!lle_terminal_write(state->terminal, color_code, strlen(color_code))) {
        return false;
    }
    
    // Cache the applied color
    strncpy(state->last_applied_color, color_code, sizeof(state->last_applied_color) - 1);
    state->last_applied_color[sizeof(state->last_applied_color) - 1] = '\0';
    
    return true;
}

/**
 * @brief Reset terminal colors to default
 */
static bool lle_display_reset_colors(lle_display_state_t *state) {
    if (!state || !state->terminal) {
        return false;
    }
    
    // ANSI reset sequence
    const char *reset_code = "\033[0m";
    if (!lle_terminal_write(state->terminal, reset_code, strlen(reset_code))) {
        return false;
    }
    
    // Clear cached color
    state->last_applied_color[0] = '\0';
    
    return true;
}

/**
 * @brief Render text with syntax highlighting
 */
bool lle_display_render_with_syntax_highlighting(lle_display_state_t *state,
                                                 const char *text,
                                                 size_t length,
                                                 size_t start_col) {
    if (!lle_display_is_syntax_highlighting_enabled(state) || !text) {
        return false;
    }
    
    // Get syntax regions
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(state->syntax_highlighter, &region_count);
    
    size_t terminal_width = state->geometry.width;
    size_t current_col = start_col;
    size_t current_line = 0;
    size_t region_index = 0;
    lle_syntax_type_t current_type = LLE_SYNTAX_NORMAL;
    
    // Apply initial color
    lle_display_apply_syntax_color(state, current_type);
    
    for (size_t i = state->display_start_offset; i < length; i++) {
        // Check if we need to change color based on syntax regions
        while (region_index < region_count && 
               regions[region_index].start + regions[region_index].length <= i) {
            region_index++;
        }
        
        lle_syntax_type_t new_type = LLE_SYNTAX_NORMAL;
        if (region_index < region_count && 
            i >= regions[region_index].start && 
            i < regions[region_index].start + regions[region_index].length) {
            new_type = regions[region_index].type;
        }
        
        // Apply color change if syntax type changed
        if (new_type != current_type) {
            if (!lle_display_apply_syntax_color(state, new_type)) {
                return false;
            }
            current_type = new_type;
        }
        
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
    
    // Reset colors after rendering
    lle_display_reset_colors(state);
    
    return true;
}