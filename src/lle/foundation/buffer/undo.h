// src/lle/foundation/buffer/undo.h
//
// LLE Undo/Redo System (Phase 1 Week 6)
//
// Change tracking system with compression for efficient undo/redo operations.
// Supports atomic operations with intelligent grouping of sequential changes.
//
// Design principles:
// - All buffer modifications are tracked and reversible
// - Sequential operations are compressed for efficiency
// - Undo/redo operations are atomic
// - Performance target: <0.5ms per undo/redo operation

#ifndef LLE_FOUNDATION_BUFFER_UNDO_H
#define LLE_FOUNDATION_BUFFER_UNDO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Include buffer.h to get lle_buffer_t definition
// This is safe since buffer.h doesn't include undo.h
#include "buffer.h"

// Result codes (consistent with buffer.h)
typedef enum {
    LLE_UNDO_OK = 0,
    LLE_UNDO_ERR_NULL_PTR = -1,
    LLE_UNDO_ERR_NO_UNDO = -2,
    LLE_UNDO_ERR_NO_REDO = -3,
    LLE_UNDO_ERR_ALLOC_FAILED = -4,
    LLE_UNDO_ERR_INVALID_STATE = -5,
} lle_undo_error_t;

// Change operation types
typedef enum {
    LLE_CHANGE_TYPE_INSERT = 1,    // Text insertion
    LLE_CHANGE_TYPE_DELETE,        // Text deletion
    LLE_CHANGE_TYPE_REPLACE,       // Text replacement (delete + insert)
} lle_change_type_t;

// Individual change operation
typedef struct lle_change_operation {
    // Operation metadata
    uint32_t operation_id;         // Unique operation identifier
    lle_change_type_t type;        // Change operation type
    uint64_t timestamp_ns;         // Operation timestamp (nanoseconds)
    
    // Position information
    size_t position;               // Operation position in buffer
    
    // Operation data
    char *text;                    // Text data (inserted or deleted)
    size_t text_length;            // Length of text data
    
    // Cursor state
    size_t cursor_before;          // Cursor position before operation
    size_t cursor_after;           // Cursor position after operation
    
    // Operation linking
    struct lle_change_operation *next; // Next operation in sequence
    struct lle_change_operation *prev; // Previous operation in sequence
    
} lle_change_operation_t;

// Change sequence (group of related operations)
typedef struct lle_change_sequence {
    // Sequence metadata
    uint32_t sequence_id;          // Unique sequence identifier
    uint64_t start_time_ns;        // Sequence start time
    uint64_t end_time_ns;          // Sequence end time
    
    // Operation chain
    lle_change_operation_t *first_op;  // First operation in sequence
    lle_change_operation_t *last_op;   // Last operation in sequence
    size_t operation_count;            // Number of operations
    
    // Sequence linking
    struct lle_change_sequence *next;  // Next sequence in history
    struct lle_change_sequence *prev;  // Previous sequence in history
    
} lle_change_sequence_t;

// Change tracker (undo/redo manager)
typedef struct lle_change_tracker {
    // Sequence chain
    lle_change_sequence_t *first_sequence;  // Oldest sequence
    lle_change_sequence_t *last_sequence;   // Newest sequence
    lle_change_sequence_t *current_sequence; // Current sequence (for undo/redo)
    
    // Current sequence being built
    lle_change_sequence_t *active_sequence;  // Active sequence for new operations
    
    // Sequence ID counter
    uint32_t next_sequence_id;     // Next sequence ID to assign
    uint32_t next_operation_id;    // Next operation ID to assign
    
    // Limits
    size_t max_sequences;          // Maximum sequences to retain
    size_t max_memory_bytes;       // Maximum memory for undo history
    
    // Statistics
    size_t total_sequences;        // Total sequences stored
    size_t total_operations;       // Total operations stored
    size_t memory_used;            // Current memory usage
    uint32_t undo_count;           // Number of undo operations performed
    uint32_t redo_count;           // Number of redo operations performed
    
    // Configuration
    bool compression_enabled;      // Enable operation compression
    uint64_t sequence_timeout_ns;  // Time before starting new sequence (nanoseconds)
    
    // State
    bool tracking_enabled;         // Change tracking enabled
    
} lle_change_tracker_t;

// Initialize change tracker
int lle_change_tracker_init(lle_change_tracker_t *tracker,
                            size_t max_sequences,
                            size_t max_memory_bytes);

// Cleanup change tracker
void lle_change_tracker_cleanup(lle_change_tracker_t *tracker);

// Enable/disable change tracking
void lle_change_tracker_set_enabled(lle_change_tracker_t *tracker, bool enabled);

// Check if tracking is enabled
bool lle_change_tracker_is_enabled(const lle_change_tracker_t *tracker);

// Begin new sequence (call before a logical group of operations)
int lle_change_tracker_begin_sequence(lle_change_tracker_t *tracker);

// End current sequence (call after logical group complete)
int lle_change_tracker_end_sequence(lle_change_tracker_t *tracker);

// Record an insert operation
int lle_change_tracker_record_insert(lle_change_tracker_t *tracker,
                                     size_t position,
                                     const char *text,
                                     size_t text_length,
                                     size_t cursor_before,
                                     size_t cursor_after);

// Record a delete operation
int lle_change_tracker_record_delete(lle_change_tracker_t *tracker,
                                     size_t position,
                                     const char *deleted_text,
                                     size_t text_length,
                                     size_t cursor_before,
                                     size_t cursor_after);

// Record a replace operation
int lle_change_tracker_record_replace(lle_change_tracker_t *tracker,
                                      size_t position,
                                      const char *deleted_text,
                                      size_t deleted_length,
                                      const char *inserted_text,
                                      size_t inserted_length,
                                      size_t cursor_before,
                                      size_t cursor_after);

// Undo last sequence
int lle_change_tracker_undo(lle_change_tracker_t *tracker,
                            lle_buffer_t *buffer);

// Redo next sequence
int lle_change_tracker_redo(lle_change_tracker_t *tracker,
                            lle_buffer_t *buffer);

// Check if undo is available
bool lle_change_tracker_can_undo(const lle_change_tracker_t *tracker);

// Check if redo is available
bool lle_change_tracker_can_redo(const lle_change_tracker_t *tracker);

// Clear all undo/redo history
void lle_change_tracker_clear(lle_change_tracker_t *tracker);

// Get undo/redo statistics
void lle_change_tracker_get_stats(const lle_change_tracker_t *tracker,
                                  size_t *total_sequences,
                                  size_t *total_operations,
                                  size_t *memory_used,
                                  uint32_t *undo_count,
                                  uint32_t *redo_count);

// Configuration
void lle_change_tracker_set_compression(lle_change_tracker_t *tracker, bool enabled);
void lle_change_tracker_set_sequence_timeout(lle_change_tracker_t *tracker,
                                             uint64_t timeout_ms);

// Utility: Convert error code to string
const char* lle_undo_error_string(int error_code);

// Default configuration values
#define LLE_UNDO_DEFAULT_MAX_SEQUENCES    100
#define LLE_UNDO_DEFAULT_MAX_MEMORY       (1024 * 1024)  // 1MB
#define LLE_UNDO_DEFAULT_SEQUENCE_TIMEOUT 500            // 500ms

#endif // LLE_FOUNDATION_BUFFER_UNDO_H
