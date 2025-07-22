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