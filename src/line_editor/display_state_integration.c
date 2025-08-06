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
    
    // Get current cursor position
    size_t cursor_row = integration->display ? integration->display->cursor_pos.absolute_row : 0;
    size_t cursor_col = integration->display ? integration->display->cursor_pos.absolute_col : 0;
    size_t terminal_width = integration->display ? integration->display->geometry.width : 80;
    
    // Perform clear operation using state tracking
    bool success = lle_terminal_state_update_clear(integration->sync_ctx, "eol",
                                                   cursor_row, cursor_col,
                                                   cursor_row, terminal_width - 1);
    
    if (success) {
        // Send actual ANSI clear sequence
        const char *clear_sequence = "\x1b[K";
        success = lle_state_sync_terminal_write(integration->sync_ctx, clear_sequence, 3);
        
        if (success) {
            lle_integration_update_display_state(integration, "clear_to_eol");
            lle_integration_conditional_sync(integration);
            
            if (integration->debug_mode) {
                LLE_INTEGRATION_DEBUG("Clear to EOL at (%zu,%zu)", cursor_row, cursor_col);
            }
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
        // Send ANSI clear line sequence
        const char *clear_sequence = "\x1b[2K";
        success = lle_state_sync_terminal_write(integration->sync_ctx, clear_sequence, 4);
        
        if (success) {
            lle_integration_update_display_state(integration, "clear_line");
            lle_integration_conditional_sync(integration);
            
            if (integration->debug_mode) {
                LLE_INTEGRATION_DEBUG("Clear line %zu", line_number);
            }
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
        // Send ANSI cursor position sequence
        char cursor_sequence[32];
        int len = snprintf(cursor_sequence, sizeof(cursor_sequence), "\x1b[%zu;%zuH", row + 1, col + 1);
        
        if (len > 0 && len < (int)sizeof(cursor_sequence)) {
            success = lle_state_sync_terminal_write(integration->sync_ctx, cursor_sequence, len);
            
            if (success) {
                // Update display state cursor position
                if (integration->display) {
                    integration->display->cursor_pos.absolute_row = row;
                    integration->display->cursor_pos.absolute_col = col;
                }
                
                lle_integration_update_display_state(integration, "move_cursor");
                lle_integration_conditional_sync(integration);
                
                if (integration->debug_mode) {
                    LLE_INTEGRATION_DEBUG("Move cursor to (%zu,%zu)", row, col);
                }
            }
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
    
    size_t current_row = integration->display ? integration->display->cursor_pos.absolute_row : 0;
    size_t current_col = integration->display ? integration->display->cursor_pos.absolute_col : 0;
    
    if (current_row >= rows) {
        return lle_display_integration_move_cursor(integration, current_row - rows, current_col);
    }
    
    return false;
}

/**
 * @brief Move cursor down by specified number of rows
 */
bool lle_display_integration_move_cursor_down(lle_display_integration_t *integration,
                                              size_t rows) {
    if (!integration || rows == 0) {
        return false;
    }
    
    size_t current_row = integration->display ? integration->display->cursor_pos.absolute_row : 0;
    size_t current_col = integration->display ? integration->display->cursor_pos.absolute_col : 0;
    size_t terminal_height = integration->display ? integration->display->geometry.height : 24;
    
    if (current_row + rows < terminal_height) {
        return lle_display_integration_move_cursor(integration, current_row + rows, current_col);
    }
    
    return false;
}

/**
 * @brief Move cursor to beginning of line
 */
bool lle_display_integration_move_cursor_home(lle_display_integration_t *integration) {
    if (!integration) {
        return false;
    }
    
    size_t current_row = integration->display ? integration->display->cursor_pos.absolute_row : 0;
    return lle_display_integration_move_cursor(integration, current_row, 0);
}

/**
 * @brief Move cursor to end of current content
 */
bool lle_display_integration_move_cursor_end(lle_display_integration_t *integration) {
    if (!integration || !integration->display || !integration->display->buffer) {
        return false;
    }
    
    size_t current_row = integration->display->cursor_pos.absolute_row;
    size_t content_length = integration->display->buffer->length;
    size_t terminal_width = integration->display->geometry.width;
    
    // Calculate end position based on content length and terminal width
    size_t end_col = content_length % terminal_width;
    size_t end_row = current_row + (content_length / terminal_width);
    
    return lle_display_integration_move_cursor(integration, end_row, end_col);
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
    if (!integration || !new_content) {
        return false;
    }
    
    bool success = true;
    
    // Step 1: Clear old content using exact backspace replication
    if (old_content && old_length > 0) {
        success = lle_display_integration_exact_backspace(integration, old_length);
        if (!success) {
            LLE_INTEGRATION_DEBUG("Failed to clear old content");
            return false;
        }
    }
    
    // Step 2: Write new content
    if (new_length > 0) {
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
    if (!integration || !integration->validation_enabled) {
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