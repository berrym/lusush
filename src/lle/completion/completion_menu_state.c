/*
 * Lusush Shell - LLE Completion Menu State Implementation
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
 * LLE COMPLETION MENU STATE IMPLEMENTATION
 * 
 * This module contains ONLY state management - NO rendering.
 */

#include "lle/completion/completion_menu_state.h"
#include <string.h>

// ============================================================================
// DEFAULT CONFIGURATION
// ============================================================================

lle_completion_menu_config_t lle_completion_menu_default_config(void) {
    lle_completion_menu_config_t config = {
        .max_visible_items = 10,
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = false,
        .enable_scrolling = true,
        .min_items_for_menu = 2
    };
    return config;
}

// ============================================================================
// CATEGORY POSITION CALCULATION
// ============================================================================

/**
 * Calculate category positions in result
 */
static lle_result_t calculate_category_positions(
    lle_completion_menu_state_t *state)
{
    if (!state || !state->result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (state->result->count == 0) {
        state->category_count = 0;
        state->category_positions = NULL;
        return LLE_SUCCESS;
    }
    
    // Count unique categories
    size_t cat_count = 0;
    lle_completion_type_t current_type = LLE_COMPLETION_TYPE_UNKNOWN;
    
    for (size_t i = 0; i < state->result->count; i++) {
        if (state->result->items[i].type != current_type) {
            current_type = state->result->items[i].type;
            cat_count++;
        }
    }
    
    if (cat_count == 0) {
        state->category_count = 0;
        state->category_positions = NULL;
        return LLE_SUCCESS;
    }
    
    // Allocate category positions array
    state->category_positions = (size_t*)lle_pool_alloc(
        sizeof(size_t) * cat_count);
    if (!state->category_positions) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    // Fill category positions
    size_t cat_index = 0;
    current_type = LLE_COMPLETION_TYPE_UNKNOWN;
    
    for (size_t i = 0; i < state->result->count; i++) {
        if (state->result->items[i].type != current_type) {
            current_type = state->result->items[i].type;
            state->category_positions[cat_index++] = i;
        }
    }
    
    state->category_count = cat_count;
    return LLE_SUCCESS;
}

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

lle_result_t lle_completion_menu_state_create(
    lle_memory_pool_t *memory_pool,
    lle_completion_result_t *result,
    const lle_completion_menu_config_t *config,
    lle_completion_menu_state_t **state)
{
    if (!memory_pool || !result || !state) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Allocate state structure
    lle_completion_menu_state_t *new_state = 
        (lle_completion_menu_state_t*)lle_pool_alloc(
            sizeof(lle_completion_menu_state_t));
    if (!new_state) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize state
    new_state->result = result;
    new_state->selected_index = 0;
    new_state->first_visible = 0;
    new_state->visible_count = 0;
    new_state->category_positions = NULL;
    new_state->category_count = 0;
    new_state->menu_active = false;
    new_state->memory_pool = memory_pool;
    
    // Copy configuration or use defaults
    if (config) {
        new_state->config = *config;
    } else {
        new_state->config = lle_completion_menu_default_config();
    }
    
    // Calculate visible count
    size_t total_items = result->count;
    if (new_state->config.enable_scrolling) {
        new_state->visible_count = total_items < new_state->config.max_visible_items
                                   ? total_items
                                   : new_state->config.max_visible_items;
    } else {
        new_state->visible_count = total_items;
    }
    
    // Calculate category positions
    lle_result_t res = calculate_category_positions(new_state);
    if (res != LLE_SUCCESS) {
        lle_pool_free(new_state);
        return res;
    }
    
    *state = new_state;
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_state_free(lle_completion_menu_state_t *state) {
    if (!state) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Free category positions if allocated
    if (state->category_positions) {
        lle_pool_free(state->category_positions);
    }
    
    // Free state structure
    // Note: result is owned by caller, so we don't free it
    lle_pool_free(state);
    
    return LLE_SUCCESS;
}

// ============================================================================
// STATE QUERIES
// ============================================================================

bool lle_completion_menu_should_show(const lle_completion_menu_state_t *state) {
    if (!state || !state->result) {
        return false;
    }
    
    return state->result->count >= state->config.min_items_for_menu;
}

const lle_completion_item_t* lle_completion_menu_get_selected(
    const lle_completion_menu_state_t *state)
{
    if (!state || !state->result) {
        return NULL;
    }
    
    if (state->selected_index >= state->result->count) {
        return NULL;
    }
    
    return &state->result->items[state->selected_index];
}

const char* lle_completion_menu_get_selected_text(
    const lle_completion_menu_state_t *state)
{
    const lle_completion_item_t *item = lle_completion_menu_get_selected(state);
    return item ? item->text : NULL;
}

size_t lle_completion_menu_get_item_count(
    const lle_completion_menu_state_t *state)
{
    if (!state || !state->result) {
        return 0;
    }
    
    return state->result->count;
}

size_t lle_completion_menu_get_selected_index(
    const lle_completion_menu_state_t *state)
{
    if (!state) {
        return 0;
    }
    
    return state->selected_index;
}

lle_result_t lle_completion_menu_get_visible_range(
    const lle_completion_menu_state_t *state,
    size_t *first_visible,
    size_t *visible_count)
{
    if (!state || !first_visible || !visible_count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    *first_visible = state->first_visible;
    *visible_count = state->visible_count;
    
    return LLE_SUCCESS;
}

bool lle_completion_menu_is_active(const lle_completion_menu_state_t *state) {
    if (!state) {
        return false;
    }
    
    return state->menu_active;
}

size_t lle_completion_menu_get_category_count(
    const lle_completion_menu_state_t *state)
{
    if (!state) {
        return 0;
    }
    
    return state->category_count;
}
