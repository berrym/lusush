/**
 * @file buffer_trace.c
 * @brief File-based buffer tracing system implementation for double-deletion bug investigation
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

#include "text_buffer.h"
#include "display.h"
#include "buffer_trace.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

// ============================================================================
// Internal State Management
// ============================================================================

static struct {
    FILE *trace_log;
    FILE *summary_log;
    lle_buffer_snapshot_t snapshots[LLE_TRACE_MAX_SNAPSHOTS];
    size_t snapshot_count;
    uint32_t sequence_counter;
    lle_trace_stats_t stats;
    
    // Configuration
    bool enabled;
    bool buffer_ops_enabled;
    bool display_ops_enabled;
    bool analysis_enabled;
    bool verbose_enabled;
    
    // Filtering
    size_t min_buffer_length;
    bool boundary_crossing_only;
    bool backspace_only;
    
    // Session tracking
    int current_operation_depth;
    bool initialized;
} g_trace_state = {0};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Get current timestamp in microseconds
 *
 * @return Timestamp in microseconds since epoch
 */
static uint64_t lle_trace_get_timestamp_us(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
    }
    return 0;
}

/**
 * @brief Check if operation passes current filters
 *
 * @param operation Operation name
 * @param buffer Buffer state
 * @param boundary_crossing Whether operation crosses boundary
 * @return true if operation should be traced
 */
static bool lle_trace_passes_filter(const char *operation, 
                                   lle_text_buffer_t *buffer,
                                   bool boundary_crossing) {
    if (!g_trace_state.enabled) return false;
    
    // Check buffer length filter
    if (buffer && buffer->length < g_trace_state.min_buffer_length) {
        return false;
    }
    
    // Check boundary crossing filter
    if (g_trace_state.boundary_crossing_only && !boundary_crossing) {
        return false;
    }
    
    // Check backspace operation filter
    if (g_trace_state.backspace_only && operation) {
        if (strstr(operation, "BACKSPACE") == NULL && 
            strstr(operation, "backspace") == NULL) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Initialize trace state from environment variables
 */
static void lle_trace_init_from_env(void) {
    // Enable tracing if requested
    const char *enabled = getenv("LLE_TRACE_ENABLED");
    g_trace_state.enabled = enabled && (strcmp(enabled, "1") == 0);
    
    if (!g_trace_state.enabled) return;
    
    // Configure trace categories
    const char *buffer_ops = getenv("LLE_TRACE_BUFFER_OPS");
    g_trace_state.buffer_ops_enabled = !buffer_ops || (strcmp(buffer_ops, "1") == 0);
    
    const char *display_ops = getenv("LLE_TRACE_DISPLAY_OPS");
    g_trace_state.display_ops_enabled = !display_ops || (strcmp(display_ops, "1") == 0);
    
    const char *analysis = getenv("LLE_TRACE_AUTO_ANALYSIS");
    g_trace_state.analysis_enabled = analysis && (strcmp(analysis, "1") == 0);
    
    const char *verbose = getenv("LLE_TRACE_VERBOSE");
    g_trace_state.verbose_enabled = verbose && (strcmp(verbose, "1") == 0);
    
    // Configure filters
    const char *min_length = getenv("LLE_TRACE_MIN_LENGTH");
    if (min_length) {
        g_trace_state.min_buffer_length = (size_t)atoi(min_length);
    }
    
    const char *boundary_only = getenv("LLE_TRACE_BOUNDARY_ONLY");
    g_trace_state.boundary_crossing_only = boundary_only && (strcmp(boundary_only, "1") == 0);
    
    const char *backspace_only = getenv("LLE_TRACE_BACKSPACE_ONLY");
    g_trace_state.backspace_only = backspace_only && (strcmp(backspace_only, "1") == 0);
}

/**
 * @brief Write trace entry to log file
 *
 * @param snapshot Snapshot to write
 * @param immediate Whether to flush immediately
 */
static void lle_trace_write_entry(const lle_buffer_snapshot_t *snapshot, bool immediate) {
    if (!g_trace_state.trace_log || !snapshot) return;
    
    fprintf(g_trace_state.trace_log, 
           "[%u] %llu %s: len=%zu, cursor=%zu, chars=%zu, depth=%d, boundary=%s, fallback=%s, content='%s'\n",
           snapshot->sequence_number,
           (unsigned long long)snapshot->timestamp_us,
           snapshot->operation,
           snapshot->buffer_length,
           snapshot->cursor_position,
           snapshot->char_count,
           snapshot->operation_depth,
           snapshot->boundary_crossing_detected ? "YES" : "NO",
           snapshot->display_fallback_triggered ? "YES" : "NO",
           snapshot->content_preview);
    
    if (immediate) {
        fflush(g_trace_state.trace_log);
    }
}

/**
 * @brief Create safe content preview
 *
 * @param buffer Buffer to preview
 * @param preview Output buffer for preview
 * @param preview_size Size of preview buffer
 */
static void lle_trace_create_content_preview(lle_text_buffer_t *buffer, 
                                            char *preview, 
                                            size_t preview_size) {
    if (!buffer || !preview || preview_size == 0) return;
    
    if (!buffer->buffer || buffer->length == 0) {
        strcpy(preview, "(empty)");
        return;
    }
    
    size_t copy_length = buffer->length < preview_size - 1 ? buffer->length : preview_size - 1;
    
    // Copy and sanitize content
    for (size_t i = 0; i < copy_length; i++) {
        char c = buffer->buffer[i];
        if (c >= 32 && c <= 126) {
            preview[i] = c; // Printable ASCII
        } else if (c == '\t') {
            preview[i] = ' '; // Convert tabs to spaces
        } else {
            preview[i] = '?'; // Non-printable characters
        }
    }
    
    preview[copy_length] = '\0';
}

// ============================================================================
// Core Tracing Functions Implementation
// ============================================================================

bool lle_trace_init(void) {
    if (g_trace_state.initialized) {
        return true; // Already initialized
    }
    
    // Initialize from environment
    lle_trace_init_from_env();
    
    if (!g_trace_state.enabled) {
        g_trace_state.initialized = true;
        return true; // Tracing disabled, but init successful
    }
    
    // Get custom log path if specified
    const char *log_path = getenv("LLE_TRACE_LOG_PATH");
    if (!log_path) log_path = LLE_TRACE_LOG_PATH;
    
    // Open trace log
    g_trace_state.trace_log = fopen(log_path, "w");
    if (!g_trace_state.trace_log) {
        g_trace_state.enabled = false;
        return false;
    }
    
    // Open summary log
    g_trace_state.summary_log = fopen(LLE_TRACE_SUMMARY_PATH, "w");
    if (!g_trace_state.summary_log) {
        fclose(g_trace_state.trace_log);
        g_trace_state.trace_log = NULL;
        g_trace_state.enabled = false;
        return false;
    }
    
    // Initialize state
    memset(&g_trace_state.stats, 0, sizeof(g_trace_state.stats));
    g_trace_state.stats.session_start_time = lle_trace_get_timestamp_us();
    g_trace_state.stats.session_active = true;
    g_trace_state.snapshot_count = 0;
    g_trace_state.sequence_counter = 1;
    g_trace_state.current_operation_depth = 0;
    g_trace_state.initialized = true;
    
    // Write initialization header
    fprintf(g_trace_state.trace_log, "=== LLE Buffer Trace Session Started ===\n");
    fprintf(g_trace_state.trace_log, "Timestamp: %llu\n", 
           (unsigned long long)g_trace_state.stats.session_start_time);
    fprintf(g_trace_state.trace_log, "PID: %d\n", getpid());
    fprintf(g_trace_state.trace_log, "Configuration: buffer_ops=%s, display_ops=%s, analysis=%s, verbose=%s\n",
           g_trace_state.buffer_ops_enabled ? "ON" : "OFF",
           g_trace_state.display_ops_enabled ? "ON" : "OFF", 
           g_trace_state.analysis_enabled ? "ON" : "OFF",
           g_trace_state.verbose_enabled ? "ON" : "OFF");
    fprintf(g_trace_state.trace_log, "Filters: min_length=%zu, boundary_only=%s, backspace_only=%s\n",
           g_trace_state.min_buffer_length,
           g_trace_state.boundary_crossing_only ? "ON" : "OFF",
           g_trace_state.backspace_only ? "ON" : "OFF");
    fprintf(g_trace_state.trace_log, "==========================================\n\n");
    fflush(g_trace_state.trace_log);
    
    // Debug output to stderr to verify tracing is working
    fprintf(stderr, "[TRACE_DEBUG] LLE Buffer Trace initialized successfully\n");
    fprintf(stderr, "[TRACE_DEBUG] Log file: %s\n", log_path);
    fprintf(stderr, "[TRACE_DEBUG] Tracing enabled: %s\n", g_trace_state.enabled ? "YES" : "NO");
    fflush(stderr);
    
    return true;
}

void lle_trace_shutdown(void) {
    if (!g_trace_state.initialized) return;
    
    if (g_trace_state.enabled && g_trace_state.stats.session_active) {
        // Generate final summary
        lle_trace_generate_summary();
        
        // Write session footer
        if (g_trace_state.trace_log) {
            uint64_t session_end = lle_trace_get_timestamp_us();
            uint64_t duration = session_end - g_trace_state.stats.session_start_time;
            
            fprintf(g_trace_state.trace_log, "\n=== LLE Buffer Trace Session Ended ===\n");
            fprintf(g_trace_state.trace_log, "End timestamp: %llu\n", (unsigned long long)session_end);
            fprintf(g_trace_state.trace_log, "Session duration: %llu microseconds\n", (unsigned long long)duration);
            fprintf(g_trace_state.trace_log, "Total operations: %zu\n", g_trace_state.stats.total_operations);
            fprintf(g_trace_state.trace_log, "Double deletions detected: %zu\n", g_trace_state.stats.double_deletions_detected);
            fprintf(g_trace_state.trace_log, "=========================================\n");
            fflush(g_trace_state.trace_log);
        }
        
        g_trace_state.stats.session_active = false;
    }
    
    // Close files
    if (g_trace_state.trace_log) {
        fclose(g_trace_state.trace_log);
        g_trace_state.trace_log = NULL;
    }
    
    if (g_trace_state.summary_log) {
        fclose(g_trace_state.summary_log);
        g_trace_state.summary_log = NULL;
    }
    
    // Reset state
    g_trace_state.enabled = false;
    g_trace_state.initialized = false;
}

bool lle_trace_is_active(void) {
    return g_trace_state.initialized && g_trace_state.enabled && g_trace_state.stats.session_active;
}

int lle_trace_capture_buffer(const char *operation, 
                            lle_text_buffer_t *buffer,
                            bool boundary_crossing,
                            bool fallback_triggered) {
    if (!lle_trace_is_active() || !operation || !buffer) {
        return -1;
    }
    
    if (!lle_trace_passes_filter(operation, buffer, boundary_crossing)) {
        return -1;
    }
    
    if (g_trace_state.snapshot_count >= LLE_TRACE_MAX_SNAPSHOTS) {
        return -1; // Snapshot buffer full
    }
    
    lle_buffer_snapshot_t *snapshot = &g_trace_state.snapshots[g_trace_state.snapshot_count];
    
    // Fill snapshot data
    strncpy(snapshot->operation, operation, LLE_TRACE_MAX_OPERATION_NAME - 1);
    snapshot->operation[LLE_TRACE_MAX_OPERATION_NAME - 1] = '\0';
    
    lle_trace_create_content_preview(buffer, snapshot->content_preview, LLE_TRACE_MAX_CONTENT_PREVIEW);
    
    snapshot->buffer_length = buffer->length;
    snapshot->cursor_position = buffer->cursor_pos;
    snapshot->char_count = buffer->char_count;
    snapshot->timestamp_us = lle_trace_get_timestamp_us();
    snapshot->sequence_number = g_trace_state.sequence_counter++;
    snapshot->boundary_crossing_detected = boundary_crossing;
    snapshot->display_fallback_triggered = fallback_triggered;
    snapshot->operation_depth = g_trace_state.current_operation_depth;
    
    // Write to log immediately for critical operations
    bool is_critical = strstr(operation, "CRITICAL") != NULL || 
                      strstr(operation, "ERROR") != NULL ||
                      boundary_crossing;
    
    lle_trace_write_entry(snapshot, is_critical);
    
    // Update statistics
    g_trace_state.stats.total_operations++;
    if (strstr(operation, "BACKSPACE") != NULL) {
        g_trace_state.stats.backspace_operations++;
    }
    if (boundary_crossing) {
        g_trace_state.stats.boundary_crossings++;
    }
    if (fallback_triggered) {
        g_trace_state.stats.fallback_renders++;
    }
    
    // Perform automatic analysis if enabled
    if (g_trace_state.analysis_enabled && g_trace_state.snapshot_count > 0) {
        lle_trace_analysis_t analysis;
        if (lle_trace_analyze_last_operation(&analysis)) {
            if (analysis.double_deletion_detected) {
                g_trace_state.stats.double_deletions_detected++;
                
                // Write critical alert to log
                fprintf(g_trace_state.trace_log, 
                       "!!! CRITICAL: DOUBLE DELETION DETECTED in operation %s !!!\n",
                       operation);
                fprintf(g_trace_state.trace_log,
                       "Expected change: %zu, Actual change: %zu\n",
                       analysis.expected_length_change, analysis.actual_length_change);
                fflush(g_trace_state.trace_log);
            }
        }
    }
    
    int snapshot_index = g_trace_state.snapshot_count;
    g_trace_state.snapshot_count++;
    
    return snapshot_index;
}

bool lle_trace_capture_display(const char *operation, lle_display_state_t *display) {
    if (!lle_trace_is_active() || !g_trace_state.display_ops_enabled || !operation || !display) {
        return false;
    }
    
    // Capture display state information
    if (g_trace_state.trace_log) {
        fprintf(g_trace_state.trace_log, "[DISPLAY] %s: display=%p\n",
               operation, (void*)display);
        
        if (g_trace_state.verbose_enabled && display) {
            // Add more display state details if needed
            fprintf(g_trace_state.trace_log, "  [DISPLAY_DETAIL] display_ptr=%p\n", (void*)display);
        }
    }
    
    return true;
}

// ============================================================================
// Specialized Tracing Functions Implementation
// ============================================================================

int lle_trace_backspace_start(lle_text_buffer_t *buffer, lle_display_state_t *display) {
    if (!lle_trace_is_active()) return -1;
    
    // Debug output to stderr for backspace operations
    fprintf(stderr, "[TRACE_DEBUG] BACKSPACE_START - buffer len=%zu, cursor=%zu\n", 
           buffer ? buffer->length : 0, buffer ? buffer->cursor_pos : 0);
    fflush(stderr);
    
    g_trace_state.current_operation_depth++;
    
    int session_id = lle_trace_capture_buffer("BACKSPACE_START", buffer, false, false);
    
    if (display && g_trace_state.display_ops_enabled) {
        lle_trace_capture_display("BACKSPACE_START", display);
    }
    
    return session_id;
}

void lle_trace_backspace_end(int session_id, lle_text_buffer_t *buffer, 
                            lle_display_state_t *display, bool success) {
    if (!lle_trace_is_active()) return;
    
    char operation[64];
    snprintf(operation, sizeof(operation), "BACKSPACE_END_%s", success ? "SUCCESS" : "FAILED");
    
    lle_trace_capture_buffer(operation, buffer, false, false);
    
    if (display && g_trace_state.display_ops_enabled) {
        lle_trace_capture_display(operation, display);
    }
    
    g_trace_state.current_operation_depth--;
    
    // If this was a backspace operation, check for double deletion
    if (success && session_id >= 0 && g_trace_state.analysis_enabled) {
        if (lle_trace_detect_double_deletion(session_id, g_trace_state.snapshot_count - 1)) {
            fprintf(g_trace_state.trace_log, 
                   "!!! DOUBLE DELETION CONFIRMED: Session %d -> %zu !!!\n",
                   session_id, g_trace_state.snapshot_count - 1);
            fflush(g_trace_state.trace_log);
        }
    }
}

void lle_trace_buffer_function(const char *function_name, lle_text_buffer_t *buffer, bool before) {
    if (!lle_trace_is_active() || !g_trace_state.buffer_ops_enabled) return;
    
    char operation[128];
    snprintf(operation, sizeof(operation), "%s_%s", function_name, before ? "BEFORE" : "AFTER");
    
    lle_trace_capture_buffer(operation, buffer, false, false);
}

void lle_trace_display_update(const char *update_type, lle_text_buffer_t *buffer,
                             bool boundary_detected, bool fallback_used) {
    if (!lle_trace_is_active() || !g_trace_state.display_ops_enabled) return;
    
    char operation[128];
    snprintf(operation, sizeof(operation), "DISPLAY_%s", update_type);
    
    lle_trace_capture_buffer(operation, buffer, boundary_detected, fallback_used);
}

// ============================================================================
// Analysis and Detection Functions Implementation
// ============================================================================

bool lle_trace_analyze_last_operation(lle_trace_analysis_t *analysis) {
    if (!analysis || g_trace_state.snapshot_count < 2) {
        return false;
    }
    
    memset(analysis, 0, sizeof(lle_trace_analysis_t));
    
    const lle_buffer_snapshot_t *current = &g_trace_state.snapshots[g_trace_state.snapshot_count - 1];
    const lle_buffer_snapshot_t *previous = &g_trace_state.snapshots[g_trace_state.snapshot_count - 2];
    
    analysis->previous_snapshot_index = g_trace_state.snapshot_count - 2;
    
    // Analyze length changes
    if (current->buffer_length != previous->buffer_length) {
        analysis->actual_length_change = previous->buffer_length > current->buffer_length ?
                                        previous->buffer_length - current->buffer_length :
                                        current->buffer_length - previous->buffer_length;
        
        // For backspace operations, expect exactly 1 character deletion
        if (strstr(current->operation, "BACKSPACE") != NULL) {
            analysis->expected_length_change = 1;
            analysis->double_deletion_detected = (analysis->actual_length_change > 1);
        }
        
        analysis->unexpected_length_change = (analysis->actual_length_change != analysis->expected_length_change);
    }
    
    // Analyze cursor position changes
    if (current->cursor_position != previous->cursor_position) {
        // For backspace, cursor should move back by 1
        if (strstr(current->operation, "BACKSPACE") != NULL) {
            size_t expected_cursor_change = 1;
            size_t actual_cursor_change = previous->cursor_position > current->cursor_position ?
                                         previous->cursor_position - current->cursor_position :
                                         current->cursor_position - previous->cursor_position;
            
            analysis->cursor_position_anomaly = (actual_cursor_change != expected_cursor_change);
        }
    }
    
    return true;
}

bool lle_trace_detect_double_deletion(int start_snapshot_id, int end_snapshot_id) {
    if (start_snapshot_id < 0 || end_snapshot_id < 0 || 
        start_snapshot_id >= g_trace_state.snapshot_count ||
        end_snapshot_id >= g_trace_state.snapshot_count ||
        start_snapshot_id >= end_snapshot_id) {
        return false;
    }
    
    const lle_buffer_snapshot_t *start = &g_trace_state.snapshots[start_snapshot_id];
    const lle_buffer_snapshot_t *end = &g_trace_state.snapshots[end_snapshot_id];
    
    // Check if this looks like a backspace sequence
    bool has_backspace = false;
    for (int i = start_snapshot_id; i <= end_snapshot_id; i++) {
        if (strstr(g_trace_state.snapshots[i].operation, "BACKSPACE") != NULL) {
            has_backspace = true;
            break;
        }
    }
    
    if (!has_backspace) return false;
    
    // Check for excessive length reduction
    if (start->buffer_length > end->buffer_length) {
        size_t length_reduction = start->buffer_length - end->buffer_length;
        // For a single backspace sequence, should be exactly 1
        return length_reduction > 1;
    }
    
    return false;
}

bool lle_trace_validate_buffer_consistency(lle_text_buffer_t *buffer) {
    if (!buffer) return false;
    
    // Basic consistency checks
    if (buffer->cursor_pos > buffer->length) {
        if (g_trace_state.trace_log) {
            fprintf(g_trace_state.trace_log, "!!! CONSISTENCY ERROR: cursor_pos (%zu) > length (%zu) !!!\n",
                   buffer->cursor_pos, buffer->length);
            fflush(g_trace_state.trace_log);
        }
        return false;
    }
    
    if (buffer->char_count > buffer->length) {
        if (g_trace_state.trace_log) {
            fprintf(g_trace_state.trace_log, "!!! CONSISTENCY ERROR: char_count (%zu) > length (%zu) !!!\n",
                   buffer->char_count, buffer->length);
            fflush(g_trace_state.trace_log);
        }
        return false;
    }
    
    return true;
}

// ============================================================================
// Reporting and Output Functions Implementation
// ============================================================================

void lle_trace_write_immediate(const char *operation, lle_text_buffer_t *buffer, bool critical) {
    if (!lle_trace_is_active()) return;
    
    char full_operation[128];
    snprintf(full_operation, sizeof(full_operation), "%s%s", 
             critical ? "CRITICAL_" : "", operation);
    
    // Debug output to stderr for critical operations
    if (critical) {
        fprintf(stderr, "[TRACE_DEBUG] CRITICAL: %s - buffer len=%zu, cursor=%zu\n", 
               operation, buffer ? buffer->length : 0, buffer ? buffer->cursor_pos : 0);
        fflush(stderr);
    }
    
    int snapshot_id = lle_trace_capture_buffer(full_operation, buffer, false, false);
    if (snapshot_id >= 0) {
        fflush(g_trace_state.trace_log);
    }
}

bool lle_trace_generate_summary(void) {
    if (!g_trace_state.summary_log) return false;
    
    uint64_t current_time = lle_trace_get_timestamp_us();
    uint64_t session_duration = current_time - g_trace_state.stats.session_start_time;
    
    fprintf(g_trace_state.summary_log, "=== LLE Buffer Trace Summary ===\n");
    fprintf(g_trace_state.summary_log, "Session start: %llu\n", (unsigned long long)g_trace_state.stats.session_start_time);
    fprintf(g_trace_state.summary_log, "Session duration: %llu microseconds (%.2f seconds)\n", 
           (unsigned long long)session_duration, session_duration / 1000000.0);
    fprintf(g_trace_state.summary_log, "\nOperation Statistics:\n");
    fprintf(g_trace_state.summary_log, "  Total operations: %zu\n", g_trace_state.stats.total_operations);
    fprintf(g_trace_state.summary_log, "  Backspace operations: %zu\n", g_trace_state.stats.backspace_operations);
    fprintf(g_trace_state.summary_log, "  Boundary crossings: %zu\n", g_trace_state.stats.boundary_crossings);
    fprintf(g_trace_state.summary_log, "  Display fallbacks: %zu\n", g_trace_state.stats.fallback_renders);
    fprintf(g_trace_state.summary_log, "\nBug Detection:\n");
    fprintf(g_trace_state.summary_log, "  Double deletions detected: %zu\n", g_trace_state.stats.double_deletions_detected);
    
    if (g_trace_state.stats.backspace_operations > 0) {
        double error_rate = (double)g_trace_state.stats.double_deletions_detected / g_trace_state.stats.backspace_operations * 100.0;
        fprintf(g_trace_state.summary_log, "  Error rate: %.2f%%\n", error_rate);
    }
    
    fprintf(g_trace_state.summary_log, "\nSnapshot Details:\n");
    fprintf(g_trace_state.summary_log, "  Total snapshots: %zu\n", g_trace_state.snapshot_count);
    fprintf(g_trace_state.summary_log, "  Snapshots per operation: %.2f\n", 
           g_trace_state.stats.total_operations > 0 ? 
           (double)g_trace_state.snapshot_count / g_trace_state.stats.total_operations : 0.0);
    
    fprintf(g_trace_state.summary_log, "\n=== Analysis Results ===\n");
    
    // Analyze all snapshots for patterns
    size_t backspace_sequences = 0;
    size_t confirmed_double_deletions = 0;
    
    for (size_t i = 0; i < g_trace_state.snapshot_count - 1; i++) {
        if (strstr(g_trace_state.snapshots[i].operation, "BACKSPACE_START") != NULL) {
            backspace_sequences++;
            
            // Look for corresponding end
            for (size_t j = i + 1; j < g_trace_state.snapshot_count; j++) {
                if (strstr(g_trace_state.snapshots[j].operation, "BACKSPACE_END") != NULL) {
                    if (lle_trace_detect_double_deletion(i, j)) {
                        confirmed_double_deletions++;
                    }
                    break;
                }
            }
        }
    }
    
    fprintf(g_trace_state.summary_log, "Backspace sequences analyzed: %zu\n", backspace_sequences);
    fprintf(g_trace_state.summary_log, "Confirmed double deletions: %zu\n", confirmed_double_deletions);
    
    if (confirmed_double_deletions > 0) {
        fprintf(g_trace_state.summary_log, "\n!!! CRITICAL: DOUBLE DELETION BUG CONFIRMED !!!\n");
        fprintf(g_trace_state.summary_log, "Root cause investigation required in buffer modification logic.\n");
    } else {
        fprintf(g_trace_state.summary_log, "\nNo double deletions detected in this session.\n");
    }
    
    fprintf(g_trace_state.summary_log, "================================\n");
    fflush(g_trace_state.summary_log);
    
    return true;
}

void lle_trace_print_stats(void) {
    if (!lle_trace_is_active()) return;
    
    fprintf(g_trace_state.trace_log, "\n=== Current Session Stats ===\n");
    fprintf(g_trace_state.trace_log, "Operations: %zu, Backspaces: %zu, Boundaries: %zu, Double deletions: %zu\n",
           g_trace_state.stats.total_operations,
           g_trace_state.stats.backspace_operations,
           g_trace_state.stats.boundary_crossings,
           g_trace_state.stats.double_deletions_detected);
    fprintf(g_trace_state.trace_log, "============================\n\n");
    fflush(g_trace_state.trace_log);
}

bool lle_trace_export_snapshots(const char *output_path) {
    if (!output_path) return false;
    
    FILE *export_file = fopen(output_path, "w");
    if (!export_file) return false;
    
    fprintf(export_file, "seq,timestamp,operation,length,cursor,chars,depth,boundary,fallback,content\n");
    
    for (size_t i = 0; i < g_trace_state.snapshot_count; i++) {
        const lle_buffer_snapshot_t *snap = &g_trace_state.snapshots[i];
        fprintf(export_file, "%u,%llu,%s,%zu,%zu,%zu,%d,%s,%s,\"%s\"\n",
               snap->sequence_number,
               (unsigned long long)snap->timestamp_us,
               snap->operation,
               snap->buffer_length,
               snap->cursor_position,
               snap->char_count,
               snap->operation_depth,
               snap->boundary_crossing_detected ? "YES" : "NO",
               snap->display_fallback_triggered ? "YES" : "NO",
               snap->content_preview);
    }
    
    fclose(export_file);
    return true;
}

// ============================================================================
// Control and Configuration Functions Implementation
// ============================================================================

void lle_trace_configure(bool buffer_ops, bool display_ops, bool analysis, bool verbose) {
    g_trace_state.buffer_ops_enabled = buffer_ops;
    g_trace_state.display_ops_enabled = display_ops;
    g_trace_state.analysis_enabled = analysis;
    g_trace_state.verbose_enabled = verbose;
    
    if (g_trace_state.trace_log) {
        fprintf(g_trace_state.trace_log, "[CONFIG] Updated: buffer_ops=%s, display_ops=%s, analysis=%s, verbose=%s\n",
               buffer_ops ? "ON" : "OFF", display_ops ? "ON" : "OFF",
               analysis ? "ON" : "OFF", verbose ? "ON" : "OFF");
        fflush(g_trace_state.trace_log);
    }
}

void lle_trace_set_filter(size_t min_buffer_length, bool boundary_crossing_only, bool backspace_only) {
    g_trace_state.min_buffer_length = min_buffer_length;
    g_trace_state.boundary_crossing_only = boundary_crossing_only;
    g_trace_state.backspace_only = backspace_only;
    
    if (g_trace_state.trace_log) {
        fprintf(g_trace_state.trace_log, "[FILTER] Updated: min_length=%zu, boundary_only=%s, backspace_only=%s\n",
               min_buffer_length, boundary_crossing_only ? "ON" : "OFF", backspace_only ? "ON" : "OFF");
        fflush(g_trace_state.trace_log);
    }
}

void lle_trace_clear_snapshots(void) {
    g_trace_state.snapshot_count = 0;
    g_trace_state.sequence_counter = 1;
    memset(&g_trace_state.stats, 0, sizeof(g_trace_state.stats));
    g_trace_state.stats.session_start_time = lle_trace_get_timestamp_us();
    g_trace_state.stats.session_active = true;
    
    if (g_trace_state.trace_log) {
        fprintf(g_trace_state.trace_log, "[CLEAR] Snapshots and statistics cleared\n");
        fflush(g_trace_state.trace_log);
    }
}

void lle_trace_flush(void) {
    if (g_trace_state.trace_log) {
        fflush(g_trace_state.trace_log);
    }
    if (g_trace_state.summary_log) {
        fflush(g_trace_state.summary_log);
    }
}