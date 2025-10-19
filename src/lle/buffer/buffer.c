/**
 * LLE Buffer Management - Implementation
 * 
 * Week 3 Implementation: Minimal gap buffer
 * 
 * Gap Buffer Algorithm:
 * - Buffer contains a "gap" at the cursor position
 * - Text before gap: [0, gap_start)
 * - Gap itself: [gap_start, gap_end)
 * - Text after gap: [gap_end, capacity)
 * - Cursor is always at gap_start
 * - Insertions fill the gap
 * - Deletions expand the gap
 * - Cursor movement moves the gap
 * 
 * Example: "Hello" with cursor after 'e' (marked with |)
 * 
 * Memory: H e l | _ _ _ _ l o
 *         0 1 2 3 4 5 6 7 8 9
 *         gap_start=3, gap_end=8, cursor=3
 * 
 * Logical text: "Hello" (5 chars)
 * Gap size: 5 (gap_end - gap_start)
 * 
 * Spec Reference: 03_buffer_management_complete.md (Week 3 subset)
 */

#include <lle/buffer.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Buffer structure (opaque)
 */
struct lle_buffer_s {
    // Gap buffer storage
    char *data;           // Buffer data
    size_t capacity;      // Total allocated size
    size_t gap_start;     // Start of gap (also cursor position)
    size_t gap_end;       // End of gap (exclusive)
    
    // Status
    bool initialized;
};

/* Default initial capacity */
#define LLE_BUFFER_DEFAULT_CAPACITY 256

/* Minimum gap size to maintain */
#define LLE_BUFFER_MIN_GAP_SIZE 32

/* Forward declarations */
static lle_buffer_result_t ensure_gap_size(lle_buffer_t *buffer, size_t required);
static void move_gap_to_cursor(lle_buffer_t *buffer, size_t target_position);
static size_t get_text_length(const lle_buffer_t *buffer);

/**
 * Initialize buffer
 */
lle_buffer_result_t lle_buffer_init(
    lle_buffer_t **buffer,
    size_t initial_capacity)
{
    if (!buffer) {
        return LLE_BUFFER_ERROR_INVALID_PARAMETER;
    }
    
    if (initial_capacity == 0) {
        initial_capacity = LLE_BUFFER_DEFAULT_CAPACITY;
    }
    
    /* Allocate buffer structure */
    lle_buffer_t *buf = calloc(1, sizeof(lle_buffer_t));
    if (!buf) {
        return LLE_BUFFER_ERROR_MEMORY;
    }
    
    /* Allocate buffer data */
    buf->data = malloc(initial_capacity);
    if (!buf->data) {
        free(buf);
        return LLE_BUFFER_ERROR_MEMORY;
    }
    
    /* Initialize gap buffer: entire buffer is gap initially */
    buf->capacity = initial_capacity;
    buf->gap_start = 0;
    buf->gap_end = initial_capacity;
    buf->initialized = true;
    
    *buffer = buf;
    return LLE_BUFFER_SUCCESS;
}

/**
 * Insert text at cursor
 */
lle_buffer_result_t lle_buffer_insert(
    lle_buffer_t *buffer,
    const char *text)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    if (!text) {
        return LLE_BUFFER_ERROR_INVALID_PARAMETER;
    }
    
    size_t text_len = strlen(text);
    if (text_len == 0) {
        return LLE_BUFFER_SUCCESS;
    }
    
    /* Ensure gap is large enough */
    lle_buffer_result_t result = ensure_gap_size(buffer, text_len);
    if (result != LLE_BUFFER_SUCCESS) {
        return result;
    }
    
    /* Copy text into gap */
    memcpy(buffer->data + buffer->gap_start, text, text_len);
    buffer->gap_start += text_len;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Insert single character
 */
lle_buffer_result_t lle_buffer_insert_char(
    lle_buffer_t *buffer,
    char ch)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    /* Ensure gap has space for 1 character */
    lle_buffer_result_t result = ensure_gap_size(buffer, 1);
    if (result != LLE_BUFFER_SUCCESS) {
        return result;
    }
    
    /* Insert character into gap */
    buffer->data[buffer->gap_start] = ch;
    buffer->gap_start++;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Delete character before cursor (backspace)
 */
lle_buffer_result_t lle_buffer_delete_before_cursor(
    lle_buffer_t *buffer)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    /* Can't delete if at start of buffer */
    if (buffer->gap_start == 0) {
        return LLE_BUFFER_SUCCESS; /* Not an error, just nothing to delete */
    }
    
    /* Expand gap backward (effectively deleting the character) */
    buffer->gap_start--;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Delete character at cursor (delete key)
 */
lle_buffer_result_t lle_buffer_delete_at_cursor(
    lle_buffer_t *buffer)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    /* Can't delete if at end of buffer */
    if (buffer->gap_end >= buffer->capacity) {
        return LLE_BUFFER_SUCCESS; /* Nothing to delete */
    }
    
    /* Expand gap forward (effectively deleting the character) */
    buffer->gap_end++;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Move cursor left
 */
lle_buffer_result_t lle_buffer_move_cursor_left(
    lle_buffer_t *buffer)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    /* Can't move left if at start */
    if (buffer->gap_start == 0) {
        return LLE_BUFFER_SUCCESS; /* Not an error */
    }
    
    /* Move gap left by moving character from before gap to after gap */
    buffer->gap_end--;
    buffer->gap_start--;
    buffer->data[buffer->gap_end] = buffer->data[buffer->gap_start];
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Move cursor right
 */
lle_buffer_result_t lle_buffer_move_cursor_right(
    lle_buffer_t *buffer)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    /* Can't move right if at end */
    if (buffer->gap_end >= buffer->capacity) {
        return LLE_BUFFER_SUCCESS; /* Not an error */
    }
    
    /* Move gap right by moving character from after gap to before gap */
    buffer->data[buffer->gap_start] = buffer->data[buffer->gap_end];
    buffer->gap_start++;
    buffer->gap_end++;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Move cursor to start (Home)
 */
lle_buffer_result_t lle_buffer_move_cursor_home(
    lle_buffer_t *buffer)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    move_gap_to_cursor(buffer, 0);
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Move cursor to end (End)
 */
lle_buffer_result_t lle_buffer_move_cursor_end(
    lle_buffer_t *buffer)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    size_t text_len = get_text_length(buffer);
    move_gap_to_cursor(buffer, text_len);
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Set cursor to specific position
 */
lle_buffer_result_t lle_buffer_set_cursor(
    lle_buffer_t *buffer,
    size_t position)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    size_t text_len = get_text_length(buffer);
    if (position > text_len) {
        return LLE_BUFFER_ERROR_INVALID_POSITION;
    }
    
    move_gap_to_cursor(buffer, position);
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Get cursor position
 */
lle_buffer_result_t lle_buffer_get_cursor(
    const lle_buffer_t *buffer,
    size_t *position)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    if (!position) {
        return LLE_BUFFER_ERROR_INVALID_PARAMETER;
    }
    
    /* Cursor is always at gap_start */
    *position = buffer->gap_start;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Get buffer content length
 */
lle_buffer_result_t lle_buffer_get_length(
    const lle_buffer_t *buffer,
    size_t *length)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    if (!length) {
        return LLE_BUFFER_ERROR_INVALID_PARAMETER;
    }
    
    *length = get_text_length(buffer);
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Get buffer content as string
 */
lle_buffer_result_t lle_buffer_get_content(
    const lle_buffer_t *buffer,
    char *output,
    size_t output_size)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    if (!output || output_size == 0) {
        return LLE_BUFFER_ERROR_INVALID_PARAMETER;
    }
    
    size_t text_len = get_text_length(buffer);
    
    /* Check if output buffer is large enough */
    if (text_len + 1 > output_size) {
        return LLE_BUFFER_ERROR_BUFFER_FULL;
    }
    
    /* Copy text before gap */
    if (buffer->gap_start > 0) {
        memcpy(output, buffer->data, buffer->gap_start);
    }
    
    /* Copy text after gap */
    size_t after_gap_len = buffer->capacity - buffer->gap_end;
    if (after_gap_len > 0) {
        memcpy(output + buffer->gap_start, 
               buffer->data + buffer->gap_end, 
               after_gap_len);
    }
    
    /* Null-terminate */
    output[text_len] = '\0';
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Clear buffer content
 */
lle_buffer_result_t lle_buffer_clear(
    lle_buffer_t *buffer)
{
    if (!buffer || !buffer->initialized) {
        return LLE_BUFFER_ERROR_NOT_INITIALIZED;
    }
    
    /* Reset gap to cover entire buffer */
    buffer->gap_start = 0;
    buffer->gap_end = buffer->capacity;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Destroy buffer
 */
void lle_buffer_destroy(
    lle_buffer_t *buffer)
{
    if (!buffer) {
        return;
    }
    
    free(buffer->data);
    free(buffer);
}

/**
 * Get error message string
 */
const char *lle_buffer_error_string(
    lle_buffer_result_t result)
{
    switch (result) {
        case LLE_BUFFER_SUCCESS:
            return "Success";
        case LLE_BUFFER_ERROR_MEMORY:
            return "Memory allocation failed";
        case LLE_BUFFER_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case LLE_BUFFER_ERROR_NOT_INITIALIZED:
            return "Buffer not initialized";
        case LLE_BUFFER_ERROR_BUFFER_FULL:
            return "Buffer full";
        case LLE_BUFFER_ERROR_INVALID_POSITION:
            return "Invalid cursor position";
        default:
            return "Unknown error";
    }
}

/* ========================================================================
 * Internal Helper Functions
 * ======================================================================== */

/**
 * Ensure gap has at least required size
 * 
 * Expands buffer if gap is too small.
 */
static lle_buffer_result_t ensure_gap_size(lle_buffer_t *buffer, size_t required)
{
    size_t current_gap_size = buffer->gap_end - buffer->gap_start;
    
    if (current_gap_size >= required) {
        return LLE_BUFFER_SUCCESS;
    }
    
    /* Need to grow buffer */
    size_t text_len = get_text_length(buffer);
    size_t new_gap_size = required + LLE_BUFFER_MIN_GAP_SIZE;
    size_t new_capacity = text_len + new_gap_size;
    
    /* Allocate new buffer */
    char *new_data = malloc(new_capacity);
    if (!new_data) {
        return LLE_BUFFER_ERROR_MEMORY;
    }
    
    /* Copy text before gap */
    if (buffer->gap_start > 0) {
        memcpy(new_data, buffer->data, buffer->gap_start);
    }
    
    /* Copy text after gap */
    size_t after_gap_len = buffer->capacity - buffer->gap_end;
    if (after_gap_len > 0) {
        memcpy(new_data + buffer->gap_start + new_gap_size,
               buffer->data + buffer->gap_end,
               after_gap_len);
    }
    
    /* Update buffer */
    free(buffer->data);
    buffer->data = new_data;
    buffer->capacity = new_capacity;
    buffer->gap_end = buffer->gap_start + new_gap_size;
    
    return LLE_BUFFER_SUCCESS;
}

/**
 * Move gap to target cursor position
 * 
 * This is used for large cursor movements (Home, End, set_cursor)
 * For single-character movements, use move_cursor_left/right for efficiency.
 */
static void move_gap_to_cursor(lle_buffer_t *buffer, size_t target_position)
{
    if (buffer->gap_start == target_position) {
        return; /* Already there */
    }
    
    if (target_position < buffer->gap_start) {
        /* Move gap left */
        size_t move_count = buffer->gap_start - target_position;
        memmove(buffer->data + buffer->gap_end - move_count,
                buffer->data + target_position,
                move_count);
        buffer->gap_start = target_position;
        buffer->gap_end -= move_count;
    } else {
        /* Move gap right */
        size_t move_count = target_position - buffer->gap_start;
        memmove(buffer->data + buffer->gap_start,
                buffer->data + buffer->gap_end,
                move_count);
        buffer->gap_start = target_position;
        buffer->gap_end += move_count;
    }
}

/**
 * Get length of text in buffer (excluding gap)
 */
static size_t get_text_length(const lle_buffer_t *buffer)
{
    return buffer->gap_start + (buffer->capacity - buffer->gap_end);
}
