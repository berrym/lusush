// src/lle/foundation/buffer/undo.c
//
// LLE Undo/Redo System Implementation

#include "undo.h"
#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get current time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Free a single operation
static void free_operation(lle_change_operation_t *op) {
    if (!op) return;
    free(op->text);
    free(op);
}

// Free a sequence and all its operations
static void free_sequence(lle_change_sequence_t *seq) {
    if (!seq) return;
    
    lle_change_operation_t *op = seq->first_op;
    while (op) {
        lle_change_operation_t *next = op->next;
        free_operation(op);
        op = next;
    }
    
    free(seq);
}

// Initialize change tracker
int lle_change_tracker_init(lle_change_tracker_t *tracker,
                            size_t max_sequences,
                            size_t max_memory_bytes) {
    if (!tracker) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    memset(tracker, 0, sizeof(lle_change_tracker_t));
    
    tracker->max_sequences = (max_sequences > 0) ? max_sequences : LLE_UNDO_DEFAULT_MAX_SEQUENCES;
    tracker->max_memory_bytes = (max_memory_bytes > 0) ? max_memory_bytes : LLE_UNDO_DEFAULT_MAX_MEMORY;
    tracker->sequence_timeout_ns = LLE_UNDO_DEFAULT_SEQUENCE_TIMEOUT * 1000000ULL;  // ms to ns
    tracker->compression_enabled = true;
    tracker->tracking_enabled = true;
    tracker->next_sequence_id = 1;
    tracker->next_operation_id = 1;
    
    return LLE_UNDO_OK;
}

// Cleanup change tracker
void lle_change_tracker_cleanup(lle_change_tracker_t *tracker) {
    if (!tracker) return;
    
    lle_change_sequence_t *seq = tracker->first_sequence;
    while (seq) {
        lle_change_sequence_t *next = seq->next;
        free_sequence(seq);
        seq = next;
    }
    
    memset(tracker, 0, sizeof(lle_change_tracker_t));
}

// Enable/disable change tracking
void lle_change_tracker_set_enabled(lle_change_tracker_t *tracker, bool enabled) {
    if (tracker) {
        tracker->tracking_enabled = enabled;
    }
}

// Check if tracking is enabled
bool lle_change_tracker_is_enabled(const lle_change_tracker_t *tracker) {
    return tracker ? tracker->tracking_enabled : false;
}

// Begin new sequence
int lle_change_tracker_begin_sequence(lle_change_tracker_t *tracker) {
    if (!tracker) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    if (!tracker->tracking_enabled) {
        return LLE_UNDO_OK;  // Tracking disabled, do nothing
    }
    
    // End previous sequence if active
    if (tracker->active_sequence) {
        lle_change_tracker_end_sequence(tracker);
    }
    
    // Allocate new sequence
    lle_change_sequence_t *seq = calloc(1, sizeof(lle_change_sequence_t));
    if (!seq) {
        return LLE_UNDO_ERR_ALLOC_FAILED;
    }
    
    seq->sequence_id = tracker->next_sequence_id++;
    seq->start_time_ns = get_time_ns();
    
    tracker->active_sequence = seq;
    
    return LLE_UNDO_OK;
}

// End current sequence
int lle_change_tracker_end_sequence(lle_change_tracker_t *tracker) {
    if (!tracker) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    if (!tracker->active_sequence) {
        return LLE_UNDO_OK;  // No active sequence
    }
    
    lle_change_sequence_t *seq = tracker->active_sequence;
    
    // Don't save empty sequences
    if (seq->operation_count == 0) {
        free_sequence(seq);
        tracker->active_sequence = NULL;
        return LLE_UNDO_OK;
    }
    
    seq->end_time_ns = get_time_ns();
    
    // When adding a new sequence, invalidate any redo history
    if (tracker->current_sequence && tracker->current_sequence->next) {
        lle_change_sequence_t *next = tracker->current_sequence->next;
        while (next) {
            lle_change_sequence_t *to_free = next;
            next = next->next;
            
            // Update statistics
            tracker->total_sequences--;
            tracker->total_operations -= to_free->operation_count;
            
            free_sequence(to_free);
        }
        tracker->current_sequence->next = NULL;
        tracker->last_sequence = tracker->current_sequence;
    }
    
    // Add sequence to history
    if (tracker->last_sequence) {
        tracker->last_sequence->next = seq;
        seq->prev = tracker->last_sequence;
        tracker->last_sequence = seq;
    } else {
        tracker->first_sequence = seq;
        tracker->last_sequence = seq;
    }
    
    tracker->current_sequence = seq;
    tracker->total_sequences++;
    tracker->active_sequence = NULL;
    
    // Check memory limits
    while (tracker->total_sequences > tracker->max_sequences ||
           tracker->memory_used > tracker->max_memory_bytes) {
        if (!tracker->first_sequence || tracker->first_sequence == tracker->current_sequence) {
            break;  // Can't remove current or only sequence
        }
        
        lle_change_sequence_t *to_remove = tracker->first_sequence;
        tracker->first_sequence = to_remove->next;
        if (tracker->first_sequence) {
            tracker->first_sequence->prev = NULL;
        }
        
        tracker->total_sequences--;
        tracker->total_operations -= to_remove->operation_count;
        
        free_sequence(to_remove);
    }
    
    return LLE_UNDO_OK;
}

// Helper: Ensure active sequence exists
static int ensure_active_sequence(lle_change_tracker_t *tracker) {
    if (!tracker->tracking_enabled) {
        return LLE_UNDO_OK;
    }
    
    if (!tracker->active_sequence) {
        return lle_change_tracker_begin_sequence(tracker);
    }
    
    // Check sequence timeout - start new sequence if timeout exceeded
    if (tracker->active_sequence->first_op) {
        uint64_t current_time = get_time_ns();
        uint64_t elapsed = current_time - tracker->active_sequence->start_time_ns;
        
        if (elapsed > tracker->sequence_timeout_ns) {
            lle_change_tracker_end_sequence(tracker);
            return lle_change_tracker_begin_sequence(tracker);
        }
    }
    
    return LLE_UNDO_OK;
}

// Record an insert operation
int lle_change_tracker_record_insert(lle_change_tracker_t *tracker,
                                     size_t position,
                                     const char *text,
                                     size_t text_length,
                                     size_t cursor_before,
                                     size_t cursor_after) {
    if (!tracker || !text) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    if (!tracker->tracking_enabled) {
        return LLE_UNDO_OK;
    }
    
    int result = ensure_active_sequence(tracker);
    if (result != LLE_UNDO_OK) {
        return result;
    }
    
    // Allocate operation
    lle_change_operation_t *op = calloc(1, sizeof(lle_change_operation_t));
    if (!op) {
        return LLE_UNDO_ERR_ALLOC_FAILED;
    }
    
    // Copy text
    op->text = malloc(text_length + 1);
    if (!op->text) {
        free(op);
        return LLE_UNDO_ERR_ALLOC_FAILED;
    }
    memcpy(op->text, text, text_length);
    op->text[text_length] = '\0';
    
    // Fill in operation details
    op->operation_id = tracker->next_operation_id++;
    op->type = LLE_CHANGE_TYPE_INSERT;
    op->timestamp_ns = get_time_ns();
    op->position = position;
    op->text_length = text_length;
    op->cursor_before = cursor_before;
    op->cursor_after = cursor_after;
    
    // Add to active sequence
    lle_change_sequence_t *seq = tracker->active_sequence;
    if (seq->last_op) {
        seq->last_op->next = op;
        op->prev = seq->last_op;
        seq->last_op = op;
    } else {
        seq->first_op = op;
        seq->last_op = op;
    }
    
    seq->operation_count++;
    tracker->total_operations++;
    tracker->memory_used += sizeof(lle_change_operation_t) + text_length + 1;
    
    return LLE_UNDO_OK;
}

// Record a delete operation
int lle_change_tracker_record_delete(lle_change_tracker_t *tracker,
                                     size_t position,
                                     const char *deleted_text,
                                     size_t text_length,
                                     size_t cursor_before,
                                     size_t cursor_after) {
    if (!tracker || !deleted_text) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    if (!tracker->tracking_enabled) {
        return LLE_UNDO_OK;
    }
    
    int result = ensure_active_sequence(tracker);
    if (result != LLE_UNDO_OK) {
        return result;
    }
    
    // Allocate operation
    lle_change_operation_t *op = calloc(1, sizeof(lle_change_operation_t));
    if (!op) {
        return LLE_UNDO_ERR_ALLOC_FAILED;
    }
    
    // Copy deleted text
    op->text = malloc(text_length + 1);
    if (!op->text) {
        free(op);
        return LLE_UNDO_ERR_ALLOC_FAILED;
    }
    memcpy(op->text, deleted_text, text_length);
    op->text[text_length] = '\0';
    
    // Fill in operation details
    op->operation_id = tracker->next_operation_id++;
    op->type = LLE_CHANGE_TYPE_DELETE;
    op->timestamp_ns = get_time_ns();
    op->position = position;
    op->text_length = text_length;
    op->cursor_before = cursor_before;
    op->cursor_after = cursor_after;
    
    // Add to active sequence
    lle_change_sequence_t *seq = tracker->active_sequence;
    if (seq->last_op) {
        seq->last_op->next = op;
        op->prev = seq->last_op;
        seq->last_op = op;
    } else {
        seq->first_op = op;
        seq->last_op = op;
    }
    
    seq->operation_count++;
    tracker->total_operations++;
    tracker->memory_used += sizeof(lle_change_operation_t) + text_length + 1;
    
    return LLE_UNDO_OK;
}

// Record a replace operation (not used yet, but part of API)
int lle_change_tracker_record_replace(lle_change_tracker_t *tracker,
                                      size_t position,
                                      const char *deleted_text,
                                      size_t deleted_length,
                                      const char *inserted_text,
                                      size_t inserted_length,
                                      size_t cursor_before,
                                      size_t cursor_after) {
    if (!tracker) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    // For now, implement as delete + insert
    int result = lle_change_tracker_record_delete(tracker, position, deleted_text,
                                                   deleted_length, cursor_before, cursor_before);
    if (result != LLE_UNDO_OK) {
        return result;
    }
    
    return lle_change_tracker_record_insert(tracker, position, inserted_text,
                                            inserted_length, cursor_before, cursor_after);
}

// Undo last sequence
int lle_change_tracker_undo(lle_change_tracker_t *tracker,
                            lle_buffer_t *buffer) {
    if (!tracker || !buffer) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    if (!tracker->current_sequence) {
        return LLE_UNDO_ERR_NO_UNDO;
    }
    
    lle_change_sequence_t *seq = tracker->current_sequence;
    
    // Disable tracking during undo
    bool was_enabled = tracker->tracking_enabled;
    tracker->tracking_enabled = false;
    
    // Process operations in reverse order
    lle_change_operation_t *op = seq->last_op;
    while (op) {
        int result = LLE_BUFFER_OK;
        
        switch (op->type) {
            case LLE_CHANGE_TYPE_INSERT:
                // Undo insert by deleting the inserted text
                result = lle_buffer_delete_range(buffer, op->position,
                                                 op->position + op->text_length);
                break;
                
            case LLE_CHANGE_TYPE_DELETE:
                // Undo delete by reinserting the deleted text
                result = lle_buffer_insert_string(buffer, op->position,
                                                  op->text, op->text_length);
                break;
                
            case LLE_CHANGE_TYPE_REPLACE:
                // Should not occur with current implementation
                break;
        }
        
        if (result != LLE_BUFFER_OK) {
            tracker->tracking_enabled = was_enabled;
            return LLE_UNDO_ERR_INVALID_STATE;
        }
        
        op = op->prev;
    }
    
    // Restore cursor position from first operation
    if (seq->first_op) {
        // Cursor position will be restored by buffer operations
        // (this is a simplification - full implementation would track cursor separately)
    }
    
    // Move current_sequence pointer back
    tracker->current_sequence = seq->prev;
    tracker->undo_count++;
    
    // Re-enable tracking
    tracker->tracking_enabled = was_enabled;
    
    return LLE_UNDO_OK;
}

// Redo next sequence
int lle_change_tracker_redo(lle_change_tracker_t *tracker,
                            lle_buffer_t *buffer) {
    if (!tracker || !buffer) {
        return LLE_UNDO_ERR_NULL_PTR;
    }
    
    // Find next sequence to redo
    lle_change_sequence_t *seq = tracker->current_sequence ?
                                  tracker->current_sequence->next :
                                  tracker->first_sequence;
    
    if (!seq) {
        return LLE_UNDO_ERR_NO_REDO;
    }
    
    // Disable tracking during redo
    bool was_enabled = tracker->tracking_enabled;
    tracker->tracking_enabled = false;
    
    // Process operations in forward order
    lle_change_operation_t *op = seq->first_op;
    while (op) {
        int result = LLE_BUFFER_OK;
        
        switch (op->type) {
            case LLE_CHANGE_TYPE_INSERT:
                // Redo insert
                result = lle_buffer_insert_string(buffer, op->position,
                                                  op->text, op->text_length);
                break;
                
            case LLE_CHANGE_TYPE_DELETE:
                // Redo delete
                result = lle_buffer_delete_range(buffer, op->position,
                                                 op->position + op->text_length);
                break;
                
            case LLE_CHANGE_TYPE_REPLACE:
                // Should not occur with current implementation
                break;
        }
        
        if (result != LLE_BUFFER_OK) {
            tracker->tracking_enabled = was_enabled;
            return LLE_UNDO_ERR_INVALID_STATE;
        }
        
        op = op->next;
    }
    
    // Move current_sequence pointer forward
    tracker->current_sequence = seq;
    tracker->redo_count++;
    
    // Re-enable tracking
    tracker->tracking_enabled = was_enabled;
    
    return LLE_UNDO_OK;
}

// Check if undo is available
bool lle_change_tracker_can_undo(const lle_change_tracker_t *tracker) {
    return tracker && tracker->current_sequence != NULL;
}

// Check if redo is available
bool lle_change_tracker_can_redo(const lle_change_tracker_t *tracker) {
    if (!tracker) return false;
    
    if (tracker->current_sequence) {
        return tracker->current_sequence->next != NULL;
    } else {
        return tracker->first_sequence != NULL;
    }
}

// Clear all undo/redo history
void lle_change_tracker_clear(lle_change_tracker_t *tracker) {
    if (!tracker) return;
    
    lle_change_sequence_t *seq = tracker->first_sequence;
    while (seq) {
        lle_change_sequence_t *next = seq->next;
        free_sequence(seq);
        seq = next;
    }
    
    tracker->first_sequence = NULL;
    tracker->last_sequence = NULL;
    tracker->current_sequence = NULL;
    tracker->active_sequence = NULL;
    tracker->total_sequences = 0;
    tracker->total_operations = 0;
    tracker->memory_used = 0;
}

// Get undo/redo statistics
void lle_change_tracker_get_stats(const lle_change_tracker_t *tracker,
                                  size_t *total_sequences,
                                  size_t *total_operations,
                                  size_t *memory_used,
                                  uint32_t *undo_count,
                                  uint32_t *redo_count) {
    if (!tracker) return;
    
    if (total_sequences) *total_sequences = tracker->total_sequences;
    if (total_operations) *total_operations = tracker->total_operations;
    if (memory_used) *memory_used = tracker->memory_used;
    if (undo_count) *undo_count = tracker->undo_count;
    if (redo_count) *redo_count = tracker->redo_count;
}

// Configuration
void lle_change_tracker_set_compression(lle_change_tracker_t *tracker, bool enabled) {
    if (tracker) {
        tracker->compression_enabled = enabled;
    }
}

void lle_change_tracker_set_sequence_timeout(lle_change_tracker_t *tracker,
                                             uint64_t timeout_ms) {
    if (tracker) {
        tracker->sequence_timeout_ns = timeout_ms * 1000000ULL;
    }
}

// Utility: Convert error code to string
const char* lle_undo_error_string(int error_code) {
    switch (error_code) {
        case LLE_UNDO_OK: return "Success";
        case LLE_UNDO_ERR_NULL_PTR: return "Null pointer";
        case LLE_UNDO_ERR_NO_UNDO: return "No undo available";
        case LLE_UNDO_ERR_NO_REDO: return "No redo available";
        case LLE_UNDO_ERR_ALLOC_FAILED: return "Memory allocation failed";
        case LLE_UNDO_ERR_INVALID_STATE: return "Invalid state";
        default: return "Unknown error";
    }
}
