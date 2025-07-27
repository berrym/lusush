/*
 * Lusush Line Editor - Undo/Redo System Implementation
 * 
 * This file implements the undo/redo system structures for the Lusush Line Editor.
 * Provides comprehensive undo/redo functionality with efficient action storage,
 * configurable stack size, and support for all editing operations with cursor
 * position tracking.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "undo.h"
#include "text_buffer.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize an undo stack with default configuration
 */
bool lle_undo_stack_init(lle_undo_stack_t *stack) {
    if (!stack) {
        return false;
    }
    
    return lle_undo_stack_init_custom(stack, LLE_DEFAULT_MAX_UNDO_ACTIONS, false, true);
}

/**
 * @brief Initialize an undo stack with custom configuration
 */
bool lle_undo_stack_init_custom(lle_undo_stack_t *stack, size_t max_actions, 
                                bool merge_similar, bool auto_cleanup) {
    if (!stack) {
        return false;
    }
    
    if (max_actions < LLE_MIN_UNDO_ACTIONS || max_actions > LLE_MAX_UNDO_ACTIONS) {
        return false;
    }
    
    // Initialize structure
    memset(stack, 0, sizeof(lle_undo_stack_t));
    
    // Allocate initial capacity (start with reasonable size)
    size_t initial_capacity = (max_actions < 50) ? max_actions : 50;
    stack->actions = malloc(initial_capacity * sizeof(lle_undo_action_t));
    if (!stack->actions) {
        return false;
    }
    
    // Set configuration
    stack->capacity = initial_capacity;
    stack->max_actions = max_actions;
    stack->merge_similar = merge_similar;
    stack->auto_cleanup = auto_cleanup;
    
    // Initialize state
    stack->count = 0;
    stack->current = 0;
    stack->can_undo = false;
    stack->can_redo = false;
    stack->undo_count = 0;
    stack->redo_count = 0;
    stack->total_memory = 0; // Only count actual action text memory
    stack->peak_memory = sizeof(lle_undo_stack_t) + initial_capacity * sizeof(lle_undo_action_t);
    stack->operations_count = 0;
    stack->merge_timeout_ms = 1000; // Default 1 second merge timeout
    
    return true;
}

/**
 * @brief Create a new undo stack with default configuration
 */
lle_undo_stack_t *lle_undo_stack_create(void) {
    lle_undo_stack_t *stack = malloc(sizeof(lle_undo_stack_t));
    if (!stack) {
        return NULL;
    }
    
    if (!lle_undo_stack_init(stack)) {
        free(stack);
        return NULL;
    }
    
    return stack;
}

/**
 * @brief Create a new undo stack with custom configuration
 */
lle_undo_stack_t *lle_undo_stack_create_custom(size_t max_actions, 
                                               bool merge_similar, bool auto_cleanup) {
    if (max_actions < LLE_MIN_UNDO_ACTIONS || max_actions > LLE_MAX_UNDO_ACTIONS) {
        return NULL;
    }
    
    lle_undo_stack_t *stack = malloc(sizeof(lle_undo_stack_t));
    if (!stack) {
        return NULL;
    }
    
    if (!lle_undo_stack_init_custom(stack, max_actions, merge_similar, auto_cleanup)) {
        free(stack);
        return NULL;
    }
    
    return stack;
}

/**
 * @brief Destroy an undo stack and free all memory
 */
void lle_undo_stack_cleanup(lle_undo_stack_t *stack) {
    if (!stack) {
        return;
    }
    
    // Free all action text data
    if (stack->actions) {
        for (size_t i = 0; i < stack->count; i++) {
            if (stack->actions[i].owns_text && stack->actions[i].text) {
                free(stack->actions[i].text);
            }
        }
        free(stack->actions);
    }
    
    // Reset structure
    memset(stack, 0, sizeof(lle_undo_stack_t));
}

/**
 * @brief Destroy and free an undo stack created with lle_undo_stack_create()
 */
void lle_undo_stack_destroy(lle_undo_stack_t *stack) {
    if (!stack) {
        return;
    }
    
    lle_undo_stack_cleanup(stack);
    free(stack);
}

/**
 * @brief Clear all actions from the undo stack
 */
bool lle_undo_stack_clear(lle_undo_stack_t *stack) {
    if (!stack) {
        return false;
    }
    
    // Free all action text data
    if (stack->actions) {
        for (size_t i = 0; i < stack->count; i++) {
            if (stack->actions[i].owns_text && stack->actions[i].text) {
                free(stack->actions[i].text);
            }
        }
    }
    
    // Reset counts and state
    stack->count = 0;
    stack->current = 0;
    stack->can_undo = false;
    stack->can_redo = false;
    stack->undo_count = 0;
    stack->redo_count = 0;
    stack->total_memory = 0; // Reset to 0, only action text counts
    stack->operations_count = 0;
    
    return true;
}

// ============================================================================
// Helper Functions for Stack Management
// ============================================================================

/**
 * @brief Ensure stack has capacity for one more action
 * 
 * Grows the stack capacity if needed or removes oldest actions if at max limit.
 * Maintains the circular buffer property while ensuring space for new actions.
 * 
 * @param stack Pointer to undo stack (must not be NULL)
 * @return true if capacity is available, false on error
 */
static bool lle_undo_ensure_capacity(lle_undo_stack_t *stack) {
    if (!stack || !stack->actions) {
        return false;
    }
    
    // If we're at max actions, remove oldest action
    if (stack->count >= stack->max_actions) {
        // Find oldest action and free its memory
        if (stack->actions[0].owns_text && stack->actions[0].text) {
            stack->total_memory -= (stack->actions[0].length + 1);
            free(stack->actions[0].text);
        }
        
        // Shift all actions forward by one position
        for (size_t i = 0; i < stack->count - 1; i++) {
            stack->actions[i] = stack->actions[i + 1];
        }
        
        stack->count--;
        if (stack->current > 0) {
            stack->current--;
        }
    }
    
    // Ensure we have capacity in the array
    if (stack->count >= stack->capacity) {
        size_t new_capacity = stack->capacity * 2;
        if (new_capacity > stack->max_actions) {
            new_capacity = stack->max_actions;
        }
        
        lle_undo_action_t *new_actions = realloc(stack->actions, 
                                                new_capacity * sizeof(lle_undo_action_t));
        if (!new_actions) {
            return false;
        }
        
        stack->actions = new_actions;
        stack->capacity = new_capacity;
    }
    
    return true;
}

/**
 * @brief Update stack state after recording an action
 * 
 * Updates all state variables including can_undo, can_redo, counts,
 * and memory tracking. Handles the transition from having redo actions
 * to losing them when new actions are recorded.
 * 
 * @param stack Pointer to undo stack (must not be NULL)
 */
static void lle_undo_update_state(lle_undo_stack_t *stack) {
    if (!stack) {
        return;
    }
    
    // Clear any redo actions when new action is recorded
    if (stack->current < stack->count) {
        // Free text memory for redo actions being discarded
        for (size_t i = stack->current; i < stack->count; i++) {
            if (stack->actions[i].owns_text && stack->actions[i].text) {
                stack->total_memory -= (stack->actions[i].length + 1);
                free(stack->actions[i].text);
            }
        }
        stack->count = stack->current;
    }
    
    // Update counts and flags
    stack->undo_count = stack->current;
    stack->redo_count = 0;
    stack->can_undo = (stack->undo_count > 0);
    stack->can_redo = false;
    
    // Update memory tracking
    size_t current_memory = sizeof(lle_undo_stack_t) + 
                           stack->capacity * sizeof(lle_undo_action_t) + 
                           stack->total_memory;
    if (current_memory > stack->peak_memory) {
        stack->peak_memory = current_memory;
    }
    
    stack->operations_count++;
}

/**
 * @brief Check if two actions can be merged
 * 
 * Determines whether a new action can be merged with the previous action
 * to create a more efficient undo history. Only certain types of similar
 * actions can be merged.
 * 
 * @param prev Previous action in the stack
 * @param new_action New action being recorded
 * @return true if actions can be merged, false otherwise
 */
static bool lle_undo_can_merge_actions(const lle_undo_action_t *prev, 
                                      const lle_undo_action_t *new_action) {
    if (!prev || !new_action) {
        return false;
    }
    
    // Only merge actions of the same type
    if (prev->type != new_action->type) {
        return false;
    }
    
    // Only merge certain types
    if (prev->type != LLE_UNDO_INSERT && prev->type != LLE_UNDO_DELETE) {
        return false;
    }
    
    // For insertions, positions should be adjacent
    if (prev->type == LLE_UNDO_INSERT) {
        return (prev->position + prev->length == new_action->position);
    }
    
    // For deletions, check if they're at the same position (backspace)
    // or adjacent positions (delete key)
    if (prev->type == LLE_UNDO_DELETE) {
        return (prev->position == new_action->position ||
                prev->position == new_action->position + new_action->length);
    }
    
    return false;
}

/**
 * @brief Merge a new action with the previous action
 * 
 * Combines two compatible actions into a single action to reduce
 * the size of the undo stack and provide more intuitive undo behavior.
 * 
 * @param prev Previous action to merge into (will be modified)
 * @param new_action New action to merge (will be consumed)
 * @return true if merge was successful, false on error
 */
static bool lle_undo_merge_actions(lle_undo_action_t *prev, 
                                  const lle_undo_action_t *new_action) {
    if (!prev || !new_action) {
        return false;
    }
    
    if (prev->type == LLE_UNDO_INSERT && new_action->type == LLE_UNDO_INSERT) {
        // Merge insertions by concatenating text
        size_t new_length = prev->length + new_action->length;
        char *new_text = malloc(new_length + 1);
        if (!new_text) {
            return false;
        }
        
        if (prev->text) {
            memcpy(new_text, prev->text, prev->length);
        }
        if (new_action->text) {
            memcpy(new_text + prev->length, new_action->text, new_action->length);
        }
        new_text[new_length] = '\0';
        
        // Update memory tracking
        if (prev->owns_text && prev->text) {
            free(prev->text);
        }
        
        prev->text = new_text;
        prev->length = new_length;
        prev->owns_text = true;
        prev->new_cursor = new_action->new_cursor;
        prev->new_char_cursor = new_action->new_char_cursor;
        
        return true;
    }
    
    if (prev->type == LLE_UNDO_DELETE && new_action->type == LLE_UNDO_DELETE) {
        // Merge deletions
        if (prev->position == new_action->position) {
            // Backspace pattern - prepend new text
            size_t new_length = prev->length + new_action->length;
            char *new_text = malloc(new_length + 1);
            if (!new_text) {
                return false;
            }
            
            if (new_action->text) {
                memcpy(new_text, new_action->text, new_action->length);
            }
            if (prev->text) {
                memcpy(new_text + new_action->length, prev->text, prev->length);
            }
            new_text[new_length] = '\0';
            
            if (prev->owns_text && prev->text) {
                free(prev->text);
            }
            
            prev->text = new_text;
            prev->length = new_length;
            prev->owns_text = true;
            prev->position = new_action->position;
            prev->old_cursor = new_action->old_cursor;
            prev->old_char_cursor = new_action->old_char_cursor;
            
            return true;
        } else if (prev->position == new_action->position + new_action->length) {
            // Delete key pattern - append new text
            size_t new_length = prev->length + new_action->length;
            char *new_text = malloc(new_length + 1);
            if (!new_text) {
                return false;
            }
            
            if (prev->text) {
                memcpy(new_text, prev->text, prev->length);
            }
            if (new_action->text) {
                memcpy(new_text + prev->length, new_action->text, new_action->length);
            }
            new_text[new_length] = '\0';
            
            if (prev->owns_text && prev->text) {
                free(prev->text);
            }
            
            prev->text = new_text;
            prev->length = new_length;
            prev->owns_text = true;
            prev->new_cursor = new_action->new_cursor;
            prev->new_char_cursor = new_action->new_char_cursor;
            
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// Public API Implementation
// ============================================================================

/**
 * @brief Create a new undo stack with specified capacity
 * 
 * Convenience function that creates and initializes an undo stack with
 * the specified maximum number of actions. Uses default configuration
 * for merging and cleanup behavior.
 * 
 * @param max_actions Maximum number of undo actions to keep (between LLE_MIN_UNDO_ACTIONS and LLE_MAX_UNDO_ACTIONS)
 * @return Pointer to newly created undo stack, or NULL on error
 * 
 * @note This is an alias for lle_undo_stack_create_custom() with default settings
 * @note The returned stack must be freed with lle_undo_destroy()
 */
lle_undo_stack_t *lle_undo_create(size_t max_actions) {
    return lle_undo_stack_create_custom(max_actions, false, true);
}

/**
 * @brief Destroy an undo stack created with lle_undo_create()
 * 
 * Convenience function that properly cleans up and frees an undo stack.
 * This is the proper cleanup function for stacks created with lle_undo_create().
 * 
 * @param stack Pointer to undo stack to destroy (can be NULL)
 * 
 * @note This is an alias for lle_undo_stack_destroy()
 * @note This function is safe to call with NULL pointer
 */
void lle_undo_destroy(lle_undo_stack_t *stack) {
    lle_undo_stack_destroy(stack);
}

/**
 * @brief Record an editing operation for undo functionality
 * 
 * Records a single editing operation in the undo stack, storing all necessary
 * information to reverse the operation later. The function handles memory
 * management, stack capacity limits, and optional action merging.
 * 
 * When the stack reaches its maximum capacity, the oldest actions are
 * automatically removed. Recording a new action clears any available
 * redo operations.
 * 
 * @param stack Pointer to undo stack (must not be NULL)
 * @param type Type of editing operation being recorded
 * @param position Byte position where the operation occurred
 * @param text Text content involved in the operation (can be NULL for cursor moves)
 * @param old_cursor Cursor position before the operation (in bytes)
 * @return true on successful recording, false on error
 * 
 * @note Text content is copied into the stack's own memory
 * @note Stack capacity is managed automatically
 * @note Recording an action invalidates any redo operations
 * @note Character positions are calculated as approximations of byte positions
 * 
 * @see lle_undo_action_type_t for supported operation types
 */
bool lle_undo_record_action(lle_undo_stack_t *stack,
                           lle_undo_action_type_t type,
                           size_t position,
                           const char *text,
                           size_t old_cursor) {
    if (!stack || !stack->actions) {
        return false;
    }
    
    // Validate action type
    if (type != LLE_UNDO_INSERT && type != LLE_UNDO_DELETE &&
        type != LLE_UNDO_MOVE_CURSOR && type != LLE_UNDO_REPLACE) {
        return false;
    }
    
    // For text operations, text must not be NULL
    if ((type == LLE_UNDO_INSERT || type == LLE_UNDO_DELETE || type == LLE_UNDO_REPLACE) && !text) {
        return false;
    }
    
    // Prepare new action
    lle_undo_action_t new_action = {0};
    new_action.type = type;
    new_action.position = position;
    new_action.old_cursor = old_cursor;
    new_action.new_cursor = old_cursor; // Will be updated by caller if needed
    
    // Approximate character positions (basic implementation)
    new_action.char_position = position;
    new_action.old_char_cursor = old_cursor;
    new_action.new_char_cursor = old_cursor;
    
    // Handle text content
    if (text) {
        new_action.length = strlen(text);
        // Always allocate memory for text, even empty strings
        new_action.text = malloc(new_action.length + 1);
        if (!new_action.text) {
            return false;
        }
        memcpy(new_action.text, text, new_action.length);
        new_action.text[new_action.length] = '\0';
        new_action.owns_text = true;
        
        // Update memory tracking
        stack->total_memory += (new_action.length + 1);
    } else {
        new_action.text = NULL;
        new_action.length = 0;
        new_action.owns_text = false;
    }
    
    // Check if we can merge with previous action
    bool merged = false;
    if (stack->merge_similar && stack->current > 0) {
        lle_undo_action_t *prev_action = &stack->actions[stack->current - 1];
        if (lle_undo_can_merge_actions(prev_action, &new_action)) {
            if (lle_undo_merge_actions(prev_action, &new_action)) {
                merged = true;
                // Free the new action's text since it was merged
                if (new_action.owns_text && new_action.text) {
                    stack->total_memory -= (new_action.length + 1);
                    free(new_action.text);
                }
            }
        }
    }
    
    if (!merged) {
        // Ensure we have capacity for the new action
        if (!lle_undo_ensure_capacity(stack)) {
            if (new_action.owns_text && new_action.text) {
                stack->total_memory -= (new_action.length + 1);
                free(new_action.text);
            }
            return false;
        }
        
        // Update state (clears redo actions if any)
        lle_undo_update_state(stack);
        
        // Add the new action
        stack->actions[stack->current] = new_action;
        stack->current++;
        stack->count = stack->current;
    }
    
    // Update final state
    stack->undo_count = stack->current;
    stack->redo_count = 0;
    stack->can_undo = (stack->undo_count > 0);
    stack->can_redo = false;
    
    return true;
}

/**
 * @brief Check if undo operation is available
 */
bool lle_undo_stack_can_undo(const lle_undo_stack_t *stack) {
    if (!stack) {
        return false;
    }
    
    return stack->can_undo && stack->undo_count > 0;
}

/**
 * @brief Check if redo operation is available
 */
bool lle_undo_stack_can_redo(const lle_undo_stack_t *stack) {
    if (!stack) {
        return false;
    }
    
    return stack->can_redo && stack->redo_count > 0;
}

/**
 * @brief Get the number of available undo operations
 */
size_t lle_undo_stack_undo_count(const lle_undo_stack_t *stack) {
    if (!stack) {
        return 0;
    }
    
    return stack->undo_count;
}

/**
 * @brief Get the number of available redo operations
 */
size_t lle_undo_stack_redo_count(const lle_undo_stack_t *stack) {
    if (!stack) {
        return 0;
    }
    
    return stack->redo_count;
}

/**
 * @brief Get total memory usage of the undo stack
 */
size_t lle_undo_stack_memory_usage(const lle_undo_stack_t *stack) {
    if (!stack) {
        return 0;
    }
    
    size_t total = sizeof(lle_undo_stack_t); // Stack structure itself
    total += stack->capacity * sizeof(lle_undo_action_t); // Actions array
    
    // Add text memory for all actions (this matches stack->total_memory)
    for (size_t i = 0; i < stack->count; i++) {
        if (stack->actions[i].owns_text && stack->actions[i].text) {
            total += stack->actions[i].length + 1; // +1 for null terminator
        }
    }
    
    return total;
}

/**
 * @brief Get peak memory usage of the undo stack
 */
size_t lle_undo_stack_peak_memory(const lle_undo_stack_t *stack) {
    if (!stack) {
        return 0;
    }
    
    return stack->peak_memory;
}

/**
 * @brief Validate undo stack structure and state
 */
bool lle_undo_stack_validate(const lle_undo_stack_t *stack) {
    if (!stack) {
        return false;
    }
    
    // Check basic structure validity
    if (!stack->actions) {
        return false;
    }
    
    if (stack->capacity == 0) {
        return false;
    }
    
    if (stack->count > stack->capacity) {
        return false;
    }
    
    if (stack->current > stack->count) {
        return false;
    }
    
    if (stack->max_actions < LLE_MIN_UNDO_ACTIONS || 
        stack->max_actions > LLE_MAX_UNDO_ACTIONS) {
        return false;
    }
    
    // Check state consistency
    if (stack->count == 0) {
        if (stack->can_undo || stack->can_redo) {
            return false;
        }
        if (stack->undo_count != 0 || stack->redo_count != 0) {
            return false;
        }
    }
    
    // Validate individual actions
    for (size_t i = 0; i < stack->count; i++) {
        const lle_undo_action_t *action = &stack->actions[i];
        
        // Check action type is valid
        if (action->type != LLE_UNDO_INSERT &&
            action->type != LLE_UNDO_DELETE &&
            action->type != LLE_UNDO_MOVE_CURSOR &&
            action->type != LLE_UNDO_REPLACE) {
            return false;
        }
        
        // Check text consistency
        if (action->text) {
            if (action->length == 0) {
                return false;
            }
        } else {
            if (action->length != 0) {
                return false;
            }
        }
    }
    
    return true;
}

// ============================================================================
// LLE-034: Undo/Redo Execution Implementation
// ============================================================================

/**
 * @brief Execute an undo operation on the text buffer
 * 
 * Reverses the most recent operation by applying the inverse of the recorded
 * action. Updates the cursor position appropriately and maintains redo capability.
 * 
 * @param stack Pointer to undo stack (must not be NULL)
 * @param buffer Pointer to text buffer to modify (must not be NULL)
 * @return true on successful undo, false on error or if no undo available
 * 
 * @note Maintains redo capability by preserving action data
 * @note Updates cursor position to pre-action state
 * @note Handles all action types (INSERT, DELETE, MOVE_CURSOR, REPLACE)
 */
bool lle_undo_execute(lle_undo_stack_t *stack, lle_text_buffer_t *buffer) {
    // Validate parameters
    if (!stack || !buffer) {
        return false;
    }
    
    // Check if undo is available
    if (!lle_undo_stack_can_undo(stack)) {
        return false;
    }
    
    // Get the action to undo (current position - 1)
    size_t action_index = stack->current - 1;
    lle_undo_action_t *action = &stack->actions[action_index];
    
    // Execute the inverse operation based on action type
    bool success = false;
    
    switch (action->type) {
        case LLE_UNDO_INSERT:
            // Undo insert: delete the inserted text
            success = lle_text_delete_range(buffer, action->position, 
                                          action->position + action->length);
            break;
            
        case LLE_UNDO_DELETE:
            // Undo delete: insert the deleted text back
            success = lle_text_insert_at(buffer, action->position, action->text);
            break;
            
        case LLE_UNDO_REPLACE:
            // Undo replace: restore original text
            // For replace operations, we need to calculate replacement length
            // Use cursor difference: if cursor moved from old_cursor to current position,
            // the replacement text length is the difference
            
            if (action->text && action->length > 0) {
                // Calculate replacement length based on cursor position difference
                // Current cursor position indicates end of replacement text
                size_t replacement_start = action->position;
                size_t replacement_end = buffer->cursor_pos;
                

                
                // If cursor is before the replacement position, assume no replacement
                if (replacement_end < replacement_start) {
                    replacement_end = replacement_start;
                }
                
                // Delete the replacement text (from start to current cursor)
                if (replacement_end > replacement_start) {

                    success = lle_text_delete_range(buffer, replacement_start, replacement_end);

                } else {
                    success = true; // Nothing to delete
                }
                
                // Then insert the original text
                if (success) {

                    success = lle_text_insert_at(buffer, replacement_start, action->text);

                }
            } else {
                success = false;
            }
            break;
            
        case LLE_UNDO_MOVE_CURSOR:
            // Undo cursor move: restore old cursor position
            success = lle_text_set_cursor(buffer, action->old_cursor);
            break;
            
        default:
            return false; // Unknown action type
    }
    
    if (!success) {
        return false;
    }
    
    // Restore cursor position to pre-action state
    if (action->type != LLE_UNDO_MOVE_CURSOR) {
        lle_text_set_cursor(buffer, action->old_cursor);
    }
    
    // Update stack state for undo
    stack->current--;
    stack->undo_count--;
    stack->redo_count++;
    stack->can_undo = (stack->current > 0);
    stack->can_redo = true;
    
    return true;
}

/**
 * @brief Execute a redo operation on the text buffer
 * 
 * Re-applies a previously undone operation. Updates the cursor position
 * appropriately and maintains undo capability.
 * 
 * @param stack Pointer to undo stack (must not be NULL)
 * @param buffer Pointer to text buffer to modify (must not be NULL)
 * @return true on successful redo, false on error or if no redo available
 * 
 * @note Maintains undo capability by preserving action data
 * @note Updates cursor position to post-action state
 * @note Handles all action types (INSERT, DELETE, MOVE_CURSOR, REPLACE)
 */
bool lle_redo_execute(lle_undo_stack_t *stack, lle_text_buffer_t *buffer) {
    // Validate parameters
    if (!stack || !buffer) {
        return false;
    }
    
    // Check if redo is available
    if (!lle_undo_stack_can_redo(stack)) {
        return false;
    }
    
    // Get the action to redo (at current position)
    lle_undo_action_t *action = &stack->actions[stack->current];
    
    // Execute the original operation based on action type
    bool success = false;
    
    switch (action->type) {
        case LLE_UNDO_INSERT:
            // Redo insert: insert the text again
            success = lle_text_insert_at(buffer, action->position, action->text);
            break;
            
        case LLE_UNDO_DELETE:
            // Redo delete: delete the text again
            success = lle_text_delete_range(buffer, action->position, 
                                          action->position + action->length);
            break;
            
        case LLE_UNDO_REPLACE:
            // Redo replace: apply the replacement again
            // First delete the original text
            if (action->text) {
                success = lle_text_delete_range(buffer, action->position, 
                                              action->position + strlen(action->text));
                if (success) {
                    // Then insert the replacement text
                    success = lle_text_insert_at(buffer, action->position, action->text);
                }
            }
            break;
            
        case LLE_UNDO_MOVE_CURSOR:
            // Redo cursor move: move to new cursor position
            success = lle_text_set_cursor(buffer, action->new_cursor);
            break;
            
        default:
            return false; // Unknown action type
    }
    
    if (!success) {
        return false;
    }
    
    // Update cursor position to post-action state
    if (action->type != LLE_UNDO_MOVE_CURSOR) {
        // For text operations, cursor should be at the end of the operation
        size_t new_cursor_pos = action->position;
        if (action->type == LLE_UNDO_INSERT || action->type == LLE_UNDO_REPLACE) {
            new_cursor_pos += action->length;
        }
        lle_text_set_cursor(buffer, new_cursor_pos);
    }
    
    // Update stack state for redo
    stack->current++;
    stack->undo_count++;
    stack->redo_count--;
    stack->can_undo = true;
    stack->can_redo = (stack->current < stack->count);
    
    return true;
}

/**
 * @brief Check if undo operation is available
 * 
 * Convenience function that determines whether there are any actions
 * available for undo. This is an alias for lle_undo_stack_can_undo().
 * 
 * @param stack Pointer to undo stack to check (must not be NULL)
 * @return true if undo is available, false otherwise
 * 
 * @note Returns false for NULL stack pointer
 */
bool lle_undo_can_undo(lle_undo_stack_t *stack) {
    return lle_undo_stack_can_undo(stack);
}

/**
 * @brief Check if redo operation is available
 * 
 * Convenience function that determines whether there are any actions
 * available for redo. This is an alias for lle_undo_stack_can_redo().
 * 
 * @param stack Pointer to undo stack to check (must not be NULL)
 * @return true if redo is available, false otherwise
 * 
 * @note Returns false for NULL stack pointer
 */
bool lle_redo_can_redo(lle_undo_stack_t *stack) {
    return lle_undo_stack_can_redo(stack);
}