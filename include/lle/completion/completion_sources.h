/*
 * Lusush Shell - LLE Completion Sources
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
 * LLE COMPLETION SOURCES
 *
 * Shell data adapters that provide completion candidates from various sources:
 * - Built-in commands
 * - Aliases
 * - PATH executables
 * - Files and directories
 * - Environment and shell variables
 * - Command history
 *
 * This module contains ONLY data retrieval logic - NO terminal I/O.
 * All rendering is handled by the display layer.
 *
 * These functions provide the strong symbols that override the weak symbols
 * declared in completion_types.c for shell integration.
 */

#ifndef LLE_COMPLETION_SOURCES_H
#define LLE_COMPLETION_SOURCES_H

#include "lle/completion/completion_types.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// SHELL INTEGRATION FUNCTIONS
// ============================================================================

/**
 * Check if text is a shell builtin command
 * Provides strong symbol to override weak declaration in completion_types.c
 *
 * @param text text to check
 * @return true if builtin, false otherwise
 */
bool lle_shell_is_builtin(const char *text);

/**
 * Check if text is a shell alias
 * Provides strong symbol to override weak declaration in completion_types.c
 *
 * @param text text to check
 * @return true if alias, false otherwise
 */
bool lle_shell_is_alias(const char *text);

// ============================================================================
// COMPLETION SOURCE FUNCTIONS
// ============================================================================

/**
 * Get builtin command completions
 *
 * @param memory_pool memory pool for allocations
 * @param prefix text prefix to match (may be empty string for all)
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_source_builtins(lle_memory_pool_t *memory_pool,
                                            const char *prefix,
                                            lle_completion_result_t *result);

/**
 * Get alias completions
 *
 * @param memory_pool memory pool for allocations
 * @param prefix text prefix to match (may be empty string for all)
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_source_aliases(lle_memory_pool_t *memory_pool,
                                           const char *prefix,
                                           lle_completion_result_t *result);

/**
 * Get PATH command completions
 * Searches all directories in PATH for executable files
 *
 * @param memory_pool memory pool for allocations
 * @param prefix text prefix to match (may be empty string for all)
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_source_commands(lle_memory_pool_t *memory_pool,
                                            const char *prefix,
                                            lle_completion_result_t *result);

/**
 * Get file and directory completions
 *
 * @param memory_pool memory pool for allocations
 * @param prefix path prefix to match (may include directory)
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_source_files(lle_memory_pool_t *memory_pool,
                                         const char *prefix,
                                         lle_completion_result_t *result);

/**
 * Get directory-only completions (for cd, pushd, popd, etc.)
 *
 * @param memory_pool memory pool for allocations
 * @param prefix path prefix to match (may include directory)
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_source_directories(lle_memory_pool_t *memory_pool,
                                               const char *prefix,
                                               lle_completion_result_t *result);

/**
 * Get environment and shell variable completions
 *
 * @param memory_pool memory pool for allocations
 * @param prefix variable name prefix (without $)
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_source_variables(lle_memory_pool_t *memory_pool,
                                             const char *prefix,
                                             lle_completion_result_t *result);

/**
 * Get command history completions
 *
 * @param memory_pool memory pool for allocations
 * @param prefix text prefix to match
 * @param result completion result to populate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_source_history(lle_memory_pool_t *memory_pool,
                                           const char *prefix,
                                           lle_completion_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* LLE_COMPLETION_SOURCES_H */
