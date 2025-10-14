// src/lle/foundation/buffer/buffer.h
//
// LLE Gap Buffer Implementation (Production)
//
// A gap buffer is a dynamic array with a movable "gap" that follows the cursor.
// This makes insertions and deletions at the cursor position O(1) amortized.
//
// Architecture validated in Phase 0:
// - Buffer operations <10μs per operation (target)
// - Memory efficiency <2x text size
// - Support for large files (10MB+)
// - Efficient undo/redo integration

#ifndef LLE_FOUNDATION_BUFFER_H
#define LLE_FOUNDATION_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Error codes
typedef enum {
    LLE_BUFFER_OK = 0,
    LLE_BUFFER_ERR_NULL_PTR = -1,
    LLE_BUFFER_ERR_INVALID_POS = -2,
    LLE_BUFFER_ERR_ALLOC_FAILED = -3,
    LLE_BUFFER_ERR_OUT_OF_BOUNDS = -4,
    LLE_BUFFER_ERR_READ_ONLY = -5,
    LLE_BUFFER_ERR_EMPTY = -6
} lle_buffer_error_t;

// Gap buffer structure
typedef struct {
    char *data;           // Buffer data (includes gap)
    size_t capacity;      // Total buffer capacity
    size_t gap_start;     // Start of gap (cursor position)
    size_t gap_end;       // End of gap (exclusive)
    
    // Buffer state
    bool read_only;       // Read-only flag
    bool modified;        // Modified since last save
    
    // Performance tracking
    uint64_t insert_count;
    uint64_t delete_count;
    uint64_t move_count;
    uint64_t total_op_time_ns;
    
    // Statistics
    size_t max_size;      // Maximum size reached
    size_t grow_count;    // Number of buffer grows
} lle_buffer_t;

// Buffer position (abstract cursor position in text)
typedef size_t lle_buffer_pos_t;

// Initialize buffer with initial capacity
int lle_buffer_init(lle_buffer_t *buffer, size_t initial_capacity);

// Initialize buffer from string
int lle_buffer_init_from_string(lle_buffer_t *buffer, const char *str, size_t len);

// Cleanup buffer
void lle_buffer_cleanup(lle_buffer_t *buffer);

// Get buffer size (excluding gap)
size_t lle_buffer_size(const lle_buffer_t *buffer);

// Get capacity
size_t lle_buffer_capacity(const lle_buffer_t *buffer);

// Check if buffer is empty
bool lle_buffer_is_empty(const lle_buffer_t *buffer);

// Get character at position
int lle_buffer_get_char(const lle_buffer_t *buffer, lle_buffer_pos_t pos, char *ch);

// Get substring [start, end)
int lle_buffer_get_substring(const lle_buffer_t *buffer,
                             lle_buffer_pos_t start,
                             lle_buffer_pos_t end,
                             char *dest,
                             size_t dest_size);

// Get entire buffer contents
int lle_buffer_get_contents(const lle_buffer_t *buffer,
                            char *dest,
                            size_t dest_size);

// Insert character at position
int lle_buffer_insert_char(lle_buffer_t *buffer, lle_buffer_pos_t pos, char ch);

// Insert string at position
int lle_buffer_insert_string(lle_buffer_t *buffer,
                             lle_buffer_pos_t pos,
                             const char *str,
                             size_t len);

// Delete character at position
int lle_buffer_delete_char(lle_buffer_t *buffer, lle_buffer_pos_t pos);

// Delete range [start, end)
int lle_buffer_delete_range(lle_buffer_t *buffer,
                            lle_buffer_pos_t start,
                            lle_buffer_pos_t end);

// Clear buffer
int lle_buffer_clear(lle_buffer_t *buffer);

// Move gap to position (internal operation, exposed for testing)
int lle_buffer_move_gap(lle_buffer_t *buffer, lle_buffer_pos_t pos);

// Ensure capacity (grow if needed)
int lle_buffer_ensure_capacity(lle_buffer_t *buffer, size_t min_capacity);

// Set read-only flag
void lle_buffer_set_read_only(lle_buffer_t *buffer, bool read_only);

// Check if modified
bool lle_buffer_is_modified(const lle_buffer_t *buffer);

// Clear modified flag
void lle_buffer_clear_modified(lle_buffer_t *buffer);

// Search forward for character
lle_buffer_pos_t lle_buffer_find_char(const lle_buffer_t *buffer,
                                      lle_buffer_pos_t start,
                                      char ch);

// Search forward for string
lle_buffer_pos_t lle_buffer_find_string(const lle_buffer_t *buffer,
                                        lle_buffer_pos_t start,
                                        const char *str,
                                        size_t len);

// Find line start position
lle_buffer_pos_t lle_buffer_line_start(const lle_buffer_t *buffer,
                                       lle_buffer_pos_t pos);

// Find line end position
lle_buffer_pos_t lle_buffer_line_end(const lle_buffer_t *buffer,
                                     lle_buffer_pos_t pos);

// Count lines
size_t lle_buffer_line_count(const lle_buffer_t *buffer);

// Get line number for position (0-based)
size_t lle_buffer_get_line_number(const lle_buffer_t *buffer,
                                  lle_buffer_pos_t pos);

// Performance metrics
void lle_buffer_get_metrics(const lle_buffer_t *buffer,
                           uint64_t *insert_count,
                           uint64_t *delete_count,
                           uint64_t *move_count,
                           double *avg_time_us);

// Get statistics
void lle_buffer_get_stats(const lle_buffer_t *buffer,
                         size_t *current_size,
                         size_t *capacity,
                         size_t *max_size,
                         size_t *grow_count,
                         double *memory_efficiency);

// Utility: Convert error code to string
const char* lle_buffer_error_string(int error_code);

// Special position constants
#define LLE_BUFFER_POS_INVALID  ((lle_buffer_pos_t)-1)
#define LLE_BUFFER_NPOS         ((lle_buffer_pos_t)-1)

#endif // LLE_FOUNDATION_BUFFER_H
