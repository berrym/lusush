/**
 * @file prompt.c
 * @brief Lusush Line Editor - Prompt handling implementation
 *
 * This module implements functions for handling complex prompts including
 * multiline prompts with ANSI escape sequences.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "prompt.h"
#include "terminal_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Initialize a prompt structure
 */
bool lle_prompt_init(lle_prompt_t *prompt) {
    if (!prompt) {
        return false;
    }
    
    prompt->text = NULL;
    prompt->length = 0;
    prompt->has_ansi_codes = false;
    prompt->lines = NULL;
    prompt->line_count = 0;
    prompt->capacity = 0;
    
    // Initialize geometry to zero
    prompt->geometry.width = 0;
    prompt->geometry.height = 0;
    prompt->geometry.last_line_width = 0;
    
    return true;
}

/**
 * @brief Create a new prompt with specified capacity
 */
lle_prompt_t *lle_prompt_create(size_t initial_capacity) {
    lle_prompt_t *prompt = malloc(sizeof(lle_prompt_t));
    if (!prompt) {
        return NULL;
    }
    
    // Initialize the structure
    if (!lle_prompt_init(prompt)) {
        free(prompt);
        return NULL;
    }
    
    // Enforce minimum capacity of 1
    if (initial_capacity == 0) {
        initial_capacity = 1;
    }
    
    // Allocate lines array
    prompt->lines = malloc(initial_capacity * sizeof(char *));
    if (!prompt->lines) {
        free(prompt);
        return NULL;
    }
    
    prompt->capacity = initial_capacity;
    
    // Initialize all line pointers to NULL
    for (size_t i = 0; i < initial_capacity; i++) {
        prompt->lines[i] = NULL;
    }
    
    return prompt;
}

/**
 * @brief Clear prompt contents while preserving structure
 */
bool lle_prompt_clear(lle_prompt_t *prompt) {
    if (!prompt) {
        return false;
    }
    
    // Free text if allocated
    if (prompt->text) {
        free(prompt->text);
        prompt->text = NULL;
    }
    
    // Free individual line strings
    if (prompt->lines) {
        for (size_t i = 0; i < prompt->line_count; i++) {
            if (prompt->lines[i]) {
                free(prompt->lines[i]);
                prompt->lines[i] = NULL;
            }
        }
    }
    
    // Reset fields
    prompt->length = 0;
    prompt->has_ansi_codes = false;
    prompt->line_count = 0;
    
    // Reset geometry
    prompt->geometry.width = 0;
    prompt->geometry.height = 0;
    prompt->geometry.last_line_width = 0;
    
    return true;
}

/**
 * @brief Destroy prompt and free all memory
 */
void lle_prompt_destroy(lle_prompt_t *prompt) {
    if (!prompt) {
        return;
    }
    
    // Clear contents first
    lle_prompt_clear(prompt);
    
    // Free lines array
    if (prompt->lines) {
        free(prompt->lines);
        prompt->lines = NULL;
    }
    
    // Free the structure itself
    free(prompt);
}

/**
 * @brief Validate prompt structure integrity
 */
bool lle_prompt_validate(const lle_prompt_t *prompt) {
    if (!prompt) {
        return false;
    }
    
    // Check basic consistency
    if (prompt->length > 0 && !prompt->text) {
        return false;
    }
    
    if (prompt->line_count > prompt->capacity) {
        return false;
    }
    
    if (prompt->line_count > 0 && !prompt->lines) {
        return false;
    }
    
    if (prompt->capacity > 0 && !prompt->lines) {
        return false;
    }
    
    return true;
}

/**
 * @brief Get display width of the widest prompt line
 */
size_t lle_prompt_get_width(const lle_prompt_t *prompt) {
    if (!prompt || !lle_prompt_validate(prompt)) {
        return 0;
    }
    
    return prompt->geometry.width;
}

/**
 * @brief Get number of lines in the prompt
 */
size_t lle_prompt_get_height(const lle_prompt_t *prompt) {
    if (!prompt || !lle_prompt_validate(prompt)) {
        return 0;
    }
    
    return prompt->geometry.height;
}

/**
 * @brief Get display width of the last prompt line
 */
size_t lle_prompt_get_last_line_width(const lle_prompt_t *prompt) {
    if (!prompt || !lle_prompt_validate(prompt)) {
        return 0;
    }
    
    return prompt->geometry.last_line_width;
}

/**
 * @brief Check if prompt contains ANSI escape sequences
 */
bool lle_prompt_has_ansi(const lle_prompt_t *prompt) {
    if (!prompt || !lle_prompt_validate(prompt)) {
        return false;
    }
    
    return prompt->has_ansi_codes;
}

/**
 * @brief Get specific line from prompt
 */
const char *lle_prompt_get_line(const lle_prompt_t *prompt, size_t line_index) {
    if (!prompt || !lle_prompt_validate(prompt)) {
        return NULL;
    }
    
    if (line_index >= prompt->line_count) {
        return NULL;
    }
    
    if (!prompt->lines) {
        return NULL;
    }
    
    return prompt->lines[line_index];
}

/**
 * @brief Copy prompt text without ANSI codes
 */
size_t lle_prompt_copy_plain_text(const lle_prompt_t *prompt, char *output, size_t output_size) {
    if (!prompt || !output || output_size == 0) {
        return 0;
    }
    
    if (!lle_prompt_validate(prompt)) {
        return 0;
    }
    
    if (!prompt->text || prompt->length == 0) {
        if (output_size > 0) {
            output[0] = '\0';
        }
        return 0;
    }
    
    // Strip ANSI codes and copy to output
    return lle_prompt_strip_ansi(prompt->text, output, output_size);
}

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
bool lle_prompt_parse(lle_prompt_t *prompt, const char *text) {
    if (!prompt || !text) {
        return false;
    }
    
    // Clear existing content
    if (!lle_prompt_clear(prompt)) {
        return false;
    }
    
    // Store the full text
    size_t text_len = strlen(text);
    prompt->text = malloc(text_len + 1);
    if (!prompt->text) {
        return false;
    }
    strcpy(prompt->text, text);
    prompt->length = text_len;
    
    // Check for ANSI codes
    prompt->has_ansi_codes = (strstr(text, "\033[") != NULL);
    
    // Split into lines
    if (!lle_prompt_split_lines(prompt)) {
        return false;
    }
    
    // Calculate geometry
    prompt->geometry.width = 0;
    prompt->geometry.height = prompt->line_count;
    
    // Find the widest line and last line width
    for (size_t i = 0; i < prompt->line_count; i++) {
        size_t line_width = lle_prompt_display_width(prompt->lines[i]);
        if (line_width > prompt->geometry.width) {
            prompt->geometry.width = line_width;
        }
        if (i == prompt->line_count - 1) {
            prompt->geometry.last_line_width = line_width;
        }
    }
    
    return true;
}

/**
 * @brief Split prompt into individual lines
 *
 * Splits the prompt text at newline characters and stores each line
 * separately in the lines array. Handles empty lines correctly.
 *
 * @param prompt Prompt structure with text to split
 * @return true on success, false on error
 */
bool lle_prompt_split_lines(lle_prompt_t *prompt) {
    if (!prompt || !prompt->text) {
        return false;
    }
    
    // Count newlines to determine number of lines
    size_t line_count = 1; // At least one line
    const char *p = prompt->text;
    while (*p) {
        if (*p == '\n') {
            line_count++;
        }
        p++;
    }
    
    // Ensure we have enough capacity
    if (line_count > prompt->capacity) {
        size_t new_capacity = line_count * 2; // Grow with some headroom
        char **new_lines = realloc(prompt->lines, new_capacity * sizeof(char *));
        if (!new_lines) {
            return false;
        }
        prompt->lines = new_lines;
        
        // Initialize new pointers to NULL
        for (size_t i = prompt->capacity; i < new_capacity; i++) {
            prompt->lines[i] = NULL;
        }
        
        prompt->capacity = new_capacity;
    }
    
    // Split the text into lines
    const char *line_start = prompt->text;
    size_t current_line = 0;
    
    p = prompt->text;
    while (*p) {
        if (*p == '\n') {
            // Found end of line
            size_t line_len = p - line_start;
            prompt->lines[current_line] = malloc(line_len + 1);
            if (!prompt->lines[current_line]) {
                return false;
            }
            
            memcpy(prompt->lines[current_line], line_start, line_len);
            prompt->lines[current_line][line_len] = '\0';
            
            current_line++;
            line_start = p + 1; // Start of next line
        }
        p++;
    }
    
    // Handle the last line (no trailing newline)
    if (line_start <= p) {
        size_t line_len = p - line_start;
        prompt->lines[current_line] = malloc(line_len + 1);
        if (!prompt->lines[current_line]) {
            return false;
        }
        
        memcpy(prompt->lines[current_line], line_start, line_len);
        prompt->lines[current_line][line_len] = '\0';
        current_line++;
    }
    
    prompt->line_count = current_line;
    return true;
}

/**
 * @brief Strip ANSI escape sequences from text
 *
 * Copies text to output buffer while removing ANSI escape sequences.
 * This is used to calculate the actual display width of text.
 *
 * @param input Input text (may contain ANSI codes)
 * @param output Output buffer for stripped text
 * @param output_size Size of output buffer
 * @return Number of bytes written to output, 0 on error
 */
bool lle_prompt_strip_ansi(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size == 0) {
        return false;
    }
    
    const char *src = input;
    char *dst = output;
    size_t remaining = output_size - 1; // Leave space for null terminator
    
    while (*src && remaining > 0) {
        if (*src == '\033' && *(src + 1) == '[') {
            // Found ANSI escape sequence, skip it
            src += 2; // Skip ESC[
            
            // Skip until we find the terminating character
            while (*src && !(*src >= 'A' && *src <= 'Z') && 
                   !(*src >= 'a' && *src <= 'z') && *src != 'm') {
                src++;
            }
            
            if (*src) {
                src++; // Skip the terminating character
            }
        } else {
            // Regular character, copy it
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
    return true;
}

/**
 * @brief Calculate display width of text (excluding ANSI codes)
 *
 * Returns the number of character positions that the text will occupy
 * on the terminal, excluding ANSI escape sequences.
 *
 * @param text Text to measure (may contain ANSI codes)
 * @return Display width in characters, 0 if text is NULL
 */
size_t lle_prompt_display_width(const char *text) {
    if (!text) {
        return 0;
    }
    
    // Use a temporary buffer to strip ANSI codes
    size_t text_len = strlen(text);
    char *stripped = malloc(text_len + 1);
    if (!stripped) {
        return 0;
    }
    
    if (!lle_prompt_strip_ansi(text, stripped, text_len + 1)) {
        free(stripped);
        return 0;
    }
    
    size_t width = strlen(stripped);
    free(stripped);
    
    return width;
}

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
) {
    // Get debug flag from environment
    static int debug_mode = -1;
    if (debug_mode == -1) {
        debug_mode = getenv("LLE_DEBUG") ? 1 : 0;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_RENDER] Starting prompt render, tm=%p, prompt=%p, clear=%d\n", 
                (void*)tm, (void*)prompt, clear_previous);
    }
    
    if (!tm || !prompt) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_RENDER] Invalid parameters: tm=%p, prompt=%p\n", 
                    (void*)tm, (void*)prompt);
        }
        return false;
    }
    
    if (!lle_prompt_validate(prompt)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_RENDER] Prompt validation failed\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_RENDER] Prompt valid, line_count=%zu, clear_previous=%d\n", 
                prompt->line_count, clear_previous);
    }
    
    // Clear previous prompt if requested
    if (clear_previous) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_RENDER] Clearing previous prompt, height=%zu\n", 
                    prompt->geometry.height);
        }
        // Move cursor to beginning of prompt area
        for (size_t i = 0; i < prompt->geometry.height; i++) {
            if (!lle_terminal_move_cursor_up(tm, 1)) {
                // If we can't move up, we're probably at the top
                if (debug_mode) {
                    fprintf(stderr, "[LLE_PROMPT_RENDER] Cannot move cursor up at iteration %zu\n", i);
                }
                break;
            }
        }
        
        // Clear all prompt lines
        for (size_t i = 0; i < prompt->geometry.height; i++) {
            if (!lle_terminal_clear_line(tm)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_PROMPT_RENDER] Clear line failed at iteration %zu\n", i);
                }
                return false;
            }
            if (i < prompt->geometry.height - 1) {
                if (!lle_terminal_move_cursor_down(tm, 1)) {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_PROMPT_RENDER] Move cursor down failed at iteration %zu\n", i);
                    }
                    return false;
                }
            }
        }
        
        // Move back to start of prompt area
        for (size_t i = 0; i < prompt->geometry.height - 1; i++) {
            if (!lle_terminal_move_cursor_up(tm, 1)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_PROMPT_RENDER] Move cursor up failed in reset at iteration %zu\n", i);
                }
                return false;
            }
        }
        if (!lle_terminal_move_cursor_to_column(tm, 0)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_PROMPT_RENDER] Move cursor to column failed\n");
            }
            return false;
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_RENDER] Rendering %zu prompt lines\n", prompt->line_count);
    }
    
    // Render each line of the prompt
    for (size_t i = 0; i < prompt->line_count; i++) {
        const char *line = prompt->lines[i];
        if (!line) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_PROMPT_RENDER] Skipping NULL line at index %zu\n", i);
            }
            continue;
        }
        
        // Write the line (includes ANSI codes)
        size_t line_len = strlen(line);
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_RENDER] Writing line %zu: length=%zu\n", i, line_len);
        }
        if (line_len > 0) {
            if (!lle_terminal_write(tm, line, line_len)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_PROMPT_RENDER] Terminal write failed for line %zu\n", i);
                }
                return false;
            }
        }
        
        // Move to next line if not the last line
        if (i < prompt->line_count - 1) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_PROMPT_RENDER] Writing newline after line %zu\n", i);
            }
            if (!lle_terminal_write(tm, "\n", 1)) {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_PROMPT_RENDER] Newline write failed after line %zu\n", i);
                }
                return false;
            }
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_RENDER] Prompt render completed successfully\n");
    }
    
    return true;
}

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
) {
    // Get debug flag from environment
    static int debug_mode = -1;
    if (debug_mode == -1) {
        debug_mode = getenv("LLE_DEBUG") ? 1 : 0;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_POSITION] Starting cursor positioning\n");
    }
    
    if (!tm || !prompt || !cursor_pos) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_POSITION] Invalid parameters: tm=%p, prompt=%p, cursor_pos=%p\n", 
                    (void*)tm, (void*)prompt, (void*)cursor_pos);
        }
        return false;
    }
    
    if (!lle_prompt_validate(prompt)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_POSITION] Prompt validation failed\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_POSITION] Parameters validated, cursor at row=%zu, col=%zu\n", 
                cursor_pos->relative_row, cursor_pos->relative_col);
    }
    
    // Calculate the absolute position where cursor should be
    size_t target_row = cursor_pos->relative_row;
    size_t target_col = cursor_pos->relative_col;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_POSITION] Initial target: row=%zu, col=%zu\n", 
                target_row, target_col);
        fprintf(stderr, "[LLE_PROMPT_POSITION] Prompt info: line_count=%zu, height=%zu, last_line_width=%zu\n", 
                prompt->line_count, prompt->geometry.height, prompt->geometry.last_line_width);
    }
    
    // For prompts, we need to account for the prompt's last line
    // The cursor position is relative to the input text, not the prompt
    if (prompt->line_count > 0) {
        // Add prompt height to the target row
        target_row += prompt->geometry.height - 1;
        
        // If cursor is on the first line of input, add last line width of prompt
        // But check if cursor_pos->relative_col already includes prompt width
        if (cursor_pos->relative_row == 0) {
            // Check if relative_col already includes prompt width (cursor math inconsistency)
            if (cursor_pos->relative_col < prompt->geometry.last_line_width) {
                target_col += prompt->geometry.last_line_width;
                if (debug_mode) {
                    fprintf(stderr, "[LLE_PROMPT_POSITION] Added prompt width to target column\n");
                }
            } else {
                if (debug_mode) {
                    fprintf(stderr, "[LLE_PROMPT_POSITION] Cursor position already includes prompt width\n");
                }
            }
        }
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_POSITION] Adjusted target: row=%zu, col=%zu\n", 
                    target_row, target_col);
        }
    }
    
    // Validate target position - for now, just log if excessive
    if (debug_mode && target_col > 120) {  // Reasonable threshold
        fprintf(stderr, "[LLE_PROMPT_POSITION] Warning: target column %zu seems excessive\n", target_col);
    }
    
    // Move cursor to the calculated position
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_POSITION] Attempting to move cursor to row=%zu, col=%zu\n", 
                target_row, target_col);
    }
    
    if (!lle_terminal_move_cursor(tm, target_row, target_col)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_PROMPT_POSITION] Terminal move cursor failed\n");
        }
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_PROMPT_POSITION] Cursor positioning completed successfully\n");
    }
    
    return true;
}

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
) {
    if (!tm || !prompt) {
        return false;
    }
    
    if (!lle_prompt_validate(prompt)) {
        return false;
    }
    
    // Save current cursor position (approximately)
    // We'll use this to restore position after clearing
    
    // Move to the beginning of the prompt
    for (size_t i = 0; i < prompt->geometry.height; i++) {
        if (!lle_terminal_move_cursor_up(tm, 1)) {
            // If we can't move up, we're at the top
            break;
        }
    }
    
    if (!lle_terminal_move_cursor_to_column(tm, 0)) {
        return false;
    }
    
    // Clear each line of the prompt
    for (size_t i = 0; i < prompt->geometry.height; i++) {
        if (!lle_terminal_clear_line(tm)) {
            return false;
        }
        
        // Move down to next line if not the last
        if (i < prompt->geometry.height - 1) {
            if (!lle_terminal_move_cursor_down(tm, 1)) {
                return false;
            }
            if (!lle_terminal_move_cursor_to_column(tm, 0)) {
                return false;
            }
        }
    }
    
    return true;
}

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
) {
    if (!prompt || !cursor_pos) {
        return false;
    }
    
    if (!lle_prompt_validate(prompt)) {
        return false;
    }
    
    // Position is at the end of the last line of the prompt
    if (prompt->line_count > 0) {
        cursor_pos->absolute_row = prompt->geometry.height - 1;
        cursor_pos->absolute_col = prompt->geometry.last_line_width;
        cursor_pos->relative_row = prompt->geometry.height - 1;
        cursor_pos->relative_col = prompt->geometry.last_line_width;
    } else {
        // Empty prompt
        cursor_pos->absolute_row = 0;
        cursor_pos->absolute_col = 0;
        cursor_pos->relative_row = 0;
        cursor_pos->relative_col = 0;
    }
    
    cursor_pos->at_boundary = false;
    
    cursor_pos->valid = true;
    
    return true;
}