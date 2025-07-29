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
#include <unistd.h>

// Platform detection types for display strategy selection
typedef enum {
    LLE_PLATFORM_MACOS,
    LLE_PLATFORM_LINUX,
    LLE_PLATFORM_UNKNOWN
} lle_platform_type_t;

/**
 * Detect the current platform for display strategy selection.
 * @return Platform type
 */
static lle_platform_type_t lle_detect_platform(void) {
#ifdef __APPLE__
    return LLE_PLATFORM_MACOS;
#elif defined(__linux__)
    return LLE_PLATFORM_LINUX;
#else
    return LLE_PLATFORM_UNKNOWN;
#endif
}

// Forward declarations for helper functions
static bool lle_display_render_plain_text(lle_display_state_t *state,
                                         const char *text,
                                         size_t text_length,
                                         size_t start_col);
static bool lle_display_update_conservative(lle_display_state_t *state);

/**
 * @brief Initialize display state structure
 */
bool lle_display_init(lle_display_state_t *state) {
    if (!state) {
        return false;
    }
    
    // Note: prompt, buffer, and terminal are set by caller
    // Don't overwrite them with NULL here
    
    // Initialize cursor position
    state->cursor_pos.absolute_row = 0;
    state->cursor_pos.absolute_col = 0;
    state->cursor_pos.relative_row = 0;
    state->cursor_pos.relative_col = 0;
    state->cursor_pos.at_boundary = false;
    state->cursor_pos.valid = false;
    
    // Initialize geometry with actual terminal size - prioritize accurate detection
    bool geometry_acquired = false;
    
    if (state->terminal) {
        // First priority: Use existing valid geometry if available
        if (state->terminal->geometry_valid && 
            state->terminal->geometry.width > 0 && 
            state->terminal->geometry.height > 0) {
            state->geometry = state->terminal->geometry;
            geometry_acquired = true;
        }
        // Second priority: Try to detect fresh terminal size
        else if (lle_terminal_get_size(state->terminal)) {
            state->geometry = state->terminal->geometry;
            geometry_acquired = true;
        }
    }
    
    // Last resort: Hardcoded fallback only if all detection fails
    if (!geometry_acquired) {
        state->geometry.width = 80;
        state->geometry.height = 24;
    }
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
    
    // Associate components BEFORE calling lle_display_init
    // so terminal size detection works during initialization
    state->prompt = prompt;
    state->buffer = buffer;
    state->terminal = terminal;
    
    if (!lle_display_init(state)) {
        free(state);
        return NULL;
    }
    
    // Update geometry from terminal (redundant now but kept for safety)
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
    
    // Update geometry from terminal in case of resize
    lle_display_update_geometry(state);
    
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
    
    // Update cursor position using mathematical framework with relative positioning
    if (!(state->display_flags & LLE_DISPLAY_FLAG_CURSOR_ONLY)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Using mathematical framework for cursor positioning\n");
        }
        
        // Use the same mathematical framework as incremental updates
        size_t prompt_last_line_width = lle_prompt_get_last_line_width(state->prompt);
        lle_terminal_geometry_t geometry = state->geometry;
        
        // Create buffer for cursor position calculation
        lle_text_buffer_t *cursor_buffer = lle_text_buffer_create(state->buffer->length + 1);
        if (cursor_buffer && state->buffer->length > 0) {
            memcpy(cursor_buffer->buffer, state->buffer->buffer, state->buffer->length);
            cursor_buffer->length = state->buffer->length;
            cursor_buffer->cursor_pos = state->buffer->cursor_pos;
            cursor_buffer->buffer[state->buffer->length] = '\0';
            
            // Calculate cursor position using mathematical framework
            lle_cursor_position_t cursor_pos = lle_calculate_cursor_position(
                cursor_buffer, &geometry, prompt_last_line_width);
            
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_RENDER] Mathematical cursor position: valid=%s, row=%zu, col=%zu\n",
                       cursor_pos.valid ? "true" : "false", cursor_pos.absolute_row, cursor_pos.absolute_col);
            }
            
            if (cursor_pos.valid) {
                // Use relative positioning from text start, not absolute terminal positioning
                if (cursor_pos.absolute_row > 0) {
                    // Move down from current position (which should be at end of text)
                    if (!lle_terminal_move_cursor_down(state->terminal, cursor_pos.absolute_row)) {
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_DISPLAY_RENDER] Failed to move cursor down %zu lines\n", cursor_pos.absolute_row);
                        }
                    }
                }
                
                // Position at correct column (this is relative to start of line)
                if (!lle_terminal_move_cursor_to_column(state->terminal, cursor_pos.absolute_col)) {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_RENDER] Failed to move cursor to column %zu\n", cursor_pos.absolute_col);
                    }
                }
                
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_RENDER] Mathematical cursor positioning completed\n");
                }
            } else {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_RENDER] Invalid cursor position, staying at current location\n");
                }
            }
            
            lle_text_buffer_destroy(cursor_buffer);
        } else {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_RENDER] No text for cursor positioning\n");
            }
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

    // Platform detection for display strategy
    lle_platform_type_t platform = lle_detect_platform();
    if (debug_mode) {
        const char *platform_name = (platform == LLE_PLATFORM_MACOS) ? "macOS" : 
                                   (platform == LLE_PLATFORM_LINUX) ? "Linux" : "Unknown";
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Platform detected: %s\n", platform_name);
    }

    // Use conservative strategy for Linux to avoid character duplication
    if (platform == LLE_PLATFORM_LINUX) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Using Linux conservative strategy\n");
        }
        return lle_display_update_conservative(state);
    }

    // Get text from buffer
    const char *text = state->buffer->buffer;
    size_t text_length = state->buffer->length;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Text buffer: length=%zu\n", text_length);
    }

    // Update geometry from terminal in case of resize
    lle_display_update_geometry(state);
    
    // Get prompt geometry for positioning
    size_t prompt_last_line_width = lle_prompt_get_last_line_width(state->prompt);
    size_t terminal_width = state->geometry.width;
    size_t terminal_height = state->geometry.height;
    
    // Check if content contains newlines - these require full render
    if (text && text_length > 0 && memchr(text, '\n', text_length)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Multiline text detected, using full render\n");
        }
        return lle_display_render(state);
    }
    
    // Keep track of the last text length for optimization
    static size_t last_text_length = 0;
    
    // Reset tracking when starting a new command session (detect significant length drop)
    // This happens when a command is completed and a new one starts
    if (last_text_length > 5 && text_length <= 2) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] New command detected: resetting last_text_length from %zu to 0\n", last_text_length);
        }
        last_text_length = 0;
    }
    
    // Optimization: if text hasn't changed, no need to update
    if (text_length == last_text_length && text_length > 0) {
        return true;
    }
    
    // For line wrapping, use true incremental approach - never reposition cursor after initial wrap
    if (text && text_length > 0) {
        size_t text_display_width = lle_calculate_display_width_ansi(text, text_length);
        size_t total_width = prompt_last_line_width + text_display_width;
        // Check if text is growing (new character) or shrinking (backspace)
        bool text_is_growing = (text_length > last_text_length);
        bool text_is_shrinking = (text_length < last_text_length);
        bool is_first_wrap = (last_text_length == 0);
        
        // Check if we're crossing the wrap boundary (unwrapping) - must be outside wrapping check
        size_t last_total_width = prompt_last_line_width + last_text_length;
        bool was_wrapped = (last_total_width > terminal_width);
        bool is_wrapped = (total_width > terminal_width);
        bool crossing_wrap_boundary = (was_wrapped && !is_wrapped);
        
        if (debug_mode && crossing_wrap_boundary) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Wrap boundary crossing detected: was_wrapped=%s, is_wrapped=%s\n",
                   was_wrapped ? "true" : "false", is_wrapped ? "true" : "false");
        }
        
        // Handle wrap boundary crossing immediately
        if (crossing_wrap_boundary && text_is_shrinking && !is_first_wrap) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Handling wrap boundary crossing during backspace\n");
            }
            
            // Move cursor up one line (back to prompt line) then to correct column
            if (!lle_terminal_move_cursor_up(state->terminal, 1)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to move cursor up for boundary crossing\n");
                }
            }
            
            // Now move to the correct column position
            if (!lle_terminal_move_cursor_to_column(state->terminal, prompt_last_line_width)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to move to text start column for boundary crossing\n");
                }
            }
            
            // Clear the line to remove wrapped content
            if (!lle_terminal_clear_to_eol(state->terminal)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to clear line for boundary crossing\n");
                }
            }
            
            // Write remaining text (now fits on single line)
            if (text_length > 0) {
                if (!lle_terminal_write(state->terminal, text, text_length)) {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to write remaining text after boundary crossing\n");
                    }
                } else {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Successfully handled wrap boundary crossing\n");
                    }
                }
            }
            
            last_text_length = text_length;
            return true;
        }
        
        if (total_width > terminal_width) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Line wrapping detected (prompt=%zu + text_display=%zu = total=%zu > terminal=%zu)\n", 
                       prompt_last_line_width, text_display_width, total_width, terminal_width);
            }
            
            // Check if we need more lines than available, we need to scroll or use simpler approach
            size_t lines_needed = (total_width / terminal_width) + 1;
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Lines needed for wrapped text: %zu\n", lines_needed);
            }
            
            // If we need more lines than available, we need to scroll or use simpler approach
            if (lines_needed > 1) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Multiple lines needed - using terminal natural wrapping\n");
                }
            }
            
            if ((text_is_growing || text_is_shrinking) && !is_first_wrap) {
                if (text_is_growing) {
                    // True incremental approach: only write new characters at cursor position
                    size_t chars_to_add = text_length - last_text_length;
                    const char *new_chars = text + last_text_length;
                    
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] True incremental: adding %zu chars '%.*s' at current cursor\n", 
                               chars_to_add, (int)chars_to_add, new_chars);
                    }
                    
                    // Simply write the new characters - cursor should already be at correct position
                    if (!lle_terminal_write(state->terminal, new_chars, chars_to_add)) {
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to write new characters\n");
                        }
                        return false;
                    }
                    
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Successfully added %zu characters incrementally\n", chars_to_add);
                    }
                } else {
                    // For backspace within wrapped text: use simple incremental backspace
                    size_t chars_removed = last_text_length - text_length;
                    
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] True incremental backspace within wrapped text: removing %zu chars\n", chars_removed);
                    }
                    
                    // Normal incremental backspace within wrapped text
                    for (size_t i = 0; i < chars_removed; i++) {
                        if (!lle_terminal_write(state->terminal, "\b \b", 3)) {
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to backspace character %zu\n", i);
                            }
                            break;
                        }
                    }
                    
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Successfully removed %zu characters with incremental backspace\n", chars_removed);
                    }
                }
                
                last_text_length = text_length;
                
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] True incremental %s completed\n", 
                           text_is_growing ? "append" : "backspace");
                }
                
                return true;
            } else {
                // For first wrap, fall back to full render to handle line wrapping properly
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] First wrap detected - falling back to full render\n");
                }
                
                last_text_length = text_length;
                return lle_display_render(state);
            }
        }
    }
    
    // Update length tracking for non-wrapping case too
    last_text_length = text_length;
    
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

    // Write the text with syntax highlighting if enabled
    if (text && text_length > 0) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Writing text: '%.*s'\n", (int)text_length, text);
        }
        
        // Use syntax highlighting if enabled and available
        if (lle_display_is_syntax_highlighting_enabled(state)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Applying syntax highlighting\n");
            }
            
            // Update syntax highlighting for current text
            if (!lle_display_update_syntax_highlighting(state)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to update syntax highlighting\n");
                }
            }
            
            // Render with syntax highlighting
            if (!lle_display_render_with_syntax_highlighting(state, text, text_length, prompt_last_line_width)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Syntax highlighting failed, falling back to plain text\n");
                }
                // Fallback to plain text
                if (!lle_terminal_write(state->terminal, text, text_length)) {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to write plain text fallback\n");
                    }
                    return false;
                }
            }
        } else {
            // Write plain text without syntax highlighting
            if (!lle_terminal_write(state->terminal, text, text_length)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to write text\n");
                }
                return false;
            }
        }
    }

    // Flush output to ensure text is written before cursor positioning
    fflush(stdout);
    fflush(stderr);

    // CRITICAL FIX: Position cursor correctly after writing text
    // The cursor needs to be positioned based on the actual buffer cursor position
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Positioning cursor after text update\n");
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Buffer cursor at: %zu, buffer length: %zu\n", 
                state->buffer->cursor_pos, state->buffer->length);
    }
    
    if (!lle_display_update_cursor(state)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Failed to update cursor position\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Cursor positioning completed\n");
    }

    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Incremental update completed successfully\n");
    }

    return true;
}

/**
 * Conservative display update strategy for Linux terminals.
 * Avoids character duplication by using targeted character operations
 * instead of clear-and-rewrite sequences that may not work reliably.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
static bool lle_display_update_conservative(lle_display_state_t *state) {
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Starting conservative display update\n");
    }

    const char *text = state->buffer->buffer;
    size_t text_length = state->buffer->length;
    
    // Track confirmed text length to avoid duplication
    static size_t last_confirmed_length = 0;
    
    // Reset tracking for new command sessions
    if (last_confirmed_length > 5 && text_length <= 2) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] New command session detected\n");
        }
        last_confirmed_length = 0;
    }

    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Current: %zu, Last: %zu\n", 
                text_length, last_confirmed_length);
    }

    // Handle text changes incrementally
    if (text_length > last_confirmed_length) {
        // Adding characters: append only new ones
        size_t new_chars = text_length - last_confirmed_length;
        const char *new_text = text + last_confirmed_length;
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Appending %zu chars: '%.*s'\n", 
                    new_chars, (int)new_chars, new_text);
        }
        
        if (!lle_terminal_write(state->terminal, new_text, new_chars)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Failed to append new characters\n");
            }
            return false;
        }
        
        last_confirmed_length = text_length;
        
    } else if (text_length < last_confirmed_length) {
        // Removing characters: use backspace sequence
        size_t removed_chars = last_confirmed_length - text_length;
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Removing %zu chars with backspace\n", 
                    removed_chars);
        }
        
        for (size_t i = 0; i < removed_chars; i++) {
            // Use backspace-space-backspace to clear character
            if (!lle_terminal_write(state->terminal, "\b \b", 3)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Failed to backspace character %zu\n", i);
                }
                return false;
            }
        }
        
        last_confirmed_length = text_length;
        
    } else {
        // No change in text length - handle cursor positioning only
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] No text change, updating cursor only\n");
        }
        
        if (!lle_display_update_cursor(state)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Failed to update cursor\n");
            }
            return false;
        }
    }

    // Force terminal synchronization on Linux
    fflush(stdout);
    fflush(stderr);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_CONSERVATIVE] Conservative update completed successfully\n");
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
    
    // Instead of doing character-by-character rendering with manual wrapping,
    // we'll render segments with appropriate colors and let the terminal handle wrapping
    size_t region_index = 0;
    size_t text_pos = state->display_start_offset;
    lle_syntax_type_t current_type = LLE_SYNTAX_NORMAL;
    
    // Apply initial color
    lle_display_apply_syntax_color(state, current_type);
    
    while (text_pos < length) {
        // Find the next syntax region boundary or end of text
        size_t segment_end = length;
        
        // Check if we're in a syntax region
        lle_syntax_type_t new_type = LLE_SYNTAX_NORMAL;
        if (region_index < region_count) {
            const lle_syntax_region_t *region = &regions[region_index];
            
            if (text_pos >= region->start && text_pos < region->start + region->length) {
                // We're inside this region
                new_type = region->type;
                segment_end = region->start + region->length;
            } else if (text_pos < region->start) {
                // We're before this region
                segment_end = region->start;
            } else {
                // We're past this region, move to next
                region_index++;
                continue;
            }
        }
        
        // Ensure we don't go past the text length
        if (segment_end > length) {
            segment_end = length;
        }
        
        // Apply color change if syntax type changed
        if (new_type != current_type) {
            if (!lle_display_apply_syntax_color(state, new_type)) {
                return false;
            }
            current_type = new_type;
        }
        
        // Write the segment (let terminal handle wrapping naturally)
        size_t segment_length = segment_end - text_pos;
        if (segment_length > 0) {
            if (!lle_terminal_write(state->terminal, text + text_pos, segment_length)) {
                return false;
            }
            text_pos = segment_end;
        }
        
        // Move to next region if we completed this one
        if (region_index < region_count && text_pos >= regions[region_index].start + regions[region_index].length) {
            region_index++;
        }
    }
    
    // Reset colors after rendering
    lle_display_reset_colors(state);
    
    return true;
}

// ============================================================================
// Cursor Movement Convenience APIs for Keybinding Integration
// ============================================================================

/**
 * Move cursor to beginning of line with proper display update.
 *
 * @param state Display state to update
 * @return true on success, false on error
 *
 * Combines buffer cursor movement with display system cursor positioning
 * for keybinding implementations. Ensures display state consistency.
 */
bool lle_display_move_cursor_home(lle_display_state_t *state) {
    // Get debug flag from environment
    static int debug_mode = -1;
    if (debug_mode == -1) {
        debug_mode = getenv("LLE_DEBUG") ? 1 : 0;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_HOME] Starting move cursor to home\n");
    }
    
    if (!lle_display_validate(state)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_MOVE_HOME] Display validation failed\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_HOME] Before move: cursor at %zu\n", state->buffer->cursor_pos);
    }
    
    // Move cursor in buffer to beginning (same as original)
    if (!lle_text_move_cursor(state->buffer, LLE_MOVE_HOME)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_MOVE_HOME] Failed to move cursor in buffer\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_HOME] After move: cursor at %zu\n", state->buffer->cursor_pos);
    }
    
    // Position cursor visually without redrawing prompt (original working approach)
    size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
    bool result = lle_terminal_move_cursor_to_column(state->terminal, prompt_width);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_HOME] Terminal cursor positioning result: %s\n", result ? "SUCCESS" : "FAILED");
    }
    
    return result;
}

/**
 * Move cursor to end of line with proper display update.
 *
 * @param state Display state to update
 * @return true on success, false on error
 *
 * Combines buffer cursor movement with display system cursor positioning
 * for keybinding implementations. Ensures display state consistency.
 */
bool lle_display_move_cursor_end(lle_display_state_t *state) {
    // Get debug flag from environment
    static int debug_mode = -1;
    if (debug_mode == -1) {
        debug_mode = getenv("LLE_DEBUG") ? 1 : 0;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_END] Starting move cursor to end\n");
    }
    
    if (!lle_display_validate(state)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_MOVE_END] Display validation failed\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_END] Before move: cursor at %zu\n", state->buffer->cursor_pos);
    }
    
    // Move cursor in buffer to end (same as original)
    if (!lle_text_move_cursor(state->buffer, LLE_MOVE_END)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_MOVE_END] Failed to move cursor in buffer\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_END] After move: cursor at %zu\n", state->buffer->cursor_pos);
    }
    
    // Position cursor visually at end of text without redrawing prompt (original working approach)
    size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
    size_t text_width = lle_calculate_display_width_ansi(state->buffer->buffer, state->buffer->length);
    bool result = lle_terminal_move_cursor_to_column(state->terminal, prompt_width + text_width);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_MOVE_END] Terminal cursor positioning result: %s\n", result ? "SUCCESS" : "FAILED");
    }
    
    return result;
}

/**
 * Clear entire line with proper display update.
 *
 * @param state Display state to update
 * @return true on success, false on error
 *
 * Clears the text buffer and updates display without breaking state
 * consistency. Used for Ctrl+U and Ctrl+G keybinding implementations.
 */
bool lle_display_clear_line(lle_display_state_t *state) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // Clear the text buffer
    lle_text_buffer_clear(state->buffer);
    
    // Use incremental update to clear text area efficiently
    return lle_display_update_incremental(state);
}

/**
 * Enter reverse search mode with proper display management.
 *
 * @param state Display state to update
 * @return true on success, false on error
 *
 * Transitions display system into reverse search mode without manual
 * terminal operations. Manages state consistency throughout search.
 */
bool lle_display_enter_search_mode(lle_display_state_t *state) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // Save current display state for restoration
    state->needs_refresh = true;
    
    // Move to new line for search, starting at column 0
    if (state->terminal && isatty(state->terminal->stdin_fd)) {
        if (!lle_terminal_write(state->terminal, "\n", 1)) {
            return false;
        }
        // Ensure we start at column 0 for search prompt
        if (!lle_terminal_move_cursor_to_column(state->terminal, 0)) {
            return false;
        }
    }
    
    return true;
}



/**
 * Exit reverse search mode with proper display restoration.
 *
 * @param state Display state to restore
 * @return true on success, false on error
 *
 * Restores normal display mode from reverse search without manual
 * terminal operations. Ensures proper state cleanup and cursor positioning.
 */
bool lle_display_exit_search_mode(lle_display_state_t *state) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    // Clear current search line and move back to original position
    if (state->terminal && isatty(state->terminal->stdin_fd)) {
        // Clear current line
        if (!lle_terminal_move_cursor_to_column(state->terminal, 0)) {
            return false;
        }
        if (!lle_terminal_clear_to_eol(state->terminal)) {
            return false;
        }
        // Move cursor up to original prompt line
        if (!lle_terminal_move_cursor_up(state->terminal, 1)) {
            return false;
        }
    }
    
    // Use display system to properly restore prompt and buffer content
    return lle_display_refresh(state);
}

/**
 * Update search prompt and results with proper display management.
 *
 * @param state Display state to update
 * @param search_term Current search term
 * @param search_length Length of search term
 * @param match_text Matched history text (can be NULL)
 * @param match_length Length of matched text
 * @return true on success, false on error
 *
 * Updates the search prompt and matched text without manual terminal
 * operations. Maintains display state consistency during search operations.
 */
bool lle_display_update_search_prompt(lle_display_state_t *state,
                                      const char *search_term,
                                      size_t search_length,
                                      const char *match_text,
                                      size_t match_length) {
    if (!lle_display_validate(state)) {
        return false;
    }
    
    if (state->terminal && isatty(state->terminal->stdin_fd)) {
        // Use direct terminal operations for immediate response
        // Move to beginning of current line and clear it completely
        if (!lle_terminal_write(state->terminal, "\r", 1)) {
            return false;
        }
        if (!lle_terminal_clear_to_eol(state->terminal)) {
            return false;
        }
        
        // Write search prompt components directly for immediate visual feedback
        if (!lle_terminal_write(state->terminal, "(reverse-i-search)`", 19)) {
            return false;
        }
        
        // Write search term if provided
        if (search_term && search_length > 0) {
            if (!lle_terminal_write(state->terminal, search_term, search_length)) {
                return false;
            }
        }
        
        // Write search prompt suffix
        if (!lle_terminal_write(state->terminal, "': ", 3)) {
            return false;
        }
        
        // Write matched text if provided (with reasonable length limit)
        if (match_text && match_length > 0) {
            size_t max_display = 60; // Conservative limit for terminal width
            size_t display_length = match_length > max_display ? max_display : match_length;
            if (!lle_terminal_write(state->terminal, match_text, display_length)) {
                return false;
            }
            if (match_length > max_display) {
                if (!lle_terminal_write(state->terminal, "...", 3)) {
                    return false;
                }
            }
        }
    }
    
    // Maintain display state consistency - mark as needing refresh for cleanup
    state->needs_refresh = true;
    
    return true;
}