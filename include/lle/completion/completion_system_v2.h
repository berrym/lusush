/*
 * Lusush Shell - LLE Completion System V2 (Spec 12)
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 *
 * COMPLETION SYSTEM V2 - Spec 12 Core Implementation
 *
 * Enhanced completion system with proper architecture:
 * - Context analysis (understand what we're completing)
 * - Source management (query multiple sources)
 * - Proper orchestration (deduplicate, sort)
 * - State tracking (for inline cycling and menu)
 *
 * This is the PROPER implementation that fixes:
 * - Duplicate completions (echo appears twice)
 * - Wrong categorization (echo in both builtin and external)
 * - No context awareness
 */

#ifndef LLE_COMPLETION_SYSTEM_V2_H
#define LLE_COMPLETION_SYSTEM_V2_H

#include "lle/completion/completion_menu_state.h"
#include "lle/completion/completion_state.h"
#include "lle/completion/completion_types.h"
#include "lle/completion/context_analyzer.h"
#include "lle/completion/source_manager.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enhanced completion system - Spec 12 architecture
 */
typedef struct lle_completion_system_v2 {
    /* Core engines */
    lle_source_manager_t *source_manager; /* Manage completion sources */

    /* Current state */
    lle_completion_state_t *current_state; /* Active completion session */
    lle_completion_menu_state_t *menu;     /* Menu state (if visible) */

    /* Memory management */
    lle_memory_pool_t *pool; /* Memory pool for allocations */

    /* Configuration */
    bool enable_history_source; /* Use history as source */
    bool enable_fuzzy_matching; /* Future: fuzzy matching */
    size_t max_completions;     /* Limit results */
} lle_completion_system_v2_t;

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

/**
 * Create enhanced completion system
 *
 * @param pool Memory pool
 * @param out_system Output system
 * @return LLE_SUCCESS or error code
 */
lle_result_t
lle_completion_system_v2_create(lle_memory_pool_t *pool,
                                lle_completion_system_v2_t **out_system);

/**
 * Destroy enhanced completion system
 *
 * @param system System to destroy
 */
void lle_completion_system_v2_destroy(lle_completion_system_v2_t *system);

/**
 * Clear active completion
 *
 * @param system Completion system
 */
void lle_completion_system_v2_clear(lle_completion_system_v2_t *system);

// ============================================================================
// COMPLETION GENERATION (Spec 12 Core)
// ============================================================================

/**
 * Generate completions using Spec 12 architecture
 *
 * This is the PROPER implementation that:
 * - Analyzes context (command vs argument vs variable)
 * - Queries only applicable sources
 * - Deduplicates results (fixes "echo" appearing twice)
 * - Sorts by relevance
 *
 * @param system Completion system
 * @param buffer Input buffer
 * @param cursor_pos Cursor position
 * @param out_result Output completion result
 * @return LLE_SUCCESS or error code
 */
lle_result_t
lle_completion_system_v2_generate(lle_completion_system_v2_t *system,
                                  const char *buffer, size_t cursor_pos,
                                  lle_completion_result_t **out_result);

// ============================================================================
// STATE QUERIES
// ============================================================================

/**
 * Check if completion is active
 *
 * @param system Completion system
 * @return true if active
 */
bool lle_completion_system_v2_is_active(
    const lle_completion_system_v2_t *system);

/**
 * Check if menu is visible
 *
 * @param system Completion system
 * @return true if menu visible
 */
bool lle_completion_system_v2_is_menu_visible(
    const lle_completion_system_v2_t *system);

/**
 * Get current completion state
 *
 * @param system Completion system
 * @return Current state or NULL
 */
lle_completion_state_t *
lle_completion_system_v2_get_state(lle_completion_system_v2_t *system);

/**
 * Get menu state
 *
 * @param system Completion system
 * @return Menu state or NULL
 */
lle_completion_menu_state_t *
lle_completion_system_v2_get_menu(lle_completion_system_v2_t *system);

#ifdef __cplusplus
}
#endif

#endif /* LLE_COMPLETION_SYSTEM_V2_H */
