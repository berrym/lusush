/**
 * @file display_state_integration.c
 * @brief Lusush Line Editor - Display State Synchronization Integration Implementation
 *
 * This module implements integration wrappers for the existing display system
 * to use the new unified bidirectional state synchronization system. It
 * provides drop-in replacements for existing terminal operations that
 * automatically maintain state consistency.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "display_state_integration.h"
#include "edit_commands.h"
#include "termcap/lle_termcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

// Debug logging macros
#define LLE_INTEGRATION_DEBUG_ENABLED 1

#if LLE_INTEGRATION_DEBUG_ENABLED
#define LLE_INTEGRATION_DEBUG(fmt, ...) \
    do { \
        if (getenv("LLE_INTEGRATION_DEBUG")) { \
            fprintf(stderr, "[LLE_INTEGRATION] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)
#else
#define LLE_INTEGRATION_DEBUG(fmt, ...)
#endif

// Performance measurement utilities
static uint64_t lle_integration_get_timestamp_us(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

// ============================================================================
// Integration Initialization and Cleanup
// ============================================================================

/**
 * @brief Initialize display state synchronization integration
 */
lle_display_integration_t *lle_display_integration_init(lle_display_state_t *display,
                                                        lle_terminal_manager_t *terminal) {
    if (!display || !terminal) {
        return NULL;
    }
    
    lle_display_integration_t *integration = calloc(1, sizeof(lle_display_integration_t));
    if (!integration) {
        return NULL;
    }
    
    // Initialize state sync context
    integration->sync_ctx = lle_state_sync_init(terminal, display);
    if (!integration->sync_ctx) {
        free(integration);
        return NULL;
    }
    
    // Set references
    integration->display = display;
    integration->terminal = terminal;
    
    // Configure integration
    integration->sync_enabled = true;
    integration->validation_enabled = true;
    integration->debug_mode = false;
    integration->sync_frequency = 5;  // Sync every 5 operations by default
    
    // Initialize tracking
    integration->operations_since_sync = 0;
    integration->pending_validation = false;
    integration->total_operations = 0;
    integration->total_sync_time = 0;
    integration->sync_skip_count = 0;
    
    LLE_INTEGRATION_DEBUG("Integration initialized successfully");
    return integration;
}

/**
 * @brief Cleanup display state synchronization integration
 */
void lle_display_integration_cleanup(lle_display_integration_t *integration) {
    if (!integration) {
        return;
    }
    
    lle_state_sync_cleanup(integration->sync_ctx);
    free(integration);
    
    LLE_INTEGRATION_DEBUG("Integration cleaned up");
}

/**
 * @brief Enable or disable state synchronization
 */
void lle_display_integration_set_sync_enabled(lle_display_integration_t *integration,
                                              bool enabled) {
    if (integration) {
        integration->sync_enabled = enabled;
        LLE_INTEGRATION_DEBUG("Sync enabled: %s", enabled ? "true" : "false");
    }
}

/**
 * @brief Set synchronization frequency
 */
void lle_display_integration_set_sync_frequency(lle_display_integration_t *integration,
                                                size_t frequency) {
    if (integration) {
        integration->sync_frequency = frequency;
        LLE_INTEGRATION_DEBUG("Sync frequency set to: %zu", frequency);
    }
}

/**
 * @brief Enable or disable debug mode
 */
void lle_display_integration_set_debug_mode(lle_display_integration_t *integration,
                                            bool enabled) {
    if (integration) {
        integration->debug_mode = enabled;
        LLE_INTEGRATION_DEBUG("Debug mode: %s", enabled ? "enabled" : "disabled");
    }
}

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Check if synchronization should be performed
 */
static bool lle_integration_should_sync(lle_display_integration_t *integration) {
    if (!integration || !integration->sync_enabled) {
        return false;
    }
    
    // Always sync if frequency is 0
    if (integration->sync_frequency == 0) {
        return true;
    }
    
    // Sync based on operation count
    return (integration->operations_since_sync >= integration->sync_frequency);
}

/**
 * @brief Perform conditional synchronization
 */
static bool lle_integration_conditional_sync(lle_display_integration_t *integration) {
    if (!integration) {
        return false;
    }
    
    integration->operations_since_sync++;
    integration->total_operations++;
    
    if (lle_integration_should_sync(integration)) {
        uint64_t start_time = lle_integration_get_timestamp_us();
        bool success = lle_state_sync_perform(integration->sync_ctx);
        uint64_t end_time = lle_integration_get_timestamp_us();
        
        integration->total_sync_time += (end_time - start_time);
        integration->operations_since_sync = 0;
        
        if (integration->debug_mode) {
            LLE_INTEGRATION_DEBUG("Conditional sync %s (time: %lu us)",
                                  success ? "SUCCESS" : "FAILED",
                                  end_time - start_time);
        }
        
        return success;
    } else {
        integration->sync_skip_count++;
        return true;
    }
}

/**
 * @brief Update display state after operation
 */
static bool lle_integration_update_display_state(lle_display_integration_t *integration,
                                                 const char *operation_type) {
    if (!integration) {
        return false;
    }
    
    // Mark validation as pending
    integration->pending_validation = true;
    
    // Update expected content if we have access to buffer
    if (integration->display && integration->display->buffer) {
        lle_display_sync_update_expected(integration->sync_ctx,
                                        integration->display->buffer->buffer,
                                        integration->display->buffer->length,
                                        integration->display->buffer->cursor_pos,
                                        1);  // Simplified row calculation
    }
    
    if (integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("Display state updated after %s", operation_type);
    }
    
    return true;
}

// ============================================================================
// Drop-in Replacements for Terminal Operations
// ============================================================================

/**
 * @brief Write data to terminal with state synchronization
 */
bool lle_display_integration_terminal_write(lle_display_integration_t *integration,
                                            const char *data,
                                            size_t length) {
    if (!integration || !data || length == 0) {
        return false;
    }
    
    // Perform the actual write using state sync wrapper
    bool success = lle_state_sync_terminal_write(integration->sync_ctx, data, length);
    
    if (success) {
        // Update display state
        lle_integration_update_display_state(integration, "terminal_write");
        
        // Conditional synchronization
        lle_integration_conditional_sync(integration);
        
        if (integration->debug_mode) {
            LLE_INTEGRATION_DEBUG("Terminal write: %zu bytes", length);
        }
    } else {
        LLE_INTEGRATION_DEBUG("Terminal write failed: %zu bytes", length);
    }
    
    return success;
}

/**
 * @brief Clear terminal to end of line with state synchronization
 */
bool lle_display_integration_clear_to_eol(lle_display_integration_t *integration) {
    if (!integration) {
        return false;
    }
    
    // Get current cursor position for state tracking
    size_t cursor_row = integration->display ? integration->display->cursor_pos.absolute_row : 0;
    size_t cursor_col = integration->display ? integration->display->cursor_pos.absolute_col : 0;
    size_t terminal_width = integration->display ? integration->display->geometry.width : 80;
    
    // Update state tracking first
    bool success = lle_terminal_state_update_clear(integration->sync_ctx, "eol",
                                                   cursor_row, cursor_col,
                                                   cursor_row, terminal_width - 1);
    
    if (success) {
        // Use termcap function and manually update state tracking
        int termcap_result = lle_termcap_clear_to_eol();
        success = (termcap_result == LLE_TERMCAP_OK);
        
        if (success) {
            // Clear to EOL doesn't change cursor position, just clears content
            if (integration->display) {
                integration->display->position_tracking_valid = true;
            }
            
            lle_integration_update_display_state(integration, "clear_to_eol");
            lle_integration_conditional_sync(integration);
            
            if (integration->debug_mode) {
                LLE_INTEGRATION_DEBUG("Clear to EOL at (%zu,%zu) via termcap with state update", cursor_row, cursor_col);
            }
        } else {
            LLE_INTEGRATION_DEBUG("Failed to clear to EOL at (%zu,%zu), termcap result: %d", cursor_row, cursor_col, termcap_result);
        }
    }
    
    return success;
}

/**
 * @brief Clear entire terminal line with state synchronization
 */
bool lle_display_integration_clear_line(lle_display_integration_t *integration,
                                        size_t line_number) {
    if (!integration) {
        return false;
    }
    
    size_t terminal_width = integration->display ? integration->display->geometry.width : 80;
    
    // Update state tracking
    bool success = lle_terminal_state_update_clear(integration->sync_ctx, "line",
                                                   line_number, 0,
                                                   line_number, terminal_width - 1);
    
    if (success) {
        // Use termcap function and manually update state tracking
        int termcap_result = lle_termcap_clear_line();
        success = (termcap_result == LLE_TERMCAP_OK);
        
        if (success) {
            // Manually update cursor position tracking after successful clear
            if (integration->display) {
                integration->display->cursor_pos.absolute_col = 0;
                integration->display->position_tracking_valid = true;
            }
            
            lle_integration_update_display_state(integration, "clear_line");
            lle_integration_conditional_sync(integration);
            
            if (integration->debug_mode) {
                LLE_INTEGRATION_DEBUG("Clear line %zu via termcap with state update", line_number);
            }
        } else {
            LLE_INTEGRATION_DEBUG("Failed to clear line %zu, termcap result: %d", line_number, termcap_result);
        }
    }
    
    return success;
}

/**
 * @brief Move cursor to specific position with state synchronization
 */
bool lle_display_integration_move_cursor(lle_display_integration_t *integration,
                                         size_t row,
                                         size_t col) {
    if (!integration) {
        return false;
    }
    
    // Update state tracking
    bool success = lle_terminal_state_update_cursor(integration->sync_ctx, row, col);
    
    if (success) {
        // Use termcap function and manually update state tracking
        int termcap_result = lle_termcap_move_cursor((int)(row + 1), (int)(col + 1));
        success = (termcap_result == LLE_TERMCAP_OK);
        
        if (success) {
            // Manually update cursor position tracking after successful move
            if (integration->display) {
                integration->display->cursor_pos.absolute_row = row;
                integration->display->cursor_pos.absolute_col = col;
                integration->display->position_tracking_valid = true;
                
                // Update content end tracking if this represents content end
                if (integration->display->buffer) {
                    size_t prompt_width = 0;
                    if (integration->display->prompt) {
                        prompt_width = lle_prompt_get_last_line_width(integration->display->prompt);
                    }
                    size_t content_length = integration->display->buffer->length;
                    size_t absolute_pos = prompt_width + content_length;
                    size_t expected_row = absolute_pos / integration->display->geometry.width;
                    size_t expected_col = absolute_pos % integration->display->geometry.width;
                    
                    // If this position matches content end, update content end tracking
                    if (row == expected_row && col == expected_col) {
                        integration->display->content_end_row = row;
                        integration->display->content_end_col = col;
                    }
                }
            }
            
            lle_integration_update_display_state(integration, "move_cursor");
            lle_integration_conditional_sync(integration);
            
            if (integration->debug_mode) {
                LLE_INTEGRATION_DEBUG("Move cursor to (%zu,%zu) via termcap with state update", row, col);
            }
        } else {
            LLE_INTEGRATION_DEBUG("Failed to move cursor to (%zu,%zu), termcap result: %d", row, col, termcap_result);
        }
    }
    
    return success;
}

/**
 * @brief Move cursor up by specified number of rows
 */
bool lle_display_integration_move_cursor_up(lle_display_integration_t *integration,
                                            size_t rows) {
    if (!integration || rows == 0) {
        return false;
    }
    
    // Use termcap function and manually update state tracking
    int termcap_result = lle_termcap_move_cursor_up((int)rows);
    bool success = (termcap_result == LLE_TERMCAP_OK);
    
    if (success) {
        // Manually update cursor position tracking after successful move
        if (integration->display) {
            if (integration->display->cursor_pos.absolute_row >= rows) {
                integration->display->cursor_pos.absolute_row -= rows;
                integration->display->position_tracking_valid = true;
            } else {
                // Can't move up that far, invalidate tracking
                integration->display->position_tracking_valid = false;
            }
        }
        
        lle_integration_update_display_state(integration, "move_cursor_up");
        lle_integration_conditional_sync(integration);
        
        if (integration->debug_mode) {
            LLE_INTEGRATION_DEBUG("Moved cursor up %zu rows via termcap with state update", rows);
        }
    } else {
        LLE_INTEGRATION_DEBUG("Failed to move cursor up %zu rows, termcap result: %d", rows, termcap_result);
    }
    
    return success;
}

/**
 * @brief Move cursor down by specified number of rows
 */
bool lle_display_integration_move_cursor_down(lle_display_integration_t *integration,
                                              size_t rows) {
    if (!integration || rows == 0) {
        return false;
    }
    
    // Use termcap function and manually update state tracking
    int termcap_result = lle_termcap_move_cursor_down((int)rows);
    bool success = (termcap_result == LLE_TERMCAP_OK);
    
    if (success) {
        // Manually update cursor position tracking after successful move
        if (integration->display) {
            integration->display->cursor_pos.absolute_row += rows;
            integration->display->position_tracking_valid = true;
        }
        
        lle_integration_update_display_state(integration, "move_cursor_down");
        lle_integration_conditional_sync(integration);
        
        if (integration->debug_mode) {
            LLE_INTEGRATION_DEBUG("Moved cursor down %zu rows via termcap with state update", rows);
        }
    } else {
        LLE_INTEGRATION_DEBUG("Failed to move cursor down %zu rows, termcap result: %d", rows, termcap_result);
    }
    
    return success;
}

/**
 * @brief Move cursor to beginning of current line (column 0)
 */
bool lle_display_integration_move_cursor_home(lle_display_integration_t *integration) {
    if (!integration || !integration->display) {
        return false;
    }
    
    // Get current row from cursor position tracking
    size_t current_row = integration->display->cursor_pos.absolute_row;
    
    LLE_INTEGRATION_DEBUG("Moving cursor to line beginning: row=%zu, col=0", current_row);
    
    // Use termcap function to move to column 0 of current line
    int termcap_result = lle_termcap_cursor_to_column(0);
    
    // Manually update state tracking after successful move
    if (termcap_result == LLE_TERMCAP_OK && integration->display) {
        integration->display->cursor_pos.absolute_col = 0;
        integration->display->position_tracking_valid = true;
        
        if (integration->debug_mode) {
            LLE_INTEGRATION_DEBUG("Moved cursor to beginning of line via termcap with state update");
        }
    } else {
        LLE_INTEGRATION_DEBUG("Failed to move cursor to line beginning, termcap result: %d", termcap_result);
    }
    
    return (termcap_result == LLE_TERMCAP_OK);
}

/**
 * @brief Move cursor to end of current content
 */
bool lle_display_integration_move_cursor_end(lle_display_integration_t *integration) {
    if (!integration || !integration->display || !integration->display->buffer) {
        return false;
    }
    
    size_t content_length = integration->display->buffer->length;
    size_t prompt_width = 0;
    
    if (integration->display->prompt) {
        prompt_width = lle_prompt_get_last_line_width(integration->display->prompt);
    }
    
    // Simple, reliable approach: just move to the column position
    // This avoids complex row tracking that can get out of sync
    size_t target_column = prompt_width + content_length;
    

    
    // Use relative cursor movement to get to the end position
    // First, move to beginning of line
    bool success = lle_display_integration_terminal_write(integration, "\r", 1);
    
    // Then move to target column
    if (success && target_column > 0) {
        char move_right[32];
        snprintf(move_right, sizeof(move_right), "\x1b[%zuC", target_column);
        success = lle_display_integration_terminal_write(integration, move_right, strlen(move_right));
    }
    
    // Update display state cursor tracking with current position
    if (success && integration->display) {
        // Don't try to calculate absolute row - just update column
        integration->display->cursor_pos.absolute_col = target_column % integration->display->geometry.width;
        integration->display->content_end_col = integration->display->cursor_pos.absolute_col;
        // Keep position tracking valid if it was already valid
    }
    

    return success;
}

// ============================================================================
// Enhanced Operations with State Awareness
// ============================================================================

/**
 * @brief Replace content with automatic clearing and state sync
 */
bool lle_display_integration_replace_content(lle_display_integration_t *integration,
                                             const char *old_content,
                                             size_t old_length,
                                             const char *new_content,
                                             size_t new_length) {
    if (!integration) {
        return false;
    }
    
    // Allow new_content to be NULL for clearing operations
    if (new_content && new_length == 0) {
        new_content = NULL;  // Normalize empty content to NULL
    }
    
    bool success = true;
    
    // Step 1: Clear old content using prompt-based approach
    if (old_content && old_length > 0) {
        // Always use comprehensive clearing that redraws prompt
        // This avoids all multiline cursor positioning issues
        
        // Move to beginning of line using state-synchronized function
        success = lle_display_integration_move_cursor_home(integration);
        
        // Clear to end of line using state-synchronized function
        if (success) {
            success = lle_display_integration_clear_to_eol(integration);
        }
        
        // For any multiline content, use precise line calculation clearing strategy
        if (success && integration->display && integration->display->prompt) {
            size_t prompt_width = lle_prompt_get_last_line_width(integration->display->prompt);
            size_t terminal_width = integration->display->geometry.width;
            if (terminal_width == 0) terminal_width = 80;
            
            // The old_length represents what's currently displayed on screen that needs clearing
            // This is the content we're replacing, so it's what the user sees right now
            size_t total_chars = prompt_width + old_length;
            if (total_chars > terminal_width) {
                // Calculate actual lines used by wrapped content
                size_t actual_lines = ((total_chars - 1) / terminal_width) + 1;
                size_t additional_lines = actual_lines > 1 ? actual_lines - 1 : 0;
                
                // Clear only the actual additional lines needed using state-synchronized functions
                for (size_t i = 0; i < additional_lines && success; i++) {
                    // Move cursor down one line and clear to end of line
                    if (lle_display_integration_move_cursor_down(integration, 1)) {
                        if (!lle_display_integration_clear_to_eol(integration)) {
                            success = false;
                            break;
                        }
                    } else {
                        success = false;
                        break;
                    }
                }
                
                // Move cursor back up only the actual lines cleared using state-synchronized function
                if (additional_lines > 0 && success) {
                    if (!lle_display_integration_move_cursor_up(integration, additional_lines)) {
                        success = false;
                    }
                }
            }
        }
        
        // Redraw prompt to ensure correct positioning
        if (success && integration->display->prompt && integration->display->prompt->text) {
            success = lle_display_integration_terminal_write(integration, 
                                                            integration->display->prompt->text,
                                                            strlen(integration->display->prompt->text));
        }
        
        if (!success) {
            LLE_INTEGRATION_DEBUG("Failed to clear old content with prompt redraw");
            return false;
        }
    }
    
    // Step 2: Write new content (only if we have content to write)
    if (new_content && new_length > 0) {
        success = lle_display_integration_terminal_write(integration, new_content, new_length);
        if (!success) {
            LLE_INTEGRATION_DEBUG("Failed to write new content");
            return false;
        }
    }
    
    // Step 3: Force synchronization after content replacement
    success = lle_display_integration_force_sync(integration);
    
    if (integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("Content replacement: %zu->%zu chars, %s",
                              old_length, new_length, success ? "SUCCESS" : "FAILED");
    }
    
    return success;
}

/**
 * @brief Clear content using proven backspace boundary logic
 * 
 * Uses the exact same backspace operations that work perfectly for wrapped
 * lines. This ensures 100% identical behavior to manual user backspacing.
 * 
 * @param integration Integration context with state synchronization
 * @param content_length Length of content to clear using backspace logic
 * @return true on success, false on error
 */
bool lle_display_integration_clear_with_backspace_logic(lle_display_integration_t *integration,
                                                       size_t content_length) {
    if (!integration || !integration->display || content_length == 0) {
        return true; // Nothing to clear
    }
    
    fprintf(stderr, "[BACKSPACE_DEBUG] Starting backspace clearing for %zu characters\n", content_length);
    
    // Use the proven backspace command for each character
    // This leverages the existing boundary crossing logic that works perfectly
    bool success = true;
    for (size_t i = 0; i < content_length && success; i++) {
        // Call the proven backspace command that handles wrapped lines correctly
        lle_command_result_t result = lle_cmd_backspace(integration->display);
        if (result != LLE_CMD_SUCCESS) {
            fprintf(stderr, "[BACKSPACE_DEBUG] Backspace failed at character %zu\n", i);
            success = false;
            break;
        }
        if (i % 10 == 0 || i == content_length - 1) {
            fprintf(stderr, "[BACKSPACE_DEBUG] Cleared %zu/%zu characters\n", i + 1, content_length);
        }
    }
    
    // Force state synchronization after backspace operations
    if (success) {
        success = lle_display_integration_force_sync(integration);
        fprintf(stderr, "[BACKSPACE_DEBUG] Backspace clearing completed successfully\n");
    } else {
        fprintf(stderr, "[BACKSPACE_DEBUG] Backspace clearing failed\n");
    }
    
    return success;
}

/**
 * @brief Replace content using proven backspace boundary logic
 * 
 * Uses the exact same backspace operations that work perfectly for wrapped
 * lines, then inserts new content. This ensures 100% identical behavior
 * to manual user input and leverages proven boundary crossing logic.
 * 
 * @param integration Integration context with state synchronization
 * @param old_content Previous content (for clearing calculation)
 * @param old_length Length of previous content
 * @param new_content New content to display
 * @param new_length Length of new content
 * @return true on success, false on error
 */
bool lle_display_integration_replace_content_backspace(lle_display_integration_t *integration,
                                                      const char *old_content,
                                                      size_t old_length,
                                                      const char *new_content,
                                                      size_t new_length) {
    if (!integration || !integration->display) {
        return false;
    }
    
    fprintf(stderr, "[BACKSPACE_DEBUG] Replace content called: old_length=%zu, new_length=%zu\n", old_length, new_length);
    
    // Allow new_content to be NULL for clearing operations
    if (new_content && new_length == 0) {
        new_content = NULL;
    }
    
    bool success = true;
    
    // Step 1: Clear old content using proven backspace logic
    if (old_content && old_length > 0) {
        fprintf(stderr, "[BACKSPACE_DEBUG] Moving cursor to end before clearing\n");
        // Move cursor to end of content first (proven approach)
        lle_command_result_t move_result = lle_cmd_move_end(integration->display);
        if (move_result != LLE_CMD_SUCCESS) {
            fprintf(stderr, "[BACKSPACE_DEBUG] Move to end failed\n");
            success = false;
        }
        
        // Use proven backspace boundary logic - exactly like user input
        if (success) {
            fprintf(stderr, "[BACKSPACE_DEBUG] Starting backspace clearing sequence\n");
            success = lle_display_integration_clear_with_backspace_logic(integration, old_length);
        }
        
        if (!success) {
            LLE_INTEGRATION_DEBUG("Failed to clear old content with backspace logic");
            return false;
        }
    }
    
    // Step 2: Insert new content character by character (proven approach)
    if (new_content && new_length > 0 && success) {
        fprintf(stderr, "[BACKSPACE_DEBUG] Starting character insertion for %zu characters\n", new_length);
        for (size_t i = 0; i < new_length && success; i++) {
            // Use proven character insertion that handles wrapping correctly
            lle_command_result_t insert_result = lle_cmd_insert_char(integration->display, new_content[i]);
            if (insert_result != LLE_CMD_SUCCESS) {
                fprintf(stderr, "[BACKSPACE_DEBUG] Character insertion failed at position %zu\n", i);
                success = false;
                break;
            }
        }
        
        if (success) {
            fprintf(stderr, "[BACKSPACE_DEBUG] Character insertion completed successfully\n");
        } else {
            LLE_INTEGRATION_DEBUG("Failed to insert new content");
            return false;
        }
    }
    
    // Step 3: Force comprehensive state synchronization
    if (success) {
        success = lle_display_integration_force_sync(integration);
    }
    
    if (integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("Backspace-based content replacement: %zu->%zu chars, %s",
                              old_length, new_length, success ? "SUCCESS" : "FAILED");
    }
    
    return success;
}

/**
 * @brief Replace content with optimized clearing for tab completion
 * 
 * Optimized version for tab completion that only clears the specific text
 * area being replaced instead of doing full boundary crossing clearing.
 * This prevents visual corruption during tab completion cycling.
 * 
 * @param integration Integration context with state synchronization
 * @param old_content Previous content (for clearing calculation)
 * @param old_length Length of previous content
 * @param new_content New content to display
 * @param new_length Length of new content
 * @return true on success, false on error
 */
bool lle_display_integration_replace_content_optimized(lle_display_integration_t *integration,
                                                      const char *old_content,
                                                      size_t old_length,
                                                      const char *new_content,
                                                      size_t new_length) {
    if (!integration || !integration->display) {
        return false;
    }
    
    LLE_INTEGRATION_DEBUG("Optimized replace: old_length=%zu, new_length=%zu", old_length, new_length);
    
    // Allow new_content to be NULL for clearing operations
    if (new_content && new_length == 0) {
        new_content = NULL;
    }
    
    bool success = true;
    
    // Step 1: Minimal but proper clearing for tab completion
    if (old_content && old_length > 0) {
        // Try cursor home operation - if it fails, skip clearing entirely
        success = lle_display_integration_move_cursor_home(integration);
        
        if (success) {
            // Clear only to end of line - avoids heavy multiline clearing
            success = lle_display_integration_clear_to_eol(integration);
            
            // Redraw prompt for consistent positioning
            if (success && integration->display->prompt && integration->display->prompt->text) {
                success = lle_display_integration_terminal_write(integration, 
                                                                integration->display->prompt->text,
                                                                strlen(integration->display->prompt->text));
            }
        } else {
            // Cursor home failed - fall back to no clearing for tab completion
            // This avoids the heavy multiline clearing that caused corruption
            LLE_INTEGRATION_DEBUG("Cursor home failed - using fallback no-clear approach for tab completion");
            success = true; // Continue with just content replacement
        }
    }
    
    // Step 2: Write new content directly
    if (new_content && new_length > 0 && success) {
        success = lle_display_integration_terminal_write(integration, new_content, new_length);
        if (!success) {
            LLE_INTEGRATION_DEBUG("Failed to write new content optimally");
            return false;
        }
    }
    
    // Step 3: Restore position tracking for menu display
    if (success && integration->display) {
        // Restore position tracking validity after successful content replacement
        integration->display->position_tracking_valid = true;
        
        // Update display state tracking
        lle_integration_update_display_state(integration, "optimized_replace");
        lle_integration_conditional_sync(integration);
        
        LLE_INTEGRATION_DEBUG("Position tracking restored after optimized replacement");
    }
    
    LLE_INTEGRATION_DEBUG("Optimized content replacement: %zu->%zu chars, %s",
                          old_length, new_length, success ? "SUCCESS" : "FAILED");
    
    return success;
}

/**
 * @brief Clear specific region with state synchronization
 */
bool lle_display_integration_clear_region(lle_display_integration_t *integration,
                                          size_t start_row,
                                          size_t start_col,
                                          size_t end_row,
                                          size_t end_col) {
    if (!integration) {
        return false;
    }
    
    // Use state sync terminal clear
    bool success = lle_state_sync_terminal_clear(integration->sync_ctx,
                                                 start_row, start_col,
                                                 end_row, end_col);
    
    if (success) {
        lle_integration_update_display_state(integration, "clear_region");
        lle_integration_conditional_sync(integration);
        
        if (integration->debug_mode) {
            LLE_INTEGRATION_DEBUG("Clear region (%zu,%zu) to (%zu,%zu)",
                                  start_row, start_col, end_row, end_col);
        }
    }
    
    return success;
}

/**
 * @brief Insert character at current cursor position
 */
bool lle_display_integration_insert_char(lle_display_integration_t *integration,
                                         char character) {
    if (!integration) {
        return false;
    }
    
    // Write single character
    bool success = lle_display_integration_terminal_write(integration, &character, 1);
    
    if (success && integration->display && integration->display->buffer) {
        // Update cursor position
        integration->display->buffer->cursor_pos++;
        if (integration->display->buffer->cursor_pos > integration->display->buffer->length) {
            integration->display->buffer->length = integration->display->buffer->cursor_pos;
        }
        
        if (integration->debug_mode) {
            LLE_INTEGRATION_DEBUG("Insert char '%c', cursor now at %zu",
                                  character, integration->display->buffer->cursor_pos);
        }
    }
    
    return success;
}

/**
 * @brief Delete character at current cursor position (backspace)
 */
bool lle_display_integration_delete_char(lle_display_integration_t *integration) {
    if (!integration) {
        return false;
    }
    
    return lle_display_integration_exact_backspace(integration, 1);
}

/**
 * @brief Perform exact backspace sequence replication
 */
bool lle_display_integration_exact_backspace(lle_display_integration_t *integration,
                                             size_t char_count) {
    if (!integration || char_count == 0) {
        return false;
    }
    
    bool success = true;
    
    // Perform character-by-character backspace with state tracking
    for (size_t i = 0; i < char_count && success; i++) {
        // Send backspace, space, backspace sequence
        const char *backspace_seq = "\b \b";
        success = lle_display_integration_terminal_write(integration, backspace_seq, 3);
        
        if (success && integration->display && integration->display->buffer) {
            // Update buffer state
            if (integration->display->buffer->cursor_pos > 0) {
                integration->display->buffer->cursor_pos--;
            }
            if (integration->display->buffer->length > 0) {
                integration->display->buffer->length--;
            }
        }
    }
    
    if (integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("Exact backspace: %zu chars, %s",
                              char_count, success ? "SUCCESS" : "FAILED");
    }
    
    return success;
}

// ============================================================================
// State Validation and Synchronization Control
// ============================================================================

/**
 * @brief Force immediate state synchronization
 */
bool lle_display_integration_force_sync(lle_display_integration_t *integration) {
    if (!integration) {
        return false;
    }
    
    uint64_t start_time = lle_integration_get_timestamp_us();
    bool success = lle_state_sync_perform(integration->sync_ctx);
    uint64_t end_time = lle_integration_get_timestamp_us();
    
    integration->total_sync_time += (end_time - start_time);
    integration->operations_since_sync = 0;
    integration->pending_validation = false;
    
    if (integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("Force sync %s (time: %lu us)",
                              success ? "SUCCESS" : "FAILED",
                              end_time - start_time);
    }
    
    return success;
}

/**
 * @brief Validate current display state consistency
 */
bool lle_display_integration_validate_state(lle_display_integration_t *integration) {
    if (!integration) {
        return false;  // NULL integration is invalid
    }
    
    if (!integration->validation_enabled) {
        return true;
    }
    
    bool valid = lle_state_sync_validate(integration->sync_ctx);
    
    if (!valid && integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("State validation FAILED - divergence detected");
    }
    
    return valid;
}

/**
 * @brief Mark display state as dirty (requiring sync)
 */
bool lle_display_integration_mark_dirty(lle_display_integration_t *integration,
                                        const char *reason) {
    if (!integration) {
        return false;
    }
    
    bool success = lle_display_sync_mark_dirty(integration->sync_ctx, reason);
    integration->pending_validation = true;
    
    if (integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("Marked dirty: %s", reason ? reason : "unknown");
    }
    
    return success;
}

/**
 * @brief Check if synchronization is currently required
 */
bool lle_display_integration_needs_sync(lle_display_integration_t *integration) {
    if (!integration) {
        return false;
    }
    
    return integration->sync_ctx->display_sync->sync_required || 
           integration->pending_validation ||
           lle_integration_should_sync(integration);
}

/**
 * @brief Reset state tracking after major operations
 */
bool lle_display_integration_reset_tracking(lle_display_integration_t *integration) {
    if (!integration) {
        return false;
    }
    
    // Reset operation counters
    integration->operations_since_sync = 0;
    integration->pending_validation = false;
    
    // Force complete resynchronization
    bool success = lle_state_sync_force_resync(integration->sync_ctx);
    
    if (integration->debug_mode) {
        LLE_INTEGRATION_DEBUG("Reset tracking: %s", success ? "SUCCESS" : "FAILED");
    }
    
    return success;
}

// ============================================================================
// Migration Helpers
// ============================================================================

/**
 * @brief Convert existing display state to use integration
 */
lle_display_integration_t *lle_display_integration_migrate(lle_display_state_t *display,
                                                           lle_terminal_manager_t *terminal) {
    // For now, migration is the same as initialization
    // In the future, this could preserve existing state
    return lle_display_integration_init(display, terminal);
}

/**
 * @brief Check if display state is compatible with integration
 */
bool lle_display_integration_is_compatible(lle_display_state_t *display,
                                           lle_terminal_manager_t *terminal) {
    // Basic compatibility check
    return (display != NULL && terminal != NULL && 
            display->initialized && terminal->termcap_initialized);
}

/**
 * @brief Get integration performance statistics
 */
bool lle_display_integration_get_stats(lle_display_integration_t *integration,
                                       uint64_t *total_ops,
                                       size_t *total_syncs,
                                       double *avg_sync_time,
                                       double *sync_skip_rate) {
    if (!integration) {
        return false;
    }
    
    if (total_ops) {
        *total_ops = integration->total_operations;
    }
    
    if (total_syncs) {
        *total_syncs = integration->sync_ctx->display_sync->sync_operations;
    }
    
    if (avg_sync_time) {
        if (integration->sync_ctx->display_sync->sync_operations > 0) {
            *avg_sync_time = (double)integration->total_sync_time / 
                           (double)integration->sync_ctx->display_sync->sync_operations;
        } else {
            *avg_sync_time = 0.0;
        }
    }
    
    if (sync_skip_rate) {
        if (integration->total_operations > 0) {
            *sync_skip_rate = (double)integration->sync_skip_count / 
                            (double)integration->total_operations;
        } else {
            *sync_skip_rate = 0.0;
        }
    }
    
    return true;
}

// ============================================================================
// Debug and Diagnostic Functions
// ============================================================================

/**
 * @brief Dump integration state for debugging
 */
void lle_display_integration_debug_dump(lle_display_integration_t *integration,
                                        FILE *output_file) {
    if (!integration) {
        return;
    }
    
    FILE *out = output_file ? output_file : stderr;
    
    fprintf(out, "\n=== INTEGRATION STATE DUMP ===\n");
    fprintf(out, "Sync enabled: %s\n", integration->sync_enabled ? "yes" : "no");
    fprintf(out, "Validation enabled: %s\n", integration->validation_enabled ? "yes" : "no");
    fprintf(out, "Debug mode: %s\n", integration->debug_mode ? "yes" : "no");
    fprintf(out, "Sync frequency: %zu\n", integration->sync_frequency);
    fprintf(out, "Operations since sync: %zu\n", integration->operations_since_sync);
    fprintf(out, "Pending validation: %s\n", integration->pending_validation ? "yes" : "no");
    fprintf(out, "Total operations: %lu\n", integration->total_operations);
    fprintf(out, "Total sync time: %lu us\n", integration->total_sync_time);
    fprintf(out, "Sync skip count: %zu\n", integration->sync_skip_count);
    
    if (integration->total_operations > 0) {
        double skip_rate = (double)integration->sync_skip_count / (double)integration->total_operations;
        fprintf(out, "Sync skip rate: %.2f%%\n", skip_rate * 100);
    }
    
    fprintf(out, "=== END INTEGRATION STATE ===\n\n");
    
    // Also dump underlying sync state
    lle_state_sync_debug_dump_terminal(integration->sync_ctx, out);
    lle_state_sync_debug_dump_display(integration->sync_ctx, out);
}

/**
 * @brief Enable detailed operation logging
 */
void lle_display_integration_set_verbose_logging(lle_display_integration_t *integration,
                                                 bool enabled) {
    if (integration) {
        integration->debug_mode = enabled;
        lle_state_sync_set_strict_mode(integration->sync_ctx, enabled);
        
        LLE_INTEGRATION_DEBUG("Verbose logging: %s", enabled ? "enabled" : "disabled");
    }
}

/**
 * @brief Validate integration state and report issues
 */
size_t lle_display_integration_validate_and_report(lle_display_integration_t *integration,
                                                   FILE *output_file) {
    if (!integration) {
        return 0;
    }
    
    FILE *out = output_file ? output_file : stderr;
    size_t issues = 0;
    
    fprintf(out, "\n=== INTEGRATION VALIDATION REPORT ===\n");
    
    // Check basic state
    if (!integration->sync_ctx) {
        fprintf(out, "ISSUE: Sync context is NULL\n");
        issues++;
    }
    
    if (!integration->display) {
        fprintf(out, "ISSUE: Display state is NULL\n");
        issues++;
    }
    
    if (!integration->terminal) {
        fprintf(out, "ISSUE: Terminal manager is NULL\n");
        issues++;
    }
    
    // Check state consistency
    if (!lle_display_integration_validate_state(integration)) {
        fprintf(out, "ISSUE: Display state is inconsistent\n");
        issues++;
    }
    
    // Check for excessive sync failures
    if (integration->sync_ctx->display_sync->sync_failure_count > 3) {
        fprintf(out, "ISSUE: High sync failure count: %zu\n",
                integration->sync_ctx->display_sync->sync_failure_count);
        issues++;
    }
    
    // Check for performance issues
    if (integration->sync_ctx->display_sync->average_sync_time > 1000.0) {
        fprintf(out, "WARNING: High average sync time: %.2f us\n",
                integration->sync_ctx->display_sync->average_sync_time);
    }
    
    if (issues == 0) {
        fprintf(out, "No issues found - integration state is healthy\n");
    }
    
    fprintf(out, "Total issues: %zu\n", issues);
    fprintf(out, "=== END VALIDATION REPORT ===\n\n");
    
    return issues;
}

// ============================================================================
// Menu-Specific Display Integration Functions
// ============================================================================

/**
 * @brief Show completion menu using state-synchronized operations
 *
 * @param integration Display state integration context
 * @param footprint Visual footprint with menu positioning data
 * @param completion_display Completion display configuration
 * @return true on success, false on error
 *
 * Displays completion menu at calculated safe position using integrated
 * terminal operations that maintain perfect state synchronization.
 */
bool lle_display_integration_show_completion_menu(
    lle_display_integration_t *integration,
    const lle_visual_footprint_t *footprint,
    lle_completion_display_t *completion_display) {
    
    if (!integration || !footprint || !completion_display) {
        return false;
    }
    
    LLE_INTEGRATION_DEBUG("Starting completion menu display");
    
    // Validate state before menu operations
    if (!lle_display_integration_validate_state(integration)) {
        LLE_INTEGRATION_DEBUG("State invalid before menu - forcing sync");
        if (!lle_display_integration_force_sync(integration)) {
            return false;
        }
    }
    
    // Move to safe menu position - add newline first to move below current content
    if (!lle_display_integration_terminal_write(integration, "\n", 1)) {
        LLE_INTEGRATION_DEBUG("Failed to write newline for menu separation");
        return false;
    }
    
    // Display each completion item using basic text operations
    for (size_t i = 0; i < completion_display->display_count; i++) {
        if (i >= completion_display->completions->count) {
            break;
        }
        
        // Move to next line for items after the first
        if (i > 0) {
            if (!lle_display_integration_terminal_write(integration, "\n", 1)) {
                LLE_INTEGRATION_DEBUG("Failed to write newline for item %zu", i);
                continue;
            }
        }
        
        // Get completion item and format it simply
        const lle_completion_item_t *item = &completion_display->completions->items[i];
        char line_buffer[256];
        
        // Simple formatting without calling private function
        snprintf(line_buffer, sizeof(line_buffer), "> %s", 
                item->text ? item->text : "");
        
        // Write item with state tracking
        if (!lle_display_integration_terminal_write(integration, line_buffer, strlen(line_buffer))) {
            LLE_INTEGRATION_DEBUG("Failed to write item %zu", i);
            continue;
        }
        
        // Validate state after each item
        lle_display_integration_validate_state(integration);
    }
    
    LLE_INTEGRATION_DEBUG("Completion menu displayed successfully");
    return true;
}

/**
 * @brief Restore cursor position after menu display
 *
 * @param integration Display state integration context
 * @param menu_lines_displayed Number of menu lines that were displayed
 * @return true on success, false on error
 *
 * Restores cursor to original input position using precise integrated
 * cursor movements that maintain state synchronization.
 */
bool lle_display_integration_restore_cursor_after_menu(
    lle_display_integration_t *integration,
    size_t menu_lines_displayed) {
    
    if (!integration) {
        return false;
    }
    
    LLE_INTEGRATION_DEBUG("Restoring cursor after %zu menu lines", menu_lines_displayed);
    
    // Move cursor back to input line using precise movements
    for (size_t i = 0; i < menu_lines_displayed; i++) {
        if (!lle_display_integration_move_cursor_up(integration, 1)) {
            LLE_INTEGRATION_DEBUG("Failed to move up line %zu", i);
            break;
        }
    }
    
    // Move to end of input text using safe termcap
    if (!lle_display_integration_move_cursor_end(integration)) {
        LLE_INTEGRATION_DEBUG("Failed to move to end of line");
        return false;
    }
    
    // Validate final state
    bool success = lle_display_integration_validate_state(integration);
    LLE_INTEGRATION_DEBUG("Cursor restoration %s", success ? "successful" : "failed");
    
    return success;
}

/**
 * @brief Move cursor to specific column using integrated termcap
 *
 * @param integration Display state integration context
 * @param column Target column (1-based)
 * @return true on success, false on error
 *
 * Moves cursor to specified column on current row using state-synchronized
 * termcap operations.
 */
bool lle_display_integration_move_to_column(
    lle_display_integration_t *integration,
    size_t column) {
    
    if (!integration) {
        return false;
    }
    
    // Use integrated termcap for column movement
    if (!lle_termcap_cursor_to_column((int)column)) {
        return false;
    }
    
    // Update state tracking
    if (integration->sync_ctx && integration->sync_ctx->terminal_state) {
        integration->sync_ctx->terminal_state->cursor_col = column - 1; // Convert to 0-based
    }
    
    return true;
}

/**
 * @brief Get terminal geometry from integration context
 *
 * @param integration Display state integration context
 * @param geometry Output geometry structure
 * @return true if geometry retrieved successfully, false otherwise
 *
 * Retrieves current terminal geometry from the display state integration
 * system for accurate positioning calculations.
 */
bool lle_display_integration_get_terminal_geometry(
    lle_display_integration_t *integration,
    lle_terminal_geometry_t *geometry) {
    
    if (!integration || !geometry || !integration->display) {
        return false;
    }
    
    // Get geometry from display state
    *geometry = integration->display->geometry;
    
    return true;
}

/**
 * @brief Validate menu positioning against actual terminal state
 *
 * @param integration Display state integration context
 * @param footprint Visual footprint with menu positioning data
 * @return true if positioning is valid, false otherwise
 *
 * Validates that calculated menu positions are consistent with actual
 * terminal state and won't cause visual corruption.
 */
bool lle_display_integration_validate_menu_positioning(
    lle_display_integration_t *integration,
    const lle_visual_footprint_t *footprint) {
    
    if (!integration || !footprint) {
        return false;
    }
    
    // Validate basic positioning data
    if (!footprint->menu_positioning_valid) {
        return false;
    }
    
    // Check against actual terminal state
    if (integration->sync_ctx && integration->sync_ctx->terminal_state) {
        size_t actual_height = integration->sync_ctx->terminal_state->height;
        size_t actual_width = integration->sync_ctx->terminal_state->width;
        
        // Validate menu fits within actual terminal bounds
        if (footprint->safe_menu_start_row > actual_height ||
            footprint->menu_required_width > actual_width) {
            return false;
        }
    }
    
    return true;
}