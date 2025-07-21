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
#include "unicode.h"
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
bool lle_text_buffer_is_valid(const lle_text_buffer_t *buffer) {
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

/**
 * @brief Check if a character is a word boundary
 *
 * Helper function to determine if a character should be considered
 * a word boundary for word-based cursor movement.
 *
 * @param c Character to check
 * @return true if character is a word boundary, false otherwise
 *
 * @note Word boundaries include whitespace and common punctuation
 */
static bool lle_is_word_boundary(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == '.' || c == ',' || c == ';' || c == ':' ||
           c == '!' || c == '?' || c == '"' || c == '\'' ||
           c == '(' || c == ')' || c == '[' || c == ']' ||
           c == '{' || c == '}' || c == '<' || c == '>' ||
           c == '/' || c == '\\' || c == '|' || c == '@' ||
           c == '#' || c == '$' || c == '%' || c == '^' ||
           c == '&' || c == '*' || c == '+' || c == '=' ||
           c == '-' || c == '_' || c == '`' || c == '~';
}

/**
 * @brief Check if a UTF-8 character at a position is a word boundary
 *
 * Unicode-aware word boundary detection that handles multibyte characters.
 * Considers ASCII punctuation, whitespace, and non-alphanumeric Unicode
 * characters as word boundaries.
 *
 * @param text UTF-8 text buffer
 * @param byte_pos Byte position in the text
 * @return true if character at position is a word boundary
 */
static bool lle_is_unicode_word_boundary(const char *text, size_t byte_pos) {
    if (!text || text[byte_pos] == '\0') {
        return true; // End of string is a boundary
    }
    
    // Get the character bytes at this position
    size_t char_bytes = lle_utf8_char_bytes(text, byte_pos);
    
    if (char_bytes == 0) {
        return true; // Invalid UTF-8 is considered a boundary
    }
    
    if (char_bytes == 1) {
        // ASCII character - use existing boundary detection
        return lle_is_word_boundary(text[byte_pos]);
    }
    
    // For multibyte UTF-8 characters, use basic heuristics
    uint8_t first_byte = (uint8_t)text[byte_pos];
    
    // CJK characters (rough detection) - each character is its own word
    if (char_bytes == 3) {
        // Common CJK ranges (simplified detection)
        if (first_byte >= 0xE4 && first_byte <= 0xE9) {
            return true; // CJK characters are word boundaries
        }
    }
    
    // For other multibyte characters (accented Latin, etc.), 
    // assume they are word characters (not boundaries)
    return false;
}

/**
 * @brief Find the start of the previous word
 *
 * Helper function to find the beginning of the word before the cursor.
 * Skips over whitespace and punctuation to find actual word content.
 *
 * @param buffer Pointer to text buffer
 * @param from_pos Starting position to search backwards from
 * @return Position of previous word start, or 0 if at beginning
 */
static size_t lle_find_prev_word_start(lle_text_buffer_t *buffer, size_t from_pos) {
    if (!buffer || !buffer->buffer || from_pos == 0) {
        return 0;
    }

    size_t pos = from_pos;
    
    // Move back by one character (Unicode-aware)
    if (pos > 0) {
        pos = lle_utf8_prev_char(buffer->buffer, pos);
    }
    
    // Skip any word boundaries (whitespace, punctuation)
    while (pos > 0 && lle_is_unicode_word_boundary(buffer->buffer, pos)) {
        pos = lle_utf8_prev_char(buffer->buffer, pos);
    }
    
    // Now find the start of this word (Unicode-aware)
    while (pos > 0) {
        size_t prev_pos = lle_utf8_prev_char(buffer->buffer, pos);
        if (lle_is_unicode_word_boundary(buffer->buffer, prev_pos)) {
            break;
        }
        pos = prev_pos;
    }
    
    return pos;
}

/**
 * @brief Find the start of the next word
 *
 * Helper function to find the beginning of the word after the cursor.
 * Skips over current word and whitespace to find next word content.
 *
 * @param buffer Pointer to text buffer
 * @param from_pos Starting position to search forwards from
 * @return Position of next word start, or buffer length if at end
 */
static size_t lle_find_next_word_start(lle_text_buffer_t *buffer, size_t from_pos) {
    if (!buffer || !buffer->buffer || from_pos >= buffer->length) {
        return buffer ? buffer->length : 0;
    }

    size_t pos = from_pos;
    
    // Skip current word (non-boundary characters) - Unicode-aware
    while (pos < buffer->length && !lle_is_unicode_word_boundary(buffer->buffer, pos)) {
        pos = lle_utf8_next_char(buffer->buffer, pos);
    }
    
    // Skip word boundaries (whitespace, punctuation) - Unicode-aware
    while (pos < buffer->length && lle_is_unicode_word_boundary(buffer->buffer, pos)) {
        pos = lle_utf8_next_char(buffer->buffer, pos);
    }
    
    return pos;
}

/**
 * @brief Move cursor within the text buffer
 *
 * Moves the cursor according to the specified movement type.
 * Performs bounds checking to ensure cursor stays within valid range.
 *
 * @param buffer Pointer to text buffer
 * @param movement Type of cursor movement to perform
 * @return true on success, false on failure or no movement possible
 *
 * @note Movement is bounded by buffer content (0 <= cursor <= length)
 * @note Word movement uses common word boundary characters
 */
bool lle_text_move_cursor(lle_text_buffer_t *buffer, lle_cursor_movement_t movement) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    // Validate current cursor position
    if (buffer->cursor_pos > buffer->length) {
        return false;
    }

    size_t new_pos = buffer->cursor_pos;
    bool moved = false;

    switch (movement) {
        case LLE_MOVE_LEFT:
            if (buffer->cursor_pos > 0) {
                new_pos = lle_utf8_prev_char(buffer->buffer, buffer->cursor_pos);
                moved = (new_pos != buffer->cursor_pos);
            }
            break;

        case LLE_MOVE_RIGHT:
            if (buffer->cursor_pos < buffer->length) {
                new_pos = lle_utf8_next_char(buffer->buffer, buffer->cursor_pos);
                moved = (new_pos != buffer->cursor_pos);
            }
            break;

        case LLE_MOVE_HOME:
            if (buffer->cursor_pos != 0) {
                new_pos = 0;
                moved = true;
            }
            break;

        case LLE_MOVE_END:
            if (buffer->cursor_pos != buffer->length) {
                new_pos = buffer->length;
                moved = true;
            }
            break;

        case LLE_MOVE_WORD_LEFT:
            new_pos = lle_find_prev_word_start(buffer, buffer->cursor_pos);
            moved = (new_pos != buffer->cursor_pos);
            break;

        case LLE_MOVE_WORD_RIGHT:
            new_pos = lle_find_next_word_start(buffer, buffer->cursor_pos);
            moved = (new_pos != buffer->cursor_pos);
            break;

        default:
            return false; // Invalid movement type
    }

    if (moved) {
        buffer->cursor_pos = new_pos;
        return true;
    }

    return false; // No movement occurred
}

/**
 * @brief Set cursor to a specific position
 *
 * Sets the cursor to the specified position with bounds checking.
 * Position must be within valid range (0 to buffer length inclusive).
 *
 * @param buffer Pointer to text buffer
 * @param position New cursor position (0-based byte offset)
 * @return true on success, false on failure
 *
 * @note Position is clamped to valid range [0, buffer->length]
 * @note Position at buffer->length is valid (cursor after last character)
 */
bool lle_text_set_cursor(lle_text_buffer_t *buffer, size_t position) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    // Validate and clamp position to valid range
    if (position > buffer->length) {
        return false;
    }

    buffer->cursor_pos = position;
    return true;
}

/**
 * @brief Get cursor position in Unicode characters (not bytes)
 *
 * Converts the byte-based cursor position to a character-based position
 * by counting Unicode characters from the beginning of the buffer.
 *
 * @param buffer Pointer to text buffer
 * @return Character position of cursor, or SIZE_MAX on error
 */
size_t lle_text_get_cursor_char_pos(const lle_text_buffer_t *buffer) {
    if (!buffer || !buffer->buffer) {
        return SIZE_MAX;
    }

    if (buffer->cursor_pos == 0) {
        return 0;
    }

    // Count characters from start to cursor position
    return lle_utf8_count_chars(buffer->buffer, buffer->cursor_pos);
}

/**
 * @brief Set cursor position by Unicode character index
 *
 * Converts a character-based position to the corresponding byte position
 * and sets the cursor to that location.
 *
 * @param buffer Pointer to text buffer
 * @param char_pos Character position (0-based Unicode character index)
 * @return true on success, false on failure
 */
bool lle_text_set_cursor_char_pos(lle_text_buffer_t *buffer, size_t char_pos) {
    if (!buffer || !buffer->buffer) {
        return false;
    }

    if (char_pos == 0) {
        buffer->cursor_pos = 0;
        return true;
    }

    // Convert character position to byte position
    size_t byte_pos = lle_utf8_char_at(buffer->buffer, char_pos);

    if (byte_pos == SIZE_MAX) {
        return false; // Invalid character position
    }

    buffer->cursor_pos = byte_pos;
    return true;
}

/**
 * @brief Get the display width of text up to cursor position
 *
 * Calculates the display width considering Unicode characters that may
 * have different display widths (e.g., CJK characters, emojis).
 * Currently uses character count as an approximation.
 *
 * @param buffer Pointer to text buffer
 * @return Display width in characters, considering Unicode width
 */
size_t lle_text_get_cursor_display_width(const lle_text_buffer_t *buffer) {
    if (!buffer || !buffer->buffer) {
        return 0;
    }

    // For now, use character count as display width
    // This is a simplification - in the future, this could be enhanced
    // to handle zero-width characters, double-width CJK characters, etc.
    return lle_utf8_count_chars(buffer->buffer, buffer->cursor_pos);
}