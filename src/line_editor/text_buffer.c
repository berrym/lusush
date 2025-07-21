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