/**
 * @file buffer_trace.h
 * @brief File-based buffer tracing system for double-deletion bug investigation
 * 
 * This module provides comprehensive buffer state tracking functionality that
 * logs to files instead of stderr to avoid contaminating terminal display
 * during interactive debugging sessions. Designed specifically to identify
 * the root cause of the double-deletion bug in backspace operations.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024-12-29
 */

#ifndef LLE_BUFFER_TRACE_H
#define LLE_BUFFER_TRACE_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Include headers for type definitions
#include "text_buffer.h"
#include "display.h"

// ============================================================================
// Configuration Constants
// ============================================================================

#define LLE_TRACE_MAX_CONTENT_PREVIEW 128
#define LLE_TRACE_MAX_OPERATION_NAME 64
#define LLE_TRACE_MAX_SNAPSHOTS 1000
#define LLE_TRACE_LOG_PATH "/tmp/lle_buffer_trace.log"
#define LLE_TRACE_SUMMARY_PATH "/tmp/lle_buffer_summary.log"

// ============================================================================
// Trace Data Structures
// ============================================================================

/**
 * @brief Buffer state snapshot for debugging
 */
typedef struct {
    char operation[LLE_TRACE_MAX_OPERATION_NAME];
    char content_preview[LLE_TRACE_MAX_CONTENT_PREVIEW];
    size_t buffer_length;
    size_t cursor_position;
    size_t char_count;
    uint64_t timestamp_us;
    uint32_t sequence_number;
    bool boundary_crossing_detected;
    bool display_fallback_triggered;
    int operation_depth;
} lle_buffer_snapshot_t;

/**
 * @brief Analysis results for detecting anomalies
 */
typedef struct {
    bool double_deletion_detected;
    bool unexpected_length_change;
    bool cursor_position_anomaly;
    size_t expected_length_change;
    size_t actual_length_change;
    size_t previous_snapshot_index;
} lle_trace_analysis_t;

/**
 * @brief Trace session statistics
 */
typedef struct {
    size_t total_operations;
    size_t backspace_operations;
    size_t boundary_crossings;
    size_t double_deletions_detected;
    size_t fallback_renders;
    uint64_t session_start_time;
    bool session_active;
} lle_trace_stats_t;

// ============================================================================
// Core Tracing Functions
// ============================================================================

/**
 * @brief Initialize the buffer tracing system
 *
 * Creates log files and initializes internal state. Must be called before
 * any other tracing functions. Safe to call multiple times.
 *
 * @return true on success, false on failure
 */
bool lle_trace_init(void);

/**
 * @brief Shutdown the buffer tracing system
 *
 * Closes log files, writes summary, and cleans up resources.
 * Safe to call multiple times.
 */
void lle_trace_shutdown(void);

/**
 * @brief Check if tracing is currently active
 *
 * @return true if tracing is enabled and initialized
 */
bool lle_trace_is_active(void);

/**
 * @brief Capture current buffer state
 *
 * Records a snapshot of the buffer state with the given operation name.
 * This is the primary function for tracking buffer changes.
 *
 * @param operation Name of the operation being performed
 * @param buffer Text buffer to capture
 * @param boundary_crossing Whether this operation crosses line wrap boundary
 * @param fallback_triggered Whether display fallback was triggered
 * @return Snapshot index for reference, or -1 on failure
 */
int lle_trace_capture_buffer(const char *operation, 
                            lle_text_buffer_t *buffer,
                            bool boundary_crossing,
                            bool fallback_triggered);

/**
 * @brief Capture display state information
 *
 * Records display-related state that might affect buffer operations.
 * Call this in conjunction with buffer captures for complete context.
 *
 * @param operation Name of the operation
 * @param display Display state to capture
 * @return true on success, false on failure
 */
bool lle_trace_capture_display(const char *operation, 
                              lle_display_state_t *display);

// ============================================================================
// Specialized Tracing Functions
// ============================================================================

/**
 * @brief Trace backspace operation start
 *
 * Call this at the beginning of lle_cmd_backspace() before any buffer
 * modifications. Returns a trace session ID for correlation.
 *
 * @param buffer Buffer before modification
 * @param display Display state
 * @return Session ID for correlation, or -1 on failure
 */
int lle_trace_backspace_start(lle_text_buffer_t *buffer, 
                             lle_display_state_t *display);

/**
 * @brief Trace backspace operation completion
 *
 * Call this at the end of lle_cmd_backspace() after all operations.
 * Uses session ID from backspace_start for correlation.
 *
 * @param session_id Session ID from backspace_start
 * @param buffer Buffer after modification
 * @param display Display state
 * @param success Whether the operation succeeded
 */
void lle_trace_backspace_end(int session_id,
                            lle_text_buffer_t *buffer,
                            lle_display_state_t *display,
                            bool success);

/**
 * @brief Trace buffer modification function calls
 *
 * Call this before and after lle_text_backspace() to track direct
 * buffer modifications.
 *
 * @param function_name Name of buffer function being called
 * @param buffer Buffer state
 * @param before true if called before function, false if after
 */
void lle_trace_buffer_function(const char *function_name,
                              lle_text_buffer_t *buffer,
                              bool before);

/**
 * @brief Trace display update operations
 *
 * Call this to track display update calls that might affect buffer state.
 *
 * @param update_type Type of display update (incremental, render, etc.)
 * @param buffer Current buffer state
 * @param boundary_detected Whether boundary crossing was detected
 * @param fallback_used Whether fallback rendering was used
 */
void lle_trace_display_update(const char *update_type,
                             lle_text_buffer_t *buffer,
                             bool boundary_detected,
                             bool fallback_used);

// ============================================================================
// Analysis and Detection Functions
// ============================================================================

/**
 * @brief Analyze last operation for anomalies
 *
 * Compares the last two snapshots to detect double deletions,
 * unexpected changes, or other anomalies.
 *
 * @param analysis Output structure for analysis results
 * @return true if analysis was performed, false if insufficient data
 */
bool lle_trace_analyze_last_operation(lle_trace_analysis_t *analysis);

/**
 * @brief Detect double deletion in backspace sequence
 *
 * Analyzes a sequence of snapshots to identify if a single backspace
 * operation resulted in multiple character deletions.
 *
 * @param start_snapshot_id Starting snapshot for analysis
 * @param end_snapshot_id Ending snapshot for analysis
 * @return true if double deletion detected, false otherwise
 */
bool lle_trace_detect_double_deletion(int start_snapshot_id, 
                                     int end_snapshot_id);

/**
 * @brief Check for buffer consistency violations
 *
 * Validates that buffer state is internally consistent and matches
 * expected invariants.
 *
 * @param buffer Buffer to validate
 * @return true if buffer is consistent, false if corrupted
 */
bool lle_trace_validate_buffer_consistency(lle_text_buffer_t *buffer);

// ============================================================================
// Reporting and Output Functions
// ============================================================================

/**
 * @brief Write immediate trace entry
 *
 * Immediately flushes a trace entry to the log file. Use for critical
 * operations that must be logged even if the program crashes.
 *
 * @param operation Operation name
 * @param buffer Buffer state
 * @param critical Whether this is a critical trace point
 */
void lle_trace_write_immediate(const char *operation,
                              lle_text_buffer_t *buffer,
                              bool critical);

/**
 * @brief Generate analysis summary
 *
 * Writes a comprehensive summary of all traced operations and detected
 * anomalies to the summary file.
 *
 * @return true on success, false on failure
 */
bool lle_trace_generate_summary(void);

/**
 * @brief Print current session statistics
 *
 * Outputs current statistics to the trace log. Useful for monitoring
 * progress during long debugging sessions.
 */
void lle_trace_print_stats(void);

/**
 * @brief Export snapshots for external analysis
 *
 * Writes all captured snapshots in a machine-readable format for
 * external analysis tools.
 *
 * @param output_path Path for output file
 * @return true on success, false on failure
 */
bool lle_trace_export_snapshots(const char *output_path);

// ============================================================================
// Control and Configuration Functions
// ============================================================================

/**
 * @brief Enable/disable specific trace categories
 *
 * Allows fine-grained control over what gets traced to reduce noise
 * during targeted investigation.
 *
 * @param buffer_ops Trace buffer operations
 * @param display_ops Trace display operations
 * @param analysis Trace analysis results
 * @param verbose Include verbose details
 */
void lle_trace_configure(bool buffer_ops, 
                        bool display_ops, 
                        bool analysis, 
                        bool verbose);

/**
 * @brief Set trace filtering criteria
 *
 * Only trace operations that match the specified criteria.
 * Useful for focusing on specific scenarios.
 *
 * @param min_buffer_length Only trace if buffer length >= this value
 * @param boundary_crossing_only Only trace boundary crossing operations
 * @param backspace_only Only trace backspace-related operations
 */
void lle_trace_set_filter(size_t min_buffer_length,
                         bool boundary_crossing_only,
                         bool backspace_only);

/**
 * @brief Clear all captured snapshots
 *
 * Resets the snapshot buffer and statistics. Useful for starting
 * a fresh debugging session.
 */
void lle_trace_clear_snapshots(void);

/**
 * @brief Force flush all pending traces
 *
 * Ensures all pending trace data is written to disk immediately.
 * Call this before critical operations or program exit.
 */
void lle_trace_flush(void);

// ============================================================================
// Utility Macros for Easy Integration
// ============================================================================

/**
 * @brief Convenience macro for tracing buffer state
 */
#define LLE_TRACE_BUFFER(op, buf) \
    do { if (lle_trace_is_active()) lle_trace_capture_buffer((op), (buf), false, false); } while(0)

/**
 * @brief Convenience macro for tracing boundary crossing operations
 */
#define LLE_TRACE_BOUNDARY(op, buf) \
    do { if (lle_trace_is_active()) lle_trace_capture_buffer((op), (buf), true, false); } while(0)

/**
 * @brief Convenience macro for tracing with fallback indication
 */
#define LLE_TRACE_FALLBACK(op, buf) \
    do { if (lle_trace_is_active()) lle_trace_capture_buffer((op), (buf), false, true); } while(0)

/**
 * @brief Convenience macro for critical trace points
 */
#define LLE_TRACE_CRITICAL(op, buf) \
    do { if (lle_trace_is_active()) lle_trace_write_immediate((op), (buf), true); } while(0)

/**
 * @brief Convenience macro for function entry/exit tracing
 */
#define LLE_TRACE_FUNCTION(func, buf, before) \
    do { if (lle_trace_is_active()) lle_trace_buffer_function((func), (buf), (before)); } while(0)

// ============================================================================
// Environment Variable Controls
// ============================================================================

/**
 * Environment variables for runtime control:
 * 
 * LLE_TRACE_ENABLED=1          - Enable tracing system
 * LLE_TRACE_VERBOSE=1          - Enable verbose output
 * LLE_TRACE_BUFFER_OPS=1       - Trace buffer operations
 * LLE_TRACE_DISPLAY_OPS=1      - Trace display operations
 * LLE_TRACE_BOUNDARY_ONLY=1    - Only trace boundary crossing operations
 * LLE_TRACE_BACKSPACE_ONLY=1   - Only trace backspace operations
 * LLE_TRACE_MIN_LENGTH=N       - Only trace if buffer length >= N
 * LLE_TRACE_LOG_PATH=path      - Custom log file path
 * LLE_TRACE_AUTO_ANALYSIS=1    - Automatically analyze each operation
 */

#endif // LLE_BUFFER_TRACE_H