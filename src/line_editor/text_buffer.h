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
// Task LLE-003: Text insertion functions  
// Task LLE-004: Text deletion functions
// Task LLE-005: Cursor movement functions

#endif // LLE_TEXT_BUFFER_H
