/*
 * Lusush Shell - LLE Completion Generator Implementation
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ============================================================================
 *
 * LLE COMPLETION GENERATOR IMPLEMENTATION
 *
 * This module contains ONLY logic - NO terminal I/O.
 * Main orchestration layer for the completion system.
 */

#include "lle/completion/completion_generator.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONTEXT ANALYSIS HELPERS
// ============================================================================

/**
 * Check if character is a command separator
 */
static bool is_command_separator(char c) {
    return c == '|' || c == ';' || c == '&' || c == '(' || c == ')';
}

/**
 * Check if character is a word boundary
 */
static bool is_word_boundary(char c) {
    return isspace(c) || is_command_separator(c);
}

// ============================================================================
// WORD EXTRACTION
// ============================================================================

lle_result_t lle_completion_extract_word(const char *buffer, size_t cursor_pos,
                                         size_t *word_start, char **word) {
    if (!buffer || !word_start || !word) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t buf_len = strlen(buffer);
    if (cursor_pos > buf_len) {
        cursor_pos = buf_len;
    }

    // Find start of current word
    size_t start = cursor_pos;
    while (start > 0 && !is_word_boundary(buffer[start - 1])) {
        start--;
    }

    // Extract word from start to cursor
    size_t word_len = cursor_pos - start;
    char *extracted = (char *)lle_pool_alloc(word_len + 1);
    if (!extracted) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    if (word_len > 0) {
        memcpy(extracted, buffer + start, word_len);
    }
    extracted[word_len] = '\0';

    *word_start = start;
    *word = extracted;

    return LLE_SUCCESS;
}

// ============================================================================
// COMMAND POSITION DETECTION
// ============================================================================

bool lle_completion_is_command_position(const char *buffer, size_t position) {
    if (!buffer) {
        return false;
    }

    if (position == 0) {
        return true; // First position is always command position
    }

    // Look backwards for command separators
    for (size_t i = position; i > 0; i--) {
        char c = buffer[i - 1];

        if (is_command_separator(c)) {
            return true; // Found separator, this is command position
        }

        if (!isspace(c)) {
            return false; // Found non-whitespace, not command position
        }
    }

    return true; // Only found whitespace, this is command position
}

// ============================================================================
// CONTEXT ANALYSIS
// ============================================================================

lle_result_t
lle_completion_analyze_context(const char *buffer, size_t cursor_pos,
                               lle_completion_context_info_t *context) {
    if (!buffer || !context) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Extract the word being completed
    size_t word_start = 0;
    char *word = NULL;
    lle_result_t result =
        lle_completion_extract_word(buffer, cursor_pos, &word_start, &word);
    if (result != LLE_SUCCESS) {
        return result;
    }

    // Determine if at command position
    bool at_cmd_pos = lle_completion_is_command_position(buffer, word_start);

    // Determine context type
    lle_completion_context_t ctx_type = LLE_COMPLETION_CONTEXT_UNKNOWN;

    if (word && word[0] == '$') {
        ctx_type = LLE_COMPLETION_CONTEXT_VARIABLE;
    } else if (at_cmd_pos) {
        ctx_type = LLE_COMPLETION_CONTEXT_COMMAND;
    } else {
        ctx_type = LLE_COMPLETION_CONTEXT_ARGUMENT;
    }

    // Fill context structure
    context->type = ctx_type;
    context->word = word;
    context->word_start = word_start;
    context->word_length = word ? strlen(word) : 0;
    context->at_command_position = at_cmd_pos;

    return LLE_SUCCESS;
}

// ============================================================================
// CONTEXT-SPECIFIC COMPLETION GENERATION
// ============================================================================

lle_result_t lle_completion_generate_commands(lle_memory_pool_t *memory_pool,
                                              const char *prefix,
                                              lle_completion_result_t *result) {
    if (!memory_pool || !prefix || !result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_result_t res;

    // Generate builtins
    res = lle_completion_source_builtins(memory_pool, prefix, result);
    if (res != LLE_SUCCESS) {
        return res;
    }

    // Generate aliases
    res = lle_completion_source_aliases(memory_pool, prefix, result);
    if (res != LLE_SUCCESS) {
        return res;
    }

    // Generate PATH commands
    res = lle_completion_source_commands(memory_pool, prefix, result);
    if (res != LLE_SUCCESS) {
        return res;
    }

    return LLE_SUCCESS;
}

lle_result_t
lle_completion_generate_arguments(lle_memory_pool_t *memory_pool,
                                  const char *prefix,
                                  lle_completion_result_t *result) {
    if (!memory_pool || !prefix || !result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // For arguments, primarily complete files and directories
    return lle_completion_source_files(memory_pool, prefix, result);
}

lle_result_t
lle_completion_generate_variables(lle_memory_pool_t *memory_pool,
                                  const char *prefix,
                                  lle_completion_result_t *result) {
    if (!memory_pool || !prefix || !result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Skip the $ if present
    const char *var_prefix = prefix;
    if (prefix[0] == '$') {
        var_prefix = prefix + 1;
    }

    return lle_completion_source_variables(memory_pool, var_prefix, result);
}

// ============================================================================
// MAIN COMPLETION GENERATION
// ============================================================================

lle_result_t lle_completion_generate(lle_memory_pool_t *memory_pool,
                                     const char *buffer, size_t cursor_pos,
                                     lle_completion_result_t **result) {
    if (!memory_pool || !buffer || !result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Analyze context
    lle_completion_context_info_t context;
    lle_result_t res =
        lle_completion_analyze_context(buffer, cursor_pos, &context);
    if (res != LLE_SUCCESS) {
        return res;
    }

    // Create result structure
    lle_completion_result_t *comp_result = NULL;
    res = lle_completion_result_create(memory_pool, 64, &comp_result);
    if (res != LLE_SUCCESS) {
        if (context.word) {
            lle_pool_free((void *)context.word);
        }
        return res;
    }

    // Generate completions based on context
    const char *prefix = context.word ? context.word : "";

    switch (context.type) {
    case LLE_COMPLETION_CONTEXT_COMMAND:
        res =
            lle_completion_generate_commands(memory_pool, prefix, comp_result);
        break;

    case LLE_COMPLETION_CONTEXT_VARIABLE:
        res =
            lle_completion_generate_variables(memory_pool, prefix, comp_result);
        break;

    case LLE_COMPLETION_CONTEXT_ARGUMENT:
        res =
            lle_completion_generate_arguments(memory_pool, prefix, comp_result);
        break;

    default:
        // Unknown context - try files as fallback
        res =
            lle_completion_generate_arguments(memory_pool, prefix, comp_result);
        break;
    }

    // Free context word
    if (context.word) {
        lle_pool_free((void *)context.word);
    }

    if (res != LLE_SUCCESS) {
        lle_completion_result_free(comp_result);
        return res;
    }

    // If no completions, try history as fallback
    if (comp_result->count == 0) {
        res = lle_completion_source_history(memory_pool, prefix, comp_result);
        if (res != LLE_SUCCESS) {
            lle_completion_result_free(comp_result);
            return res;
        }
    }

    // Sort results by type and relevance
    res = lle_completion_result_sort(comp_result);
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(comp_result);
        return res;
    }

    *result = comp_result;
    return LLE_SUCCESS;
}
