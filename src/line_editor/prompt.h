#ifndef LLE_PROMPT_H
#define LLE_PROMPT_H

#include <stdbool.h>
#include <stddef.h>
#include "cursor_math.h"
#include "terminal_manager.h"

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

/**
 * @brief Parse and analyze prompt structure
 *
 * Parses the given prompt text, splitting it into lines and calculating
 * display geometry. Handles ANSI escape sequences and multiline prompts.
 *
 * @param prompt Prompt structure to populate
 * @param text Prompt text to parse (may contain ANSI codes and newlines)
 * @return true on success, false on error
 */
bool lle_prompt_parse(lle_prompt_t *prompt, const char *text);

/**
 * @brief Parse and analyze prompt structure with terminal width awareness
 *
 * Parses the given prompt text, splitting it into lines and calculating
 * display geometry. Handles ANSI escape sequences, multiline prompts, and
 * automatic line wrapping when prompt exceeds terminal width.
 *
 * @param prompt Prompt structure to populate
 * @param text Prompt text to parse (may contain ANSI codes and newlines)
 * @param terminal_width Terminal width for wrapping calculation
 * @return true on success, false on error
 */
bool lle_prompt_parse_with_terminal_width(lle_prompt_t *prompt, const char *text, size_t terminal_width);

/**
 * @brief Split prompt into individual lines
 *
 * Splits the prompt text at newline characters and stores each line
 * separately in the lines array. Handles empty lines correctly.
 *
 * @param prompt Prompt structure with text to split
 * @return true on success, false on error
 */
bool lle_prompt_split_lines(lle_prompt_t *prompt);

/**
 * @brief Strip ANSI escape sequences from text
 *
 * Copies text to output buffer while removing ANSI escape sequences.
 * This is used to calculate the actual display width of text.
 *
 * @param input Input text (may contain ANSI codes)
 * @param output Output buffer for stripped text
 * @param output_size Size of output buffer
 * @return true on success, false on error
 */
bool lle_prompt_strip_ansi(const char *input, char *output, size_t output_size);

/**
 * @brief Calculate display width of text (excluding ANSI codes)
 *
 * Returns the number of character positions that the text will occupy
 * on the terminal, excluding ANSI escape sequences.
 *
 * @param text Text to measure (may contain ANSI codes)
 * @return Display width in characters, 0 if text is NULL
 */
size_t lle_prompt_display_width(const char *text);

/**
 * @brief Render prompt to terminal
 *
 * Renders the parsed prompt to the terminal, handling multiline prompts
 * and ANSI escape sequences correctly.
 *
 * @param tm Terminal manager for output
 * @param prompt Parsed prompt to render
 * @param clear_previous Whether to clear previous prompt first
 * @return true on success, false on error
 */
bool lle_prompt_render(
    lle_terminal_manager_t *tm,
    const lle_prompt_t *prompt,
    bool clear_previous
);

/**
 * @brief Position cursor after prompt
 *
 * Positions the terminal cursor at the correct location after the prompt,
 * taking into account multiline prompts and cursor position within input text.
 *
 * @param tm Terminal manager for cursor operations
 * @param prompt Rendered prompt structure
 * @param cursor_pos Desired cursor position in input text
 * @return true on success, false on error
 */
bool lle_prompt_position_cursor(
    lle_terminal_manager_t *tm,
    const lle_prompt_t *prompt,
    const lle_cursor_position_t *cursor_pos
);

/**
 * @brief Clear prompt from terminal
 *
 * Clears the prompt from the terminal by moving to the beginning
 * of the prompt area and clearing all prompt lines.
 *
 * @param tm Terminal manager for operations
 * @param prompt Prompt structure to clear
 * @return true on success, false on error
 */
bool lle_prompt_clear_from_terminal(
    lle_terminal_manager_t *tm,
    const lle_prompt_t *prompt
);

/**
 * @brief Get cursor position after prompt
 *
 * Calculates where the cursor should be positioned immediately after
 * the prompt, which is where input text begins.
 *
 * @param prompt Prompt structure
 * @param cursor_pos Output cursor position structure
 * @return true on success, false on error
 */
bool lle_prompt_get_end_position(
    const lle_prompt_t *prompt,
    lle_cursor_position_t *cursor_pos
);

#endif // LLE_PROMPT_H