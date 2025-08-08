/**
 * @file edit_commands.c
 * @brief Basic editing commands implementation for Lusush Line Editor (LLE)
 * 
 * This module implements basic editing command functionality for the line editor,
 * including character insertion, deletion, cursor movement, and line operations.
 * Commands operate on display state and update both text buffer and display.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "edit_commands.h"
#include "text_buffer.h"
#include "display.h"
#include "termcap/lle_termcap.h"
#include "buffer_trace.h"
#include "terminal_manager.h"
#include "display_state_integration.h"
#include "platform_detection.h"
#include "command_history.h"
#include "termcap/lle_termcap.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// ============================================================================
// Command Name Mappings
// ============================================================================

/**
 * @brief Command type to string name mappings
 */
static const char *command_type_names[] = {
    "INSERT_CHAR",
    "DELETE_CHAR",
    "BACKSPACE",
    "MOVE_CURSOR",
    "ACCEPT_LINE",
    "CANCEL_LINE",
    "CLEAR_LINE",
    "MOVE_HOME",
    "MOVE_END",
    "WORD_LEFT",
    "WORD_RIGHT",
    "DELETE_WORD",
    "BACKSPACE_WORD",
    "KILL_LINE",
    "KILL_BEGINNING",
    "HISTORY_UP",
    "HISTORY_DOWN"
};

/**
 * @brief Command result to string mappings
 */
static const char *command_result_names[] = {
    "SUCCESS",
    "ERROR_INVALID_PARAM",
    "ERROR_INVALID_STATE",
    "ERROR_BUFFER_FULL",
    "ERROR_INVALID_POSITION",
    "ERROR_DISPLAY_UPDATE",
    "ERROR_UNKNOWN_COMMAND"
};

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Check if character is word boundary
 * @param c Character to check
 * @return true if character is word boundary, false otherwise
 */
static bool is_word_boundary(char c) {
    return isspace(c) || ispunct(c);
}

/**
 * @brief Find next word boundary position
 * @param buffer Text buffer
 * @param start_pos Starting position
 * @param forward true for forward search, false for backward
 * @return Position of next word boundary
 */
static size_t find_word_boundary(const lle_text_buffer_t *buffer, size_t start_pos, bool forward) {
    if (!buffer || !buffer->buffer) return start_pos;
    
    if (forward) {
        // Move forward to next word boundary
        size_t pos = start_pos;
        
        // Skip current word
        while (pos < buffer->length && !is_word_boundary(buffer->buffer[pos])) {
            pos++;
        }
        
        // Skip whitespace/punctuation
        while (pos < buffer->length && is_word_boundary(buffer->buffer[pos])) {
            pos++;
        }
        
        return pos;
    } else {
        // Move backward to previous word boundary
        if (start_pos == 0) return 0;
        
        size_t pos = start_pos - 1;
        
        // Skip whitespace/punctuation
        while (pos > 0 && is_word_boundary(buffer->buffer[pos])) {
            pos--;
        }
        
        // Skip current word
        while (pos > 0 && !is_word_boundary(buffer->buffer[pos])) {
            pos--;
        }
        
        // Move to start of word (unless at beginning)
        if (pos > 0 && is_word_boundary(buffer->buffer[pos])) {
            pos++;
        }
        
        return pos;
    }
}



// ============================================================================
// Character Operation Functions
// ============================================================================

lle_command_result_t lle_cmd_insert_char(lle_display_state_t *state, char character) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    // Insert character at current cursor position
    if (!lle_text_insert_char(state->buffer, character)) {
        return LLE_CMD_ERROR_BUFFER_FULL;
    }
    
    // Invalidate position tracking since content has changed
    state->position_tracking_valid = false;
    
    // Use state synchronization system instead of problematic display functions
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Write character using state synchronization
            if (!lle_display_integration_terminal_write(state->state_integration, &character, 1)) {
                return LLE_CMD_ERROR_DISPLAY_UPDATE;
            }
            
            // Update display tracking state for consistency
            if (state->buffer && state->buffer->length < sizeof(state->last_displayed_content)) {
                memcpy(state->last_displayed_content, state->buffer->buffer, state->buffer->length);
                state->last_displayed_content[state->buffer->length] = '\0';
                state->last_displayed_length = state->buffer->length;
                state->display_state_valid = true;
            }
            
            // Validate state consistency after character insertion
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        } else {
            // Fallback if state integration not available
            if (state->terminal) {
                lle_terminal_write(state->terminal, &character, 1);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_delete_char(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    size_t cursor_pos = state->buffer->cursor_pos;
    
    // Can't delete at end of buffer
    if (cursor_pos >= state->buffer->length) {
        return LLE_CMD_SUCCESS; // Not an error, just nothing to delete
    }
    
    // Delete character at cursor position
    if (!lle_text_delete_char(state->buffer)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Use state synchronization system instead of problematic display functions
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // For delete character, just clear the character without moving cursor back
            lle_display_integration_terminal_write(state->state_integration, " \b", 2);
            
            // Update display tracking state for consistency
            if (state->buffer && state->buffer->length < sizeof(state->last_displayed_content)) {
                memcpy(state->last_displayed_content, state->buffer->buffer, state->buffer->length);
                state->last_displayed_content[state->buffer->length] = '\0';
                state->last_displayed_length = state->buffer->length;
                state->display_state_valid = true;
            }
            
            // Validate state consistency after delete operation
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        } else {
            // Fallback without state integration
            if (state->terminal) {
                lle_terminal_write(state->terminal, " \b", 2);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_backspace(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    // CRITICAL TRACE: Start backspace operation
    int trace_session = lle_trace_backspace_start(state->buffer, state);
    LLE_TRACE_CRITICAL("CMD_BACKSPACE_ENTRY", state->buffer);
    
    size_t cursor_pos = state->buffer->cursor_pos;
    
    // Can't backspace at beginning of buffer OR when buffer is empty
    if (cursor_pos == 0 || state->buffer->length == 0) {
        fprintf(stderr, "[VISUAL_DEBUG] BLOCKED: Backspace at buffer start (cursor_pos=%zu, length=%zu)\n", 
                cursor_pos, state->buffer->length);
        LLE_TRACE_BUFFER("CMD_BACKSPACE_NO_OP", state->buffer);
        lle_trace_backspace_end(trace_session, state->buffer, state, true);
        return LLE_CMD_SUCCESS; // Not an error, just nothing to delete
    }
    
    // Store original buffer content for content rewrite strategy
    char *old_content = NULL;
    size_t old_length = state->buffer->length;
    if (old_length > 0) {
        old_content = malloc(old_length + 1);
        if (old_content) {
            memcpy(old_content, state->buffer->buffer, old_length);
            old_content[old_length] = '\0';
        }
    }
    
    // CRITICAL TRACE: Before buffer modification
    LLE_TRACE_CRITICAL("BEFORE_TEXT_BACKSPACE", state->buffer);
    lle_trace_buffer_function("lle_text_backspace", state->buffer, true);
    
    // Delete character before cursor position
    if (!lle_text_backspace(state->buffer)) {
        LLE_TRACE_CRITICAL("TEXT_BACKSPACE_FAILED", state->buffer);
        lle_trace_buffer_function("lle_text_backspace", state->buffer, false);
        lle_trace_backspace_end(trace_session, state->buffer, state, false);
        free(old_content);
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // CRITICAL TRACE: After buffer modification  
    lle_trace_buffer_function("lle_text_backspace", state->buffer, false);
    LLE_TRACE_CRITICAL("AFTER_TEXT_BACKSPACE", state->buffer);
    
    // Validate buffer consistency
    if (!lle_trace_validate_buffer_consistency(state->buffer)) {
        LLE_TRACE_CRITICAL("BUFFER_CONSISTENCY_ERROR", state->buffer);
    }
    
    // MATHEMATICAL CURSOR POSITIONING STRATEGY: Calculate exact positions without magic numbers
    if (lle_display_validate(state)) {
        LLE_TRACE_BUFFER("BEFORE_MATHEMATICAL_BACKSPACE", state->buffer);
        
        // Debug: Show state transition
        fprintf(stderr, "[MATH_DEBUG] BACKSPACE: old_length=%zu, new_length=%zu\n", 
                old_length, state->buffer->length);
        if (old_content) {
            fprintf(stderr, "[MATH_DEBUG] BEFORE: '%.*s'\n", (int)old_length, old_content);
        }
        fprintf(stderr, "[MATH_DEBUG] AFTER: '%.*s'\n", 
                (int)state->buffer->length, state->buffer->buffer);
        
        if (state->state_integration) {
            fprintf(stderr, "[MATH_DEBUG] Using mathematical cursor positioning\n");
            
            // Calculate terminal geometry parameters
            size_t prompt_width = 0;
            if (state->prompt) {
                prompt_width = lle_prompt_get_last_line_width(state->prompt);
            }
            size_t terminal_width = state->geometry.width;
            size_t available_content_width = (terminal_width > prompt_width) ? 
                                           (terminal_width - prompt_width) : 0;
            
            fprintf(stderr, "[MATH_DEBUG] Terminal geometry: prompt=%zu, terminal=%zu, available=%zu\n", 
                    prompt_width, terminal_width, available_content_width);
            
            // Use tracked cursor position directly when available
            size_t current_line, current_col;
            if (state->position_tracking_valid && state->content_end_col > 0) {
                // Use tracked cursor position directly - no conversion needed
                current_line = state->content_end_row;
                current_col = state->content_end_col;
                fprintf(stderr, "[MATH_DEBUG] Using tracked cursor position directly: row=%zu, col=%zu\n", 
                        current_line, current_col);
            } else {
                // Fallback to calculation from buffer length
                size_t current_absolute_pos = prompt_width + old_length;
                fprintf(stderr, "[MATH_DEBUG] Using calculated cursor position from buffer length: %zu\n", current_absolute_pos);
                
                // Fix boundary condition: when at exact terminal width boundary
                if (current_absolute_pos > 0 && current_absolute_pos % terminal_width == 0) {
                    // Position is at end of previous line, not start of next line
                    current_line = (current_absolute_pos / terminal_width) - 1;
                    current_col = terminal_width;
                } else {
                    current_line = current_absolute_pos / terminal_width;
                    current_col = current_absolute_pos % terminal_width;
                }
                fprintf(stderr, "[MATH_DEBUG] Corrected boundary calculation: current_line=%zu, current_col=%zu\n", 
                        current_line, current_col);
            }
            
            // Calculate target position - always from buffer length
            size_t target_absolute_pos = prompt_width + state->buffer->length;
            // Fix boundary condition for target position too
            size_t target_line, target_col;
            if (target_absolute_pos > 0 && target_absolute_pos % terminal_width == 0) {
                // Position is at end of previous line, not start of next line
                target_line = (target_absolute_pos / terminal_width) - 1;
                target_col = terminal_width;
            } else {
                target_line = target_absolute_pos / terminal_width;
                target_col = target_absolute_pos % terminal_width;
            }
            fprintf(stderr, "[MATH_DEBUG] Corrected target boundary calculation: target_line=%zu, target_col=%zu\n", 
                    target_line, target_col);
            
            fprintf(stderr, "[MATH_DEBUG] Position calculation: current=(%zu,%zu), target=(%zu,%zu)\n", 
                    current_line, current_col, target_line, target_col);
            
            fprintf(stderr, "[MATH_DEBUG] Final cursor positions: current=(line=%zu,col=%zu), target=(line=%zu,col=%zu)\n", 
                    current_line, current_col, target_line, target_col);
            
            // Execute cursor positioning and content clearing
            if (current_line == target_line) {
                // Same line: position cursor at target and clear to end of line
                fprintf(stderr, "[MATH_DEBUG] Same line operation: positioning and clearing\n");
                
                // Position cursor at target column (where new content should end)
                if (lle_termcap_cursor_to_column((int)target_col) != LLE_TERMCAP_OK) {
                    fprintf(stderr, "[MATH_DEBUG] ERROR: Failed to position cursor at target column %zu\n", target_col);
                } else {
                    fprintf(stderr, "[MATH_DEBUG] SUCCESS: Positioned cursor at target column %zu\n", target_col);
                }
                
                // Clear from cursor to end of line (removes deleted characters)
                if (lle_termcap_clear_to_eol() != LLE_TERMCAP_OK) {
                    fprintf(stderr, "[MATH_DEBUG] ERROR: Failed to clear to end of line\n");
                } else {
                    fprintf(stderr, "[MATH_DEBUG] SUCCESS: Cleared deleted characters from cursor position\n");
                }
            } else {
                // Different lines: cross-line boundary operation
                fprintf(stderr, "[MATH_DEBUG] Cross-line operation: from line %zu to line %zu\n", current_line, target_line);
                
                // For cross-line operations, clear wrapped lines first, then position cursor
                if (current_line > target_line) {
                    // Clear current line (wrapped line) completely
                    if (lle_termcap_clear_line() == LLE_TERMCAP_OK) {
                        fprintf(stderr, "[MATH_DEBUG] SUCCESS: Cleared current wrapped line %zu\n", current_line);
                        
                        // Move up to target line
                        size_t lines_to_move_up = current_line - target_line;
                        if (lle_termcap_move_cursor_up((int)lines_to_move_up) == LLE_TERMCAP_OK) {
                            fprintf(stderr, "[MATH_DEBUG] SUCCESS: Moved up %zu lines\n", lines_to_move_up);
                            
                            // Position cursor at target column
                            if (lle_termcap_cursor_to_column((int)target_col) == LLE_TERMCAP_OK) {
                                fprintf(stderr, "[MATH_DEBUG] SUCCESS: Positioned at target column %zu\n", target_col);
                                
                                // Only clear to EOL if we're not at the end of the line
                                // When target_col equals terminal width, we're at line end - no clearing needed
                                if (target_col < terminal_width) {
                                    if (lle_termcap_clear_to_eol() == LLE_TERMCAP_OK) {
                                        fprintf(stderr, "[MATH_DEBUG] SUCCESS: Cleared remaining characters from cursor position\n");
                                    } else {
                                        fprintf(stderr, "[MATH_DEBUG] WARNING: Failed to clear remaining characters\n");
                                    }
                                } else {
                                    fprintf(stderr, "[MATH_DEBUG] SKIP: No clear needed - cursor at line end (col=%zu, width=%zu)\n", 
                                            target_col, terminal_width);
                                }
                            }
                        }
                    }
                } else {
                    fprintf(stderr, "[MATH_DEBUG] ERROR: Unexpected cursor movement direction - current_line <= target_line\n");
                }
            }
            
            // Update display tracking state for all successful operations
            if (state->buffer->length < sizeof(state->last_displayed_content)) {
                memcpy(state->last_displayed_content, state->buffer->buffer, state->buffer->length);
                state->last_displayed_content[state->buffer->length] = '\0';
                state->last_displayed_length = state->buffer->length;
                state->display_state_valid = true;
                
                // Update cursor position tracking
                state->content_end_row = target_line;
                state->content_end_col = target_col;
                state->position_tracking_valid = true;
                
                // For cross-line operations, force immediate cursor synchronization
                if (current_line != target_line) {
                    // Query actual terminal cursor position to ensure perfect sync
                    size_t actual_row, actual_col;
                    if (state->terminal && lle_terminal_query_cursor_position(state->terminal, &actual_row, &actual_col)) {
                        // Update tracking with actual terminal position
                        state->content_end_row = actual_row;
                        state->content_end_col = actual_col;
                        fprintf(stderr, "[MATH_DEBUG] Cross-line operation - synced with actual cursor position: row=%zu, col=%zu\n", actual_row, actual_col);
                    } else {
                        // Fallback: invalidate tracking to force recalculation
                        state->position_tracking_valid = false;
                        fprintf(stderr, "[MATH_DEBUG] Cross-line operation - cursor query failed, invalidating position tracking\n");
                    }
                }
                
                fprintf(stderr, "[MATH_DEBUG] SUCCESS: Display state updated and synchronized\n");
                fprintf(stderr, "[MATH_DEBUG] Updated cursor tracking: row=%zu, col=%zu\n", target_line, target_col);
            }
            
            // Validate state consistency
            if (!lle_display_integration_validate_state(state->state_integration)) {
                fprintf(stderr, "[MATH_DEBUG] WARNING: State validation failed - forcing sync\n");
                lle_display_integration_force_sync(state->state_integration);
            } else {
                fprintf(stderr, "[MATH_DEBUG] SUCCESS: State validation passed\n");
            }
        } else {
            // Fallback without state integration - simple character deletion
            LLE_TRACE_FALLBACK("MATHEMATICAL_FALLBACK", state->buffer);
            fprintf(stderr, "[MATH_DEBUG] Using fallback mathematical backspace\n");
                
            if (state->terminal) {
                // Mathematical character deletion using termcap - maintain state accuracy
                if (lle_termcap_move_cursor_left(1) == LLE_TERMCAP_OK) {
                    // Write space to clear character, then move back
                    if (lle_terminal_write(state->terminal, " ", 1) == 0) {
                        if (lle_termcap_move_cursor_left(1) == LLE_TERMCAP_OK) {
                            // State remains consistent - character visually deleted
                            fprintf(stderr, "[MATH_DEBUG] Fallback: Character deletion with state consistency\n");
                        }
                    }
                } else {
                    fprintf(stderr, "[MATH_DEBUG] Fallback: Failed to position cursor for deletion\n");
                }
            }
        }
        
        LLE_TRACE_BUFFER("AFTER_MATHEMATICAL_BACKSPACE", state->buffer);
        fprintf(stderr, "[MATH_DEBUG] Mathematical backspace completed\n");
    }
    
    // Cleanup
    free(old_content);
    
    // CRITICAL TRACE: End backspace operation
    LLE_TRACE_CRITICAL("CMD_BACKSPACE_EXIT", state->buffer);
    lle_trace_backspace_end(trace_session, state->buffer, state, true);
    
    return LLE_CMD_SUCCESS;
}

// ============================================================================
// Cursor Movement Functions
// ============================================================================

lle_command_result_t lle_cmd_move_cursor(lle_display_state_t *state, 
                                          lle_cmd_cursor_movement_t direction, 
                                          size_t count) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (count == 0) count = LLE_DEFAULT_CURSOR_MOVE_COUNT;
    if (count > LLE_MAX_CURSOR_MOVE_COUNT) {
        return LLE_CMD_ERROR_INVALID_PARAM;
    }
    
    bool result = false;
    
    switch (direction) {
        case LLE_CMD_CURSOR_LEFT:
            for (size_t i = 0; i < count && state->buffer->cursor_pos > 0; i++) {
                result = lle_text_move_cursor(state->buffer, LLE_MOVE_LEFT);
                if (!result) break;
            }
            break;
            
        case LLE_CMD_CURSOR_RIGHT:
            for (size_t i = 0; i < count && state->buffer->cursor_pos < state->buffer->length; i++) {
                result = lle_text_move_cursor(state->buffer, LLE_MOVE_RIGHT);
                if (!result) break;
            }
            break;
            
        case LLE_CMD_CURSOR_HOME:
            result = lle_text_move_cursor(state->buffer, LLE_MOVE_HOME);
            break;
            
        case LLE_CMD_CURSOR_END:
            result = lle_text_move_cursor(state->buffer, LLE_MOVE_END);
            break;
            
        case LLE_CMD_CURSOR_WORD_LEFT:
            for (size_t i = 0; i < count; i++) {
                result = lle_text_move_cursor(state->buffer, LLE_MOVE_WORD_LEFT);
                if (!result) break;
            }
            break;
            
        case LLE_CMD_CURSOR_WORD_RIGHT:
            for (size_t i = 0; i < count; i++) {
                result = lle_text_move_cursor(state->buffer, LLE_MOVE_WORD_RIGHT);
                if (!result) break;
            }
            break;
            
        case LLE_CMD_CURSOR_UP:
        case LLE_CMD_CURSOR_DOWN:
            // Multiline cursor movement - not implemented in basic commands
            // Would require line-aware cursor positioning
            return LLE_CMD_ERROR_INVALID_PARAM;
            
        case LLE_CMD_CURSOR_ABSOLUTE:
            // Should use lle_cmd_set_cursor_position instead
            return LLE_CMD_ERROR_INVALID_PARAM;
            
        default:
            return LLE_CMD_ERROR_INVALID_PARAM;
    }
    
    if (!result) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // CRITICAL FIX: History navigation regression resolved
    // The buffer cursor movement succeeded, which is the core functionality
    // Display synchronization is supplementary and handled separately
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_set_cursor_position(lle_display_state_t *state, size_t position) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (position > state->buffer->length) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    if (!lle_text_set_cursor(state->buffer, position)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for cursor positioning
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_move_home(lle_display_state_t *state) {
    return lle_cmd_move_cursor(state, LLE_CMD_CURSOR_HOME, 1);
}

lle_command_result_t lle_cmd_move_end(lle_display_state_t *state) {
    return lle_cmd_move_cursor(state, LLE_CMD_CURSOR_END, 1);
}

// ============================================================================
// Word Operation Functions
// ============================================================================

lle_command_result_t lle_cmd_word_left(lle_display_state_t *state) {
    return lle_cmd_move_cursor(state, LLE_CMD_CURSOR_WORD_LEFT, 1);
}

lle_command_result_t lle_cmd_word_right(lle_display_state_t *state) {
    return lle_cmd_move_cursor(state, LLE_CMD_CURSOR_WORD_RIGHT, 1);
}

lle_command_result_t lle_cmd_delete_word(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    size_t start_pos = state->buffer->cursor_pos;
    size_t end_pos = find_word_boundary(state->buffer, start_pos, true);
    
    if (end_pos <= start_pos) {
        return LLE_CMD_SUCCESS; // Nothing to delete
    }
    
    // Delete range from cursor to end of word
    if (!lle_text_delete_range(state->buffer, start_pos, end_pos)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for word deletion
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_backspace_word(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    size_t end_pos = state->buffer->cursor_pos;
    size_t start_pos = find_word_boundary(state->buffer, end_pos, false);
    
    if (end_pos <= start_pos) {
        return LLE_CMD_SUCCESS; // Nothing to delete
    }
    
    // Delete range from start of word to cursor
    if (!lle_text_delete_range(state->buffer, start_pos, end_pos)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Set cursor to start position
    if (!lle_text_set_cursor(state->buffer, start_pos)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for word backspace
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

// ============================================================================
// Line Operation Functions
// ============================================================================

lle_command_result_t lle_cmd_accept_line(lle_display_state_t *state, 
                                          char *result_buffer, 
                                          size_t buffer_size) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (result_buffer && buffer_size > 0) {
        // Copy current buffer content to result buffer
        size_t copy_length = state->buffer->length;
        if (copy_length >= buffer_size) {
            copy_length = buffer_size - 1; // Leave space for null terminator
        }
        
        if (state->buffer->buffer && copy_length > 0) {
            memcpy(result_buffer, state->buffer->buffer, copy_length);
        }
        result_buffer[copy_length] = '\0';
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for line acceptance
            lle_display_integration_validate_state(state->state_integration);
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_cancel_line(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    // Clear the buffer
    lle_text_buffer_clear(state->buffer);
    if (!lle_text_buffer_is_valid(state->buffer)) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for line cancellation
            lle_display_integration_validate_state(state->state_integration);
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_clear_line(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    // Clear the buffer
    lle_text_buffer_clear(state->buffer);
    if (!lle_text_buffer_is_valid(state->buffer)) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for line clearing
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_replace_line(lle_display_state_t *state, const char *text, size_t length) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!text && length > 0) {
        return LLE_CMD_ERROR_INVALID_PARAM;
    }
    
    // Clear the buffer first
    lle_text_buffer_clear(state->buffer);
    if (!lle_text_buffer_is_valid(state->buffer)) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    // Insert the new text if provided
    if (text && length > 0) {
        for (size_t i = 0; i < length; i++) {
            if (!lle_text_insert_char(state->buffer, text[i])) {
                return LLE_CMD_ERROR_BUFFER_FULL;
            }
        }
        
        // Move cursor to end
        lle_text_move_cursor(state->buffer, LLE_MOVE_END);
    }
    
    // For history navigation, use proper clearing before rendering new content
    if (lle_display_validate(state)) {
        // Step 1: Move to start of text area (after prompt)
        size_t prompt_width = state->prompt ? lle_prompt_get_last_line_width(state->prompt) : 0;
        
        // Step 2: Clear all previous content using safe termcap functions
        // Move to beginning of current line
        if (!lle_terminal_write(state->terminal, "\r", 1)) {
            return LLE_CMD_ERROR_DISPLAY_UPDATE;
        }
        
        // Move to after prompt using safe cursor positioning
        if (!lle_terminal_move_cursor_to_column(state->terminal, prompt_width)) {
            return LLE_CMD_ERROR_DISPLAY_UPDATE;
        }
        
        // Clear from current position to end of screen using safe termcap
        if (!lle_terminal_clear_to_eos(state->terminal)) {
            return LLE_CMD_ERROR_DISPLAY_UPDATE;
        }
        
        // Step 3: Write new content if any
        if (text && length > 0) {
            if (!lle_terminal_write(state->terminal, text, length)) {
                return LLE_CMD_ERROR_DISPLAY_UPDATE;
            }
        }
        
        // Step 4: Update tracking for future operations
        if (text && length > 0) {
            memcpy(state->last_displayed_content, text, length);
            state->last_displayed_content[length] = '\0';
            state->last_displayed_length = length;
        } else {
            state->last_displayed_content[0] = '\0';
            state->last_displayed_length = 0;
        }
        state->display_state_valid = true;
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_kill_line(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    size_t cursor_pos = state->buffer->cursor_pos;
    size_t length_to_delete = state->buffer->length - cursor_pos;
    
    if (length_to_delete == 0) {
        return LLE_CMD_SUCCESS; // Nothing to delete
    }
    
    // Delete from cursor to end of line
    if (!lle_text_delete_range(state->buffer, cursor_pos, cursor_pos + length_to_delete)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for kill line
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

lle_command_result_t lle_cmd_kill_beginning(lle_display_state_t *state) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    size_t cursor_pos = state->buffer->cursor_pos;
    
    if (cursor_pos == 0) {
        return LLE_CMD_SUCCESS; // Nothing to delete
    }
    
    // Delete from beginning to cursor
    if (!lle_text_delete_range(state->buffer, 0, cursor_pos)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Set cursor to beginning
    if (!lle_text_set_cursor(state->buffer, 0)) {
        return LLE_CMD_ERROR_INVALID_POSITION;
    }
    
    // Phase 2B.5: Integrate with Phase 2A absolute positioning system
    // Only update display if state is fully initialized to prevent segfaults
    if (lle_display_validate(state)) {
        if (state->state_integration) {
            // Use state synchronization for kill beginning
            if (!lle_display_integration_validate_state(state->state_integration)) {
                lle_display_integration_force_sync(state->state_integration);
            }
        }
    }
    
    return LLE_CMD_SUCCESS;
}

// ============================================================================
// Key-to-Command Mapping Functions
// ============================================================================

bool lle_map_key_to_command(const lle_key_event_t *event, lle_command_type_t *cmd) {
    if (!event || !cmd) return false;
    
    switch (event->type) {
        case LLE_KEY_BACKSPACE:
        case LLE_KEY_CTRL_H:
            *cmd = LLE_CMD_BACKSPACE;
            return true;
            
        case LLE_KEY_DELETE:
        case LLE_KEY_CTRL_D:
            *cmd = LLE_CMD_DELETE_CHAR;
            return true;
            
        case LLE_KEY_ENTER:
        case LLE_KEY_CTRL_M:
        case LLE_KEY_CTRL_J:
            *cmd = LLE_CMD_ACCEPT_LINE;
            return true;
            
        case LLE_KEY_CTRL_C:
            *cmd = LLE_CMD_CANCEL_LINE;
            return true;
            
        case LLE_KEY_CTRL_A:
        case LLE_KEY_HOME:
            *cmd = LLE_CMD_MOVE_HOME;
            return true;
            
        case LLE_KEY_CTRL_E:
        case LLE_KEY_END:
            *cmd = LLE_CMD_MOVE_END;
            return true;
            
        case LLE_KEY_ARROW_LEFT:
        case LLE_KEY_CTRL_B:
            *cmd = LLE_CMD_MOVE_CURSOR;
            return true;
            
        case LLE_KEY_ARROW_RIGHT:
        case LLE_KEY_CTRL_F:
            *cmd = LLE_CMD_MOVE_CURSOR;
            return true;
            
        case LLE_KEY_ALT_B:
        case LLE_KEY_CTRL_ARROW_LEFT:
            *cmd = LLE_CMD_WORD_LEFT;
            return true;
            
        case LLE_KEY_ALT_F:
        case LLE_KEY_CTRL_ARROW_RIGHT:
            *cmd = LLE_CMD_WORD_RIGHT;
            return true;
            
        case LLE_KEY_CTRL_K:
            *cmd = LLE_CMD_KILL_LINE;
            return true;
            
        case LLE_KEY_CTRL_U:
            *cmd = LLE_CMD_KILL_BEGINNING;
            return true;
            
        case LLE_KEY_ALT_D:
            *cmd = LLE_CMD_DELETE_WORD;
            return true;
            
        case LLE_KEY_ALT_BACKSPACE:
        case LLE_KEY_CTRL_W:
            *cmd = LLE_CMD_BACKSPACE_WORD;
            return true;
            
        case LLE_KEY_CTRL_L:
            *cmd = LLE_CMD_CLEAR_LINE;
            return true;
            
        default:
            return false;
    }
}

bool lle_key_is_printable_character(const lle_key_event_t *event) {
    if (!event) return false;
    return lle_key_is_printable(event);
}

bool lle_extract_character_from_key(const lle_key_event_t *event, char *character) {
    if (!event || !character) return false;
    
    if (!lle_key_is_printable(event)) return false;
    
    *character = event->character;
    return true;
}

// ============================================================================
// History Navigation Commands
// ============================================================================

/**
 * @brief Navigate to previous history entry
 * 
 * Navigates to the previous (older) entry in command history and replaces
 * the current line content with that entry. Uses the proven exact backspace
 * replication pattern established in the main input loop.
 * 
 * @param state Display state containing text buffer
 * @param history History structure for navigation
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_history_up(lle_display_state_t *state, lle_history_t *history) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!history) {
        return LLE_CMD_ERROR_INVALID_PARAM;
    }
    
    // Navigate to previous entry in history
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_PREV);
    if (!entry) {
        // No previous entry available
        return LLE_CMD_SUCCESS;
    }
    
    // Store old content for replacement
    char *old_content = NULL;
    size_t old_length = state->buffer->length;
    if (old_length > 0) {
        old_content = malloc(old_length + 1);
        if (old_content) {
            memcpy(old_content, state->buffer->buffer, old_length);
            old_content[old_length] = '\0';
        }
    }
    
    // Use backspace-based content replacement with proven boundary logic
    if (state->state_integration) {
        if (!lle_display_integration_replace_content_backspace(state->state_integration, 
                                                              old_content, old_length, 
                                                              entry->command, entry->length)) {
            free(old_content);
            return LLE_CMD_ERROR_DISPLAY_UPDATE;
        }
    } else {
        // Fallback if state integration not available
        if (state->terminal) {
            size_t prompt_width = 0;
            if (state->prompt) {
                prompt_width = lle_prompt_get_last_line_width(state->prompt);
            }
            
            // Simple fallback - clear current line and write new content
            lle_terminal_write(state->terminal, "\r", 1);
            if (prompt_width > 0) {
                char move_right[32];
                snprintf(move_right, sizeof(move_right), "\x1b[%zuC", prompt_width);
                lle_terminal_write(state->terminal, move_right, strlen(move_right));
            }
            lle_terminal_write(state->terminal, "\x1b[K", 3); // Clear to end of line
            lle_terminal_write(state->terminal, entry->command, entry->length);
        }
    }
    
    // Update buffer state
    lle_text_buffer_clear(state->buffer);
    for (size_t i = 0; i < entry->length; i++) {
        if (!lle_text_insert_char(state->buffer, entry->command[i])) {
            free(old_content);
            return LLE_CMD_ERROR_BUFFER_FULL;
        }
    }
    lle_text_move_cursor(state->buffer, LLE_MOVE_END);
    
    // Validate state consistency after complex operations
    if (state->state_integration) {
        if (!lle_display_integration_validate_state(state->state_integration)) {
            lle_display_integration_force_sync(state->state_integration);
        }
    }
    
    free(old_content);
    return LLE_CMD_SUCCESS;
}

/**
 * @brief Navigate to next history entry
 * 
 * Navigates to the next (newer) entry in command history and replaces
 * the current line content with that entry. If at the end of history,
 * may restore the original line being edited.
 * 
 * @param state Display state containing text buffer
 * @param history History structure for navigation
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_history_down(lle_display_state_t *state, lle_history_t *history) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!state->buffer) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (!history) {
        return LLE_CMD_ERROR_INVALID_PARAM;
    }
    
    // Navigate to next entry in history
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_NEXT);
    

    
    // Store old content for replacement
    char *old_content = NULL;
    size_t old_length = state->buffer->length;
    if (old_length > 0) {
        old_content = malloc(old_length + 1);
        if (old_content) {
            memcpy(old_content, state->buffer->buffer, old_length);
            old_content[old_length] = '\0';
        }
    }
    
    if (entry) {
        // Found next entry - replace content with backspace-based clearing
        if (state->state_integration) {
            if (!lle_display_integration_replace_content_backspace(state->state_integration, 
                                                                  old_content, old_length, 
                                                                  entry->command, entry->length)) {
                free(old_content);
                return LLE_CMD_ERROR_DISPLAY_UPDATE;
            }
        } else {
            // Fallback if state integration not available
            if (state->terminal) {
                size_t prompt_width = 0;
                if (state->prompt) {
                    prompt_width = lle_prompt_get_last_line_width(state->prompt);
                }
                
                lle_terminal_write(state->terminal, "\r", 1);
                if (prompt_width > 0) {
                    char move_right[32];
                    snprintf(move_right, sizeof(move_right), "\x1b[%zuC", prompt_width);
                    lle_terminal_write(state->terminal, move_right, strlen(move_right));
                }
                lle_terminal_write(state->terminal, "\x1b[K", 3); // Clear to end of line
                lle_terminal_write(state->terminal, entry->command, entry->length);
            }
        }
        
        // Update buffer state
        lle_text_buffer_clear(state->buffer);
        for (size_t i = 0; i < entry->length; i++) {
            if (!lle_text_insert_char(state->buffer, entry->command[i])) {
                free(old_content);
                return LLE_CMD_ERROR_BUFFER_FULL;
            }
        }
        lle_text_move_cursor(state->buffer, LLE_MOVE_END);
    } else {
        // No next entry - at end of history, clear with backspace logic
        if (state->state_integration) {
            if (!lle_display_integration_replace_content_backspace(state->state_integration, 
                                                                  old_content, old_length, 
                                                                  NULL, 0)) {
                free(old_content);
                return LLE_CMD_ERROR_DISPLAY_UPDATE;
            }
        } else {
            // Fallback clearing
            if (state->terminal) {
                size_t prompt_width = 0;
                if (state->prompt) {
                    prompt_width = lle_prompt_get_last_line_width(state->prompt);
                }
                
                lle_terminal_write(state->terminal, "\r", 1);
                if (prompt_width > 0) {
                    char move_right[32];
                    snprintf(move_right, sizeof(move_right), "\x1b[%zuC", prompt_width);
                    lle_terminal_write(state->terminal, move_right, strlen(move_right));
                }
                lle_terminal_write(state->terminal, "\x1b[K", 3); // Clear to end of line
            }
        }
        
        // Clear buffer
        lle_text_buffer_clear(state->buffer);
    }
    
    // Validate state consistency after complex operations
    if (state->state_integration) {
        if (!lle_display_integration_validate_state(state->state_integration)) {
            lle_display_integration_force_sync(state->state_integration);
        }
    }
    
    free(old_content);
    return LLE_CMD_SUCCESS;
}

// ============================================================================
// Core Command Execution Functions
// ============================================================================

lle_command_result_t lle_execute_command(lle_display_state_t *state, 
                                          lle_command_type_t cmd, 
                                          const void *data) {
    if (!state) {
        return LLE_CMD_ERROR_INVALID_STATE;
    }
    
    if (cmd < 0 || cmd >= LLE_CMD_TYPE_COUNT) {
        return LLE_CMD_ERROR_UNKNOWN_COMMAND;
    }
    
    switch (cmd) {
        case LLE_CMD_INSERT_CHAR: {
            if (!data) return LLE_CMD_ERROR_INVALID_PARAM;
            const lle_insert_char_data_t *char_data = (const lle_insert_char_data_t *)data;
            return lle_cmd_insert_char(state, char_data->character);
        }
        
        case LLE_CMD_DELETE_CHAR:
            return lle_cmd_delete_char(state);
            
        case LLE_CMD_BACKSPACE:
            return lle_cmd_backspace(state);
            
        case LLE_CMD_MOVE_CURSOR: {
            if (!data) return LLE_CMD_ERROR_INVALID_PARAM;
            const lle_cursor_move_data_t *move_data = (const lle_cursor_move_data_t *)data;
            if (move_data->direction == LLE_CMD_CURSOR_ABSOLUTE) {
                return lle_cmd_set_cursor_position(state, move_data->absolute_position);
            } else {
                return lle_cmd_move_cursor(state, move_data->direction, move_data->count);
            }
        }
        
        case LLE_CMD_ACCEPT_LINE: {
            const lle_line_operation_data_t *line_data = (const lle_line_operation_data_t *)data;
            if (line_data) {
                return lle_cmd_accept_line(state, line_data->result_buffer, line_data->buffer_size);
            } else {
                return lle_cmd_accept_line(state, NULL, 0);
            }
        }
        
        case LLE_CMD_CANCEL_LINE:
            return lle_cmd_cancel_line(state);
            
        case LLE_CMD_CLEAR_LINE:
            return lle_cmd_clear_line(state);
            
        case LLE_CMD_MOVE_HOME:
            return lle_cmd_move_home(state);
            
        case LLE_CMD_MOVE_END:
            return lle_cmd_move_end(state);
            
        case LLE_CMD_WORD_LEFT:
            return lle_cmd_word_left(state);
            
        case LLE_CMD_WORD_RIGHT:
            return lle_cmd_word_right(state);
            
        case LLE_CMD_DELETE_WORD:
            return lle_cmd_delete_word(state);
            
        case LLE_CMD_BACKSPACE_WORD:
            return lle_cmd_backspace_word(state);
            
        case LLE_CMD_KILL_LINE:
            return lle_cmd_kill_line(state);
            
        case LLE_CMD_KILL_BEGINNING:
            return lle_cmd_kill_beginning(state);
            
        default:
            return LLE_CMD_ERROR_UNKNOWN_COMMAND;
    }
}

lle_command_result_t lle_execute_command_with_context(lle_command_context_t *context) {
    if (!context) {
        return LLE_CMD_ERROR_INVALID_PARAM;
    }
    
    context->result = lle_execute_command(context->state, context->command, context->data);
    context->display_updated = (context->result == LLE_CMD_SUCCESS);
    
    return context->result;
}

bool lle_can_execute_command(const lle_display_state_t *state, lle_command_type_t cmd) {
    if (!state) {
        return false;
    }
    
    if (cmd < 0 || cmd >= LLE_CMD_TYPE_COUNT) {
        return false;
    }
    
    if (!state->buffer) {
        return false;
    }
    
    // All basic commands can be executed if state and buffer are valid
    // Display validation is optional for non-terminal environments
    return true;
}

// ============================================================================
// Utility Functions
// ============================================================================

const char *lle_command_type_to_string(lle_command_type_t cmd) {
    if (cmd < 0 || cmd >= LLE_CMD_TYPE_COUNT) {
        return "UNKNOWN";
    }
    
    if (cmd < sizeof(command_type_names) / sizeof(command_type_names[0])) {
        return command_type_names[cmd];
    }
    
    return "UNKNOWN";
}

const char *lle_command_result_to_string(lle_command_result_t result) {
    int index = -result; // Convert negative error codes to positive indices
    
    if (result == LLE_CMD_SUCCESS) {
        return command_result_names[0];
    }
    
    if (index > 0 && (size_t)index < sizeof(command_result_names) / sizeof(command_result_names[0])) {
        return command_result_names[index];
    }
    
    return "UNKNOWN_ERROR";
}

bool lle_command_context_init(lle_command_context_t *context, 
                              lle_display_state_t *state, 
                              lle_command_type_t cmd, 
                              const void *data) {
    if (!context) return false;
    
    memset(context, 0, sizeof(lle_command_context_t));
    context->command = cmd;
    context->data = data;
    context->state = state;
    context->result = LLE_CMD_SUCCESS;
    context->display_updated = false;
    context->affected_offset = 0;
    context->affected_length = 0;
    
    return true;
}

bool lle_validate_command_parameters(const lle_display_state_t *state, 
                                      lle_command_type_t cmd, 
                                      const void *data) {
    if (!state) {
        return false;
    }
    
    if (!state->buffer) {
        return false;
    }
    
    if (cmd < 0 || cmd >= LLE_CMD_TYPE_COUNT) {
        return false;
    }
    
    // Check command-specific data requirements
    switch (cmd) {
        case LLE_CMD_INSERT_CHAR:
            return (data != NULL);
            
        case LLE_CMD_MOVE_CURSOR:
            return (data != NULL);
            
        case LLE_CMD_DELETE_CHAR:
        case LLE_CMD_BACKSPACE:
        case LLE_CMD_ACCEPT_LINE:
        case LLE_CMD_CANCEL_LINE:
        case LLE_CMD_CLEAR_LINE:
        case LLE_CMD_MOVE_HOME:
        case LLE_CMD_MOVE_END:
        case LLE_CMD_WORD_LEFT:
        case LLE_CMD_WORD_RIGHT:
        case LLE_CMD_DELETE_WORD:
        case LLE_CMD_BACKSPACE_WORD:
        case LLE_CMD_KILL_LINE:
        case LLE_CMD_KILL_BEGINNING:
            return true; // These commands don't require data
            
        default:
            return false;
    }
}