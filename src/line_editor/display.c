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
#include "display_state_sync.h"
#include "display_state_integration.h"
#include "edit_commands.h"
#include "buffer_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// Platform detection types for display strategy selection
typedef enum {
    LLE_PLATFORM_MACOS,
    LLE_PLATFORM_LINUX,
    LLE_PLATFORM_UNKNOWN
} lle_platform_type_t;

// Cached platform detection for Phase 2C performance optimization
static lle_platform_type_t cached_platform = LLE_PLATFORM_UNKNOWN;
static bool platform_detected = false;

/**
 * @brief Detect the current platform for display strategy selection (Phase 2C optimized)
 * @return Platform type
 */
static lle_platform_type_t lle_detect_platform(void) {
    if (platform_detected) {
        return cached_platform;
    }
    
#ifdef __APPLE__
    cached_platform = LLE_PLATFORM_MACOS;
#elif defined(__linux__)
    cached_platform = LLE_PLATFORM_LINUX;
#else
    cached_platform = LLE_PLATFORM_UNKNOWN;
#endif
    
    platform_detected = true;
    return cached_platform;
}

// ============================================================================
// Phase 2C: Performance Optimization Utilities
// ============================================================================

/**
 * @brief Get current time in microseconds for performance measurement
 * @return Current time in microseconds, 0 on error
 */
static uint64_t lle_get_time_microseconds(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

/**
 * @brief Fast validation for performance-critical paths (Phase 2C)
 * @param state Display state to validate
 * @return true if state is valid for fast operations
 */
static bool lle_display_fast_validate(const lle_display_state_t *state) {
    return (state && state->initialized && state->prompt && state->buffer && state->terminal);
}

// Forward declarations for helper functions
static bool lle_display_render_plain_text(lle_display_state_t *state,
                                         const char *text,
                                         size_t text_length,
                                         size_t start_col);
static bool lle_display_clear_to_eol_linux_safe(lle_display_state_t *state);
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
    
    // Initialize boundary crossing tracking
    state->boundary_crossing_handled = false;
    
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
    
    // Initialize multi-line absolute position tracking (Phase 1A: Architecture Rewrite)
    state->prompt_start_row = 0;
    state->prompt_start_col = 0;
    state->prompt_end_row = 0;
    state->prompt_end_col = 0;
    state->content_start_row = 0;
    state->content_start_col = 0;
    state->content_end_row = 0;
    state->content_end_col = 0;
    state->position_tracking_valid = false;
    
    // Initialize syntax highlighting integration
    state->syntax_highlighter = NULL;
    state->theme_integration = NULL;
    state->syntax_highlighting_enabled = true; // Enable by default
    state->last_applied_color[0] = '\0';
    
    // Phase 2C: Initialize performance optimization components
    memset(&state->display_cache, 0, sizeof(lle_display_cache_t));
    memset(&state->terminal_batch, 0, sizeof(lle_terminal_batch_t));
    memset(&state->performance_metrics, 0, sizeof(lle_display_performance_t));
    state->performance_optimization_enabled = true;
    
    // Initialize display state tracking for true incremental updates (Character Duplication Fix)
    state->last_displayed_content[0] = '\0';
    state->last_displayed_length = 0;
    state->display_state_valid = false;  // Will be set on first use
    
    // Initialize enhanced visual footprint tracking for backspace refinement
    state->last_visual_rows = 1;
    state->last_visual_end_col = 0;
    state->last_total_chars = 0;
    state->last_had_wrapping = false;
    
    // Initialize consistency tracking
    state->last_content_hash = 0;
    state->syntax_highlighting_applied = false;
    
    // Initialize clearing region tracking
    state->clear_start_row = 0;
    state->clear_start_col = 0;
    state->clear_end_row = 0;
    state->clear_end_col = 0;
    state->clear_region_valid = false;
    
    // Initialize performance optimization components
    if (!lle_display_cache_init(&state->display_cache, 4096)) {
        // Cache initialization failed - continue without caching
        state->performance_optimization_enabled = false;
    }
    
    if (!lle_terminal_batch_init(&state->terminal_batch, 2048)) {
        // Batch initialization failed - continue without batching
        if (state->display_cache.cached_content) {
            lle_display_cache_cleanup(&state->display_cache);
        }
        state->performance_optimization_enabled = false;
    }
    
    if (!lle_display_performance_init(&state->performance_metrics)) {
        // Performance metrics initialization failed - continue without metrics
        if (state->display_cache.cached_content) {
            lle_display_cache_cleanup(&state->display_cache);
        }
        if (state->terminal_batch.batch_buffer) {
            lle_terminal_batch_cleanup(&state->terminal_batch);
        }
        state->performance_optimization_enabled = false;
    }
    
    // Initialize unified display state synchronization integration
    // ENABLED - Integration successfully resolves display corruption
    state->state_integration = NULL; // Will be set by parent line editor
    
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
    state->initialized = true;
    
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
    
    // Clean up unified display state synchronization integration
    // ENABLED - Integration successfully resolves display corruption
    state->state_integration = NULL; // Managed by parent line editor
    
    // Reset fields but don't free associated structures
    state->prompt = NULL;
    state->buffer = NULL;
    state->terminal = NULL;
    state->initialized = false;
    
    // Clear syntax highlighting references (don't destroy - we don't own them)
    state->syntax_highlighter = NULL;
    state->theme_integration = NULL;
    state->syntax_highlighting_enabled = true; // Enable by default
    state->last_applied_color[0] = '\0';
    
    // Phase 2C: Clean up performance optimization components
    if (state->performance_optimization_enabled) {
        lle_display_cache_cleanup(&state->display_cache);
        lle_terminal_batch_cleanup(&state->terminal_batch);
    }
    state->performance_optimization_enabled = false;
    
    return true;
}

/**
 * @brief Destroy display state and free memory
 */
void lle_display_destroy(lle_display_state_t *state) {
    if (!state) {
        return;
    }
    
    // Clean up unified display state synchronization integration
    // ENABLED - Integration successfully resolves display corruption
    state->state_integration = NULL; // Managed by parent line editor
    
    // Phase 2C: Ensure performance optimization components are cleaned up
    if (state->performance_optimization_enabled) {
        lle_display_cache_cleanup(&state->display_cache);
        lle_terminal_batch_cleanup(&state->terminal_batch);
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
    
    // Clear display if flag is set OR on initialization to prevent content bleeding
    bool should_clear = (state->display_flags & LLE_DISPLAY_FLAG_CLEAR_FIRST) || 
                       (state->buffer && state->buffer->length == 0 && state->geometry.width > 0);
    
    if (should_clear) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Clearing display to prevent content bleeding\n");
        }
        if (!lle_display_clear(state)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_DISPLAY_RENDER] Display clear failed\n");
            }
            return false;
        }
    }
    
    // CRITICAL FIX: Disable cursor queries during interactive mode to prevent input contamination
    // Cursor queries send ^[[6n and responses like ^[[37;1R contaminate stdin
    // Use mathematical positioning instead
    state->prompt_start_row = 0;
    state->prompt_start_col = 0;
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_RENDER] Cursor queries disabled - using mathematical positioning\n");
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

    // Calculate prompt end position for content positioning (Phase 2A: Position Tracking)
    lle_prompt_geometry_t prompt_geom;
    prompt_geom.width = lle_prompt_get_width(state->prompt);
    prompt_geom.height = lle_prompt_get_height(state->prompt);
    prompt_geom.last_line_width = lle_prompt_get_last_line_width(state->prompt);
    
    lle_terminal_coordinates_t content_start = lle_calculate_content_start_coordinates(
        state->prompt_start_row, state->prompt_start_col, &prompt_geom);
    
    if (content_start.valid) {
        state->content_start_row = content_start.terminal_row;
        state->content_start_col = content_start.terminal_col;
        state->position_tracking_valid = true;
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Content start position: row=%zu, col=%zu\n", 
                   content_start.terminal_row, content_start.terminal_col);
        }
    } else {
        state->position_tracking_valid = false;
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_RENDER] Failed to calculate content start position\n");
        }
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
            
            if (cursor_pos.valid && state->position_tracking_valid) {
                // Phase 2A: Convert relative cursor position to absolute terminal coordinates
                lle_terminal_coordinates_t terminal_pos = lle_convert_to_terminal_coordinates(
                    &cursor_pos, state->content_start_row, state->content_start_col);
                
                if (debug_mode) {
                    fprintf(stderr, "[LLE_DISPLAY_RENDER] Converted cursor position: terminal_row=%zu, terminal_col=%zu, valid=%s\n",
                           terminal_pos.terminal_row, terminal_pos.terminal_col, terminal_pos.valid ? "true" : "false");
                }
                
                // Validate coordinates before using
                if (terminal_pos.valid && lle_validate_terminal_coordinates(&terminal_pos, &state->geometry)) {
                    // Use absolute positioning instead of relative positioning
                    if (!lle_terminal_move_cursor(state->terminal, terminal_pos.terminal_row, terminal_pos.terminal_col)) {
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_DISPLAY_RENDER] Failed to move cursor to absolute position (%zu, %zu)\n", 
                                   terminal_pos.terminal_row, terminal_pos.terminal_col);
                        }
                    } else {
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_DISPLAY_RENDER] Absolute cursor positioning completed successfully\n");
                        }
                    }
                } else {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_DISPLAY_RENDER] Invalid terminal coordinates, cursor positioning skipped\n");
                    }
                }
            } else {
                if (debug_mode) {
                    if (!cursor_pos.valid) {
                        fprintf(stderr, "[LLE_DISPLAY_RENDER] Invalid cursor position, staying at current location\n");
                    } else {
                        fprintf(stderr, "[LLE_DISPLAY_RENDER] Position tracking invalid, skipping cursor positioning\n");
                    }
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
    
    // Validate display state consistency after complex operations
    // TEMPORARILY DISABLED - Integration causing display corruption
    
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
    
    // Comprehensive terminal clearing to prevent content bleeding
    // Use proven backspace logic approach for multiline content
    if (state->state_integration && state->buffer && state->buffer->length > 0) {
        // Clear existing buffer content using proven backspace boundary logic
        size_t content_length = state->buffer->length;
        for (size_t i = 0; i < content_length; i++) {
            lle_command_result_t result = lle_cmd_backspace(state);
            if (result != LLE_CMD_SUCCESS) {
                break; // Continue with other clearing methods
            }
        }
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
 * @brief Validate boundary crossing state for Phase 1 mathematical framework
 *
 * Performs comprehensive validation of visual footprint calculations and
 * terminal coordinate mapping during boundary crossing operations.
 *
 * @param before_footprint Visual footprint before boundary crossing
 * @param after_footprint Visual footprint after boundary crossing
 * @param terminal_width Current terminal width for validation
 * @return true if state is valid, false if validation fails
 */
// REMOVED: Unused boundary validation function

/**
 * @brief Single authoritative boundary crossing handler for Phase 2
 *
 * Consolidates all boundary crossing logic into one comprehensive system.
 * Handles both visual footprint-based and coordinate-based boundary detection
 * with comprehensive error handling and fallback mechanisms.
 *
 * @param state Display state
 * @param footprint_before Visual footprint before operation
 * @param footprint_after Visual footprint after operation
 * @param text Current text content
 * @param text_length Length of current text
 * @param terminal_width Current terminal width
 * @return true if boundary crossing handled successfully, false if fallback needed
 */
// REMOVED: Broken boundary crossing function causing display corruption

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
    // Phase 2C: Start performance timing
    uint64_t start_time = 0;
    if (state && state->performance_optimization_enabled) {
        start_time = lle_display_performance_start_timing();
    }
    
    // Reset boundary crossing flag at start of update cycle
    if (state) {
        state->boundary_crossing_handled = false;
    }
    
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Starting incremental display update\n");
    }

    // Phase 2C: Use fast validation for performance-critical path
    bool validation_result = state && state->performance_optimization_enabled ? 
                            lle_display_fast_validate(state) : lle_display_validate(state);
    
    if (!validation_result) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Display validation failed\n");
        }
        return false;
    }

    // Phase 2C: Check cache validity for performance optimization
    if (state->performance_optimization_enabled && lle_display_cache_is_valid(state)) {
        state->display_cache.cache_hits++;
        if (debug_mode) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Cache hit - using cached content\n");
        }
        
        // End timing and record cache hit
        if (start_time > 0) {
            lle_display_performance_end_timing(&state->performance_metrics, start_time, "incremental");
        }
        return true;
    } else if (state->performance_optimization_enabled) {
        state->display_cache.cache_misses++;
    }

    // Phase 2C: Use cached platform detection for performance
    lle_platform_type_t platform = lle_detect_platform();
    if (debug_mode) {
        const char *platform_name = (platform == LLE_PLATFORM_MACOS) ? "macOS" : 
                                   (platform == LLE_PLATFORM_LINUX) ? "Linux" : "Unknown";
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Platform detected: %s\n", platform_name);
    }

    // Phase 2C: Start terminal batching if enabled
    bool batching_started = false;
    if (state->performance_optimization_enabled) {
        batching_started = lle_terminal_batch_start(&state->terminal_batch);
        if (debug_mode && batching_started) {
            fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Terminal batching started\n");
        }
    }

    // Continue with normal incremental update for all platforms
    // Linux compatibility is handled by using safe clear operations

    // Get text from buffer
    const char *text = state->buffer->buffer;
    size_t text_length = state->buffer->length;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_DISPLAY_INCREMENTAL] Text buffer: length=%zu\n", text_length);
    }

    // CRITICAL FIX: True incremental character updates
    
    // Initialize display state tracking if needed
    if (!state->display_state_valid) {
        memset(state->last_displayed_content, 0, sizeof(state->last_displayed_content));
        state->last_displayed_length = 0;
        state->display_state_valid = true;
    }
    
    // Reset tracking when starting new command session
    if (state->last_displayed_length > 5 && text_length <= 2) {
        state->last_displayed_length = 0;
        state->display_state_valid = true;
    }
    
    // CASE 1: Single character addition (MOST COMMON)
    if (text && text_length == state->last_displayed_length + 1 && 
        text_length > 0 &&
        memcmp(text, state->last_displayed_content, state->last_displayed_length) == 0) {
        
        // Just write the new character - NO CLEARING NEEDED
        char new_char = text[text_length - 1];
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_INCREMENTAL] True incremental: adding char '%c'\n", new_char);
        }
        
        if (!lle_display_integration_terminal_write(state->state_integration, &new_char, 1)) {
            return false;
        }
        
        // Update tracking
        memcpy(state->last_displayed_content, text, text_length);
        state->last_displayed_content[text_length] = '\0';
        state->last_displayed_length = text_length;
        
        return true;
    }
    
    // CASE 2: Single character deletion (BACKSPACE) - Enhanced with visual footprint calculation
    // CASE 2A: Simple backspace detection (MOST COMMON for deletion)
    if (text && text_length == state->last_displayed_length - 1 &&
        state->last_displayed_length > 0 &&
        memcmp(text, state->last_displayed_content, text_length) == 0) {
            
        if (debug_mode) {
            fprintf(stderr, "[LLE_INCREMENTAL] Simple backspace: deleting char\n");
        }
            
        // Simple terminal backspace with state synchronization
        if (!lle_display_integration_terminal_write(state->state_integration, "\b \b", 3)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_INCREMENTAL] Simple backspace failed\n");
            }
            return false;
        }
            
        // Update tracking
        if (text && text_length > 0) {
            memcpy(state->last_displayed_content, text, text_length);
            state->last_displayed_content[text_length] = '\0';
        } else {
            state->last_displayed_content[0] = '\0';
        }
        state->last_displayed_length = text_length;
            
        if (debug_mode) {
            fprintf(stderr, "[LLE_INCREMENTAL] Simple backspace completed, new length: %zu\n", state->last_displayed_length);
        }
            
        return true;
    }
    
    // CASE 3: No change detection (same content, same length)
    if (text_length == state->last_displayed_length && 
        text_length > 0 &&
        memcmp(text, state->last_displayed_content, text_length) == 0) {
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_INCREMENTAL] No change detected - content identical\n");
        }
        
        return true; // No update needed
    }
    
    // CASE 4: Complex changes - continue with controlled rewrite
    if (debug_mode) {
        fprintf(stderr, "[LLE_INCREMENTAL] Complex change: was %zu chars, now %zu chars\n", 
           state->last_displayed_length, text_length);
    }

// Use full redraw approach for multiline content with prompt redraw
if (debug_mode) {
    fprintf(stderr, "[LLE_INCREMENTAL] Using full redraw approach with prompt redraw\n");
}

// Get prompt width for positioning
size_t prompt_width = state->prompt ? lle_prompt_get_last_line_width(state->prompt) : 0;

// Force all history navigation to use full redraw with prompt redraw for consistency
size_t terminal_width = state->geometry.width;

if (debug_mode) {
    fprintf(stderr, "[LLE_INCREMENTAL] Using full redraw with prompt redraw for all history navigation\n");
}

// Clear all content completely using multiline approach
if (!lle_terminal_clear_multiline_content(state->terminal,
                                        state->last_displayed_length,
                                        prompt_width,
                                        terminal_width)) {
    if (debug_mode) {
        fprintf(stderr, "[LLE_INCREMENTAL] Content clearing failed\n");
    }
    return false;
}

// Always redraw prompt line completely for consistency
if (state->prompt) {
    if (!lle_prompt_render(state->terminal, state->prompt, false)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_INCREMENTAL] Prompt redraw failed\n");
        }
        return false;
    }
}

// Ensure cursor is positioned exactly at prompt end before writing content
if (!lle_display_integration_terminal_write(state->state_integration, "\r", 1)) {
    return false;
}
char move_right[32];
snprintf(move_right, sizeof(move_right), "\x1b[%zuC", prompt_width);
if (!lle_display_integration_terminal_write(state->state_integration, move_right, strlen(move_right))) {
    return false;
}

// Write new content
if (text && text_length > 0) {
    if (!lle_display_integration_terminal_write(state->state_integration, text, text_length)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_INCREMENTAL] Failed to write new content\n");
        }
        return false;
    }
}

if (debug_mode) {
    fprintf(stderr, "[LLE_INCREMENTAL] Full redraw approach completed\n");
}
    
// Update tracking
if (text && text_length > 0) {
    memcpy(state->last_displayed_content, text, text_length);
    state->last_displayed_content[text_length] = '\0';
} else {
    state->last_displayed_content[0] = '\0';
}
state->last_displayed_length = text_length;
    
if (debug_mode) {
    fprintf(stderr, "[LLE_INCREMENTAL] Simple rewrite completed\n");
}
    
// Position cursor correctly after text update
if (!lle_display_update_cursor(state)) {
    if (debug_mode) {
        fprintf(stderr, "[LLE_INCREMENTAL] Failed to update cursor position\n");
    }
    return false;
}
    
// Validate state after complex content replacement
if (!lle_display_integration_validate_state(state->state_integration)) {
    lle_display_integration_force_sync(state->state_integration);
}

if (debug_mode) {
    fprintf(stderr, "[LLE_INCREMENTAL] Simple strategy completed with cursor positioning\n");
}

// Validate display state consistency after incremental update
// TEMPORARILY DISABLED - Integration causing display corruption
    
return true;
}

/**
 * Linux-safe clear to end of line that avoids character duplication.
 * Uses character-by-character clearing instead of escape sequences.
 *
 * @param state Display state to use for terminal operations
 * @return true on success, false on error
 */
static bool lle_display_clear_to_eol_linux_safe(lle_display_state_t *state) {
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    lle_platform_type_t platform = lle_detect_platform();
    
    // On macOS, use the fast escape sequence method
    if (platform == LLE_PLATFORM_MACOS) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_CLEAR_EOL] Using fast macOS clear method\n");
        }
        return lle_terminal_clear_to_eol(state->terminal);
    }
    
    // On Linux, use character-by-character clearing to avoid duplication
    if (debug_mode) {
        fprintf(stderr, "[LLE_CLEAR_EOL] Using Linux-safe character clearing\n");
    }
    
    // For Linux, implement smart clearing based on terminal width and cursor position
    size_t terminal_width = state->geometry.width;
    size_t prompt_width = 0;
    if (state->prompt) {
        prompt_width = lle_prompt_get_last_line_width(state->prompt);
    }
    
    // Calculate maximum safe clearing distance to avoid wrapping
    size_t cursor_to_edge = terminal_width > prompt_width ? terminal_width - prompt_width : 0;
    size_t max_safe_clear = cursor_to_edge > 10 ? cursor_to_edge - 10 : 5; // Leave safety margin
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_CLEAR_EOL] Linux: Safe clearing distance: %zu (terminal=%zu, prompt=%zu)\n", 
               max_safe_clear, terminal_width, prompt_width);
    }
    
    // Write spaces to clear content, respecting terminal boundaries
    for (size_t i = 0; i < max_safe_clear; i++) {
        if (!lle_display_integration_terminal_write(state->state_integration, " ", 1)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_CLEAR_EOL] Failed to write clearing space %zu\n", i);
            }
            break;
        }
    }
    
    // Return cursor to starting position
    for (size_t i = 0; i < max_safe_clear; i++) {
        if (!lle_display_integration_terminal_write(state->state_integration, "\b", 1)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_CLEAR_EOL] Failed to backspace %zu\n", i);
            }
            break;
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_CLEAR_EOL] Linux-safe clear completed\n");
    }
    
    return true;
}

/**
 * Conservative display update strategy for Linux terminals.
 * NOTE: This function is kept for potential future use but is currently unused.
 * The main strategy now uses the normal incremental update with Linux-safe clearing.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
static bool __attribute__((unused)) lle_display_update_conservative(lle_display_state_t *state) {
    // This is now unused - kept for reference
    // The main incremental update handles Linux compatibility
    // through lle_display_clear_to_eol_linux_safe()
    return lle_display_update_incremental(state);
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
    
    // Validate state after clear-to-end-of-line operation
    if (!lle_display_integration_validate_state(state->state_integration)) {
        lle_display_integration_force_sync(state->state_integration);
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
    (void)start_col; // Terminal handles positioning naturally
    if (!state || !text) {
        return false;
    }
    
    // Process each character, letting terminal handle wrapping
    
    for (size_t i = state->display_start_offset; i < text_length; i++) {
        char c = text[i];
        
        // Handle newlines in input text
        // Handle newlines in input text
        if (c == '\n') {
            if (!lle_display_integration_terminal_write(state->state_integration, "\n", 1)) {
                return false;
            }
            continue;
        }
        
        // Let terminal handle wrapping naturally - no manual newlines needed
        // Terminal will automatically wrap when it reaches the right edge
        
        // Write the character - terminal handles positioning and wrapping
        if (!lle_display_integration_terminal_write(state->state_integration, &c, 1)) {
            return false;
        }
    }
    
    // Validate state after character rendering
    if (!lle_display_integration_validate_state(state->state_integration)) {
        lle_display_integration_force_sync(state->state_integration);
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
    
    // Always allow enabling - components may be connected later
    // The actual rendering will check for component availability
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
    if (!lle_display_integration_terminal_write(state->state_integration, color_code, strlen(color_code))) {
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
    if (!lle_display_integration_terminal_write(state->state_integration, reset_code, strlen(reset_code))) {
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

    // Phase 2B.3: Validate position tracking is available for absolute positioning
    // For incremental updates, position tracking may not be set yet, so initialize if needed
    if (!state->position_tracking_valid) {
        // Try to initialize position tracking for syntax highlighting
        state->content_start_row = 0;
        state->content_start_col = lle_prompt_get_last_line_width(state->prompt);
        state->position_tracking_valid = true;
        
        // If we still can't get valid positioning, fallback to plain text
        if (state->content_start_col == 0 && !state->prompt) {
            return lle_display_render_plain_text(state, text, length, start_col);
        }
    }
    
    // Get syntax regions
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(state->syntax_highlighter, &region_count);
    
    // Phase 2B.3: Calculate content start position for absolute positioning
    // For syntax highlighting, we start at the beginning of the content area
    
    // Create a cursor position representing the start of content (before any text)
    lle_cursor_position_t content_start = {0};
    content_start.absolute_row = 0;
    content_start.absolute_col = 0; // Start of content, not including prompt
    content_start.valid = true;
    
    // Phase 2B.3: Convert to absolute terminal coordinates using Phase 2A system
    lle_terminal_coordinates_t render_pos = lle_convert_to_terminal_coordinates(
        &content_start, state->content_start_row, state->content_start_col);
    
    if (!render_pos.valid) {
        // If coordinate conversion fails, just proceed with sequential rendering
        // The terminal write will handle positioning naturally
    } else {
        // Position cursor at content start using absolute positioning
        if (!lle_terminal_move_cursor(state->terminal, render_pos.terminal_row, render_pos.terminal_col)) {
            // If positioning fails, continue with sequential rendering
        }
    }
    
    // Render segments with appropriate colors using absolute positioning integration
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
        
        // Phase 2B.3: Write the segment with absolute positioning awareness
        // The terminal write will naturally wrap, and the absolute positioning
        // foundation ensures proper multi-line coordinate handling
        size_t segment_length = segment_end - text_pos;
        if (segment_length > 0) {
            if (!lle_display_integration_terminal_write(state->state_integration, text + text_pos, segment_length)) {
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
    
    // Phase 2A.3: Use absolute positioning for cursor home
    bool result = false;
    if (state->position_tracking_valid) {
        // Calculate cursor position at text start (home position)
        lle_cursor_position_t home_pos = {
            .absolute_row = 0,
            .absolute_col = 0,
            .relative_row = 0,
            .relative_col = 0,
            .at_boundary = false,
            .valid = true
        };
        
        // Convert to absolute terminal coordinates
        lle_terminal_coordinates_t terminal_pos = lle_convert_to_terminal_coordinates(
            &home_pos, state->content_start_row, state->content_start_col);
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_MOVE_HOME] Absolute position: terminal_row=%zu, terminal_col=%zu\n",
                   terminal_pos.terminal_row, terminal_pos.terminal_col);
        }
        
        // Use absolute positioning
        if (terminal_pos.valid && lle_validate_terminal_coordinates(&terminal_pos, &state->geometry)) {
            result = lle_terminal_move_cursor(state->terminal, terminal_pos.terminal_row, terminal_pos.terminal_col);
        } else {
            if (debug_mode) {
                fprintf(stderr, "[LLE_MOVE_HOME] Invalid coordinates, fallback to column positioning\n");
            }
            // Fallback to old method if coordinate conversion fails
            size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
            result = lle_terminal_move_cursor_to_column(state->terminal, prompt_width);
        }
    } else {
        if (debug_mode) {
            fprintf(stderr, "[LLE_MOVE_HOME] Position tracking invalid, using fallback method\n");
        }
        // Fallback to old method if position tracking is invalid
        size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
        result = lle_terminal_move_cursor_to_column(state->terminal, prompt_width);
    }
    
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
    
    // Phase 2A.3: Use absolute positioning for cursor end
    bool result = false;
    if (state->position_tracking_valid) {
        // Calculate cursor position using mathematical framework
        size_t prompt_last_line_width = lle_prompt_get_last_line_width(state->prompt);
        lle_cursor_position_t cursor_pos = lle_calculate_cursor_position(
            state->buffer, &state->geometry, prompt_last_line_width);
        
        if (cursor_pos.valid) {
            // Convert to absolute terminal coordinates
            lle_terminal_coordinates_t terminal_pos = lle_convert_to_terminal_coordinates(
                &cursor_pos, state->content_start_row, state->content_start_col);
            
            if (debug_mode) {
                fprintf(stderr, "[LLE_MOVE_END] Cursor position: row=%zu, col=%zu, terminal_row=%zu, terminal_col=%zu\n",
                       cursor_pos.absolute_row, cursor_pos.absolute_col, 
                       terminal_pos.terminal_row, terminal_pos.terminal_col);
            }
            
            // Use absolute positioning
            if (terminal_pos.valid && lle_validate_terminal_coordinates(&terminal_pos, &state->geometry)) {
                result = lle_terminal_move_cursor(state->terminal, terminal_pos.terminal_row, terminal_pos.terminal_col);
            } else {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_MOVE_END] Invalid coordinates, fallback to column positioning\n");
                }
                // Fallback to old method if coordinate conversion fails
                size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
                size_t text_width = lle_calculate_display_width_ansi(state->buffer->buffer, state->buffer->length);
                result = lle_terminal_move_cursor_to_column(state->terminal, prompt_width + text_width);
            }
        } else {
            if (debug_mode) {
                fprintf(stderr, "[LLE_MOVE_END] Invalid cursor position, fallback to column positioning\n");
            }
            // Fallback to old method if cursor calculation fails
            size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
            size_t text_width = lle_calculate_display_width_ansi(state->buffer->buffer, state->buffer->length);
            result = lle_terminal_move_cursor_to_column(state->terminal, prompt_width + text_width);
        }
    } else {
        if (debug_mode) {
            fprintf(stderr, "[LLE_MOVE_END] Position tracking invalid, using fallback method\n");
        }
        // Fallback to old method if position tracking is invalid
        size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
        size_t text_width = lle_calculate_display_width_ansi(state->buffer->buffer, state->buffer->length);
        result = lle_terminal_move_cursor_to_column(state->terminal, prompt_width + text_width);
    }
    
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
        if (!lle_display_integration_terminal_write(state->state_integration, "\n", 1)) {
            return false;
        }
        // Phase 2A.3: Use absolute positioning for search mode entry
        // Move to beginning of current line for search prompt
        size_t current_row, current_col;
        if (lle_terminal_query_cursor_position(state->terminal, &current_row, &current_col)) {
            if (!lle_terminal_move_cursor(state->terminal, current_row, 0)) {
                return false;
            }
        } else {
            // Fallback to column positioning if cursor query fails
            if (!lle_terminal_move_cursor_to_column(state->terminal, 0)) {
                return false;
            }
        }
    }
        
    // Validate state after search mode entry
    if (state->state_integration && !lle_display_integration_validate_state(state->state_integration)) {
        lle_display_integration_force_sync(state->state_integration);
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
        // Phase 2A.3: Use absolute positioning for search mode exit
        // Move to beginning of current line for clearing
        size_t current_row, current_col;
        if (lle_terminal_query_cursor_position(state->terminal, &current_row, &current_col)) {
            if (!lle_terminal_move_cursor(state->terminal, current_row, 0)) {
                return false;
            }
        } else {
            // Fallback to column positioning if cursor query fails
            if (!lle_terminal_move_cursor_to_column(state->terminal, 0)) {
                return false;
            }
        }
        if (!lle_display_clear_to_eol_linux_safe(state)) {
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
        if (!lle_display_integration_terminal_write(state->state_integration, "\r", 1)) {
            return false;
        }
        if (!lle_display_clear_to_eol_linux_safe(state)) {
            return false;
        }
        
        // Write search prompt components directly for immediate visual feedback
        if (!lle_display_integration_terminal_write(state->state_integration, "(reverse-i-search)`", 19)) {
            return false;
        }
        
        // Write search term if provided
        if (search_term && search_length > 0) {
            if (!lle_display_integration_terminal_write(state->state_integration, search_term, search_length)) {
                return false;
            }
        }
        
        // Write search prompt suffix
        if (!lle_display_integration_terminal_write(state->state_integration, "': ", 3)) {
            return false;
        }
        
        // Write matched text if provided (with reasonable length limit)
        if (match_text && match_length > 0) {
            size_t max_display = 60; // Conservative limit for terminal width
            size_t display_length = match_length > max_display ? max_display : match_length;
            if (!lle_display_integration_terminal_write(state->state_integration, match_text, display_length)) {
                return false;
            }
            if (match_length > max_display) {
                if (!lle_display_integration_terminal_write(state->state_integration, "...", 3)) {
                    return false;
                }
            }
        }
    }
    
    // Maintain display state consistency - mark as needing refresh for cleanup
    state->needs_refresh = true;
    
    return true;
}

// ============================================================================
// Phase 2C: Performance Optimization Function Implementations
// ============================================================================

/**
 * @brief Initialize display cache for performance optimization
 */
bool lle_display_cache_init(lle_display_cache_t *cache, size_t buffer_size) {
    if (!cache || buffer_size == 0) {
        return false;
    }
    
    cache->cached_content = malloc(buffer_size);
    if (!cache->cached_content) {
        return false;
    }
    
    cache->cache_size = buffer_size;
    cache->cached_length = 0;
    cache->cache_valid = false;
    cache->cached_text_length = 0;
    cache->cached_cursor_position = 0;
    cache->cached_display_flags = 0;
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    cache->cache_updates = 0;
    
    return true;
}

/**
 * @brief Clean up display cache resources
 */
bool lle_display_cache_cleanup(lle_display_cache_t *cache) {
    if (!cache) {
        return false;
    }
    
    if (cache->cached_content) {
        free(cache->cached_content);
        cache->cached_content = NULL;
    }
    
    cache->cache_size = 0;
    cache->cached_length = 0;
    cache->cache_valid = false;
    cache->cached_text_length = 0;
    cache->cached_cursor_position = 0;
    cache->cached_display_flags = 0;
    
    return true;
}

/**
 * @brief Check if cached content is valid for current state
 */
bool lle_display_cache_is_valid(const lle_display_state_t *state) {
    if (!state || !state->performance_optimization_enabled) {
        return false;
    }
    
    const lle_display_cache_t *cache = &state->display_cache;
    
    if (!cache->cache_valid || !cache->cached_content) {
        return false;
    }
    
    // Check if current state matches cached state
    size_t current_text_length = state->buffer ? state->buffer->length : 0;
    size_t current_cursor_position = state->buffer ? state->buffer->cursor_pos : 0;
    
    if (current_text_length != cache->cached_text_length ||
        current_cursor_position != cache->cached_cursor_position ||
        state->display_flags != cache->cached_display_flags) {
        return false;
    }
    
    // Validate state after color application
    if (!lle_display_integration_validate_state(state->state_integration)) {
        lle_display_integration_force_sync(state->state_integration);
    }
    
    return true;
}

/**
 * @brief Update display cache with current rendered content
 */
bool lle_display_cache_update(lle_display_state_t *state, const char *content, size_t length) {
    if (!state || !state->performance_optimization_enabled || !content) {
        return false;
    }
    
    lle_display_cache_t *cache = &state->display_cache;
    
    if (!cache->cached_content || length > cache->cache_size) {
        return false;
    }
    
    // Update cache content
    memcpy(cache->cached_content, content, length);
    cache->cached_length = length;
    cache->cache_valid = true;
    
    // Update cache metadata
    cache->cached_text_length = state->buffer ? state->buffer->length : 0;
    cache->cached_cursor_position = state->buffer ? state->buffer->cursor_pos : 0;
    cache->cached_display_flags = state->display_flags;
    cache->cache_updates++;
    
    return true;
}

/**
 * @brief Initialize terminal batching system
 */
bool lle_terminal_batch_init(lle_terminal_batch_t *batch, size_t buffer_size) {
    if (!batch || buffer_size == 0) {
        return false;
    }
    
    batch->batch_buffer = malloc(buffer_size);
    if (!batch->batch_buffer) {
        return false;
    }
    
    batch->buffer_size = buffer_size;
    batch->buffer_used = 0;
    batch->batch_active = false;
    batch->operations_batched = 0;
    batch->total_writes = 0;
    batch->bytes_written = 0;
    
    return true;
}

/**
 * @brief Clean up terminal batching resources
 */
bool lle_terminal_batch_cleanup(lle_terminal_batch_t *batch) {
    if (!batch) {
        return false;
    }
    
    if (batch->batch_buffer) {
        free(batch->batch_buffer);
        batch->batch_buffer = NULL;
    }
    
    batch->buffer_size = 0;
    batch->buffer_used = 0;
    batch->batch_active = false;
    batch->operations_batched = 0;
    
    return true;
}

/**
 * @brief Start terminal operation batching
 */
bool lle_terminal_batch_start(lle_terminal_batch_t *batch) {
    if (!batch || !batch->batch_buffer) {
        return false;
    }
    
    batch->buffer_used = 0;
    batch->batch_active = true;
    batch->operations_batched = 0;
    
    return true;
}

/**
 * @brief Add operation to terminal batch
 */
bool lle_terminal_batch_add(lle_terminal_batch_t *batch, const char *data, size_t length) {
    if (!batch || !batch->batch_active || !data || length == 0) {
        return false;
    }
    
    if (batch->buffer_used + length > batch->buffer_size) {
        return false; // Buffer full
    }
    
    memcpy(batch->batch_buffer + batch->buffer_used, data, length);
    batch->buffer_used += length;
    batch->operations_batched++;
    
    return true;
}

/**
 * @brief Flush all batched terminal operations
 */
bool lle_terminal_batch_flush(lle_display_state_t *state) {
    if (!state || !state->performance_optimization_enabled) {
        return false;
    }
    
    lle_terminal_batch_t *batch = &state->terminal_batch;
    
    if (!batch->batch_active || batch->buffer_used == 0) {
        return true; // Nothing to flush
    }
    
    // Write all batched operations in single call
    bool result = lle_display_integration_terminal_write(state->state_integration, batch->batch_buffer, batch->buffer_used);
    
    // Update statistics
    batch->total_writes++;
    batch->bytes_written += batch->buffer_used;
    
    // Reset batch for next operations
    batch->buffer_used = 0;
    batch->batch_active = false;
    
    return result;
}

/**
 * @brief Initialize performance metrics tracking
 */
bool lle_display_performance_init(lle_display_performance_t *metrics) {
    if (!metrics) {
        return false;
    }
    
    // Initialize timing metrics
    metrics->total_render_time = 0;
    metrics->total_incremental_time = 0;
    metrics->total_cache_time = 0;
    
    // Initialize operation counters
    metrics->render_calls = 0;
    metrics->incremental_calls = 0;
    metrics->cache_operations = 0;
    
    // Set performance targets (in microseconds)
    metrics->target_char_insert_time = 1000;    // 1ms
    metrics->target_cursor_move_time = 1000;    // 1ms
    metrics->target_display_update_time = 5000; // 5ms
    
    // Initialize efficiency metrics
    metrics->cache_hit_rate = 0.0;
    metrics->batch_efficiency = 0.0;
    
    return true;
}

/**
 * @brief Start timing a display operation
 */
uint64_t lle_display_performance_start_timing(void) {
    uint64_t time = lle_get_time_microseconds();
    return time > 0 ? time : 1; // Return 1 if time is 0 to indicate valid timing
}

/**
 * @brief End timing and record performance metric
 */
uint64_t lle_display_performance_end_timing(lle_display_performance_t *metrics, 
                                           uint64_t start_time, 
                                           const char *operation_type) {
    if (!metrics || start_time == 0) {
        return 0;
    }
    
    uint64_t end_time = lle_get_time_microseconds();
    uint64_t elapsed = (end_time > start_time) ? (end_time - start_time) : 0;
    
    // Update appropriate timing metrics based on operation type
    if (operation_type) {
        if (strcmp(operation_type, "render") == 0) {
            metrics->total_render_time += elapsed;
            metrics->render_calls++;
        } else if (strcmp(operation_type, "incremental") == 0) {
            metrics->total_incremental_time += elapsed;
            metrics->incremental_calls++;
        } else if (strcmp(operation_type, "cache") == 0) {
            metrics->total_cache_time += elapsed;
            metrics->cache_operations++;
        }
    }
    
    return elapsed;
}

/**
 * @brief Get current performance statistics
 */
bool lle_display_get_performance_stats(const lle_display_state_t *state,
                                      uint64_t *avg_render_time,
                                      uint64_t *avg_incremental_time,
                                      double *cache_hit_rate,
                                      double *batch_efficiency) {
    if (!state || !avg_render_time || !avg_incremental_time || !cache_hit_rate || !batch_efficiency) {
        return false;
    }
    
    const lle_display_performance_t *metrics = &state->performance_metrics;
    const lle_display_cache_t *cache = &state->display_cache;
    const lle_terminal_batch_t *batch = &state->terminal_batch;
    
    // Calculate average timing
    *avg_render_time = metrics->render_calls > 0 ? 
                       metrics->total_render_time / metrics->render_calls : 0;
    *avg_incremental_time = metrics->incremental_calls > 0 ? 
                           metrics->total_incremental_time / metrics->incremental_calls : 0;
    
    // Calculate cache hit rate
    size_t total_cache_requests = cache->cache_hits + cache->cache_misses;
    *cache_hit_rate = total_cache_requests > 0 ? 
                      (double)cache->cache_hits / total_cache_requests * 100.0 : 0.0;
    
    // Calculate batch efficiency
    *batch_efficiency = batch->total_writes > 0 ? 
                       (double)batch->operations_batched / batch->total_writes * 100.0 : 0.0;
    
    return true;
}

/**
 * @brief Enable or disable performance optimizations
 */
bool lle_display_set_performance_optimization(lle_display_state_t *state, bool enabled) {
    if (!state || !state->initialized) {
        return false;
    }
    
    state->performance_optimization_enabled = enabled;
    
    // If disabling, clean up resources
    if (!enabled) {
        lle_display_cache_cleanup(&state->display_cache);
        lle_terminal_batch_cleanup(&state->terminal_batch);
    }
    // If enabling, reinitialize resources
    else if (!state->display_cache.cached_content || !state->terminal_batch.batch_buffer) {
        if (!lle_display_cache_init(&state->display_cache, 4096) ||
            !lle_terminal_batch_init(&state->terminal_batch, 2048) ||
            !lle_display_performance_init(&state->performance_metrics)) {
            state->performance_optimization_enabled = false;
            return false;
        }
    }
    
    // Validate state after multiline text rendering
    if (!lle_display_integration_validate_state(state->state_integration)) {
        lle_display_integration_force_sync(state->state_integration);
    }
    
    return true;
}

/**
 * @brief Calculate the exact visual footprint of text content
 *
 * Calculates how much visual space text content will occupy on the terminal,
 * including line wrapping behavior and total display dimensions.
 *
 * @param text Text content to analyze
 * @param length Length of text content
 * @param prompt_width Width of prompt on first line
 * @param terminal_width Width of terminal for wrapping calculations
 * @param footprint Output structure for calculated footprint
 * @return true on success, false on error
 */
bool lle_calculate_visual_footprint(const char *text, size_t length,
                                   size_t prompt_width, size_t terminal_width,
                                   lle_visual_footprint_t *footprint) {
    if (!text || !footprint || terminal_width == 0) {
        return false;
    }
    
    // Initialize footprint structure
    memset(footprint, 0, sizeof(lle_visual_footprint_t));
    
    if (length == 0) {
        footprint->rows_used = 1;
        footprint->end_column = prompt_width;
        footprint->wraps_lines = false;
        footprint->total_visual_width = prompt_width;
        return true;
    }
    
    // Calculate display width of text (handling ANSI escape sequences)
    size_t text_display_width = lle_calculate_display_width_ansi(text, length);
    
    // Total width includes prompt on first line
    size_t total_width = prompt_width + text_display_width;
    footprint->total_visual_width = total_width;
    
    // Check if content wraps lines
    // CRITICAL FIX: Content that exactly fills terminal width should wrap (no room for cursor)
    if (total_width >= terminal_width) {
        footprint->wraps_lines = true;
        
        // Calculate prompt rows and position where text starts
        size_t prompt_rows = 0;
        size_t prompt_end_col = 0;
        
        if (prompt_width > 0) {
            prompt_rows = (prompt_width + terminal_width - 1) / terminal_width;
            prompt_end_col = prompt_width % terminal_width;
            if (prompt_end_col == 0) {
                prompt_end_col = terminal_width;
            }
        }
        
        // Calculate available space on the last prompt row
        size_t first_line_capacity;
        if (prompt_end_col == terminal_width) {
            // Prompt exactly fills last row, text starts on new line
            first_line_capacity = terminal_width;
        } else {
            // Text continues on same row as prompt end
            first_line_capacity = terminal_width - prompt_end_col;
        }
        
        if (text_display_width < first_line_capacity) {
            // All text fits on the same row as prompt end
            if (prompt_width == 0 || prompt_end_col == terminal_width) {
                // Text starts on new line (either no prompt or prompt fills row)
                footprint->rows_used = prompt_rows + 1;
                footprint->end_column = text_display_width;
            } else {
                // Text continues on prompt's last row
                footprint->rows_used = prompt_rows;
                footprint->end_column = prompt_end_col + text_display_width;
            }
        } else if (text_display_width == first_line_capacity) {
            // Text exactly fills the remaining space on current line
            // Cursor wraps to start of next line
            footprint->rows_used = prompt_rows + 1;
            footprint->end_column = 1;
        } else {
            // Text spans multiple lines beyond prompt
            size_t remaining_chars = text_display_width - first_line_capacity;
            size_t additional_rows = (remaining_chars + terminal_width - 1) / terminal_width;
            
            if (prompt_width == 0 || prompt_end_col == terminal_width) {
                // Text starts on new line after prompt
                footprint->rows_used = prompt_rows + 1 + additional_rows;
            } else {
                // Text continues on prompt's last row then spans additional rows
                footprint->rows_used = prompt_rows + additional_rows;
            }
            
            // Calculate final column position
            size_t chars_on_last_row = remaining_chars % terminal_width;
            if (chars_on_last_row == 0 && remaining_chars > 0) {
                chars_on_last_row = terminal_width;
            }
            footprint->end_column = chars_on_last_row;
        }
    } else {
        // Content fits on single line
        footprint->wraps_lines = false;
        footprint->rows_used = 1;
        
        // CRITICAL FIX: For single line content, end column is simply the calculated position
        // Since we're in the single-line branch, total_width <= terminal_width is guaranteed
        footprint->end_column = prompt_width + text_display_width;
    }
    
    return true;
}



// REMOVED: Broken visual region clearing causing display corruption

// REMOVED: Broken multi-line clearing function causing display corruption

/**
 * @brief Calculate the hash of content for change detection
 * @param content Content string to hash
 * @param length Length of content
 * @return Hash value for content
 */
// REMOVED: Unused content hash function

// REMOVED: Unused position cursor safe function

// REMOVED: Broken boundary safe update function

/**
 * @brief Clear a terminal line completely and safely
 * @param tm Terminal manager
 * @return true on success, false on error
 */
// REMOVED: Broken terminal clear line safe function

/**
 * @brief Get terminal cursor position with timeout protection
 * @param tm Terminal manager
 * @param row Pointer to store row position
 * @param col Pointer to store column position
 * @return true on success, false on error or timeout
 */
// REMOVED: Broken terminal query cursor safe function



// REMOVED: Broken enhanced functions




/**
 * @brief Ensure consistent rendering regardless of path taken
 *
 * Provides unified rendering behavior to prevent inconsistencies between
 * incremental updates and fallback rewrites.
 *
 * @param display Display state for rendering
 * @param force_full_render Whether to force complete rerender
 * @return true on success, false on error
 */
bool lle_display_update_unified(lle_display_state_t *display,
                               bool force_full_render) {
    if (!lle_display_validate(display)) {
        return false;
    }
    
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_UNIFIED] Starting unified rendering, force_full=%s\n",
               force_full_render ? "true" : "false");
    }
    
    // Get current text content
    const char *text = display->buffer->buffer;
    size_t text_length = display->buffer->length;
    
    // Calculate current visual footprint
    lle_visual_footprint_t current_footprint;
    size_t prompt_width = lle_prompt_get_last_line_width(display->prompt);
    if (!lle_calculate_visual_footprint(text, text_length, prompt_width, 
                                       display->geometry.width, &current_footprint)) {
        return false;
    }
    
    // Calculate hash of current content for consistency tracking
    uint32_t content_hash = 0;
    for (size_t i = 0; i < text_length; i++) {
        content_hash = content_hash * 31 + (uint32_t)text[i];
    }
    
    // Check if content has actually changed
    bool content_changed = (content_hash != display->last_content_hash);
    bool needs_full_render = force_full_render || content_changed || !display->display_state_valid;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_UNIFIED] Content changed: %s, needs_full_render: %s\n",
               content_changed ? "true" : "false", needs_full_render ? "true" : "false");
    }
    
    // Store previous footprint for clearing
    lle_visual_footprint_t old_footprint = {
        .rows_used = display->last_visual_rows,
        .end_column = display->last_visual_end_col,
        .wraps_lines = display->last_had_wrapping,
        .total_visual_width = display->last_total_chars
    };
    
    // Clear old content if necessary
    if (needs_full_render && display->display_state_valid) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_UNIFIED] Clearing old content\n");
        }
        // Simple clearing instead of broken visual region clearing
        lle_terminal_clear_to_eol(display->terminal);
    }
    
    // Render content with consistent highlighting policy
    bool render_success;
    if (needs_full_render) {
        render_success = lle_render_with_consistent_highlighting(display, &old_footprint, &current_footprint);
    } else {
        // No change needed
        render_success = true;
    }
    
    // Update display state tracking
    if (render_success) {
        display->last_visual_rows = current_footprint.rows_used;
        display->last_visual_end_col = current_footprint.end_column;
        display->last_total_chars = current_footprint.total_visual_width;
        display->last_had_wrapping = current_footprint.wraps_lines;
        display->last_content_hash = content_hash;
        display->syntax_highlighting_applied = display->syntax_highlighting_enabled;
        display->display_state_valid = true;
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_UNIFIED] Display state updated: rows=%zu, end_col=%zu, wraps=%s\n",
                   current_footprint.rows_used, current_footprint.end_column,
                   current_footprint.wraps_lines ? "true" : "false");
        }
    }
    
    return render_success;
}

/**
 * @brief Apply consistent highlighting policy
 *
 * Ensures syntax highlighting is applied consistently regardless of
 * whether content arrived via incremental updates or fallback rewrites.
 *
 * @param display Display state for highlighting
 * @param old_footprint Previous content footprint
 * @param new_footprint New content footprint
 * @return true on success, false on error
 */
bool lle_render_with_consistent_highlighting(lle_display_state_t *display,
                                            const lle_visual_footprint_t *old_footprint,
                                            const lle_visual_footprint_t *new_footprint) {
    if (!lle_display_validate(display)) {
        return false;
    }
    
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_CONSISTENT] Applying consistent highlighting\n");
    }
    
    // Get current text content
    const char *text = display->buffer->buffer;
    size_t text_length = display->buffer->length;
    
    if (!text || text_length == 0) {
        // No content to render
        return true;
    }
    
    // Determine whether to apply syntax highlighting
    // Apply highlighting consistently: if enabled and not already applied, or if force refresh
    bool should_apply_highlighting = display->syntax_highlighting_enabled && 
                                   display->syntax_highlighter &&
                                   (!display->syntax_highlighting_applied || 
                                    old_footprint->rows_used != new_footprint->rows_used);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_CONSISTENT] Should apply highlighting: %s (enabled=%s, highlighter=%p, applied=%s)\n",
               should_apply_highlighting ? "true" : "false",
               display->syntax_highlighting_enabled ? "true" : "false",
               (void*)display->syntax_highlighter,
               display->syntax_highlighting_applied ? "true" : "false");
    }
    
    // Render content with or without highlighting
    bool render_success;
    if (should_apply_highlighting) {
        // Use syntax highlighting render path
        if (debug_mode) {
            fprintf(stderr, "[LLE_CONSISTENT] Rendering with syntax highlighting\n");
        }
        render_success = lle_display_render_with_syntax_highlighting(display, text, text_length, 0);
        
        if (render_success) {
            display->syntax_highlighting_applied = true;
        }
    } else {
        // Use plain text render path
        if (debug_mode) {
            fprintf(stderr, "[LLE_CONSISTENT] Rendering without syntax highlighting\n");
        }
        
        // For backspace boundary crossing, don't rewrite content during editing
        // Only write content when doing full renders or final display
        if (display->syntax_highlighting_applied) {
            // Write text directly to terminal only if we're not in mid-edit
            render_success = display->state_integration ? 
                lle_display_integration_terminal_write(display->state_integration, text, text_length) :
                lle_terminal_write(display->terminal, text, text_length);
        } else {
            // During editing after boundary clear, just mark as successful without writing
            render_success = true;
        }
        
        if (render_success) {
            display->syntax_highlighting_applied = false;
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_CONSISTENT] Consistent rendering completed: %s\n",
               render_success ? "success" : "failed");
    }
    
    return render_success;
}

