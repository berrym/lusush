/*
 * Lusush Line Editor - Undo/Redo System
 * 
 * This file defines the undo/redo system structures for the Lusush Line Editor.
 * It provides comprehensive undo/redo functionality with efficient action storage,
 * configurable stack size, and support for all editing operations with cursor
 * position tracking.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#ifndef LLE_UNDO_H
#define LLE_UNDO_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_undo_action lle_undo_action_t;
typedef struct lle_undo_stack lle_undo_stack_t;

/**
 * @brief Default maximum number of undo actions to keep
 */
#define LLE_DEFAULT_MAX_UNDO_ACTIONS 100

/**
 * @brief Minimum number of undo actions (safety limit)
 */
#define LLE_MIN_UNDO_ACTIONS 10

/**
 * @brief Maximum number of undo actions (memory limit)
 */
#define LLE_MAX_UNDO_ACTIONS 1000

/**
 * @brief Types of undo actions that can be recorded
 * 
 * These action types cover all possible editing operations that can be
 * undone or redone. Each action type requires different data to be stored
 * for proper reversal.
 */
typedef enum {
    LLE_UNDO_INSERT,        /**< Text insertion operation */
    LLE_UNDO_DELETE,        /**< Text deletion operation */
    LLE_UNDO_MOVE_CURSOR,   /**< Cursor movement operation */
    LLE_UNDO_REPLACE        /**< Text replacement operation */
} lle_undo_action_type_t;

/**
 * @brief Individual undo action structure
 * 
 * Represents a single undoable action with all necessary information
 * to reverse the operation. The structure is designed to be memory-efficient
 * while providing complete state restoration.
 */
struct lle_undo_action {
    lle_undo_action_type_t type;    /**< Type of action performed */
    size_t position;                /**< Byte position where action occurred */
    char *text;                     /**< Text involved in action (NULL for cursor moves) */
    size_t length;                  /**< Length of text in bytes */
    size_t old_cursor;              /**< Cursor position before action */
    
    // Additional context for complex operations
    size_t new_cursor;              /**< Cursor position after action */
    size_t char_position;           /**< Character position (for UTF-8 support) */
    size_t old_char_cursor;         /**< Character cursor position before action */
    size_t new_char_cursor;         /**< Character cursor position after action */
    
    // Memory management
    bool owns_text;                 /**< True if this action owns the text memory */
};

/**
 * @brief Undo stack structure for managing undo/redo operations
 * 
 * Maintains a circular buffer of undo actions with efficient insertion,
 * removal, and navigation. The stack supports both undo and redo operations
 * with configurable size limits and automatic cleanup.
 */
struct lle_undo_stack {
    lle_undo_action_t *actions;     /**< Array of undo actions */
    size_t count;                   /**< Current number of actions in stack */
    size_t capacity;                /**< Allocated capacity of actions array */
    size_t current;                 /**< Current position in undo stack (for redo) */
    size_t max_actions;             /**< Maximum actions to keep (configurable) */
    
    // State tracking
    bool can_undo;                  /**< True if undo operation is available */
    bool can_redo;                  /**< True if redo operation is available */
    size_t undo_count;              /**< Number of available undo operations */
    size_t redo_count;              /**< Number of available redo operations */
    
    // Memory and performance statistics
    size_t total_memory;            /**< Total memory used by all actions */
    size_t peak_memory;             /**< Peak memory usage */
    size_t operations_count;        /**< Total operations performed */
    
    // Configuration
    bool merge_similar;             /**< True to merge similar consecutive actions */
    size_t merge_timeout_ms;        /**< Timeout for merging operations (milliseconds) */
    bool auto_cleanup;              /**< True to automatically clean old actions */
};

/**
 * @brief Initialize an undo stack with default configuration
 * 
 * Initializes the undo stack structure with sensible defaults. The stack
 * will be ready for use with the default maximum action limit and 
 * automatic cleanup enabled.
 * 
 * @param stack Pointer to undo stack structure to initialize (must not be NULL)
 * @return true on successful initialization, false on error
 * 
 * @note The stack must be destroyed with lle_undo_stack_destroy() when no longer needed
 * @note Initial capacity is allocated based on LLE_DEFAULT_MAX_UNDO_ACTIONS
 */
bool lle_undo_stack_init(lle_undo_stack_t *stack);

/**
 * @brief Initialize an undo stack with custom configuration
 * 
 * Initializes the undo stack with a specific maximum action limit and
 * optional configuration parameters for advanced use cases.
 * 
 * @param stack Pointer to undo stack structure to initialize (must not be NULL)
 * @param max_actions Maximum number of actions to keep (between LLE_MIN_UNDO_ACTIONS and LLE_MAX_UNDO_ACTIONS)
 * @param merge_similar True to enable merging of similar consecutive actions
 * @param auto_cleanup True to enable automatic cleanup of old actions
 * @return true on successful initialization, false on error or invalid parameters
 * 
 * @note The stack must be destroyed with lle_undo_stack_destroy() when no longer needed
 */
bool lle_undo_stack_init_custom(lle_undo_stack_t *stack, size_t max_actions, 
                                bool merge_similar, bool auto_cleanup);

/**
 * @brief Create a new undo stack with default configuration
 * 
 * Allocates and initializes a new undo stack structure on the heap
 * with default configuration suitable for most use cases.
 * 
 * @return Pointer to newly created undo stack, or NULL on allocation failure
 * 
 * @note The returned stack must be freed with lle_undo_stack_destroy()
 * @note Use lle_undo_stack_create_custom() for non-default configuration
 */
lle_undo_stack_t *lle_undo_stack_create(void);

/**
 * @brief Create a new undo stack with custom configuration
 * 
 * Allocates and initializes a new undo stack structure on the heap
 * with specified configuration parameters.
 * 
 * @param max_actions Maximum number of actions to keep
 * @param merge_similar True to enable merging of similar consecutive actions
 * @param auto_cleanup True to enable automatic cleanup of old actions
 * @return Pointer to newly created undo stack, or NULL on allocation failure or invalid parameters
 * 
 * @note The returned stack must be freed with lle_undo_stack_destroy()
 */
lle_undo_stack_t *lle_undo_stack_create_custom(size_t max_actions, 
                                               bool merge_similar, bool auto_cleanup);

/**
 * @brief Destroy an undo stack and free all memory
 * 
 * Cleans up all actions in the stack, frees all allocated memory,
 * and resets the stack structure. After calling this function,
 * the stack structure should not be used until re-initialized.
 * 
 * @param stack Pointer to undo stack to destroy (can be NULL)
 * 
 * @note This function is safe to call with NULL pointer
 * @note All text memory owned by actions will be freed
 * @note The stack structure itself is not freed (use free() for heap-allocated stacks)
 */
void lle_undo_stack_cleanup(lle_undo_stack_t *stack);

/**
 * @brief Destroy and free an undo stack created with lle_undo_stack_create()
 * 
 * Cleans up all actions, frees all memory, and frees the stack structure
 * itself. This is the proper cleanup function for heap-allocated stacks.
 * 
 * @param stack Pointer to undo stack to destroy and free (can be NULL)
 * 
 * @note This function is safe to call with NULL pointer
 * @note After calling this function, the stack pointer becomes invalid
 */
void lle_undo_stack_destroy(lle_undo_stack_t *stack);

/**
 * @brief Clear all actions from the undo stack
 * 
 * Removes all actions from the stack and resets state, but keeps
 * the stack structure and configuration intact for continued use.
 * 
 * @param stack Pointer to undo stack to clear (must not be NULL)
 * @return true on success, false on invalid parameters
 * 
 * @note All text memory owned by actions will be freed
 * @note Stack capacity and configuration remain unchanged
 */
bool lle_undo_stack_clear(lle_undo_stack_t *stack);

/**
 * @brief Check if undo operation is available
 * 
 * Determines whether there are any actions available for undo.
 * 
 * @param stack Pointer to undo stack to check (must not be NULL)
 * @return true if undo is available, false otherwise
 * 
 * @note Returns false for NULL stack pointer
 */
bool lle_undo_stack_can_undo(const lle_undo_stack_t *stack);

/**
 * @brief Check if redo operation is available
 * 
 * Determines whether there are any actions available for redo.
 * 
 * @param stack Pointer to undo stack to check (must not be NULL)
 * @return true if redo is available, false otherwise
 * 
 * @note Returns false for NULL stack pointer
 */
bool lle_undo_stack_can_redo(const lle_undo_stack_t *stack);

/**
 * @brief Get the number of available undo operations
 * 
 * Returns the count of actions that can be undone from the current position.
 * 
 * @param stack Pointer to undo stack to query (must not be NULL)
 * @return Number of available undo operations, 0 if none or on error
 */
size_t lle_undo_stack_undo_count(const lle_undo_stack_t *stack);

/**
 * @brief Get the number of available redo operations
 * 
 * Returns the count of actions that can be redone from the current position.
 * 
 * @param stack Pointer to undo stack to query (must not be NULL)
 * @return Number of available redo operations, 0 if none or on error
 */
size_t lle_undo_stack_redo_count(const lle_undo_stack_t *stack);

/**
 * @brief Get total memory usage of the undo stack
 * 
 * Calculates the total memory usage including the stack structure,
 * actions array, and all text data stored in actions.
 * 
 * @param stack Pointer to undo stack to analyze (must not be NULL)
 * @return Total memory usage in bytes, 0 on error
 * 
 * @note This includes all dynamically allocated memory
 */
size_t lle_undo_stack_memory_usage(const lle_undo_stack_t *stack);

/**
 * @brief Get peak memory usage of the undo stack
 * 
 * Returns the highest memory usage recorded during the stack's lifetime.
 * Useful for memory usage analysis and optimization.
 * 
 * @param stack Pointer to undo stack to query (must not be NULL)
 * @return Peak memory usage in bytes, 0 on error
 */
size_t lle_undo_stack_peak_memory(const lle_undo_stack_t *stack);

/**
 * @brief Validate undo stack structure and state
 * 
 * Performs comprehensive validation of the undo stack structure,
 * checking for consistency, valid pointers, and proper state.
 * Useful for debugging and testing.
 * 
 * @param stack Pointer to undo stack to validate (can be NULL)
 * @return true if stack is valid and consistent, false otherwise
 * 
 * @note Returns false for NULL stack pointer
 * @note Checks action array consistency, memory ownership, and state variables
 */
bool lle_undo_stack_validate(const lle_undo_stack_t *stack);

#ifdef __cplusplus
}
#endif

#endif // LLE_UNDO_H