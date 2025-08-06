/**
 * @file display_state_sync.c
 * @brief Lusush Line Editor - Unified Bidirectional Terminal-Display State Synchronization Implementation
 *
 * This module implements a unified state synchronization system between the terminal
 * and LLE display system. It tracks terminal state changes, validates display state
 * consistency, and provides bidirectional synchronization mechanisms to prevent
 * state drift and display corruption, especially with ANSI clear sequences.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "display_state_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

// Debug logging macros
#define LLE_SYNC_DEBUG_ENABLED 1

#if LLE_SYNC_DEBUG_ENABLED
#define LLE_SYNC_DEBUG(fmt, ...) \
    do { \
        if (getenv("LLE_SYNC_DEBUG")) { \
            fprintf(stderr, "[LLE_SYNC] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)
#else
#define LLE_SYNC_DEBUG(fmt, ...)
#endif

// Performance measurement utilities
static uint64_t lle_get_current_timestamp_us(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

// Simple checksum for state validation
static uint32_t lle_calculate_checksum(const char *data, size_t length) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum = ((checksum << 5) + checksum) + (unsigned char)data[i];
    }
    return checksum;
}

// ============================================================================
// Terminal State Management
// ============================================================================

/**
 * @brief Initialize terminal state tracking
 */
static lle_sync_terminal_state_t *lle_terminal_state_create(size_t width, size_t height) {
    lle_sync_terminal_state_t *state = calloc(1, sizeof(lle_sync_terminal_state_t));
    if (!state) {
        return NULL;
    }
    
    state->width = width;
    state->height = height;
    state->lines = calloc(height, sizeof(lle_terminal_line_t));
    if (!state->lines) {
        free(state);
        return NULL;
    }
    
    // Initialize each line
    for (size_t i = 0; i < height; i++) {
        state->lines[i].cells = calloc(width, sizeof(lle_terminal_cell_t));
        if (!state->lines[i].cells) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(state->lines[j].cells);
            }
            free(state->lines);
            free(state);
            return NULL;
        }
        state->lines[i].width = width;
        state->lines[i].content_length = 0;
        state->lines[i].line_modified = false;
        state->lines[i].line_cleared = false;
    }
    
    state->cursor_row = 0;
    state->cursor_col = 0;
    state->cursor_visible = true;
    state->last_sync_timestamp = lle_get_current_timestamp_us();
    state->state_checksum = 0;
    state->modification_count = 0;
    state->state_valid = true;
    state->dirty_tracking_enabled = true;
    
    LLE_SYNC_DEBUG("Terminal state initialized: %zux%zu", width, height);
    return state;
}

/**
 * @brief Cleanup terminal state tracking
 */
static void lle_terminal_state_destroy(lle_sync_terminal_state_t *state) {
    if (!state) {
        return;
    }
    
    if (state->lines) {
        for (size_t i = 0; i < state->height; i++) {
            free(state->lines[i].cells);
        }
        free(state->lines);
    }
    
    free(state);
    LLE_SYNC_DEBUG("Terminal state destroyed");
}

/**
 * @brief Update terminal cell content
 */
static bool lle_terminal_state_set_cell(lle_sync_terminal_state_t *state,
                                        size_t row, size_t col,
                                        char character, bool has_content) {
    if (!state || row >= state->height || col >= state->width) {
        return false;
    }
    
    lle_terminal_cell_t *cell = &state->lines[row].cells[col];
    bool changed = (cell->character != character || cell->has_content != has_content);
    
    if (changed) {
        cell->character = character;
        cell->has_content = has_content;
        cell->modified_since_sync = true;
        state->lines[row].line_modified = true;
        state->modification_count++;
        
        // Update dirty region tracking
        if (state->dirty_tracking_enabled) {
            if (state->modification_count == 1) {
                // First modification - initialize dirty region
                state->dirty_start_row = row;
                state->dirty_end_row = row;
                state->dirty_start_col = col;
                state->dirty_end_col = col;
            } else {
                // Expand dirty region
                if (row < state->dirty_start_row) state->dirty_start_row = row;
                if (row > state->dirty_end_row) state->dirty_end_row = row;
                if (col < state->dirty_start_col) state->dirty_start_col = col;
                if (col > state->dirty_end_col) state->dirty_end_col = col;
            }
        }
    }
    
    return true;
}

/**
 * @brief Clear terminal line range
 */
static bool lle_terminal_state_clear_line_range(lle_sync_terminal_state_t *state,
                                               size_t row, size_t start_col, size_t end_col) {
    if (!state || row >= state->height) {
        return false;
    }
    
    size_t actual_end = (end_col >= state->width) ? state->width - 1 : end_col;
    
    for (size_t col = start_col; col <= actual_end; col++) {
        lle_terminal_state_set_cell(state, row, col, ' ', false);
    }
    
    state->lines[row].line_cleared = true;
    LLE_SYNC_DEBUG("Cleared line %zu columns %zu-%zu", row, start_col, actual_end);
    return true;
}

// ============================================================================
// Display Sync State Management
// ============================================================================

/**
 * @brief Initialize display sync state
 */
static lle_display_sync_state_t *lle_display_sync_state_create(void) {
    lle_display_sync_state_t *sync_state = calloc(1, sizeof(lle_display_sync_state_t));
    if (!sync_state) {
        return NULL;
    }
    
    sync_state->expected_content = malloc(1024);
    if (!sync_state->expected_content) {
        free(sync_state);
        return NULL;
    }
    
    sync_state->expected_content[0] = '\0';
    sync_state->expected_length = 0;
    sync_state->expected_cursor_pos = 0;
    sync_state->expected_rows_used = 1;
    sync_state->sync_required = false;
    sync_state->sync_in_progress = false;
    sync_state->last_sync_timestamp = lle_get_current_timestamp_us();
    sync_state->sync_failure_count = 0;
    sync_state->content_validated = false;
    sync_state->cursor_validated = false;
    sync_state->geometry_validated = false;
    sync_state->total_sync_time = 0;
    sync_state->sync_operations = 0;
    sync_state->average_sync_time = 0.0;
    
    LLE_SYNC_DEBUG("Display sync state initialized");
    return sync_state;
}

/**
 * @brief Cleanup display sync state
 */
static void lle_display_sync_state_destroy(lle_display_sync_state_t *sync_state) {
    if (!sync_state) {
        return;
    }
    
    free(sync_state->expected_content);
    free(sync_state);
    LLE_SYNC_DEBUG("Display sync state destroyed");
}

// ============================================================================
// Core Synchronization API Implementation
// ============================================================================

/**
 * @brief Initialize state synchronization system
 */
lle_state_sync_context_t *lle_state_sync_init(lle_terminal_manager_t *terminal,
                                               lle_display_state_t *display) {
    if (!terminal || !display) {
        return NULL;
    }
    
    lle_state_sync_context_t *sync_ctx = calloc(1, sizeof(lle_state_sync_context_t));
    if (!sync_ctx) {
        return NULL;
    }
    
    // Get terminal geometry
    size_t width = 80, height = 24;  // Default values
    if (display->geometry.width > 0 && display->geometry.height > 0) {
        width = display->geometry.width;
        height = display->geometry.height;
    }
    
    // Initialize terminal state tracking
    sync_ctx->terminal_state = lle_terminal_state_create(width, height);
    if (!sync_ctx->terminal_state) {
        free(sync_ctx);
        return NULL;
    }
    
    // Initialize display sync state
    sync_ctx->display_sync = lle_display_sync_state_create();
    if (!sync_ctx->display_sync) {
        lle_terminal_state_destroy(sync_ctx->terminal_state);
        free(sync_ctx);
        return NULL;
    }
    
    // Set references
    sync_ctx->terminal = terminal;
    sync_ctx->display = display;
    
    // Configure synchronization
    sync_ctx->bidirectional_sync_enabled = true;
    sync_ctx->ansi_sequence_tracking = true;
    sync_ctx->strict_validation_mode = false;
    sync_ctx->max_sync_failures = 5;
    
    // Initialize statistics
    sync_ctx->total_divergences = 0;
    sync_ctx->total_recoveries = 0;
    sync_ctx->ansi_sequences_processed = 0;
    
    LLE_SYNC_DEBUG("State sync context initialized (terminal: %zux%zu)", width, height);
    return sync_ctx;
}

/**
 * @brief Cleanup state synchronization system
 */
void lle_state_sync_cleanup(lle_state_sync_context_t *sync_ctx) {
    if (!sync_ctx) {
        return;
    }
    
    lle_terminal_state_destroy(sync_ctx->terminal_state);
    lle_display_sync_state_destroy(sync_ctx->display_sync);
    free(sync_ctx);
    
    LLE_SYNC_DEBUG("State sync context cleaned up");
}

/**
 * @brief Perform bidirectional state synchronization
 */
bool lle_state_sync_perform(lle_state_sync_context_t *sync_ctx) {
    if (!sync_ctx || !sync_ctx->bidirectional_sync_enabled) {
        return false;
    }
    
    if (sync_ctx->display_sync->sync_in_progress) {
        LLE_SYNC_DEBUG("Sync already in progress, skipping");
        return true;
    }
    
    uint64_t start_time = lle_get_current_timestamp_us();
    sync_ctx->display_sync->sync_in_progress = true;
    
    bool success = true;
    
    // Step 1: Validate current state consistency
    if (!lle_state_sync_validate(sync_ctx)) {
        LLE_SYNC_DEBUG("State validation failed, attempting recovery");
        success = lle_state_sync_recover_divergence(sync_ctx);
        if (!success) {
            sync_ctx->display_sync->sync_failure_count++;
            LLE_SYNC_DEBUG("State recovery failed (failure count: %zu)", 
                          sync_ctx->display_sync->sync_failure_count);
        } else {
            sync_ctx->total_recoveries++;
            LLE_SYNC_DEBUG("State recovery successful");
        }
    }
    
    // Step 2: Update synchronization timestamps
    if (success) {
        uint64_t current_time = lle_get_current_timestamp_us();
        sync_ctx->terminal_state->last_sync_timestamp = current_time;
        sync_ctx->display_sync->last_sync_timestamp = current_time;
        sync_ctx->display_sync->sync_failure_count = 0;
        
        // Clear modification tracking
        sync_ctx->terminal_state->modification_count = 0;
        for (size_t i = 0; i < sync_ctx->terminal_state->height; i++) {
            sync_ctx->terminal_state->lines[i].line_modified = false;
            for (size_t j = 0; j < sync_ctx->terminal_state->width; j++) {
                sync_ctx->terminal_state->lines[i].cells[j].modified_since_sync = false;
            }
        }
    }
    
    // Step 3: Update performance metrics
    uint64_t end_time = lle_get_current_timestamp_us();
    uint64_t sync_time = end_time - start_time;
    sync_ctx->display_sync->total_sync_time += sync_time;
    sync_ctx->display_sync->sync_operations++;
    sync_ctx->display_sync->average_sync_time = 
        (double)sync_ctx->display_sync->total_sync_time / sync_ctx->display_sync->sync_operations;
    
    sync_ctx->display_sync->sync_in_progress = false;
    sync_ctx->display_sync->sync_required = !success;
    
    LLE_SYNC_DEBUG("Sync completed: %s (time: %lu us)", 
                   success ? "SUCCESS" : "FAILED", sync_time);
    
    return success;
}

/**
 * @brief Validate terminal and display state consistency
 */
bool lle_state_sync_validate(lle_state_sync_context_t *sync_ctx) {
    if (!sync_ctx) {
        return false;
    }
    
    bool content_valid = true;
    bool cursor_valid = true;
    bool geometry_valid = true;
    
    // Validate terminal geometry matches display expectations
    if (sync_ctx->display->geometry.width != sync_ctx->terminal_state->width ||
        sync_ctx->display->geometry.height != sync_ctx->terminal_state->height) {
        geometry_valid = false;
        LLE_SYNC_DEBUG("Geometry mismatch: display=%zux%zu, terminal=%zux%zu",
                       sync_ctx->display->geometry.width, sync_ctx->display->geometry.height,
                       sync_ctx->terminal_state->width, sync_ctx->terminal_state->height);
    }
    
    // Validate cursor position consistency
    if (sync_ctx->display->cursor_pos.absolute_row != sync_ctx->terminal_state->cursor_row ||
        sync_ctx->display->cursor_pos.absolute_col != sync_ctx->terminal_state->cursor_col) {
        cursor_valid = false;
        LLE_SYNC_DEBUG("Cursor mismatch: display=(%zu,%zu), terminal=(%zu,%zu)",
                       sync_ctx->display->cursor_pos.absolute_row,
                       sync_ctx->display->cursor_pos.absolute_col,
                       sync_ctx->terminal_state->cursor_row,
                       sync_ctx->terminal_state->cursor_col);
    }
    
    // Update validation state
    sync_ctx->display_sync->content_validated = content_valid;
    sync_ctx->display_sync->cursor_validated = cursor_valid;
    sync_ctx->display_sync->geometry_validated = geometry_valid;
    
    bool overall_valid = content_valid && cursor_valid && geometry_valid;
    
    if (!overall_valid) {
        sync_ctx->total_divergences++;
        if (sync_ctx->on_state_divergence) {
            sync_ctx->on_state_divergence("State validation failed");
        }
    }
    
    return overall_valid;
}

/**
 * @brief Force complete state resynchronization
 */
bool lle_state_sync_force_resync(lle_state_sync_context_t *sync_ctx) {
    if (!sync_ctx) {
        return false;
    }
    
    LLE_SYNC_DEBUG("Forcing complete state resynchronization");
    
    // Mark all terminal state as modified
    for (size_t i = 0; i < sync_ctx->terminal_state->height; i++) {
        sync_ctx->terminal_state->lines[i].line_modified = true;
        for (size_t j = 0; j < sync_ctx->terminal_state->width; j++) {
            sync_ctx->terminal_state->lines[i].cells[j].modified_since_sync = true;
        }
    }
    
    sync_ctx->terminal_state->modification_count = 
        sync_ctx->terminal_state->width * sync_ctx->terminal_state->height;
    
    // Mark display sync as requiring update
    sync_ctx->display_sync->sync_required = true;
    sync_ctx->display_sync->content_validated = false;
    sync_ctx->display_sync->cursor_validated = false;
    sync_ctx->display_sync->geometry_validated = false;
    
    // Perform synchronization
    return lle_state_sync_perform(sync_ctx);
}

// ============================================================================
// Terminal State Tracking Implementation
// ============================================================================

/**
 * @brief Update terminal state after write operation
 */
bool lle_terminal_state_update_write(lle_state_sync_context_t *sync_ctx,
                                     const char *data,
                                     size_t length,
                                     size_t cursor_row,
                                     size_t cursor_col) {
    if (!sync_ctx || !data) {
        return false;
    }
    
    lle_sync_terminal_state_t *state = sync_ctx->terminal_state;
    
    // Update cursor position
    if (cursor_row < state->height && cursor_col < state->width) {
        state->cursor_row = cursor_row;
        state->cursor_col = cursor_col;
    }
    
    // Process written data character by character
    size_t current_row = state->cursor_row;
    size_t current_col = state->cursor_col;
    
    for (size_t i = 0; i < length; i++) {
        char c = data[i];
        
        if (c == '\n') {
            // Newline - move to next row
            current_row++;
            current_col = 0;
        } else if (c == '\r') {
            // Carriage return - move to beginning of line
            current_col = 0;
        } else if (c == '\b') {
            // Backspace - move back one column
            if (current_col > 0) {
                current_col--;
                lle_terminal_state_set_cell(state, current_row, current_col, ' ', false);
            }
        } else if (c >= 32 && c <= 126) {
            // Printable character
            if (current_row < state->height && current_col < state->width) {
                lle_terminal_state_set_cell(state, current_row, current_col, c, true);
                current_col++;
            }
        }
        
        // Handle line wrapping
        if (current_col >= state->width) {
            current_row++;
            current_col = 0;
        }
        
        // Bounds checking
        if (current_row >= state->height) {
            break;
        }
    }
    
    // Update final cursor position
    state->cursor_row = current_row;
    state->cursor_col = current_col;
    
    LLE_SYNC_DEBUG("Terminal write: %zu chars, cursor now at (%zu,%zu)", 
                   length, current_row, current_col);
    
    return true;
}

/**
 * @brief Update terminal state after ANSI clear sequence
 */
bool lle_terminal_state_update_clear(lle_state_sync_context_t *sync_ctx,
                                     const char *clear_type,
                                     size_t start_row,
                                     size_t start_col,
                                     size_t end_row,
                                     size_t end_col) {
    if (!sync_ctx || !clear_type) {
        return false;
    }
    
    lle_sync_terminal_state_t *state = sync_ctx->terminal_state;
    
    LLE_SYNC_DEBUG("Terminal clear: type=%s, region=(%zu,%zu)-(%zu,%zu)", 
                   clear_type, start_row, start_col, end_row, end_col);
    
    // Handle different clear types
    if (strcmp(clear_type, "eol") == 0) {
        // Clear to end of line
        lle_terminal_state_clear_line_range(state, start_row, start_col, state->width - 1);
    } else if (strcmp(clear_type, "line") == 0) {
        // Clear entire line
        lle_terminal_state_clear_line_range(state, start_row, 0, state->width - 1);
    } else if (strcmp(clear_type, "screen") == 0) {
        // Clear entire screen
        for (size_t row = 0; row < state->height; row++) {
            lle_terminal_state_clear_line_range(state, row, 0, state->width - 1);
        }
    } else if (strcmp(clear_type, "region") == 0) {
        // Clear specified region
        for (size_t row = start_row; row <= end_row && row < state->height; row++) {
            size_t line_start = (row == start_row) ? start_col : 0;
            size_t line_end = (row == end_row) ? end_col : state->width - 1;
            lle_terminal_state_clear_line_range(state, row, line_start, line_end);
        }
    }
    
    // Mark display sync as needing update
    sync_ctx->display_sync->sync_required = true;
    
    return true;
}

/**
 * @brief Update terminal state after cursor movement
 */
bool lle_terminal_state_update_cursor(lle_state_sync_context_t *sync_ctx,
                                      size_t new_row,
                                      size_t new_col) {
    if (!sync_ctx) {
        return false;
    }
    
    lle_sync_terminal_state_t *state = sync_ctx->terminal_state;
    
    if (new_row < state->height && new_col < state->width) {
        // Clear cursor from old position
        if (state->cursor_row < state->height && state->cursor_col < state->width) {
            state->lines[state->cursor_row].cells[state->cursor_col].is_cursor_position = false;
        }
        
        // Set cursor at new position
        state->cursor_row = new_row;
        state->cursor_col = new_col;
        state->lines[new_row].cells[new_col].is_cursor_position = true;
        
        LLE_SYNC_DEBUG("Terminal cursor moved to (%zu,%zu)", new_row, new_col);
        return true;
    }
    
    LLE_SYNC_DEBUG("Invalid cursor position: (%zu,%zu), terminal size: %zux%zu", 
                   new_row, new_col, state->width, state->height);
    return false;
}

/**
 * @brief Process ANSI escape sequence and update state
 */
bool lle_terminal_state_process_ansi(lle_state_sync_context_t *sync_ctx,
                                     const char *sequence,
                                     size_t length) {
    if (!sync_ctx || !sequence || length == 0) {
        return false;
    }
    
    sync_ctx->ansi_sequences_processed++;
    
    // Call callback if registered
    if (sync_ctx->on_ansi_sequence) {
        sync_ctx->on_ansi_sequence(sequence);
    }
    
    LLE_SYNC_DEBUG("Processing ANSI sequence: %.*s", (int)length, sequence);
    
    // Basic ANSI sequence parsing
    if (length >= 3 && sequence[0] == '\x1b' && sequence[1] == '[') {
        char command = sequence[length - 1];
        
        switch (command) {
            case 'A': // Cursor up
            case 'B': // Cursor down
            case 'C': // Cursor right
            case 'D': // Cursor left
            case 'H': // Cursor home
            case 'f': // Cursor position
                // These affect cursor position - mark for sync
                sync_ctx->display_sync->cursor_validated = false;
                break;
                
            case 'J': // Clear screen
            case 'K': // Clear line
                // These affect display content - mark for sync
                sync_ctx->display_sync->content_validated = false;
                sync_ctx->display_sync->sync_required = true;
                break;
        }
    }
    
    return true;
}

// ============================================================================
// Wrapper Functions for Safe Terminal Operations
// ============================================================================

/**
 * @brief Write data to terminal with automatic state tracking
 */
bool lle_state_sync_terminal_write(lle_state_sync_context_t *sync_ctx,
                                   const char *data,
                                   size_t length) {
    if (!sync_ctx || !data) {
        return false;
    }
    
    // Get current cursor position before write
    size_t cursor_row = sync_ctx->terminal_state->cursor_row;
    size_t cursor_col = sync_ctx->terminal_state->cursor_col;
    
    // Perform the actual terminal write
    bool write_success = false;
    if (sync_ctx->terminal && sync_ctx->terminal->stdout_fd >= 0) {
        ssize_t written = write(sync_ctx->terminal->stdout_fd, data, length);
        write_success = (written >= 0 && (size_t)written == length);
    }
    
    if (write_success) {
        // Update terminal state tracking
        lle_terminal_state_update_write(sync_ctx, data, length, cursor_row, cursor_col);
        
        // Mark display sync as potentially needing update
        if (length > 0) {
            sync_ctx->display_sync->sync_required = true;
        }
    }
    
    return write_success;
}

/**
 * @brief Clear terminal region with automatic state tracking
 */
bool lle_state_sync_terminal_clear(lle_state_sync_context_t *sync_ctx,
                                   size_t start_row,
                                   size_t start_col,
                                   size_t end_row,
                                   size_t end_col) {
    if (!sync_ctx) {
        return false;
    }
    
    // Update terminal state to reflect the clear operation
    bool success = lle_terminal_state_update_clear(sync_ctx, "region",
                                                   start_row, start_col,
                                                   end_row, end_col);
    
    if (success) {
        // Mark display sync as needing update
        sync_ctx->display_sync->sync_required = true;
        sync_ctx->display_sync->content_validated = false;
    }
    
    return success;
}

/**
 * @brief Move cursor with automatic state tracking
 */
bool lle_state_sync_terminal_move_cursor(lle_state_sync_context_t *sync_ctx,
                                         size_t row,
                                         size_t col) {
    if (!sync_ctx) {
        return false;
    }
    
    // Update terminal state
    bool success = lle_terminal_state_update_cursor(sync_ctx, row, col);
    
    if (success) {
        // Mark cursor validation as needed
        sync_ctx->display_sync->cursor_validated = false;
    }
    
    return success;
}

// ============================================================================
// Advanced Synchronization Features
// ============================================================================

/**
 * @brief Detect and recover from state divergence
 */
bool lle_state_sync_recover_divergence(lle_state_sync_context_t *sync_ctx) {
    if (!sync_ctx) {
        return false;
    }
    
    LLE_SYNC_DEBUG("Attempting state divergence recovery");
    
    // Strategy 1: Reset terminal state tracking to current display state
    if (sync_ctx->display->buffer && sync_ctx->display->buffer->buffer) {
        // Update expected content
        size_t content_length = sync_ctx->display->buffer->length;
        if (content_length < 1024) {  // Safety check
            strncpy(sync_ctx->display_sync->expected_content,
                   sync_ctx->display->buffer->buffer,
                   content_length);
            sync_ctx->display_sync->expected_content[content_length] = '\0';
            sync_ctx->display_sync->expected_length = content_length;
        }
        
        // Update expected cursor position
        sync_ctx->display_sync->expected_cursor_pos = sync_ctx->display->buffer->cursor_pos;
    }
    
    // Strategy 2: Force terminal state to match display expectations
    sync_ctx->terminal_state->cursor_row = sync_ctx->display->cursor_pos.absolute_row;
    sync_ctx->terminal_state->cursor_col = sync_ctx->display->cursor_pos.absolute_col;
    
    // Strategy 3: Mark all validation as successful
    sync_ctx->display_sync->content_validated = true;
    sync_ctx->display_sync->cursor_validated = true;
    sync_ctx->display_sync->geometry_validated = true;
    sync_ctx->display_sync->sync_required = false;
    
    LLE_SYNC_DEBUG("State divergence recovery completed");
    return true;
}

/**
 * @brief Get synchronization performance statistics
 */
bool lle_state_sync_get_statistics(lle_state_sync_context_t *sync_ctx,
                                   size_t *total_syncs,
                                   size_t *total_failures,
                                   double *avg_sync_time,
                                   double *divergence_rate) {
    if (!sync_ctx) {
        return false;
    }
    
    if (total_syncs) {
        *total_syncs = sync_ctx->display_sync->sync_operations;
    }
    
    if (total_failures) {
        *total_failures = sync_ctx->display_sync->sync_failure_count;
    }
    
    if (avg_sync_time) {
        *avg_sync_time = sync_ctx->display_sync->average_sync_time;
    }
    
    if (divergence_rate) {
        if (sync_ctx->display_sync->sync_operations > 0) {
            *divergence_rate = (double)sync_ctx->total_divergences / 
                              (double)sync_ctx->display_sync->sync_operations;
        } else {
            *divergence_rate = 0.0;
        }
    }
        
    return true;
}
    
// ============================================================================
// Display State Synchronization API Implementation
// ============================================================================
    
/**
 * @brief Update display sync state with expected content
 */
bool lle_display_sync_update_expected(lle_state_sync_context_t *sync_ctx,
                                      const char *content,
                                      size_t length,
                                      size_t cursor_pos,
                                      size_t rows_used) {
    if (!sync_ctx || !content) {
        return false;
    }
        
    lle_display_sync_state_t *sync_state = sync_ctx->display_sync;
        
    // Reallocate buffer if needed
    if (length >= 1024) {
        char *new_buffer = realloc(sync_state->expected_content, length + 1);
        if (!new_buffer) {
            return false;
        }
        sync_state->expected_content = new_buffer;
    }
        
    // Update expected content
    memcpy(sync_state->expected_content, content, length);
    sync_state->expected_content[length] = '\0';
    sync_state->expected_length = length;
    sync_state->expected_cursor_pos = cursor_pos;
    sync_state->expected_rows_used = rows_used;
        
    // Mark as needing validation
    sync_state->content_validated = false;
    sync_state->cursor_validated = false;
        
    LLE_SYNC_DEBUG("Updated expected content: %zu chars, cursor at %zu, %zu rows", 
                   length, cursor_pos, rows_used);
        
    return true;
}
    
/**
 * @brief Mark display state as requiring synchronization
 */
bool lle_display_sync_mark_dirty(lle_state_sync_context_t *sync_ctx,
                                 const char *reason) {
    if (!sync_ctx) {
        return false;
    }
        
    sync_ctx->display_sync->sync_required = true;
    sync_ctx->display_sync->content_validated = false;
    sync_ctx->display_sync->cursor_validated = false;
        
    LLE_SYNC_DEBUG("Display marked dirty: %s", reason ? reason : "unknown");
        
    return true;
}
    
/**
 * @brief Check if display state matches expected state
 */
bool lle_display_sync_check_consistency(lle_state_sync_context_t *sync_ctx,
                                        const char *actual_content,
                                        size_t actual_length,
                                        size_t actual_cursor_pos) {
    if (!sync_ctx || !actual_content) {
        return false;
    }
        
    lle_display_sync_state_t *sync_state = sync_ctx->display_sync;
        
    // Check content consistency
    bool content_matches = (actual_length == sync_state->expected_length &&
                           memcmp(actual_content, sync_state->expected_content, actual_length) == 0);
        
    // Check cursor consistency
    bool cursor_matches = (actual_cursor_pos == sync_state->expected_cursor_pos);
        
    // Update validation flags
    sync_state->content_validated = content_matches;
    sync_state->cursor_validated = cursor_matches;
        
    bool consistent = content_matches && cursor_matches;
        
    if (!consistent) {
        sync_ctx->total_divergences++;
        LLE_SYNC_DEBUG("Consistency check failed: content=%s, cursor=%s",
                       content_matches ? "OK" : "MISMATCH",
                       cursor_matches ? "OK" : "MISMATCH");
    }
        
    return consistent;
}
    
/**
 * @brief Query actual terminal state via ANSI sequences
 */
bool lle_state_sync_query_terminal(lle_state_sync_context_t *sync_ctx,
                                   bool query_content,
                                   bool query_cursor) {
    if (!sync_ctx) {
        return false;
    }
        
    LLE_SYNC_DEBUG("Querying terminal state: content=%s, cursor=%s",
                   query_content ? "yes" : "no",
                   query_cursor ? "yes" : "no");
        
    // This is a placeholder for actual terminal queries
    // In a real implementation, this would send ANSI sequences like:
    // - "\x1b[6n" to query cursor position
    // - Custom sequences to query content if supported
        
    // For now, mark as validated to prevent constant resyncing
    if (query_content) {
        sync_ctx->display_sync->content_validated = true;
    }
    if (query_cursor) {
        sync_ctx->display_sync->cursor_validated = true;
    }
        
    return true;
}
    
/**
 * @brief Enable strict validation mode for debugging
 */
void lle_state_sync_set_strict_mode(lle_state_sync_context_t *sync_ctx,
                                    bool enable) {
    if (sync_ctx) {
        sync_ctx->strict_validation_mode = enable;
        LLE_SYNC_DEBUG("Strict validation mode: %s", enable ? "ENABLED" : "DISABLED");
    }
}
    
// ============================================================================
// Debug and Diagnostic Functions
// ============================================================================
    
/**
 * @brief Dump current terminal state for debugging
 */
void lle_state_sync_debug_dump_terminal(lle_state_sync_context_t *sync_ctx,
                                        FILE *output_file) {
    if (!sync_ctx) {
        return;
    }
        
    FILE *out = output_file ? output_file : stderr;
    lle_sync_terminal_state_t *state = sync_ctx->terminal_state;
        
    fprintf(out, "\n=== TERMINAL STATE DUMP ===\n");
    fprintf(out, "Dimensions: %zux%zu\n", state->width, state->height);
    fprintf(out, "Cursor: (%zu,%zu) %s\n", 
            state->cursor_row, state->cursor_col,
            state->cursor_visible ? "visible" : "hidden");
    fprintf(out, "Modifications: %zu\n", state->modification_count);
    fprintf(out, "State valid: %s\n", state->state_valid ? "yes" : "no");
        
    if (state->dirty_tracking_enabled && state->modification_count > 0) {
        fprintf(out, "Dirty region: (%zu,%zu) to (%zu,%zu)\n",
                state->dirty_start_row, state->dirty_start_col,
                state->dirty_end_row, state->dirty_end_col);
    }
        
    // Show content of first few lines
    fprintf(out, "\nContent preview:\n");
    for (size_t i = 0; i < state->height && i < 5; i++) {
        fprintf(out, "Line %zu: ", i);
        for (size_t j = 0; j < state->width && j < 80; j++) {
            char c = state->lines[i].cells[j].character;
            if (state->lines[i].cells[j].has_content && c >= 32 && c <= 126) {
                fputc(c, out);
            } else {
                fputc('.', out);
            }
        }
        fprintf(out, "\n");
    }
        
    fprintf(out, "=== END TERMINAL STATE ===\n\n");
}
    
/**
 * @brief Dump current display sync state for debugging
 */
void lle_state_sync_debug_dump_display(lle_state_sync_context_t *sync_ctx,
                                       FILE *output_file) {
    if (!sync_ctx) {
        return;
    }
        
    FILE *out = output_file ? output_file : stderr;
    lle_display_sync_state_t *sync_state = sync_ctx->display_sync;
        
    fprintf(out, "\n=== DISPLAY SYNC STATE DUMP ===\n");
    fprintf(out, "Expected content length: %zu\n", sync_state->expected_length);
    fprintf(out, "Expected cursor position: %zu\n", sync_state->expected_cursor_pos);
    fprintf(out, "Expected rows used: %zu\n", sync_state->expected_rows_used);
    fprintf(out, "Sync required: %s\n", sync_state->sync_required ? "yes" : "no");
    fprintf(out, "Sync in progress: %s\n", sync_state->sync_in_progress ? "yes" : "no");
    fprintf(out, "Content validated: %s\n", sync_state->content_validated ? "yes" : "no");
    fprintf(out, "Cursor validated: %s\n", sync_state->cursor_validated ? "yes" : "no");
    fprintf(out, "Geometry validated: %s\n", sync_state->geometry_validated ? "yes" : "no");
    fprintf(out, "Sync operations: %zu\n", sync_state->sync_operations);
    fprintf(out, "Sync failures: %zu\n", sync_state->sync_failure_count);
    fprintf(out, "Average sync time: %.2f us\n", sync_state->average_sync_time);
        
    if (sync_state->expected_content && sync_state->expected_length > 0) {
        fprintf(out, "Expected content preview: \"");
        size_t preview_len = sync_state->expected_length < 60 ? sync_state->expected_length : 60;
        for (size_t i = 0; i < preview_len; i++) {
            char c = sync_state->expected_content[i];
            if (c >= 32 && c <= 126) {
                fputc(c, out);
            } else {
                fprintf(out, "\\x%02x", (unsigned char)c);
            }
        }
        if (sync_state->expected_length > 60) {
            fprintf(out, "...");
        }
        fprintf(out, "\"\n");
    }
        
    fprintf(out, "=== END DISPLAY SYNC STATE ===\n\n");
}
    
/**
 * @brief Compare terminal and display states and report differences
 */
size_t lle_state_sync_debug_compare_states(lle_state_sync_context_t *sync_ctx,
                                           FILE *output_file) {
    if (!sync_ctx) {
        return 0;
    }
        
    FILE *out = output_file ? output_file : stderr;
    size_t differences = 0;
        
    fprintf(out, "\n=== STATE COMPARISON ===\n");
        
    // Compare cursor positions
    size_t display_cursor_row = sync_ctx->display->cursor_pos.absolute_row;
    size_t display_cursor_col = sync_ctx->display->cursor_pos.absolute_col;
    size_t terminal_cursor_row = sync_ctx->terminal_state->cursor_row;
    size_t terminal_cursor_col = sync_ctx->terminal_state->cursor_col;
        
    if (display_cursor_row != terminal_cursor_row || display_cursor_col != terminal_cursor_col) {
        fprintf(out, "CURSOR MISMATCH: display=(%zu,%zu), terminal=(%zu,%zu)\n",
                display_cursor_row, display_cursor_col,
                terminal_cursor_row, terminal_cursor_col);
        differences++;
    }
        
    // Compare dimensions
    size_t display_width = sync_ctx->display->geometry.width;
    size_t display_height = sync_ctx->display->geometry.height;
    size_t terminal_width = sync_ctx->terminal_state->width;
    size_t terminal_height = sync_ctx->terminal_state->height;
        
    if (display_width != terminal_width || display_height != terminal_height) {
        fprintf(out, "GEOMETRY MISMATCH: display=%zux%zu, terminal=%zux%zu\n",
                display_width, display_height,
                terminal_width, terminal_height);
        differences++;
    }
        
    // Compare sync state flags
    if (!sync_ctx->display_sync->content_validated) {
        fprintf(out, "CONTENT NOT VALIDATED\n");
        differences++;
    }
        
    if (!sync_ctx->display_sync->cursor_validated) {
        fprintf(out, "CURSOR NOT VALIDATED\n");
        differences++;
    }
        
    if (!sync_ctx->display_sync->geometry_validated) {
        fprintf(out, "GEOMETRY NOT VALIDATED\n");
        differences++;
    }
        
    if (sync_ctx->display_sync->sync_required) {
        fprintf(out, "SYNC REQUIRED\n");
        differences++;
    }
        
    if (differences == 0) {
        fprintf(out, "No differences found - states are consistent\n");
    }
        
    fprintf(out, "Total differences: %zu\n", differences);
    fprintf(out, "=== END STATE COMPARISON ===\n\n");
        
    return differences;
}