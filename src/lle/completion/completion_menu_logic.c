/**
 * @file completion_menu_logic.c
 * @brief Completion Menu Navigation Logic Implementation
 */

#include "lle/completion/completion_menu_logic.h"
#include <stddef.h>

/**
 * Ensure selected item is visible in the current view
 * Adjusts first_visible if needed
 */
static void ensure_visible(lle_completion_menu_state_t *state)
{
    if (state == NULL || state->result == NULL) {
        return;
    }

    size_t total_items = state->result->count;
    if (total_items == 0) {
        return;
    }

    // If selected is before visible range, scroll up
    if (state->selected_index < state->first_visible) {
        state->first_visible = state->selected_index;
    }
    // If selected is after visible range, scroll down
    else if (state->selected_index >= state->first_visible + state->visible_count) {
        state->first_visible = state->selected_index - state->visible_count + 1;
    }

    // Ensure first_visible doesn't go past the end
    if (state->first_visible + state->visible_count > total_items) {
        if (total_items >= state->visible_count) {
            state->first_visible = total_items - state->visible_count;
        } else {
            state->first_visible = 0;
        }
    }
}

lle_result_t lle_completion_menu_move_down(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t total_items = state->result->count;

    // Move down, wrap to beginning if at end
    state->selected_index++;
    if (state->selected_index >= total_items) {
        state->selected_index = 0;
    }

    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_move_up(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t total_items = state->result->count;

    // Move up, wrap to end if at beginning
    if (state->selected_index == 0) {
        state->selected_index = total_items - 1;
    } else {
        state->selected_index--;
    }

    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_page_down(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t total_items = state->result->count;

    // Move down by visible_count, stop at last item (no wrap)
    state->selected_index += state->visible_count;
    if (state->selected_index >= total_items) {
        state->selected_index = total_items - 1;
    }

    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_page_up(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Move up by visible_count, stop at first item (no wrap)
    if (state->selected_index < state->visible_count) {
        state->selected_index = 0;
    } else {
        state->selected_index -= state->visible_count;
    }

    ensure_visible(state);
    return LLE_SUCCESS;
}

/**
 * Calculate how many columns based on terminal width
 * Currently returns fixed 3 columns - will be enhanced to calculate
 * dynamically based on terminal width and item widths
 */
static size_t calculate_columns(void) {
    // Fixed 3-column layout for initial implementation
    // Will be enhanced to calculate based on actual terminal width and item widths
    return 3;
}

lle_result_t lle_completion_menu_move_right(
    lle_completion_menu_state_t *state)
{
    if (state == NULL || !state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t total_items = state->result->count;
    size_t columns = calculate_columns();
    
    // Calculate current row and column
    size_t current_row = state->selected_index / columns;
    size_t current_col = state->selected_index % columns;
    
    // Move to next column
    current_col++;
    
    // Calculate new index
    size_t new_index = current_row * columns + current_col;
    
    // If we went past the last column or past total items, wrap to first column
    if (current_col >= columns || new_index >= total_items) {
        // Go to first column of same row
        new_index = current_row * columns;
    }
    
    state->selected_index = new_index;
    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_move_left(
    lle_completion_menu_state_t *state)
{
    if (state == NULL || !state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t total_items = state->result->count;
    size_t columns = calculate_columns();
    
    // Calculate current row and column
    size_t current_row = state->selected_index / columns;
    size_t current_col = state->selected_index % columns;
    
    // Move to previous column
    if (current_col == 0) {
        // Wrap to last valid column of this row
        size_t row_start = current_row * columns;
        size_t row_end = row_start + columns;
        
        // Find last valid item in this row
        if (row_end > total_items) {
            row_end = total_items;
        }
        
        // Go to last item in row
        if (row_end > row_start) {
            state->selected_index = row_end - 1;
        }
    } else {
        // Simply move left one column
        state->selected_index--;
    }
    
    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_next_category(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->category_count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Find current category
    size_t current_category = 0;
    for (size_t i = 0; i < state->category_count; i++) {
        if (state->selected_index >= state->category_positions[i]) {
            current_category = i;
        } else {
            break;
        }
    }

    // Move to next category (wrap to first if at end)
    current_category++;
    if (current_category >= state->category_count) {
        current_category = 0;
    }

    state->selected_index = state->category_positions[current_category];
    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_prev_category(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->category_count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Find current category
    size_t current_category = 0;
    for (size_t i = 0; i < state->category_count; i++) {
        if (state->selected_index >= state->category_positions[i]) {
            current_category = i;
        } else {
            break;
        }
    }

    // Move to previous category (wrap to last if at beginning)
    if (current_category == 0) {
        current_category = state->category_count - 1;
    } else {
        current_category--;
    }

    state->selected_index = state->category_positions[current_category];
    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_select_first(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    state->selected_index = 0;
    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_select_last(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    state->selected_index = state->result->count - 1;
    ensure_visible(state);
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_accept(
    lle_completion_menu_state_t *state,
    const lle_completion_item_t **selected_item)
{
    if (state == NULL || selected_item == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->result == NULL || state->result->count == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!state->menu_active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (state->selected_index >= state->result->count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Return pointer to selected item
    *selected_item = &state->result->items[state->selected_index];

    // Deactivate menu
    state->menu_active = false;

    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_cancel(
    lle_completion_menu_state_t *state)
{
    if (state == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    state->menu_active = false;
    return LLE_SUCCESS;
}

lle_result_t lle_completion_menu_handle_char(
    lle_completion_menu_state_t *state,
    char c,
    bool *should_cancel)
{
    if (state == NULL || should_cancel == NULL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // For now, any character input cancels the menu
    // Future: implement incremental filtering
    (void)c;  // Unused for now
    *should_cancel = true;
    state->menu_active = false;

    return LLE_SUCCESS;
}
