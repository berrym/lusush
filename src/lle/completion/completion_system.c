/*
 * Lusush Shell - LLE Completion System Implementation
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
 */

#include "lle/completion/completion_system.h"
#include "lle/completion/completion_types.h"
#include "lle/completion/completion_menu_state.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

lle_result_t lle_completion_system_create(
    lle_memory_pool_t *memory_pool,
    lle_completion_system_t **system)
{
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Allocate system
    lle_completion_system_t *new_system = 
        (lle_completion_system_t *)lle_pool_alloc(sizeof(lle_completion_system_t));
    if (!new_system) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize fields
    new_system->result = NULL;
    new_system->menu = NULL;
    new_system->word_being_completed = NULL;
    new_system->word_start_pos = 0;
    new_system->word_length = 0;
    new_system->active = false;
    new_system->menu_visible = false;
    new_system->memory_pool = memory_pool;
    
    *system = new_system;
    return LLE_SUCCESS;
}

lle_result_t lle_completion_system_destroy(lle_completion_system_t *system)
{
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Clear active completion first
    lle_completion_system_clear(system);
    
    // Free system itself
    lle_pool_free(system);
    
    return LLE_SUCCESS;
}

lle_result_t lle_completion_system_clear(lle_completion_system_t *system)
{
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Free menu state
    if (system->menu) {
        lle_completion_menu_state_free(system->menu);
        system->menu = NULL;
    }
    
    // Free result
    if (system->result) {
        lle_completion_result_free(system->result);
        system->result = NULL;
    }
    
    // Free word string
    if (system->word_being_completed) {
        lle_pool_free(system->word_being_completed);
        system->word_being_completed = NULL;
    }
    
    // Reset state
    system->word_start_pos = 0;
    system->word_length = 0;
    system->active = false;
    system->menu_visible = false;
    
    return LLE_SUCCESS;
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

lle_result_t lle_completion_system_set_completion(
    lle_completion_system_t *system,
    lle_completion_result_t *result,
    const char *word_being_completed,
    size_t word_start_pos)
{
    if (!system || !result || !word_being_completed) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Clear any existing completion
    lle_completion_system_clear(system);
    
    // Take ownership of result
    system->result = result;
    
    // Copy word being completed
    size_t word_len = strlen(word_being_completed);
    system->word_being_completed = 
        (char *)lle_pool_alloc(word_len + 1);
    if (!system->word_being_completed) {
        system->result = NULL;  // Don't take ownership if we can't allocate
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    strcpy(system->word_being_completed, word_being_completed);
    system->word_length = word_len;
    system->word_start_pos = word_start_pos;
    
    // Create menu state
    lle_result_t menu_result = lle_completion_menu_state_create(
        system->memory_pool,
        system->result,
        NULL,  // Use default config
        &system->menu);
    
    if (menu_result != LLE_SUCCESS) {
        // Clean up on failure
        lle_pool_free(system->word_being_completed);
        system->word_being_completed = NULL;
        system->result = NULL;
        return menu_result;
    }
    
    // Mark active
    system->active = true;
    system->menu_visible = lle_completion_menu_should_show(system->menu);
    
    return LLE_SUCCESS;
}

bool lle_completion_system_is_active(const lle_completion_system_t *system)
{
    return system && system->active;
}

bool lle_completion_system_is_menu_visible(const lle_completion_system_t *system)
{
    return system && system->menu_visible;
}

lle_completion_menu_state_t* lle_completion_system_get_menu(
    lle_completion_system_t *system)
{
    return system ? system->menu : NULL;
}

const char* lle_completion_system_get_selected_text(
    const lle_completion_system_t *system)
{
    if (!system || !system->menu) {
        return NULL;
    }
    
    return lle_completion_menu_get_selected_text(system->menu);
}

const char* lle_completion_system_get_word(const lle_completion_system_t *system)
{
    return system ? system->word_being_completed : NULL;
}

size_t lle_completion_system_get_word_start(const lle_completion_system_t *system)
{
    return system ? system->word_start_pos : 0;
}
