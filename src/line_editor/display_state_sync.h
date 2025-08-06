/**
 * @file display_state_sync.h
 * @brief Lusush Line Editor - Unified Bidirectional Terminal-Display State Synchronization
 *
 * This module provides a unified state synchronization system between the terminal
 * and LLE display system. It tracks terminal state changes, validates display state
 * consistency, and provides bidirectional synchronization mechanisms to prevent
 * state drift and display corruption, especially with ANSI clear sequences.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#ifndef LLE_DISPLAY_STATE_SYNC_H
#define LLE_DISPLAY_STATE_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "terminal_manager.h"
#include "display.h"

// Forward declarations - avoid conflicts with existing types

// ============================================================================
// Terminal State Tracking
// ============================================================================

/**
 * @brief Terminal cell state representation
 */
typedef struct {
    char character;                 /**< Character at this position */
    bool has_content;              /**< True if cell contains visible content */
    bool is_cursor_position;       /**< True if cursor is at this position */
    uint32_t color_code;           /**< ANSI color code if any */
    bool modified_since_sync;      /**< True if modified since last sync */
} lle_terminal_cell_t;

/**
 * @brief Terminal line state representation
 */
typedef struct {
    lle_terminal_cell_t *cells;    /**< Array of cells in this line */
    size_t width;                  /**< Number of cells in line */
    size_t content_length;         /**< Number of cells with content */
    bool line_modified;            /**< True if line modified since last sync */
    bool line_cleared;             /**< True if line was cleared */
} lle_terminal_line_t;

/**
 * @brief Complete terminal state representation
 */
typedef struct {
    lle_terminal_line_t *lines;    /**< Array of terminal lines */
    size_t height;                 /**< Number of lines */
    size_t width;                  /**< Terminal width */
    size_t cursor_row;             /**< Current cursor row (0-based) */
    size_t cursor_col;             /**< Current cursor column (0-based) */
    bool cursor_visible;           /**< True if cursor is visible */
    
    // State tracking
    uint64_t last_sync_timestamp;  /**< Last synchronization timestamp */
    uint32_t state_checksum;       /**< Checksum of terminal state */
    size_t modification_count;     /**< Number of modifications since sync */
    bool state_valid;              /**< True if state is considered valid */
    
    // ANSI sequence tracking
    bool in_ansi_sequence;         /**< True if currently processing ANSI */
    char ansi_buffer[64];          /**< Buffer for incomplete ANSI sequences */
    size_t ansi_buffer_length;     /**< Length of buffered ANSI data */
    
    // Performance optimization
    bool dirty_tracking_enabled;   /**< Enable dirty region tracking */
    size_t dirty_start_row;        /**< Start of dirty region */
    size_t dirty_end_row;          /**< End of dirty region */
    size_t dirty_start_col;        /**< Start column of dirty region */
    size_t dirty_end_col;          /**< End column of dirty region */
} lle_sync_terminal_state_t;

// ============================================================================
// Display State Synchronization
// ============================================================================

/**
 * @brief Display state tracking information
 */
typedef struct {
    char *expected_content;        /**< What LLE thinks is displayed */
    size_t expected_length;        /**< Length of expected content */
    size_t expected_cursor_pos;    /**< Where LLE thinks cursor is */
    size_t expected_rows_used;     /**< Number of rows LLE thinks are used */
    
    // Synchronization state
    bool sync_required;            /**< True if sync is needed */
    bool sync_in_progress;         /**< True if sync is currently running */
    uint64_t last_sync_timestamp;  /**< Last successful sync timestamp */
    size_t sync_failure_count;     /**< Number of consecutive sync failures */
    
    // Validation state
    bool content_validated;        /**< True if content matches terminal */
    bool cursor_validated;         /**< True if cursor position matches */
    bool geometry_validated;       /**< True if geometry matches */
    
    // Performance tracking
    uint64_t total_sync_time;      /**< Total time spent in synchronization */
    size_t sync_operations;        /**< Number of sync operations performed */
    double average_sync_time;      /**< Average synchronization time */
} lle_display_sync_state_t;

/**
 * @brief Unified state synchronization context
 */
typedef struct {
    lle_sync_terminal_state_t *terminal_state;    /**< Terminal state tracking */
    lle_display_sync_state_t *display_sync;  /**< Display sync state */
    lle_terminal_manager_t *terminal;        /**< Terminal manager reference */
    lle_display_state_t *display;           /**< Display state reference */
    
    // Synchronization configuration
    bool bidirectional_sync_enabled;        /**< Enable bidirectional sync */
    bool ansi_sequence_tracking;            /**< Track ANSI sequences */
    bool strict_validation_mode;            /**< Enable strict validation */
    size_t max_sync_failures;               /**< Max failures before fallback */
    
    // Event callbacks
    void (*on_state_divergence)(const char *reason);  /**< Called on divergence */
    void (*on_sync_failure)(const char *error);       /**< Called on sync failure */
    void (*on_ansi_sequence)(const char *sequence);   /**< Called on ANSI sequence */
    
    // Statistics
    size_t total_divergences;               /**< Total state divergences detected */
    size_t total_recoveries;                /**< Total successful recoveries */
    size_t ansi_sequences_processed;        /**< ANSI sequences handled */
} lle_state_sync_context_t;

// ============================================================================
// Core Synchronization API
// ============================================================================

/**
 * @brief Initialize state synchronization system
 * 
 * @param terminal Terminal manager instance
 * @param display Display state instance
 * @return Allocated sync context, NULL on error
 */
lle_state_sync_context_t *lle_state_sync_init(lle_terminal_manager_t *terminal,
                                               lle_display_state_t *display);

/**
 * @brief Cleanup state synchronization system
 * 
 * @param sync_ctx Sync context to cleanup
 */
void lle_state_sync_cleanup(lle_state_sync_context_t *sync_ctx);

/**
 * @brief Perform bidirectional state synchronization
 * 
 * @param sync_ctx Sync context
 * @return true on success, false on error
 */
bool lle_state_sync_perform(lle_state_sync_context_t *sync_ctx);

/**
 * @brief Validate terminal and display state consistency
 * 
 * @param sync_ctx Sync context
 * @return true if states are consistent, false if diverged
 */
bool lle_state_sync_validate(lle_state_sync_context_t *sync_ctx);

/**
 * @brief Force complete state resynchronization
 * 
 * @param sync_ctx Sync context
 * @return true on success, false on error
 */
bool lle_state_sync_force_resync(lle_state_sync_context_t *sync_ctx);

// ============================================================================
// Terminal State Tracking API
// ============================================================================

/**
 * @brief Update terminal state after write operation
 * 
 * @param sync_ctx Sync context
 * @param data Data written to terminal
 * @param length Length of data written
 * @param cursor_row Row position after write
 * @param cursor_col Column position after write
 * @return true on success, false on error
 */
bool lle_terminal_state_update_write(lle_state_sync_context_t *sync_ctx,
                                     const char *data,
                                     size_t length,
                                     size_t cursor_row,
                                     size_t cursor_col);

/**
 * @brief Update terminal state after ANSI clear sequence
 * 
 * @param sync_ctx Sync context
 * @param clear_type Type of clear operation
 * @param start_row Start row of clear region
 * @param start_col Start column of clear region
 * @param end_row End row of clear region
 * @param end_col End column of clear region
 * @return true on success, false on error
 */
bool lle_terminal_state_update_clear(lle_state_sync_context_t *sync_ctx,
                                     const char *clear_type,
                                     size_t start_row,
                                     size_t start_col,
                                     size_t end_row,
                                     size_t end_col);

/**
 * @brief Update terminal state after cursor movement
 * 
 * @param sync_ctx Sync context
 * @param new_row New cursor row
 * @param new_col New cursor column
 * @return true on success, false on error
 */
bool lle_terminal_state_update_cursor(lle_state_sync_context_t *sync_ctx,
                                      size_t new_row,
                                      size_t new_col);

/**
 * @brief Process ANSI escape sequence and update state
 * 
 * @param sync_ctx Sync context
 * @param sequence ANSI escape sequence
 * @param length Length of sequence
 * @return true on success, false on error
 */
bool lle_terminal_state_process_ansi(lle_state_sync_context_t *sync_ctx,
                                     const char *sequence,
                                     size_t length);

// ============================================================================
// Display State Synchronization API
// ============================================================================

/**
 * @brief Update display sync state with expected content
 * 
 * @param sync_ctx Sync context
 * @param content Expected display content
 * @param length Length of expected content
 * @param cursor_pos Expected cursor position
 * @param rows_used Expected number of rows used
 * @return true on success, false on error
 */
bool lle_display_sync_update_expected(lle_state_sync_context_t *sync_ctx,
                                      const char *content,
                                      size_t length,
                                      size_t cursor_pos,
                                      size_t rows_used);

/**
 * @brief Mark display state as requiring synchronization
 * 
 * @param sync_ctx Sync context
 * @param reason Reason for sync requirement
 * @return true on success, false on error
 */
bool lle_display_sync_mark_dirty(lle_state_sync_context_t *sync_ctx,
                                 const char *reason);

/**
 * @brief Check if display state matches expected state
 * 
 * @param sync_ctx Sync context
 * @param actual_content Actual content from terminal query
 * @param actual_length Length of actual content
 * @param actual_cursor_pos Actual cursor position
 * @return true if states match, false if diverged
 */
bool lle_display_sync_check_consistency(lle_state_sync_context_t *sync_ctx,
                                        const char *actual_content,
                                        size_t actual_length,
                                        size_t actual_cursor_pos);

// ============================================================================
// Advanced Synchronization Features
// ============================================================================

/**
 * @brief Query actual terminal state via ANSI sequences
 * 
 * @param sync_ctx Sync context
 * @param query_content Whether to query displayed content
 * @param query_cursor Whether to query cursor position
 * @return true on success, false on error
 */
bool lle_state_sync_query_terminal(lle_state_sync_context_t *sync_ctx,
                                   bool query_content,
                                   bool query_cursor);

/**
 * @brief Detect and recover from state divergence
 * 
 * @param sync_ctx Sync context
 * @return true if recovery successful, false if unrecoverable
 */
bool lle_state_sync_recover_divergence(lle_state_sync_context_t *sync_ctx);

/**
 * @brief Enable strict validation mode for debugging
 * 
 * @param sync_ctx Sync context
 * @param enable Whether to enable strict mode
 */
void lle_state_sync_set_strict_mode(lle_state_sync_context_t *sync_ctx,
                                    bool enable);

/**
 * @brief Get synchronization performance statistics
 * 
 * @param sync_ctx Sync context
 * @param total_syncs Output: total sync operations
 * @param total_failures Output: total sync failures
 * @param avg_sync_time Output: average sync time in microseconds
 * @param divergence_rate Output: rate of state divergences
 * @return true on success, false on error
 */
bool lle_state_sync_get_statistics(lle_state_sync_context_t *sync_ctx,
                                   size_t *total_syncs,
                                   size_t *total_failures,
                                   double *avg_sync_time,
                                   double *divergence_rate);

// ============================================================================
// Wrapper Functions for Safe Terminal Operations
// ============================================================================

/**
 * @brief Write data to terminal with automatic state tracking
 * 
 * @param sync_ctx Sync context
 * @param data Data to write
 * @param length Length of data
 * @return true on success, false on error
 */
bool lle_state_sync_terminal_write(lle_state_sync_context_t *sync_ctx,
                                   const char *data,
                                   size_t length);

/**
 * @brief Clear terminal region with automatic state tracking
 * 
 * @param sync_ctx Sync context
 * @param start_row Start row to clear
 * @param start_col Start column to clear
 * @param end_row End row to clear
 * @param end_col End column to clear
 * @return true on success, false on error
 */
bool lle_state_sync_terminal_clear(lle_state_sync_context_t *sync_ctx,
                                   size_t start_row,
                                   size_t start_col,
                                   size_t end_row,
                                   size_t end_col);

/**
 * @brief Move cursor with automatic state tracking
 * 
 * @param sync_ctx Sync context
 * @param row Target row
 * @param col Target column
 * @return true on success, false on error
 */
bool lle_state_sync_terminal_move_cursor(lle_state_sync_context_t *sync_ctx,
                                         size_t row,
                                         size_t col);

// ============================================================================
// Debug and Diagnostic Functions
// ============================================================================

/**
 * @brief Dump current terminal state for debugging
 * 
 * @param sync_ctx Sync context
 * @param output_file File to write debug output (NULL for stderr)
 */
void lle_state_sync_debug_dump_terminal(lle_state_sync_context_t *sync_ctx,
                                        FILE *output_file);

/**
 * @brief Dump current display sync state for debugging
 * 
 * @param sync_ctx Sync context
 * @param output_file File to write debug output (NULL for stderr)
 */
void lle_state_sync_debug_dump_display(lle_state_sync_context_t *sync_ctx,
                                       FILE *output_file);

/**
 * @brief Compare terminal and display states and report differences
 * 
 * @param sync_ctx Sync context
 * @param output_file File to write comparison (NULL for stderr)
 * @return Number of differences found
 */
size_t lle_state_sync_debug_compare_states(lle_state_sync_context_t *sync_ctx,
                                           FILE *output_file);

#endif // LLE_DISPLAY_STATE_SYNC_H