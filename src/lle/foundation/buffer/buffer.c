// src/lle/foundation/buffer/buffer.c
//
// LLE Gap Buffer Implementation

#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Default initial capacity
#define DEFAULT_CAPACITY 1024

// Minimum gap size
#define MIN_GAP_SIZE 256

// Growth factor for buffer expansion
#define GROWTH_FACTOR 1.5

// Get timestamp in nanoseconds
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Get gap size
static inline size_t gap_size(const lle_buffer_t *buffer) {
    return buffer->gap_end - buffer->gap_start;
}

// Get text size (excluding gap)
static inline size_t text_size(const lle_buffer_t *buffer) {
    return buffer->capacity - gap_size(buffer);
}

int lle_buffer_init(lle_buffer_t *buffer, size_t initial_capacity) {
    if (!buffer) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    if (initial_capacity == 0) {
        initial_capacity = DEFAULT_CAPACITY;
    }
    
    // Zero-initialize
    memset(buffer, 0, sizeof(*buffer));
    
    // Allocate buffer
    buffer->data = malloc(initial_capacity);
    if (!buffer->data) {
        return LLE_BUFFER_ERR_ALLOC_FAILED;
    }
    
    buffer->capacity = initial_capacity;
    buffer->gap_start = 0;
    buffer->gap_end = initial_capacity;  // Entire buffer is gap initially
    buffer->read_only = false;
    buffer->modified = false;
    
    return LLE_BUFFER_OK;
}

int lle_buffer_init_from_string(lle_buffer_t *buffer, const char *str, size_t len) {
    if (!buffer || !str) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    // Initialize with enough capacity for string + gap
    size_t capacity = len + MIN_GAP_SIZE;
    if (capacity < DEFAULT_CAPACITY) {
        capacity = DEFAULT_CAPACITY;
    }
    
    int result = lle_buffer_init(buffer, capacity);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Copy string before gap
    if (len > 0) {
        memcpy(buffer->data, str, len);
        buffer->gap_start = len;
    }
    
    buffer->modified = false;
    
    return LLE_BUFFER_OK;
}

void lle_buffer_cleanup(lle_buffer_t *buffer) {
    if (!buffer) {
        return;
    }
    
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    
    memset(buffer, 0, sizeof(*buffer));
}

size_t lle_buffer_size(const lle_buffer_t *buffer) {
    if (!buffer) {
        return 0;
    }
    return text_size(buffer);
}

size_t lle_buffer_capacity(const lle_buffer_t *buffer) {
    if (!buffer) {
        return 0;
    }
    return buffer->capacity;
}

bool lle_buffer_is_empty(const lle_buffer_t *buffer) {
    return lle_buffer_size(buffer) == 0;
}

int lle_buffer_move_gap(lle_buffer_t *buffer, lle_buffer_pos_t pos) {
    if (!buffer || !buffer->data) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    size_t size = text_size(buffer);
    if (pos > size) {
        return LLE_BUFFER_ERR_INVALID_POS;
    }
    
    uint64_t start = get_timestamp_ns();
    
    if (pos < buffer->gap_start) {
        // Move gap left
        size_t move_size = buffer->gap_start - pos;
        memmove(buffer->data + buffer->gap_end - move_size,
                buffer->data + pos,
                move_size);
        buffer->gap_end -= move_size;
        buffer->gap_start = pos;
    } else if (pos > buffer->gap_start) {
        // Move gap right
        size_t move_size = pos - buffer->gap_start;
        memmove(buffer->data + buffer->gap_start,
                buffer->data + buffer->gap_end,
                move_size);
        buffer->gap_start += move_size;
        buffer->gap_end += move_size;
    }
    // else: gap already at position
    
    uint64_t end = get_timestamp_ns();
    buffer->total_op_time_ns += (end - start);
    buffer->move_count++;
    
    return LLE_BUFFER_OK;
}

int lle_buffer_ensure_capacity(lle_buffer_t *buffer, size_t min_capacity) {
    if (!buffer || !buffer->data) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    if (buffer->capacity >= min_capacity) {
        return LLE_BUFFER_OK;  // Already sufficient
    }
    
    // Calculate new capacity with growth factor
    size_t new_capacity = buffer->capacity;
    while (new_capacity < min_capacity) {
        new_capacity = (size_t)(new_capacity * GROWTH_FACTOR);
    }
    
    // Ensure minimum gap size
    if (new_capacity < min_capacity + MIN_GAP_SIZE) {
        new_capacity = min_capacity + MIN_GAP_SIZE;
    }
    
    // Allocate new buffer
    char *new_data = malloc(new_capacity);
    if (!new_data) {
        return LLE_BUFFER_ERR_ALLOC_FAILED;
    }
    
    // Copy data before gap
    if (buffer->gap_start > 0) {
        memcpy(new_data, buffer->data, buffer->gap_start);
    }
    
    // Copy data after gap
    size_t after_gap_size = buffer->capacity - buffer->gap_end;
    if (after_gap_size > 0) {
        memcpy(new_data + new_capacity - after_gap_size,
               buffer->data + buffer->gap_end,
               after_gap_size);
    }
    
    // Update buffer
    free(buffer->data);
    buffer->data = new_data;
    size_t old_gap_end = buffer->gap_end;
    buffer->gap_end = new_capacity - (buffer->capacity - old_gap_end);
    buffer->capacity = new_capacity;
    buffer->grow_count++;
    
    // Update statistics
    size_t current_size = text_size(buffer);
    if (current_size > buffer->max_size) {
        buffer->max_size = current_size;
    }
    
    return LLE_BUFFER_OK;
}

int lle_buffer_get_char(const lle_buffer_t *buffer, lle_buffer_pos_t pos, char *ch) {
    if (!buffer || !buffer->data || !ch) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    size_t size = text_size(buffer);
    if (pos >= size) {
        return LLE_BUFFER_ERR_OUT_OF_BOUNDS;
    }
    
    // Translate position to actual buffer index
    if (pos < buffer->gap_start) {
        *ch = buffer->data[pos];
    } else {
        *ch = buffer->data[pos + gap_size(buffer)];
    }
    
    return LLE_BUFFER_OK;
}

int lle_buffer_get_substring(const lle_buffer_t *buffer,
                             lle_buffer_pos_t start,
                             lle_buffer_pos_t end,
                             char *dest,
                             size_t dest_size) {
    if (!buffer || !buffer->data || !dest) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    size_t size = text_size(buffer);
    if (start > size || end > size || start > end) {
        return LLE_BUFFER_ERR_OUT_OF_BOUNDS;
    }
    
    size_t len = end - start;
    if (len >= dest_size) {
        return LLE_BUFFER_ERR_OUT_OF_BOUNDS;  // Not enough space
    }
    
    size_t dest_idx = 0;
    
    // Copy before gap
    if (start < buffer->gap_start) {
        size_t copy_len = (end <= buffer->gap_start) ? len : (buffer->gap_start - start);
        memcpy(dest + dest_idx, buffer->data + start, copy_len);
        dest_idx += copy_len;
    }
    
    // Copy after gap
    if (end > buffer->gap_start) {
        size_t after_start = (start > buffer->gap_start) ? start : buffer->gap_start;
        size_t after_len = end - after_start;
        size_t buf_idx = after_start + gap_size(buffer);
        memcpy(dest + dest_idx, buffer->data + buf_idx, after_len);
        dest_idx += after_len;
    }
    
    dest[dest_idx] = '\0';
    return LLE_BUFFER_OK;
}

int lle_buffer_get_contents(const lle_buffer_t *buffer,
                            char *dest,
                            size_t dest_size) {
    if (!buffer || !buffer->data || !dest) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    size_t size = text_size(buffer);
    if (size >= dest_size) {
        return LLE_BUFFER_ERR_OUT_OF_BOUNDS;
    }
    
    return lle_buffer_get_substring(buffer, 0, size, dest, dest_size);
}

int lle_buffer_insert_char(lle_buffer_t *buffer, lle_buffer_pos_t pos, char ch) {
    if (!buffer || !buffer->data) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    if (buffer->read_only) {
        return LLE_BUFFER_ERR_READ_ONLY;
    }
    
    size_t size = text_size(buffer);
    if (pos > size) {
        return LLE_BUFFER_ERR_INVALID_POS;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Ensure we have space
    if (gap_size(buffer) < 1) {
        int result = lle_buffer_ensure_capacity(buffer, buffer->capacity + MIN_GAP_SIZE);
        if (result != LLE_BUFFER_OK) {
            return result;
        }
    }
    
    // Move gap to insertion position
    int result = lle_buffer_move_gap(buffer, pos);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Insert character at gap start
    buffer->data[buffer->gap_start] = ch;
    buffer->gap_start++;
    buffer->modified = true;
    
    uint64_t end = get_timestamp_ns();
    buffer->total_op_time_ns += (end - start);
    buffer->insert_count++;
    
    // Update statistics
    size_t current_size = text_size(buffer);
    if (current_size > buffer->max_size) {
        buffer->max_size = current_size;
    }
    
    return LLE_BUFFER_OK;
}

int lle_buffer_insert_string(lle_buffer_t *buffer,
                             lle_buffer_pos_t pos,
                             const char *str,
                             size_t len) {
    if (!buffer || !buffer->data || !str) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    if (buffer->read_only) {
        return LLE_BUFFER_ERR_READ_ONLY;
    }
    
    if (len == 0) {
        return LLE_BUFFER_OK;
    }
    
    size_t size = text_size(buffer);
    if (pos > size) {
        return LLE_BUFFER_ERR_INVALID_POS;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Ensure we have space
    if (gap_size(buffer) < len) {
        int result = lle_buffer_ensure_capacity(buffer,
                                               buffer->capacity + len + MIN_GAP_SIZE);
        if (result != LLE_BUFFER_OK) {
            return result;
        }
    }
    
    // Move gap to insertion position
    int result = lle_buffer_move_gap(buffer, pos);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Insert string at gap start
    memcpy(buffer->data + buffer->gap_start, str, len);
    buffer->gap_start += len;
    buffer->modified = true;
    
    uint64_t end = get_timestamp_ns();
    buffer->total_op_time_ns += (end - start);
    buffer->insert_count++;
    
    // Update statistics
    size_t current_size = text_size(buffer);
    if (current_size > buffer->max_size) {
        buffer->max_size = current_size;
    }
    
    return LLE_BUFFER_OK;
}

int lle_buffer_delete_char(lle_buffer_t *buffer, lle_buffer_pos_t pos) {
    if (!buffer || !buffer->data) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    if (buffer->read_only) {
        return LLE_BUFFER_ERR_READ_ONLY;
    }
    
    size_t size = text_size(buffer);
    if (pos >= size) {
        return LLE_BUFFER_ERR_OUT_OF_BOUNDS;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Move gap to deletion position
    int result = lle_buffer_move_gap(buffer, pos);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Delete by expanding gap forward
    buffer->gap_end++;
    buffer->modified = true;
    
    uint64_t end = get_timestamp_ns();
    buffer->total_op_time_ns += (end - start);
    buffer->delete_count++;
    
    return LLE_BUFFER_OK;
}

int lle_buffer_delete_range(lle_buffer_t *buffer,
                            lle_buffer_pos_t start,
                            lle_buffer_pos_t end) {
    if (!buffer || !buffer->data) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    if (buffer->read_only) {
        return LLE_BUFFER_ERR_READ_ONLY;
    }
    
    size_t size = text_size(buffer);
    if (start > size || end > size || start > end) {
        return LLE_BUFFER_ERR_OUT_OF_BOUNDS;
    }
    
    if (start == end) {
        return LLE_BUFFER_OK;  // Nothing to delete
    }
    
    uint64_t op_start = get_timestamp_ns();
    
    // Move gap to start of deletion range
    int result = lle_buffer_move_gap(buffer, start);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Expand gap to cover range
    size_t delete_len = end - start;
    buffer->gap_end += delete_len;
    buffer->modified = true;
    
    uint64_t op_end = get_timestamp_ns();
    buffer->total_op_time_ns += (op_end - op_start);
    buffer->delete_count++;
    
    return LLE_BUFFER_OK;
}

int lle_buffer_clear(lle_buffer_t *buffer) {
    if (!buffer || !buffer->data) {
        return LLE_BUFFER_ERR_NULL_PTR;
    }
    
    if (buffer->read_only) {
        return LLE_BUFFER_ERR_READ_ONLY;
    }
    
    // Make entire buffer the gap
    buffer->gap_start = 0;
    buffer->gap_end = buffer->capacity;
    buffer->modified = true;
    
    return LLE_BUFFER_OK;
}

void lle_buffer_set_read_only(lle_buffer_t *buffer, bool read_only) {
    if (buffer) {
        buffer->read_only = read_only;
    }
}

bool lle_buffer_is_modified(const lle_buffer_t *buffer) {
    return buffer ? buffer->modified : false;
}

void lle_buffer_clear_modified(lle_buffer_t *buffer) {
    if (buffer) {
        buffer->modified = false;
    }
}

void lle_buffer_get_metrics(const lle_buffer_t *buffer,
                           uint64_t *insert_count,
                           uint64_t *delete_count,
                           uint64_t *move_count,
                           double *avg_time_us) {
    if (!buffer) {
        return;
    }
    
    if (insert_count) *insert_count = buffer->insert_count;
    if (delete_count) *delete_count = buffer->delete_count;
    if (move_count) *move_count = buffer->move_count;
    
    if (avg_time_us) {
        uint64_t total_ops = buffer->insert_count + buffer->delete_count + buffer->move_count;
        if (total_ops > 0) {
            *avg_time_us = (double)buffer->total_op_time_ns / total_ops / 1000.0;
        } else {
            *avg_time_us = 0.0;
        }
    }
}

void lle_buffer_get_stats(const lle_buffer_t *buffer,
                         size_t *current_size,
                         size_t *capacity,
                         size_t *max_size,
                         size_t *grow_count,
                         double *memory_efficiency) {
    if (!buffer) {
        return;
    }
    
    size_t size = text_size(buffer);
    
    if (current_size) *current_size = size;
    if (capacity) *capacity = buffer->capacity;
    if (max_size) *max_size = buffer->max_size;
    if (grow_count) *grow_count = buffer->grow_count;
    
    if (memory_efficiency && buffer->capacity > 0) {
        *memory_efficiency = (double)size / buffer->capacity;
    }
}

const char* lle_buffer_error_string(int error_code) {
    switch (error_code) {
        case LLE_BUFFER_OK:
            return "Success";
        case LLE_BUFFER_ERR_NULL_PTR:
            return "Null pointer argument";
        case LLE_BUFFER_ERR_INVALID_POS:
            return "Invalid position";
        case LLE_BUFFER_ERR_ALLOC_FAILED:
            return "Memory allocation failed";
        case LLE_BUFFER_ERR_OUT_OF_BOUNDS:
            return "Out of bounds";
        case LLE_BUFFER_ERR_READ_ONLY:
            return "Buffer is read-only";
        case LLE_BUFFER_ERR_EMPTY:
            return "Buffer is empty";
        default:
            return "Unknown error";
    }
}
