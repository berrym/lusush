#ifndef LLE_PROMPT_H
#define LLE_PROMPT_H

#include <stdbool.h>
#include <stddef.h>
#include "cursor_math.h"

/**
 * @file prompt.h
 * @brief Lusush Line Editor - Prompt handling structures and functions
 *
 * This module defines structures and functions for handling complex prompts
 * including multiline prompts with ANSI escape sequences.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

/**
 * @brief Structure representing a parsed prompt
 *
 * The lle_prompt_t structure contains all information needed to handle
 * complex prompts including multiline prompts with ANSI escape sequences.
 * The structure supports efficient rendering and geometry calculations.
 */
typedef struct {
    char *text;                     /**< Full prompt text including ANSI codes */
    size_t length;                  /**< Text length in bytes */
    lle_prompt_geometry_t geometry; /**< Calculated display geometry */
    bool has_ansi_codes;            /**< True if prompt contains ANSI escape sequences */
    char **lines;                   /**< Array of individual prompt lines */
    size_t line_count;              /**< Number of lines in the prompt */
    size_t capacity;                /**< Allocated capacity for lines array */
} lle_prompt_t;

/**
 * @brief Initialize a prompt structure
 *
 * Initializes all fields of the prompt structure to safe defaults.
 * Must be called before using any other prompt functions.
 *
 * @param prompt Prompt structure to initialize
 * @return true on success, false if prompt is NULL
 */
bool lle_prompt_init(lle_prompt_t *prompt);

/**
 * @brief Create a new prompt with specified capacity
 *
 * Allocates a new prompt structure with initial capacity for lines.
 * The caller is responsible for calling lle_prompt_destroy() when done.
 *
 * @param initial_capacity Initial capacity for lines array (minimum 1)
 * @return Pointer to new prompt structure, NULL on failure
 */
lle_prompt_t *lle_prompt_create(size_t initial_capacity);

/**
 * @brief Clear prompt contents while preserving structure
 *
 * Clears all prompt text and resets geometry while keeping the
 * structure intact for reuse.
 *
 * @param prompt Prompt structure to clear
 * @return true on success, false if prompt is NULL
 */
bool lle_prompt_clear(lle_prompt_t *prompt);

/**
 * @brief Destroy prompt and free all memory
 *
 * Frees all memory associated with the prompt structure including
 * text, lines array, and the structure itself if it was created
 * with lle_prompt_create().
 *
 * @param prompt Prompt structure to destroy (may be NULL)
 */
void lle_prompt_destroy(lle_prompt_t *prompt);

/**
 * @brief Validate prompt structure integrity
 *
 * Checks that all fields of the prompt structure are in a valid state.
 * Useful for debugging and ensuring structure consistency.
 *
 * @param prompt Prompt structure to validate
 * @return true if structure is valid, false otherwise
 */
bool lle_prompt_validate(const lle_prompt_t *prompt);

/**
 * @brief Get display width of the widest prompt line
 *
 * Returns the display width (excluding ANSI codes) of the widest
 * line in the prompt. This is used for cursor positioning.
 *
 * @param prompt Prompt structure to measure
 * @return Display width in characters, 0 if prompt is invalid
 */
size_t lle_prompt_get_width(const lle_prompt_t *prompt);

/**
 * @brief Get number of lines in the prompt
 *
 * Returns the total number of lines that the prompt occupies,
 * including explicit newlines and line wrapping.
 *
 * @param prompt Prompt structure to measure
 * @return Number of lines, 0 if prompt is invalid
 */
size_t lle_prompt_get_height(const lle_prompt_t *prompt);

/**
 * @brief Get display width of the last prompt line
 *
 * Returns the display width of the final line of the prompt.
 * This is critical for positioning the cursor after the prompt.
 *
 * @param prompt Prompt structure to measure
 * @return Display width of last line, 0 if prompt is invalid
 */
size_t lle_prompt_get_last_line_width(const lle_prompt_t *prompt);

/**
 * @brief Check if prompt contains ANSI escape sequences
 *
 * Returns true if the prompt text contains ANSI escape sequences
 * that affect display but not cursor positioning.
 *
 * @param prompt Prompt structure to check
 * @return true if ANSI codes present, false otherwise
 */
bool lle_prompt_has_ansi(const lle_prompt_t *prompt);

/**
 * @brief Get specific line from prompt
 *
 * Returns a pointer to the specified line within the prompt.
 * The returned pointer is valid until the prompt is modified.
 *
 * @param prompt Prompt structure
 * @param line_index Zero-based line index
 * @return Pointer to line text, NULL if invalid index or prompt
 */
const char *lle_prompt_get_line(const lle_prompt_t *prompt, size_t line_index);

/**
 * @brief Copy prompt text without ANSI codes
 *
 * Copies the prompt text to the output buffer, stripping all ANSI
 * escape sequences. Used for measuring display width.
 *
 * @param prompt Source prompt structure
 * @param output Output buffer for plain text
 * @param output_size Size of output buffer
 * @return Number of bytes written, 0 on error
 */
size_t lle_prompt_copy_plain_text(const lle_prompt_t *prompt, char *output, size_t output_size);

#endif // LLE_PROMPT_H