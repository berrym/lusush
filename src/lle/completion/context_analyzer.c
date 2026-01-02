/**
 * @file context_analyzer.c
 * @brief LLE Context Analyzer Implementation
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Analyzes buffer to determine completion context.
 */

#include "lle/completion/context_analyzer.h"
#include <ctype.h>
#include <string.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Check if character is a word boundary
 * @param c Character to check
 * @return true if word boundary, false otherwise
 */
static bool is_word_boundary(char c) {
    return isspace(c) || c == '|' || c == ';' || c == '&' || c == '(' ||
           c == ')' || c == '<' || c == '>' || c == '\0';
}

/**
 * @brief Find start of current word in buffer
 * @param buffer Input buffer
 * @param cursor_pos Current cursor position
 * @return Index of word start
 */
static size_t find_word_start(const char *buffer, size_t cursor_pos) {
    if (cursor_pos == 0) {
        return 0;
    }

    size_t pos = cursor_pos;

    /* Move back to start of word */
    while (pos > 0 && !is_word_boundary(buffer[pos - 1])) {
        pos--;
    }

    return pos;
}

/**
 * @brief Extract word from buffer between positions
 * @param buffer Input buffer
 * @param start Start index
 * @param end End index
 * @param pool Memory pool for allocation
 * @return Allocated word string or NULL on failure
 */
static char *extract_word(const char *buffer, size_t start, size_t end,
                          lle_memory_pool_t *pool) {
    (void)pool; /* Reserved for pool-based allocation */
    if (end <= start) {
        /* Empty string - must null-terminate! */
        char *empty = lle_pool_alloc(1);
        if (empty) {
            empty[0] = '\0';
        }
        return empty;
    }

    size_t len = end - start;
    char *word = lle_pool_alloc(len + 1);
    if (!word) {
        return NULL;
    }

    memcpy(word, buffer + start, len);
    word[len] = '\0';

    return word;
}

/**
 * @brief Check if position is at start of command
 * @param buffer Input buffer
 * @param pos Position to check
 * @return true if command position, false otherwise
 */
static bool is_command_position(const char *buffer, size_t pos) {
    /* Start of buffer is always command position */
    if (pos == 0) {
        return true;
    }

    /* Look backwards for command separator */
    for (size_t i = pos; i > 0; i--) {
        char c = buffer[i - 1];

        if (c == '|' || c == ';' || c == '&') {
            return true; /* After separator = command position */
        }

        if (!isspace(c)) {
            return false; /* Found non-separator = argument position */
        }
    }

    return true; /* Only whitespace before = command position */
}

/**
 * @brief Check if position is inside quoted string
 * @param buffer Input buffer
 * @param pos Position to check
 * @return true if inside quotes, false otherwise
 */
static bool is_inside_quotes(const char *buffer, size_t pos) {
    bool in_single = false;
    bool in_double = false;

    for (size_t i = 0; i < pos; i++) {
        char c = buffer[i];

        if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        }
    }

    return in_single || in_double;
}

/**
 * @brief Check if position is after redirect operator
 * @param buffer Input buffer
 * @param pos Position to check
 * @return true if after redirect, false otherwise
 */
static bool is_after_redirect(const char *buffer, size_t pos) {
    if (pos == 0) {
        return false;
    }

    /* Look backwards for redirect operator */
    for (size_t i = pos; i > 0; i--) {
        char c = buffer[i - 1];

        if (c == '>' || c == '<') {
            return true;
        }

        if (!isspace(c)) {
            return false; /* Found non-whitespace before redirect */
        }
    }

    return false;
}

/**
 * @brief Check if position is in a variable assignment
 * @param buffer Input buffer
 * @param pos Position to check
 * @return true if in assignment, false otherwise
 */
static bool is_in_assignment(const char *buffer, size_t pos) {
    /* Look backwards for '=' without spaces before it */
    for (size_t i = pos; i > 0; i--) {
        char c = buffer[i - 1];

        if (c == '=') {
            /* Check if there's a valid variable name before = */
            if (i > 1 && (isalnum(buffer[i - 2]) || buffer[i - 2] == '_')) {
                return true;
            }
        }

        if (isspace(c) || c == ';' || c == '|') {
            return false; /* Hit separator */
        }
    }

    return false;
}

/**
 * @brief Determine completion context type for position
 * @param buffer Input buffer
 * @param word_start Start of current word
 * @return Context type enum value
 */
static lle_completion_context_type_t determine_context_type(const char *buffer,
                                                            size_t word_start) {
    /* Check for variable expansion */
    if (word_start > 0 && buffer[word_start - 1] == '$') {
        return LLE_CONTEXT_VARIABLE;
    }

    /* Check for assignment */
    if (is_in_assignment(buffer, word_start)) {
        return LLE_CONTEXT_ASSIGNMENT;
    }

    /* Check for redirect */
    if (is_after_redirect(buffer, word_start)) {
        return LLE_CONTEXT_REDIRECT;
    }

    /* Check if at command position */
    if (is_command_position(buffer, word_start)) {
        return LLE_CONTEXT_COMMAND;
    }

    /* Default: argument completion */
    return LLE_CONTEXT_ARGUMENT;
}

/**
 * @brief Extract command context for argument position
 * @param buffer Input buffer
 * @param pos Current position
 * @param command Output for command name
 * @param arg_index Output for argument index
 * @param pool Memory pool for allocations
 */
static void extract_command_context(const char *buffer, size_t pos,
                                    char **command, int *arg_index,
                                    lle_memory_pool_t *pool) {
    *command = NULL;
    *arg_index = -1;

    /* Find start of current command */
    size_t cmd_start = 0;
    for (size_t i = pos; i > 0; i--) {
        char c = buffer[i - 1];
        if (c == '|' || c == ';' || c == '&') {
            cmd_start = i;
            break;
        }
    }

    /* Skip whitespace */
    while (cmd_start < pos && isspace(buffer[cmd_start])) {
        cmd_start++;
    }

    if (cmd_start >= pos) {
        return; /* No command found */
    }

    /* Extract command name */
    size_t cmd_end = cmd_start;
    while (cmd_end < pos && !isspace(buffer[cmd_end])) {
        cmd_end++;
    }

    *command = extract_word(buffer, cmd_start, cmd_end, pool);

    /* Count arguments */
    int arg_count = 0;
    bool in_word = false;

    for (size_t i = cmd_end; i < pos; i++) {
        if (isspace(buffer[i])) {
            in_word = false;
        } else if (!in_word) {
            arg_count++;
            in_word = true;
        }
    }

    *arg_index = arg_count;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Analyze buffer to determine completion context
 * @param buffer Input buffer text
 * @param cursor_pos Current cursor position
 * @param pool Memory pool for allocations
 * @param out_context Output for context analyzer
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_context_analyze(const char *buffer, size_t cursor_pos,
                                 lle_memory_pool_t *pool,
                                 lle_context_analyzer_t **out_context) {
    if (!buffer || !pool || !out_context) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Find word being completed */
    size_t word_start = find_word_start(buffer, cursor_pos);
    size_t word_end = cursor_pos;

    /* Determine context type */
    lle_completion_context_type_t type =
        determine_context_type(buffer, word_start);

    /* Extract partial word */
    char *partial_word = extract_word(buffer, word_start, word_end, pool);
    if (!partial_word) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Extract command context if in argument position */
    char *command_name = NULL;
    int arg_index = -1;
    if (type == LLE_CONTEXT_ARGUMENT) {
        extract_command_context(buffer, word_start, &command_name, &arg_index,
                                pool);
    }

    /* Create context structure */
    lle_context_analyzer_t *context = lle_pool_alloc(sizeof(*context));
    if (!context) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    context->type = type;
    context->word_start = word_start;
    context->word_end = word_end;
    context->partial_word = partial_word;
    context->command_name = command_name;
    context->argument_index = arg_index;
    context->in_quotes = is_inside_quotes(buffer, cursor_pos);
    context->after_redirect = is_after_redirect(buffer, word_start);
    context->in_assignment = is_in_assignment(buffer, word_start);
    context->pool = pool;

    *out_context = context;
    return LLE_SUCCESS;
}

/**
 * @brief Free context analyzer resources
 * @param context Context to free
 */
void lle_context_analyzer_free(lle_context_analyzer_t *context) {
    if (!context) {
        return;
    }

    /* Memory is pool-allocated, will be freed with pool */
    /* Just clear the pointer */
    (void)context;
}
