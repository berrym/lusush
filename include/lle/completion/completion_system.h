/*
 * Lusush Shell - LLE Completion System
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
 * LLE COMPLETION SYSTEM
 * 
 * Runtime state for the completion system. This structure is stored in
 * lle_editor_t and tracks the active completion session.
 * 
 * Responsibilities:
 * - Store active completion result
 * - Store active menu state
 * - Track word being completed and its position
 * - Provide lifecycle management
 * 
 * Phase: 5.4 - Event Wiring
 * Spec: docs/lle_specification/spec_12_completion_system.md
 */

#ifndef LLE_COMPLETION_SYSTEM_H
#define LLE_COMPLETION_SYSTEM_H

#include "lle/completion/completion_types.h"
#include "lle/completion/completion_menu_state.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// COMPLETION SYSTEM STRUCTURE
// ============================================================================

/**
 * Completion system runtime state
 */
typedef struct lle_completion_system_t {
    // Active completion data
    lle_completion_result_t *result;      // Current completion result (owned)
    lle_completion_menu_state_t *menu;    // Current menu state (owned)
    
    // Completion context
    char *word_being_completed;           // Word being completed (owned)
    size_t word_start_pos;                // Start position of word in buffer
    size_t word_length;                   // Length of word being completed
    
    // State flags
    bool active;                          // True if completion is active
    bool menu_visible;                    // True if menu is visible
    
    // Memory pool
    lle_memory_pool_t *memory_pool;       // Memory pool for allocations
} lle_completion_system_t;

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

/**
 * Create completion system
 * 
 * @param memory_pool memory pool for allocations
 * @param system output parameter for created system
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_system_create(
    lle_memory_pool_t *memory_pool,
    lle_completion_system_t **system);

/**
 * Destroy completion system
 * Frees all resources including active result and menu
 * 
 * @param system completion system to destroy
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_system_destroy(lle_completion_system_t *system);

/**
 * Clear active completion
 * Clears current completion but keeps system allocated
 * 
 * @param system completion system
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_system_clear(lle_completion_system_t *system);

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

/**
 * Set active completion
 * Takes ownership of result and creates menu state
 * 
 * @param system completion system
 * @param result completion result (ownership transferred)
 * @param word_being_completed word being completed (copied)
 * @param word_start_pos start position of word in buffer
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_completion_system_set_completion(
    lle_completion_system_t *system,
    lle_completion_result_t *result,
    const char *word_being_completed,
    size_t word_start_pos);

/**
 * Check if completion is active
 * 
 * @param system completion system
 * @return true if completion is active
 */
bool lle_completion_system_is_active(const lle_completion_system_t *system);

/**
 * Check if menu is visible
 * 
 * @param system completion system
 * @return true if menu is visible
 */
bool lle_completion_system_is_menu_visible(const lle_completion_system_t *system);

/**
 * Get menu state
 * 
 * @param system completion system
 * @return menu state, or NULL if none
 */
lle_completion_menu_state_t* lle_completion_system_get_menu(
    lle_completion_system_t *system);

/**
 * Get selected completion text
 * 
 * @param system completion system
 * @return selected completion text, or NULL if none
 */
const char* lle_completion_system_get_selected_text(
    const lle_completion_system_t *system);

/**
 * Get word being completed
 * 
 * @param system completion system
 * @return word being completed, or NULL if none
 */
const char* lle_completion_system_get_word(const lle_completion_system_t *system);

/**
 * Get word start position
 * 
 * @param system completion system
 * @return word start position in buffer
 */
size_t lle_completion_system_get_word_start(const lle_completion_system_t *system);

#ifdef __cplusplus
}
#endif

#endif /* LLE_COMPLETION_SYSTEM_H */
