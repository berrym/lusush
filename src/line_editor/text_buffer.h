#ifndef LLE_TEXT_BUFFER_H
#define LLE_TEXT_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file text_buffer.h
 * @brief Lusush Line Editor - Text Buffer
 *
 * Core text manipulation and storage functionality for the Lusush Line Editor.
 * Provides UTF-8 compatible text buffer operations with mathematical correctness
 * for cursor positioning and memory-efficient storage.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

/**
 * @brief Default initial capacity for text buffers (in bytes)
 */
#define LLE_DEFAULT_BUFFER_CAPACITY 256

/**
 * @brief Minimum buffer capacity (in bytes)
 */
#define LLE_MIN_BUFFER_CAPACITY 64

/**
 * @brief Maximum buffer capacity (in bytes) - 100KB limit
 */
#define LLE_MAX_BUFFER_CAPACITY (100 * 1024)

/**
 * @brief Text buffer structure for UTF-8 text storage and manipulation
 *
 * This structure maintains both byte-level and character-level information
 * to properly handle UTF-8 encoded text. The distinction between byte
 * positions and character positions is critical for mathematical correctness
 * in cursor positioning calculations.
 *
 * Memory layout is optimized for performance with frequently accessed
 * fields (length, cursor_pos) placed first.
 */
typedef struct {
    char *buffer;           /**< UTF-8 text buffer - dynamically allocated */
    size_t length;          /**< Current text length in bytes (NOT characters) */
    size_t capacity;        /**< Total buffer capacity in bytes */
    size_t cursor_pos;      /**< Cursor position as byte offset from start */
    size_t char_count;      /**< Number of Unicode characters (for display width) */
} lle_text_buffer_t;

// Function declarations will be added as tasks are implemented

// Task LLE-002: Text buffer initialization functions
/**
 * @brief Create a new text buffer with specified initial capacity
 * @param initial_capacity Initial buffer capacity in bytes
 * @return Pointer to newly created text buffer, or NULL on failure
 */
lle_text_buffer_t* lle_text_buffer_create(size_t initial_capacity);

/**
 * @brief Initialize an existing text buffer structure
 * @param buffer Pointer to text buffer structure to initialize
 * @param initial_capacity Initial buffer capacity in bytes
 * @return true on success, false on failure
 */
bool lle_text_buffer_init(lle_text_buffer_t *buffer, size_t initial_capacity);

/**
 * @brief Destroy a text buffer and free all associated memory
 * @param buffer Pointer to text buffer to destroy
 */
void lle_text_buffer_destroy(lle_text_buffer_t *buffer);

/**
 * @brief Clear the text buffer contents without deallocating memory
 * @param buffer Pointer to text buffer to clear
 */
void lle_text_buffer_clear(lle_text_buffer_t *buffer);

/**
 * @brief Check if a text buffer is valid and properly initialized
 * @param buffer Pointer to text buffer to validate
 * @return true if buffer is valid, false otherwise
 */
bool lle_text_buffer_is_valid(lle_text_buffer_t *buffer);

// Task LLE-003: Text insertion functions
/**
 * @brief Insert a single character at the cursor position
 * @param buffer Pointer to text buffer
 * @param c Character to insert
 * @return true on success, false on failure
 */
bool lle_text_insert_char(lle_text_buffer_t *buffer, char c);

/**
 * @brief Insert a string at the cursor position
 * @param buffer Pointer to text buffer
 * @param str String to insert (must be null-terminated)
 * @return true on success, false on failure
 */
bool lle_text_insert_string(lle_text_buffer_t *buffer, const char *str);

/**
 * @brief Insert a string at an arbitrary position
 * @param buffer Pointer to text buffer
 * @param pos Byte position where to insert (0-based)
 * @param str String to insert (must be null-terminated)
 * @return true on success, false on failure
 */
bool lle_text_insert_at(lle_text_buffer_t *buffer, size_t pos, const char *str);

// Task LLE-004: Text deletion functions
/**
 * @brief Delete character at the cursor position
 * @param buffer Pointer to text buffer
 * @return true on success, false on failure or no operation
 */
bool lle_text_delete_char(lle_text_buffer_t *buffer);

/**
 * @brief Delete character before the cursor position (backspace)
 * @param buffer Pointer to text buffer
 * @return true on success, false on failure or no operation
 */
bool lle_text_backspace(lle_text_buffer_t *buffer);

/**
 * @brief Delete a range of characters
 * @param buffer Pointer to text buffer
 * @param start Start position (inclusive, 0-based)
 * @param end End position (exclusive, 0-based)
 * @return true on success, false on failure
 */
bool lle_text_delete_range(lle_text_buffer_t *buffer, size_t start, size_t end);

// Task LLE-005: Cursor movement functions

#endif // LLE_TEXT_BUFFER_H
