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
    
    // For now, just copy the text as-is
    // TODO: In LLE-016, implement ANSI stripping
    size_t copy_len = prompt->length;
    if (copy_len >= output_size) {
        copy_len = output_size - 1;
    }
    
    memcpy(output, prompt->text, copy_len);
    output[copy_len] = '\0';
    
    return copy_len;
}