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
#include <stdlib.h>
#include <string.h>

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