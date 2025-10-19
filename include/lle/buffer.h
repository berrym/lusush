/**
 * LLE Buffer Management - Public API
 * 
 * Week 3 Implementation: Minimal gap buffer
 * 
 * Goal: Simple text buffer with basic editing operations
 *       that integrates with Week 2 display system.
 * 
 * Design Principles (MANDATORY):
 * - Simple gap buffer implementation
 * - ASCII only for Week 3 (UTF-8 in future weeks)
 * - Single line only for Week 3 (multiline in future weeks)
 * - No undo/redo yet (Week 5+)
 * - Focus on correctness, not optimization
 * 
 * Spec Reference: 03_buffer_management_complete.md (Week 3 minimal subset)
 */

#ifndef LLE_BUFFER_H
#define LLE_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Buffer handle (opaque)
 */
typedef struct lle_buffer_s lle_buffer_t;

/**
 * Buffer operation result codes
 */
typedef enum {
    LLE_BUFFER_SUCCESS = 0,
    LLE_BUFFER_ERROR_MEMORY,
    LLE_BUFFER_ERROR_INVALID_PARAMETER,
    LLE_BUFFER_ERROR_NOT_INITIALIZED,
    LLE_BUFFER_ERROR_BUFFER_FULL,
    LLE_BUFFER_ERROR_INVALID_POSITION,
} lle_buffer_result_t;

/**
 * Initialize buffer with initial capacity
 * 
 * Creates a gap buffer for text editing.
 * 
 * @param buffer Output: Buffer handle (allocated by function)
 * @param initial_capacity Initial buffer capacity (bytes)
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_init(
    lle_buffer_t **buffer,
    size_t initial_capacity
);

/**
 * Insert text at cursor position
 * 
 * Inserts text at the current cursor position and advances cursor.
 * 
 * @param buffer Buffer handle
 * @param text Text to insert (NULL-terminated)
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_insert(
    lle_buffer_t *buffer,
    const char *text
);

/**
 * Insert single character at cursor position
 * 
 * @param buffer Buffer handle
 * @param ch Character to insert
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_insert_char(
    lle_buffer_t *buffer,
    char ch
);

/**
 * Delete character before cursor (backspace)
 * 
 * Deletes the character before the cursor and moves cursor back.
 * 
 * @param buffer Buffer handle
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_delete_before_cursor(
    lle_buffer_t *buffer
);

/**
 * Delete character at cursor (delete key)
 * 
 * Deletes the character at the cursor position.
 * 
 * @param buffer Buffer handle
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_delete_at_cursor(
    lle_buffer_t *buffer
);

/**
 * Move cursor left
 * 
 * Moves cursor one position to the left.
 * 
 * @param buffer Buffer handle
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_move_cursor_left(
    lle_buffer_t *buffer
);

/**
 * Move cursor right
 * 
 * Moves cursor one position to the right.
 * 
 * @param buffer Buffer handle
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_move_cursor_right(
    lle_buffer_t *buffer
);

/**
 * Move cursor to start of buffer (Home)
 * 
 * @param buffer Buffer handle
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_move_cursor_home(
    lle_buffer_t *buffer
);

/**
 * Move cursor to end of buffer (End)
 * 
 * @param buffer Buffer handle
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_move_cursor_end(
    lle_buffer_t *buffer
);

/**
 * Set cursor to specific position
 * 
 * @param buffer Buffer handle
 * @param position Position to set cursor (0-based)
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_set_cursor(
    lle_buffer_t *buffer,
    size_t position
);

/**
 * Get cursor position
 * 
 * @param buffer Buffer handle
 * @param position Output: Current cursor position
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_get_cursor(
    const lle_buffer_t *buffer,
    size_t *position
);

/**
 * Get buffer content length
 * 
 * Returns the length of text in the buffer (excluding gap).
 * 
 * @param buffer Buffer handle
 * @param length Output: Content length in bytes
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_get_length(
    const lle_buffer_t *buffer,
    size_t *length
);

/**
 * Get buffer content as string
 * 
 * Copies buffer content (excluding gap) to output buffer.
 * 
 * @param buffer Buffer handle
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_get_content(
    const lle_buffer_t *buffer,
    char *output,
    size_t output_size
);

/**
 * Clear buffer content
 * 
 * Removes all content from buffer and resets cursor to 0.
 * 
 * @param buffer Buffer handle
 * @return LLE_BUFFER_SUCCESS or error code
 */
lle_buffer_result_t lle_buffer_clear(
    lle_buffer_t *buffer
);

/**
 * Destroy buffer
 * 
 * @param buffer Buffer handle to destroy
 */
void lle_buffer_destroy(
    lle_buffer_t *buffer
);

/**
 * Get human-readable error message
 * 
 * @param result Result code
 * @return Error message string (static, do not free)
 */
const char *lle_buffer_error_string(
    lle_buffer_result_t result
);

#endif /* LLE_BUFFER_H */
