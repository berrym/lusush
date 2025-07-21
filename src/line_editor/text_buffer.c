/**
 * @file text_buffer.c
 * @brief Lusush Line Editor - Text Buffer Implementation
 *
 * Core text manipulation and storage functionality for the Lusush Line Editor.
 * Provides UTF-8 compatible text buffer operations with mathematical correctness
 * for cursor positioning and memory-efficient storage.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "text_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * @brief Create a new text buffer with specified initial capacity
 *
 * Allocates memory for a new text buffer structure and initializes it
 * with the specified capacity. The capacity will be adjusted to meet
 * minimum requirements if necessary.
 *
 * @param initial_capacity Initial buffer capacity in bytes (minimum LLE_MIN_BUFFER_CAPACITY)
 * @return Pointer to newly created text buffer, or NULL on failure
 *
 * @note The returned buffer must be freed with lle_text_buffer_destroy()
 * @note Capacity will be rounded up to LLE_MIN_BUFFER_CAPACITY if smaller
 */
lle_text_buffer_t* lle_text_buffer_create(size_t initial_capacity) {
    // Allocate the buffer structure
    lle_text_buffer_t *buffer = malloc(sizeof(lle_text_buffer_t));
    if (!buffer) {
        return NULL;
    }

    // Initialize the buffer using the init function
    if (!lle_text_buffer_init(buffer, initial_capacity)) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

/**
 * @brief Initialize an existing text buffer structure
 *
 * Initializes an already allocated text buffer structure with the specified
 * capacity. This function can be used to reinitialize a buffer or to
 * initialize a buffer that was allocated on the stack.
 *
 * @param buffer Pointer to text buffer structure to initialize
 * @param initial_capacity Initial buffer capacity in bytes
 * @return true on success, false on failure
 *
 * @note If buffer is NULL, this function returns false
 * @note Capacity will be adjusted to meet minimum/maximum limits
 */
bool lle_text_buffer_init(lle_text_buffer_t *buffer, size_t initial_capacity) {
    if (!buffer) {
        return false;
    }

    // Adjust capacity to meet constraints
    if (initial_capacity < LLE_MIN_BUFFER_CAPACITY) {
        initial_capacity = LLE_MIN_BUFFER_CAPACITY;
    }
    if (initial_capacity > LLE_MAX_BUFFER_CAPACITY) {
        initial_capacity = LLE_MAX_BUFFER_CAPACITY;
    }

    // Allocate the text buffer
    buffer->buffer = malloc(initial_capacity);
    if (!buffer->buffer) {
        return false;
    }

    // Initialize all fields
    buffer->capacity = initial_capacity;
    buffer->length = 0;
    buffer->cursor_pos = 0;
    buffer->char_count = 0;

    // Zero-initialize the buffer for safety
    memset(buffer->buffer, 0, initial_capacity);

    return true;
}

/**
 * @brief Destroy a text buffer and free all associated memory
 *
 * Frees the internal buffer memory and then frees the text buffer
 * structure itself. After calling this function, the buffer pointer
 * should not be used.
 *
 * @param buffer Pointer to text buffer to destroy
 *
 * @note Safe to call with NULL pointer (no-op)
 * @note After destruction, the buffer pointer becomes invalid
 */
void lle_text_buffer_destroy(lle_text_buffer_t *buffer) {
    if (!buffer) {
        return;
    }

    // Free the internal text buffer
    if (buffer->buffer) {
        free(buffer->buffer);
        buffer->buffer = NULL;
    }

    // Clear other fields for safety
    buffer->length = 0;
    buffer->capacity = 0;
    buffer->cursor_pos = 0;
    buffer->char_count = 0;

    // Free the structure itself
    free(buffer);
}

/**
 * @brief Clear the text buffer contents without deallocating memory
 *
 * Resets the buffer to an empty state while preserving the allocated
 * memory. This is more efficient than destroying and recreating the
 * buffer when you need to start fresh.
 *
 * @param buffer Pointer to text buffer to clear
 *
 * @note Safe to call with NULL pointer (no-op)
 * @note Buffer capacity remains unchanged
 * @note Buffer memory is zeroed for safety
 */
void lle_text_buffer_clear(lle_text_buffer_t *buffer) {
    if (!buffer || !buffer->buffer) {
        return;
    }

    // Reset all state fields
    buffer->length = 0;
    buffer->cursor_pos = 0;
    buffer->char_count = 0;

    // Zero the buffer for safety and consistency
    memset(buffer->buffer, 0, buffer->capacity);
}

/**
 * @brief Check if a text buffer is valid and properly initialized
 *
 * Validates that a text buffer structure is in a consistent state
 * with proper memory allocation and valid field values.
 *
 * @param buffer Pointer to text buffer to validate
 * @return true if buffer is valid, false otherwise
 *
 * @note This function is useful for debugging and assertions
 */
bool lle_text_buffer_is_valid(lle_text_buffer_t *buffer) {
    if (!buffer) {
        return false;
    }

    // Check for valid buffer allocation
    if (!buffer->buffer) {
        return false;
    }

    // Check capacity constraints
    if (buffer->capacity < LLE_MIN_BUFFER_CAPACITY || 
        buffer->capacity > LLE_MAX_BUFFER_CAPACITY) {
        return false;
    }

    // Check length constraints
    if (buffer->length > buffer->capacity) {
        return false;
    }

    // Check cursor position constraints
    if (buffer->cursor_pos > buffer->length) {
        return false;
    }

    return true;
}

/**
 * @brief Resize text buffer to accommodate new capacity
 *
 * Internal helper function to resize the buffer when more space is needed.
 * Ensures the new capacity meets minimum requirements and doesn't exceed limits.
 *
 * @param buffer Pointer to text buffer to resize
 * @param new_capacity New desired capacity in bytes
 * @return true on success, false on failure
 *
 * @note This function preserves existing content
 * @note New capacity is adjusted to meet constraints
 */
static bool lle_text_buffer_resize(lle_text_buffer_t *buffer, size_t new_capacity) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    // Adjust capacity to meet constraints
    if (new_capacity < buffer->length) {
        new_capacity = buffer->length + LLE_MIN_BUFFER_CAPACITY;
    }
    if (new_capacity < LLE_MIN_BUFFER_CAPACITY) {
        new_capacity = LLE_MIN_BUFFER_CAPACITY;
    }
    if (new_capacity > LLE_MAX_BUFFER_CAPACITY) {
        return false; // Cannot exceed maximum capacity
    }

    // Reallocate buffer
    char *new_buffer = realloc(buffer->buffer, new_capacity);
    if (!new_buffer) {
        return false;
    }

    // Update buffer structure
    buffer->buffer = new_buffer;
    
    // Zero-initialize new space
    if (new_capacity > buffer->capacity) {
        memset(buffer->buffer + buffer->capacity, 0, new_capacity - buffer->capacity);
    }
    
    buffer->capacity = new_capacity;
    return true;
}

/**
 * @brief Insert a single character at the cursor position
 *
 * Inserts a character at the current cursor position, shifting existing
 * text to the right. The cursor advances by one position after insertion.
 * Buffer automatically resizes if needed.
 *
 * @param buffer Pointer to text buffer
 * @param c Character to insert
 * @return true on success, false on failure
 *
 * @note For ASCII characters only - UTF-8 support in later tasks
 * @note Buffer will resize automatically if space is needed
 */
bool lle_text_insert_char(lle_text_buffer_t *buffer, char c) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    // Validate cursor position
    if (buffer->cursor_pos > buffer->length) {
        return false;
    }

    // Check if we need to resize (need space for new char + null terminator)
    if (buffer->length + 1 + 1 > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        if (!lle_text_buffer_resize(buffer, new_capacity)) {
            return false;
        }
    }

    // Shift existing text to the right if needed
    if (buffer->cursor_pos < buffer->length) {
        memmove(buffer->buffer + buffer->cursor_pos + 1,
                buffer->buffer + buffer->cursor_pos,
                buffer->length - buffer->cursor_pos);
    }

    // Insert the character
    buffer->buffer[buffer->cursor_pos] = c;
    
    // Update counters
    buffer->length++;
    buffer->cursor_pos++;
    buffer->char_count++; // For ASCII, byte count equals character count
    
    // Ensure null termination
    buffer->buffer[buffer->length] = '\0';

    return true;
}

/**
 * @brief Insert a string at the cursor position
 *
 * Inserts a null-terminated string at the current cursor position,
 * shifting existing text to the right. The cursor advances by the
 * length of the inserted string.
 *
 * @param buffer Pointer to text buffer
 * @param str String to insert (must be null-terminated)
 * @return true on success, false on failure
 *
 * @note String must be valid and null-terminated
 * @note Buffer will resize automatically if space is needed
 */
bool lle_text_insert_string(lle_text_buffer_t *buffer, const char *str) {
    if (!buffer || !buffer->buffer || !str) {
        return false;
    }

    // Validate cursor position
    if (buffer->cursor_pos > buffer->length) {
        return false;
    }

    size_t str_len = strlen(str);
    if (str_len == 0) {
        return true; // Nothing to insert, but not an error
    }

    // Check if we need to resize (need space for string + null terminator)
    if (buffer->length + str_len + 1 > buffer->capacity) {
        size_t new_capacity = buffer->capacity;
        while (buffer->length + str_len + 1 > new_capacity) {
            new_capacity *= 2;
        }
        if (!lle_text_buffer_resize(buffer, new_capacity)) {
            return false;
        }
    }

    // Shift existing text to the right if needed
    if (buffer->cursor_pos < buffer->length) {
        memmove(buffer->buffer + buffer->cursor_pos + str_len,
                buffer->buffer + buffer->cursor_pos,
                buffer->length - buffer->cursor_pos);
    }

    // Insert the string
    memcpy(buffer->buffer + buffer->cursor_pos, str, str_len);
    
    // Update counters
    buffer->length += str_len;
    buffer->cursor_pos += str_len;
    buffer->char_count += str_len; // For ASCII, byte count equals character count
    
    // Ensure null termination
    buffer->buffer[buffer->length] = '\0';

    return true;
}

/**
 * @brief Insert a string at an arbitrary position
 *
 * Inserts a null-terminated string at the specified position,
 * shifting existing text to the right. The cursor position is
 * not automatically updated.
 *
 * @param buffer Pointer to text buffer
 * @param pos Byte position where to insert (0-based)
 * @param str String to insert (must be null-terminated)
 * @return true on success, false on failure
 *
 * @note Position must be valid (0 <= pos <= length)
 * @note Cursor position remains unchanged
 * @note Buffer will resize automatically if space is needed
 */
bool lle_text_insert_at(lle_text_buffer_t *buffer, size_t pos, const char *str) {
    if (!buffer || !buffer->buffer || !str) {
        return false;
    }

    // Validate position
    if (pos > buffer->length) {
        return false;
    }

    size_t str_len = strlen(str);
    if (str_len == 0) {
        return true; // Nothing to insert, but not an error
    }

    // Check if we need to resize (need space for string + null terminator)
    if (buffer->length + str_len + 1 > buffer->capacity) {
        size_t new_capacity = buffer->capacity;
        while (buffer->length + str_len + 1 > new_capacity) {
            new_capacity *= 2;
        }
        if (!lle_text_buffer_resize(buffer, new_capacity)) {
            return false;
        }
    }

    // Shift existing text to the right if needed
    if (pos < buffer->length) {
        memmove(buffer->buffer + pos + str_len,
                buffer->buffer + pos,
                buffer->length - pos);
    }

    // Insert the string
    memcpy(buffer->buffer + pos, str, str_len);
    
    // Update counters
    buffer->length += str_len;
    buffer->char_count += str_len; // For ASCII, byte count equals character count
    
    // Update cursor position if insertion was before cursor
    if (pos <= buffer->cursor_pos) {
        buffer->cursor_pos += str_len;
    }
    
    // Ensure null termination
    buffer->buffer[buffer->length] = '\0';

    return true;
}

/**
 * @brief Delete character at the cursor position
 *
 * Deletes the character at the current cursor position, shifting all
 * subsequent text to the left. The cursor position remains unchanged.
 * If the cursor is at the end of the buffer, no deletion occurs.
 *
 * @param buffer Pointer to text buffer
 * @return true on success, false on failure or no operation
 *
 * @note Cursor position remains unchanged after deletion
 * @note No operation if cursor is at end of buffer
 */
bool lle_text_delete_char(lle_text_buffer_t *buffer) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    // Validate cursor position
    if (buffer->cursor_pos > buffer->length) {
        return false;
    }

    // Nothing to delete if cursor is at end of buffer
    if (buffer->cursor_pos == buffer->length) {
        return false;
    }

    // Shift text to the left
    memmove(buffer->buffer + buffer->cursor_pos,
            buffer->buffer + buffer->cursor_pos + 1,
            buffer->length - buffer->cursor_pos - 1);

    // Update counters
    buffer->length--;
    buffer->char_count--; // For ASCII, byte count equals character count

    // Ensure null termination
    buffer->buffer[buffer->length] = '\0';

    return true;
}

/**
 * @brief Delete character before the cursor position (backspace)
 *
 * Deletes the character immediately before the cursor position,
 * shifting all subsequent text to the left. The cursor moves back
 * by one position. If the cursor is at the beginning, no deletion occurs.
 *
 * @param buffer Pointer to text buffer
 * @return true on success, false on failure or no operation
 *
 * @note Cursor position moves back by one after successful deletion
 * @note No operation if cursor is at beginning of buffer
 */
bool lle_text_backspace(lle_text_buffer_t *buffer) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    // Validate cursor position
    if (buffer->cursor_pos > buffer->length) {
        return false;
    }

    // Nothing to delete if cursor is at beginning
    if (buffer->cursor_pos == 0) {
        return false;
    }

    // Move cursor back first
    buffer->cursor_pos--;

    // Shift text to the left
    if (buffer->cursor_pos < buffer->length - 1) {
        memmove(buffer->buffer + buffer->cursor_pos,
                buffer->buffer + buffer->cursor_pos + 1,
                buffer->length - buffer->cursor_pos - 1);
    }

    // Update counters
    buffer->length--;
    buffer->char_count--; // For ASCII, byte count equals character count

    // Ensure null termination
    buffer->buffer[buffer->length] = '\0';

    return true;
}

/**
 * @brief Delete a range of characters
 *
 * Deletes characters from start position to end position (exclusive),
 * shifting all subsequent text to the left. The cursor position is
 * adjusted if it falls within or after the deleted range.
 *
 * @param buffer Pointer to text buffer
 * @param start Start position (inclusive, 0-based)
 * @param end End position (exclusive, 0-based)
 * @return true on success, false on failure
 *
 * @note Range is [start, end) - start inclusive, end exclusive
 * @note Cursor position is adjusted if affected by deletion
 * @note start must be <= end and both within buffer bounds
 */
bool lle_text_delete_range(lle_text_buffer_t *buffer, size_t start, size_t end) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    // Validate range
    if (start > end || start > buffer->length || end > buffer->length) {
        return false;
    }

    // Nothing to delete if range is empty
    if (start == end) {
        return true;
    }

    size_t delete_length = end - start;

    // Shift text to the left if there's text after the range
    if (end < buffer->length) {
        memmove(buffer->buffer + start,
                buffer->buffer + end,
                buffer->length - end);
    }

    // Update counters
    buffer->length -= delete_length;
    buffer->char_count -= delete_length; // For ASCII, byte count equals character count

    // Adjust cursor position if needed
    if (buffer->cursor_pos >= end) {
        // Cursor was after the deleted range - move it back
        buffer->cursor_pos -= delete_length;
    } else if (buffer->cursor_pos > start) {
        // Cursor was within the deleted range - move to start
        buffer->cursor_pos = start;
    }
    // If cursor was before the range, it remains unchanged

    // Ensure null termination
    buffer->buffer[buffer->length] = '\0';

    return true;
}