/*
 * Lusush Shell - LLE Completion Generator
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
 * LLE COMPLETION GENERATOR
 *
 * Main completion orchestration layer that:
 * - Analyzes input context (command position, variable, etc.)
 * - Determines which completion sources to use
 * - Calls appropriate sources to generate candidates
 * - Combines and sorts results
 * - Returns final completion result
 *
 * This module contains ONLY logic - NO terminal I/O.
 * All rendering is handled by the display layer.
 */

#ifndef LLE_COMPLETION_GENERATOR_H
#define LLE_COMPLETION_GENERATOR_H

#include "lle/completion/completion_sources.h"
#include "lle/completion/completion_types.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// COMPLETION CONTEXT
// ============================================================================

/**
 * Completion context type
 */
typedef enum {
    LLE_COMPLETION_CONTEXT_COMMAND,  // Command position (first word or after
                                     // |;&)
    LLE_COMPLETION_CONTEXT_ARGUMENT, // Command argument position
    LLE_COMPLETION_CONTEXT_VARIABLE, // Variable completion (starts with $)
    LLE_COMPLETION_CONTEXT_UNKNOWN   // Unknown context
} lle_completion_context_t;

/**
 * Completion context information
 */
typedef struct {
    lle_completion_context_t type; // Context type
    const char *word;              // Word being completed
    size_t word_start;             // Start position of word in buffer
    size_t word_length;            // Length of word
    bool at_command_position;      // True if at command position
} lle_completion_context_info_t;

// ============================================================================
// CONTEXT ANALYSIS
// ============================================================================

/**
 * Analyze buffer to determine completion context
 *
 * @param buffer input buffer
 * @param cursor_pos cursor position in buffer
 * @param context output parameter for context info
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_completion_analyze_context(const char *buffer, size_t cursor_pos,
                               lle_completion_context_info_t *context);

/**
 * Extract the word being completed from buffer
 *
 * @param buffer input buffer
 * @param cursor_pos cursor position in buffer
 * @param word_start output parameter for word start position
 * @param word output parameter for extracted word (must be freed by caller)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_extract_word(const char *buffer, size_t cursor_pos,
                                         size_t *word_start, char **word);

/**
 * Check if position is at command position
 *
 * @param buffer input buffer
 * @param position position to check
 * @return true if at command position, false otherwise
 */
bool lle_completion_is_command_position(const char *buffer, size_t position);

// ============================================================================
// COMPLETION GENERATION
// ============================================================================

/**
 * Generate completions for given buffer and cursor position
 * Main entry point for completion generation
 *
 * @param memory_pool memory pool for allocations
 * @param buffer input buffer
 * @param cursor_pos cursor position in buffer
 * @param result output parameter for completion result
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_generate(lle_memory_pool_t *memory_pool,
                                     const char *buffer, size_t cursor_pos,
                                     lle_completion_result_t **result);

/**
 * Generate completions for command context
 * Combines builtins, aliases, and PATH commands
 *
 * @param memory_pool memory pool for allocations
 * @param prefix command prefix to complete
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_generate_commands(lle_memory_pool_t *memory_pool,
                                              const char *prefix,
                                              lle_completion_result_t *result);

/**
 * Generate completions for argument context
 * Primarily file/directory completions
 *
 * @param memory_pool memory pool for allocations
 * @param prefix argument prefix to complete
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_generate_arguments(lle_memory_pool_t *memory_pool,
                                               const char *prefix,
                                               lle_completion_result_t *result);

/**
 * Generate completions for variable context
 * Environment and shell variables
 *
 * @param memory_pool memory pool for allocations
 * @param prefix variable prefix to complete (without $)
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_generate_variables(lle_memory_pool_t *memory_pool,
                                               const char *prefix,
                                               lle_completion_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* LLE_COMPLETION_GENERATOR_H */
