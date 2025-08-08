/**
 * @file display_state_integration.h
 * @brief Lusush Line Editor - Display State Synchronization Integration
 *
 * This module provides integration wrappers for the existing display system
 * to use the new unified bidirectional state synchronization system. It
 * provides drop-in replacements for existing terminal operations that
 * automatically maintain state consistency.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#ifndef LLE_DISPLAY_STATE_INTEGRATION_H
#define LLE_DISPLAY_STATE_INTEGRATION_H

#include "display_state_sync.h"
#include "display.h"
#include "terminal_manager.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// ============================================================================
// Integration Context
// ============================================================================

/**
 * @brief Integration context for display state synchronization
 */
typedef struct lle_display_integration {
    lle_state_sync_context_t *sync_ctx;    /**< State sync context */
    lle_display_state_t *display;          /**< Display state reference */
    lle_terminal_manager_t *terminal;      /**< Terminal manager reference */
    
    // Integration configuration
    bool sync_enabled;                      /**< Enable state synchronization */
    bool validation_enabled;                /**< Enable state validation */
    bool debug_mode;                        /**< Enable debug logging */
    size_t sync_frequency;                  /**< Operations between syncs */
    
    // Operation tracking
    size_t operations_since_sync;           /**< Operations since last sync */
    bool pending_validation;                /**< Validation needed flag */
    
    // Performance tracking
    uint64_t total_operations;              /**< Total operations performed */
    uint64_t total_sync_time;               /**< Total time in sync operations */
    size_t sync_skip_count;                 /**< Number of skipped syncs */
} lle_display_integration_t;

// ============================================================================
// Integration Initialization and Cleanup
// ============================================================================

/**
 * @brief Initialize display state synchronization integration
 * 
 * @param display Display state to integrate with
 * @param terminal Terminal manager to integrate with
 * @return Integration context, NULL on error
 */
lle_display_integration_t *lle_display_integration_init(lle_display_state_t *display,
                                                        lle_terminal_manager_t *terminal);

/**
 * @brief Cleanup display state synchronization integration
 * 
 * @param integration Integration context to cleanup
 */
void lle_display_integration_cleanup(lle_display_integration_t *integration);

/**
 * @brief Enable or disable state synchronization
 * 
 * @param integration Integration context
 * @param enabled Whether to enable synchronization
 */
void lle_display_integration_set_sync_enabled(lle_display_integration_t *integration,
                                              bool enabled);

/**
 * @brief Set synchronization frequency
 * 
 * @param integration Integration context
 * @param frequency Number of operations between automatic syncs (0 = every operation)
 */
void lle_display_integration_set_sync_frequency(lle_display_integration_t *integration,
                                                size_t frequency);

/**
 * @brief Enable or disable debug mode
 * 
 * @param integration Integration context
 * @param enabled Whether to enable debug logging
 */
void lle_display_integration_set_debug_mode(lle_display_integration_t *integration,
                                            bool enabled);

// ============================================================================
// Drop-in Replacements for Terminal Operations
// ============================================================================

/**
 * @brief Write data to terminal with state synchronization
 * 
 * This is a drop-in replacement for lle_terminal_write() that automatically
 * maintains display state synchronization.
 * 
 * @param integration Integration context
 * @param data Data to write
 * @param length Length of data
 * @return true on success, false on error
 */
bool lle_display_integration_terminal_write(lle_display_integration_t *integration,
                                            const char *data,
                                            size_t length);

/**
 * @brief Clear terminal to end of line with state synchronization
 * 
 * This is a drop-in replacement for lle_terminal_clear_to_eol() that automatically
 * maintains display state synchronization.
 * 
 * @param integration Integration context
 * @return true on success, false on error
 */
bool lle_display_integration_clear_to_eol(lle_display_integration_t *integration);

/**
 * @brief Clear entire terminal line with state synchronization
 * 
 * @param integration Integration context
 * @param line_number Line number to clear (0-based)
 * @return true on success, false on error
 */
bool lle_display_integration_clear_line(lle_display_integration_t *integration,
                                        size_t line_number);

/**
 * @brief Move cursor to specific position with state synchronization
 * 
 * This is a drop-in replacement for lle_terminal_move_cursor() that automatically
 * maintains display state synchronization.
 * 
 * @param integration Integration context
 * @param row Target row (0-based)
 * @param col Target column (0-based)
 * @return true on success, false on error
 */
bool lle_display_integration_move_cursor(lle_display_integration_t *integration,
                                         size_t row,
                                         size_t col);

/**
 * @brief Move cursor up by specified number of rows
 * 
 * @param integration Integration context
 * @param rows Number of rows to move up
 * @return true on success, false on error
 */
bool lle_display_integration_move_cursor_up(lle_display_integration_t *integration,
                                            size_t rows);

/**
 * @brief Move cursor down by specified number of rows
 * 
 * @param integration Integration context
 * @param rows Number of rows to move down
 * @return true on success, false on error
 */
bool lle_display_integration_move_cursor_down(lle_display_integration_t *integration,
                                              size_t rows);

/**
 * @brief Move cursor to beginning of line
 * 
 * @param integration Integration context
 * @return true on success, false on error
 */
bool lle_display_integration_move_cursor_home(lle_display_integration_t *integration);

/**
 * @brief Move cursor to end of current content
 * 
 * @param integration Integration context
 * @return true on success, false on error
 */
bool lle_display_integration_move_cursor_end(lle_display_integration_t *integration);

// ============================================================================
// Enhanced Operations with State Awareness
// ============================================================================

/**
 * @brief Replace content with automatic clearing and state sync
 * 
 * This function safely replaces existing content with new content, handling
 * multiline clearing and maintaining perfect state synchronization.
 * 
 * @param integration Integration context
 * @param old_content Previous content (for clearing calculation)
 * @param old_length Length of previous content
 * @param new_content New content to display
 * @param new_length Length of new content
 * @return true on success, false on error
 */
bool lle_display_integration_replace_content(lle_display_integration_t *integration,
                                             const char *old_content,
                                             size_t old_length,
                                             const char *new_content,
                                             size_t new_length);

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
                                                       size_t content_length);

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
                                                      size_t new_length);

/**
 * @brief Clear specific region with state synchronization
 * 
 * @param integration Integration context
 * @param start_row Start row of region to clear
 * @param start_col Start column of region to clear
 * @param end_row End row of region to clear
 * @param end_col End column of region to clear
 * @return true on success, false on error
 */
bool lle_display_integration_clear_region(lle_display_integration_t *integration,
                                          size_t start_row,
                                          size_t start_col,
                                          size_t end_row,
                                          size_t end_col);

/**
 * @brief Insert character at current cursor position
 * 
 * @param integration Integration context
 * @param character Character to insert
 * @return true on success, false on error
 */
bool lle_display_integration_insert_char(lle_display_integration_t *integration,
                                         char character);

/**
 * @brief Delete character at current cursor position (backspace)
 * 
 * @param integration Integration context
 * @return true on success, false on error
 */
bool lle_display_integration_delete_char(lle_display_integration_t *integration);

/**
 * @brief Perform exact backspace sequence replication
 * 
 * This function implements the proven exact backspace replication approach
 * with integrated state synchronization.
 * 
 * @param integration Integration context
 * @param char_count Number of characters to backspace
 * @return true on success, false on error
 */
bool lle_display_integration_exact_backspace(lle_display_integration_t *integration,
                                             size_t char_count);

// ============================================================================
// State Validation and Synchronization Control
// ============================================================================

/**
 * @brief Force immediate state synchronization
 * 
 * @param integration Integration context
 * @return true on success, false on error
 */
bool lle_display_integration_force_sync(lle_display_integration_t *integration);

/**
 * @brief Validate current display state consistency
 * 
 * @param integration Integration context
 * @return true if states are consistent, false if diverged
 */
bool lle_display_integration_validate_state(lle_display_integration_t *integration);

/**
 * @brief Mark display state as dirty (requiring sync)
 * 
 * @param integration Integration context
 * @param reason Reason for marking dirty (for debugging)
 * @return true on success, false on error
 */
bool lle_display_integration_mark_dirty(lle_display_integration_t *integration,
                                        const char *reason);

/**
 * @brief Check if synchronization is currently required
 * 
 * @param integration Integration context
 * @return true if sync is required, false if states are consistent
 */
bool lle_display_integration_needs_sync(lle_display_integration_t *integration);

/**
 * @brief Reset state tracking after major operations
 * 
 * This function should be called after operations that completely change
 * the display content (like history navigation or command execution).
 * 
 * @param integration Integration context
 * @return true on success, false on error
 */
bool lle_display_integration_reset_tracking(lle_display_integration_t *integration);

// ============================================================================
// Macro Wrappers for Easy Migration
// ============================================================================

/**
 * @brief Macro to replace lle_terminal_write calls
 * 
 * Usage: LLE_INTEGRATED_WRITE(integration, data, length)
 */
#define LLE_INTEGRATED_WRITE(integration, data, length) \
    lle_display_integration_terminal_write((integration), (data), (length))

/**
 * @brief Macro to replace lle_terminal_clear_to_eol calls
 * 
 * Usage: LLE_INTEGRATED_CLEAR_EOL(integration)
 */
#define LLE_INTEGRATED_CLEAR_EOL(integration) \
    lle_display_integration_clear_to_eol(integration)

/**
 * @brief Macro to replace lle_terminal_move_cursor calls
 * 
 * Usage: LLE_INTEGRATED_MOVE_CURSOR(integration, row, col)
 */
#define LLE_INTEGRATED_MOVE_CURSOR(integration, row, col) \
    lle_display_integration_move_cursor((integration), (row), (col))

/**
 * @brief Macro for safe content replacement
 * 
 * Usage: LLE_INTEGRATED_REPLACE(integration, old_content, old_len, new_content, new_len)
 */
#define LLE_INTEGRATED_REPLACE(integration, old_content, old_len, new_content, new_len) \
    lle_display_integration_replace_content((integration), (old_content), (old_len), (new_content), (new_len))

// ============================================================================
// Migration Helpers
// ============================================================================

/**
 * @brief Convert existing display state to use integration
 * 
 * This helper function modifies an existing display state to use the
 * integrated state synchronization system.
 * 
 * @param display Existing display state
 * @param terminal Terminal manager
 * @return Integration context, NULL on error
 */
lle_display_integration_t *lle_display_integration_migrate(lle_display_state_t *display,
                                                           lle_terminal_manager_t *terminal);

/**
 * @brief Check if display state is compatible with integration
 * 
 * @param display Display state to check
 * @param terminal Terminal manager to check
 * @return true if compatible, false if migration needed
 */
bool lle_display_integration_is_compatible(lle_display_state_t *display,
                                           lle_terminal_manager_t *terminal);

/**
 * @brief Get integration performance statistics
 * 
 * @param integration Integration context
 * @param total_ops Output: total operations performed
 * @param total_syncs Output: total sync operations
 * @param avg_sync_time Output: average sync time in microseconds
 * @param sync_skip_rate Output: rate of skipped syncs
 * @return true on success, false on error
 */
bool lle_display_integration_get_stats(lle_display_integration_t *integration,
                                       uint64_t *total_ops,
                                       size_t *total_syncs,
                                       double *avg_sync_time,
                                       double *sync_skip_rate);

// ============================================================================
// Debug and Diagnostic Functions
// ============================================================================

/**
 * @brief Dump integration state for debugging
 * 
 * @param integration Integration context
 * @param output_file File to write debug output (NULL for stderr)
 */
void lle_display_integration_debug_dump(lle_display_integration_t *integration,
                                        FILE *output_file);

/**
 * @brief Enable detailed operation logging
 * 
 * @param integration Integration context
 * @param enabled Whether to enable detailed logging
 */
void lle_display_integration_set_verbose_logging(lle_display_integration_t *integration,
                                                 bool enabled);

/**
 * @brief Validate integration state and report issues
 * 
 * @param integration Integration context
 * @param output_file File to write validation report (NULL for stderr)
 * @return Number of issues found
 */
size_t lle_display_integration_validate_and_report(lle_display_integration_t *integration,
                                                   FILE *output_file);

#endif // LLE_DISPLAY_STATE_INTEGRATION_H