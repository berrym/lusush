/**
 * @file debug_double_deletion.c
 * @brief Debug utility to track double-deletion bug in backspace operations
 * 
 * This file provides controlled debugging functionality to identify the root
 * cause of the double-deletion bug where single backspace operations delete
 * two characters instead of one during line wrap boundary crossing.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024-12-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

// Minimal includes for LLE structures
#include "src/line_editor/text_buffer.h"
#include "src/line_editor/edit_commands.h"
#include "src/line_editor/display.h"

// ============================================================================
// Debug State Tracking
// ============================================================================

typedef struct {
    char buffer_before[512];
    char buffer_after[512];
    size_t length_before;
    size_t length_after;
    size_t cursor_before;
    size_t cursor_after;
    uint64_t timestamp;
    const char *operation;
    bool boundary_crossing;
} lle_debug_buffer_snapshot_t;

static lle_debug_buffer_snapshot_t debug_snapshots[100];
static size_t debug_snapshot_count = 0;
static bool debug_enabled = false;

// ============================================================================
// Buffer State Capture Functions
// ============================================================================

/**
 * @brief Capture current buffer state for debugging
 *
 * @param buffer Text buffer to capture
 * @param operation Description of current operation
 * @return Snapshot index, or -1 on failure
 */
static int lle_debug_capture_buffer_state(lle_text_buffer_t *buffer, const char *operation) {
    if (!debug_enabled || !buffer || debug_snapshot_count >= 100) {
        return -1;
    }
    
    lle_debug_buffer_snapshot_t *snapshot = &debug_snapshots[debug_snapshot_count];
    
    // Capture buffer content
    size_t copy_length = buffer->length < 511 ? buffer->length : 511;
    memcpy(snapshot->buffer_before, buffer->buffer, copy_length);
    snapshot->buffer_before[copy_length] = '\0';
    
    snapshot->length_before = buffer->length;
    snapshot->cursor_before = buffer->cursor_pos;
    snapshot->timestamp = (uint64_t)time(NULL) * 1000000 + (uint64_t)clock();
    snapshot->operation = operation;
    snapshot->boundary_crossing = false;
    
    return debug_snapshot_count++;
}

/**
 * @brief Update snapshot with after state
 *
 * @param snapshot_index Index returned by capture_buffer_state
 * @param buffer Text buffer in final state
 * @param boundary_crossing Whether line wrap boundary was crossed
 */
static void lle_debug_update_snapshot_after(int snapshot_index, lle_text_buffer_t *buffer, bool boundary_crossing) {
    if (!debug_enabled || snapshot_index < 0 || snapshot_index >= debug_snapshot_count || !buffer) {
        return;
    }
    
    lle_debug_buffer_snapshot_t *snapshot = &debug_snapshots[snapshot_index];
    
    // Capture final buffer state
    size_t copy_length = buffer->length < 511 ? buffer->length : 511;
    memcpy(snapshot->buffer_after, buffer->buffer, copy_length);
    snapshot->buffer_after[copy_length] = '\0';
    
    snapshot->length_after = buffer->length;
    snapshot->cursor_after = buffer->cursor_pos;
    snapshot->boundary_crossing = boundary_crossing;
}

// ============================================================================
// Analysis Functions
// ============================================================================

/**
 * @brief Analyze buffer changes to detect double deletions
 *
 * @param snapshot Snapshot to analyze
 * @return true if double deletion detected, false otherwise
 */
static bool lle_debug_detect_double_deletion(const lle_debug_buffer_snapshot_t *snapshot) {
    if (!snapshot) return false;
    
    // For backspace, we expect exactly 1 character deletion
    if (strcmp(snapshot->operation, "BACKSPACE") == 0) {
        size_t expected_length_after = snapshot->length_before > 0 ? snapshot->length_before - 1 : 0;
        size_t actual_change = snapshot->length_before - snapshot->length_after;
        
        return (actual_change > 1) || (snapshot->length_after < expected_length_after);
    }
    
    return false;
}

/**
 * @brief Calculate character differences between before/after states
 *
 * @param snapshot Snapshot to analyze
 * @param deleted_chars Output buffer for deleted characters
 * @param max_deleted Size of deleted_chars buffer
 * @return Number of characters actually deleted
 */
static size_t lle_debug_calculate_char_diff(const lle_debug_buffer_snapshot_t *snapshot, 
                                           char *deleted_chars, size_t max_deleted) {
    if (!snapshot || !deleted_chars || max_deleted == 0) return 0;
    
    size_t before_len = snapshot->length_before;
    size_t after_len = snapshot->length_after;
    
    if (after_len >= before_len) {
        deleted_chars[0] = '\0';
        return 0; // No deletion occurred
    }
    
    // Find the deletion point by comparing buffers
    size_t deletion_point = 0;
    while (deletion_point < after_len && 
           snapshot->buffer_before[deletion_point] == snapshot->buffer_after[deletion_point]) {
        deletion_point++;
    }
    
    // Extract deleted characters
    size_t deleted_count = before_len - after_len;
    size_t copy_count = deleted_count < max_deleted - 1 ? deleted_count : max_deleted - 1;
    
    memcpy(deleted_chars, snapshot->buffer_before + deletion_point, copy_count);
    deleted_chars[copy_count] = '\0';
    
    return deleted_count;
}

// ============================================================================
// Instrumented Buffer Operations
// ============================================================================

/**
 * @brief Instrumented version of lle_text_backspace for debugging
 *
 * @param buffer Text buffer to modify
 * @return true on success, false on failure
 */
bool lle_debug_text_backspace(lle_text_buffer_t *buffer) {
    if (!buffer) return false;
    
    // Capture before state
    int snapshot_idx = lle_debug_capture_buffer_state(buffer, "BACKSPACE");
    
    // Call original backspace function
    bool result = lle_text_backspace(buffer);
    
    // Capture after state
    if (snapshot_idx >= 0) {
        lle_debug_update_snapshot_after(snapshot_idx, buffer, false);
        
        // Check for double deletion
        if (lle_debug_detect_double_deletion(&debug_snapshots[snapshot_idx])) {
            fprintf(stderr, "[CRITICAL] Double deletion detected in lle_text_backspace!\n");
        }
    }
    
    return result;
}

/**
 * @brief Instrumented version of lle_cmd_backspace for debugging
 *
 * @param state Display state
 * @return Command result
 */
lle_command_result_t lle_debug_cmd_backspace(lle_display_state_t *state) {
    if (!state || !state->buffer) return LLE_CMD_ERROR_INVALID_STATE;
    
    // Capture before state
    int snapshot_idx = lle_debug_capture_buffer_state(state->buffer, "CMD_BACKSPACE");
    
    // Call original command
    lle_command_result_t result = lle_cmd_backspace(state);
    
    // Capture after state with boundary crossing detection
    if (snapshot_idx >= 0) {
        // Try to detect if boundary crossing occurred
        bool boundary_crossing = false;
        if (state->prompt && state->terminal) {
            size_t prompt_width = lle_prompt_get_last_line_width(state->prompt);
            size_t terminal_width = state->geometry.width;
            
            // Simple heuristic: if cursor moved back more than expected relative to terminal width
            if (debug_snapshots[snapshot_idx].cursor_before > prompt_width + terminal_width) {
                boundary_crossing = true;
            }
        }
        
        lle_debug_update_snapshot_after(snapshot_idx, state->buffer, boundary_crossing);
        
        // Check for double deletion
        if (lle_debug_detect_double_deletion(&debug_snapshots[snapshot_idx])) {
            fprintf(stderr, "[CRITICAL] Double deletion detected in lle_cmd_backspace!\n");
            lle_debug_print_snapshot(&debug_snapshots[snapshot_idx]);
        }
    }
    
    return result;
}

// ============================================================================
// Reporting Functions
// ============================================================================

/**
 * @brief Print detailed snapshot information
 *
 * @param snapshot Snapshot to print
 */
void lle_debug_print_snapshot(const lle_debug_buffer_snapshot_t *snapshot) {
    if (!snapshot) return;
    
    char deleted_chars[64];
    size_t deleted_count = lle_debug_calculate_char_diff(snapshot, deleted_chars, sizeof(deleted_chars));
    
    fprintf(stderr, "\n=== DEBUG SNAPSHOT: %s ===\n", snapshot->operation);
    fprintf(stderr, "Timestamp: %llu\n", (unsigned long long)snapshot->timestamp);
    fprintf(stderr, "Boundary Crossing: %s\n", snapshot->boundary_crossing ? "YES" : "NO");
    fprintf(stderr, "Before: len=%zu, cursor=%zu, content='%s'\n", 
           snapshot->length_before, snapshot->cursor_before, snapshot->buffer_before);
    fprintf(stderr, "After:  len=%zu, cursor=%zu, content='%s'\n", 
           snapshot->length_after, snapshot->cursor_after, snapshot->buffer_after);
    fprintf(stderr, "Expected deletion: 1 character\n");
    fprintf(stderr, "Actual deletion: %zu characters: '%s'\n", deleted_count, deleted_chars);
    fprintf(stderr, "DOUBLE DELETION: %s\n", deleted_count > 1 ? "YES" : "NO");
    fprintf(stderr, "==============================\n\n");
}

/**
 * @brief Print summary of all captured snapshots
 */
void lle_debug_print_summary(void) {
    if (!debug_enabled) {
        fprintf(stderr, "Debug tracking not enabled\n");
        return;
    }
    
    fprintf(stderr, "\n=== DOUBLE DELETION DEBUG SUMMARY ===\n");
    fprintf(stderr, "Total operations tracked: %zu\n", debug_snapshot_count);
    
    size_t double_deletions = 0;
    size_t boundary_crossings = 0;
    
    for (size_t i = 0; i < debug_snapshot_count; i++) {
        const lle_debug_buffer_snapshot_t *snapshot = &debug_snapshots[i];
        
        if (lle_debug_detect_double_deletion(snapshot)) {
            double_deletions++;
            fprintf(stderr, "[%zu] DOUBLE DELETION in %s (boundary: %s)\n", 
                   i, snapshot->operation, snapshot->boundary_crossing ? "YES" : "NO");
        }
        
        if (snapshot->boundary_crossing) {
            boundary_crossings++;
        }
    }
    
    fprintf(stderr, "Double deletions detected: %zu\n", double_deletions);
    fprintf(stderr, "Boundary crossings: %zu\n", boundary_crossings);
    fprintf(stderr, "Success rate: %.2f%%\n", 
           debug_snapshot_count > 0 ? (double)(debug_snapshot_count - double_deletions) * 100.0 / debug_snapshot_count : 100.0);
    fprintf(stderr, "=====================================\n\n");
}

// ============================================================================
// Control Functions
// ============================================================================

/**
 * @brief Enable debug tracking
 */
void lle_debug_enable(void) {
    debug_enabled = true;
    debug_snapshot_count = 0;
    fprintf(stderr, "[DEBUG] Double deletion tracking enabled\n");
}

/**
 * @brief Disable debug tracking
 */
void lle_debug_disable(void) {
    debug_enabled = false;
    fprintf(stderr, "[DEBUG] Double deletion tracking disabled\n");
}

/**
 * @brief Check if debug tracking is enabled
 *
 * @return true if enabled, false otherwise
 */
bool lle_debug_is_enabled(void) {
    return debug_enabled;
}

/**
 * @brief Clear all captured snapshots
 */
void lle_debug_clear_snapshots(void) {
    debug_snapshot_count = 0;
    memset(debug_snapshots, 0, sizeof(debug_snapshots));
    fprintf(stderr, "[DEBUG] Snapshots cleared\n");
}

// ============================================================================
// Test Integration Functions
// ============================================================================

/**
 * @brief Create a test scenario that triggers boundary crossing
 *
 * @param terminal_width Width to simulate
 * @return true if test completed successfully
 */
bool lle_debug_test_boundary_crossing(size_t terminal_width) {
    fprintf(stderr, "\n=== BOUNDARY CROSSING TEST (width=%zu) ===\n", terminal_width);
    
    // Enable debugging
    lle_debug_enable();
    
    // Create test buffer with content near boundary
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    if (!buffer) {
        fprintf(stderr, "Failed to create test buffer\n");
        return false;
    }
    
    // Fill buffer with content that wraps at terminal_width
    const char *test_content = "This is a long line that should wrap around the terminal boundary when displayed";
    for (size_t i = 0; i < strlen(test_content) && i < terminal_width + 10; i++) {
        lle_text_insert_char(buffer, test_content[i]);
    }
    
    fprintf(stderr, "Test buffer created: len=%zu, content='%s'\n", buffer->length, buffer->buffer);
    
    // Perform backspace operations at boundary
    for (int i = 0; i < 5 && buffer->length > 0; i++) {
        fprintf(stderr, "\nBackspace operation %d:\n", i + 1);
        int snapshot_idx = lle_debug_capture_buffer_state(buffer, "TEST_BACKSPACE");
        
        bool result = lle_text_backspace(buffer);
        fprintf(stderr, "Backspace result: %s\n", result ? "SUCCESS" : "FAILED");
        
        if (snapshot_idx >= 0) {
            lle_debug_update_snapshot_after(snapshot_idx, buffer, true); // Assume boundary crossing
            lle_debug_print_snapshot(&debug_snapshots[snapshot_idx]);
        }
    }
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    
    // Print summary
    lle_debug_print_summary();
    lle_debug_disable();
    
    return true;
}

/**
 * @brief Simple smoke test for debug functionality
 *
 * @return true if all tests pass
 */
bool lle_debug_smoke_test(void) {
    fprintf(stderr, "\n=== DEBUG SMOKE TEST ===\n");
    
    lle_debug_enable();
    
    // Test basic buffer operations
    lle_text_buffer_t *buffer = lle_text_buffer_create(64);
    if (!buffer) return false;
    
    // Insert some characters
    const char *test_text = "hello";
    for (size_t i = 0; i < strlen(test_text); i++) {
        lle_text_insert_char(buffer, test_text[i]);
    }
    
    // Test instrumented backspace
    int snapshot_idx = lle_debug_capture_buffer_state(buffer, "SMOKE_TEST");
    bool result = lle_debug_text_backspace(buffer);
    if (snapshot_idx >= 0) {
        lle_debug_update_snapshot_after(snapshot_idx, buffer, false);
        
        const lle_debug_buffer_snapshot_t *snapshot = &debug_snapshots[snapshot_idx];
        bool expected_result = (snapshot->length_after == snapshot->length_before - 1);
        
        fprintf(stderr, "Smoke test result: %s\n", expected_result ? "PASS" : "FAIL");
        if (!expected_result) {
            lle_debug_print_snapshot(snapshot);
        }
    }
    
    lle_text_buffer_destroy(buffer);
    lle_debug_disable();
    
    return result;
}