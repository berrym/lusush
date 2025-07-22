/**
 * @file test_lle_032_undo_stack_structure.c
 * @brief Tests for LLE-032: Undo Stack Structure
 * 
 * Tests the undo/redo system data structures and basic functionality including
 * creation, initialization, validation, configuration, and memory management.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "undo.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Create a test undo action for testing
 * @param type Action type
 * @param position Byte position
 * @param text Text content (will be copied)
 * @param old_cursor Old cursor position
 * @param new_cursor New cursor position
 * @return Pointer to created action (must be freed)
 */
static lle_undo_action_t *create_test_action(lle_undo_action_type_t type, 
                                            size_t position, 
                                            const char *text,
                                            size_t old_cursor,
                                            size_t new_cursor) {
    lle_undo_action_t *action = malloc(sizeof(lle_undo_action_t));
    if (!action) {
        return NULL;
    }
    
    action->type = type;
    action->position = position;
    action->old_cursor = old_cursor;
    action->new_cursor = new_cursor;
    action->char_position = position; // Simple approximation for tests
    action->old_char_cursor = old_cursor;
    action->new_char_cursor = new_cursor;
    
    if (text) {
        action->length = strlen(text);
        action->text = malloc(action->length + 1);
        if (action->text) {
            memcpy(action->text, text, action->length);
            action->text[action->length] = '\0';
            action->owns_text = true;
        } else {
            free(action);
            return NULL;
        }
    } else {
        action->text = NULL;
        action->length = 0;
        action->owns_text = false;
    }
    
    return action;
}

/**
 * @brief Free a test action created with create_test_action
 * @param action Action to free
 */
static void free_test_action(lle_undo_action_t *action) {
    if (!action) return;
    
    if (action->owns_text && action->text) {
        free(action->text);
    }
    free(action);
}

/**
 * @brief Verify undo action content
 * @param action Action to verify
 * @param expected_type Expected action type
 * @param expected_position Expected position
 * @param expected_text Expected text (can be NULL)
 * @param expected_old_cursor Expected old cursor position
 * @return true if action matches expected content
 */
static bool verify_undo_action(const lle_undo_action_t *action,
                              lle_undo_action_type_t expected_type,
                              size_t expected_position,
                              const char *expected_text,
                              size_t expected_old_cursor) {
    if (!action) {
        return false;
    }
    
    if (action->type != expected_type) {
        return false;
    }
    
    if (action->position != expected_position) {
        return false;
    }
    
    if (action->old_cursor != expected_old_cursor) {
        return false;
    }
    
    if (expected_text) {
        if (!action->text) {
            return false;
        }
        if (strcmp(action->text, expected_text) != 0) {
            return false;
        }
        if (action->length != strlen(expected_text)) {
            return false;
        }
    } else {
        if (action->text != NULL) {
            return false;
        }
        if (action->length != 0) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// Basic Structure Tests
// ============================================================================

LLE_TEST(undo_stack_init_default) {
    printf("Testing undo stack initialization with defaults... ");
    
    lle_undo_stack_t stack;
    LLE_ASSERT(lle_undo_stack_init(&stack));
    
    // Verify default initialization
    LLE_ASSERT_NOT_NULL(stack.actions);
    LLE_ASSERT_EQ(stack.count, 0);
    LLE_ASSERT(stack.capacity > 0);
    LLE_ASSERT_EQ(stack.current, 0);
    LLE_ASSERT_EQ(stack.max_actions, LLE_DEFAULT_MAX_UNDO_ACTIONS);
    
    // Verify state
    LLE_ASSERT(!stack.can_undo);
    LLE_ASSERT(!stack.can_redo);
    LLE_ASSERT_EQ(stack.undo_count, 0);
    LLE_ASSERT_EQ(stack.redo_count, 0);
    
    // Verify statistics
    LLE_ASSERT_EQ(stack.total_memory, 0);
    LLE_ASSERT_EQ(stack.operations_count, 0);
    
    LLE_ASSERT(lle_undo_stack_validate(&stack));
    
    lle_undo_stack_cleanup(&stack);
    printf("PASSED\n");
}

LLE_TEST(undo_stack_init_custom) {
    printf("Testing undo stack initialization with custom parameters... ");
    
    lle_undo_stack_t stack;
    size_t max_actions = 50;
    bool merge_similar = true;
    bool auto_cleanup = false;
    
    LLE_ASSERT(lle_undo_stack_init_custom(&stack, max_actions, merge_similar, auto_cleanup));
    
    // Verify custom initialization
    LLE_ASSERT_NOT_NULL(stack.actions);
    LLE_ASSERT_EQ(stack.count, 0);
    LLE_ASSERT(stack.capacity > 0);
    LLE_ASSERT_EQ(stack.current, 0);
    LLE_ASSERT_EQ(stack.max_actions, max_actions);
    
    // Verify configuration
    LLE_ASSERT_EQ(stack.merge_similar, merge_similar);
    LLE_ASSERT_EQ(stack.auto_cleanup, auto_cleanup);
    
    LLE_ASSERT(lle_undo_stack_validate(&stack));
    
    lle_undo_stack_cleanup(&stack);
    printf("PASSED\n");
}

LLE_TEST(undo_stack_create_default) {
    printf("Testing undo stack creation with defaults... ");
    
    lle_undo_stack_t *stack = lle_undo_stack_create();
    LLE_ASSERT_NOT_NULL(stack);
    
    // Verify default creation
    LLE_ASSERT_NOT_NULL(stack->actions);
    LLE_ASSERT_EQ(stack->count, 0);
    LLE_ASSERT(stack->capacity > 0);
    LLE_ASSERT_EQ(stack->current, 0);
    LLE_ASSERT_EQ(stack->max_actions, LLE_DEFAULT_MAX_UNDO_ACTIONS);
    
    LLE_ASSERT(lle_undo_stack_validate(stack));
    
    lle_undo_stack_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_stack_create_custom) {
    printf("Testing undo stack creation with custom parameters... ");
    
    size_t max_actions = 200;
    bool merge_similar = false;
    bool auto_cleanup = true;
    
    lle_undo_stack_t *stack = lle_undo_stack_create_custom(max_actions, merge_similar, auto_cleanup);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Verify custom creation
    LLE_ASSERT_EQ(stack->max_actions, max_actions);
    LLE_ASSERT_EQ(stack->merge_similar, merge_similar);
    LLE_ASSERT_EQ(stack->auto_cleanup, auto_cleanup);
    
    LLE_ASSERT(lle_undo_stack_validate(stack));
    
    lle_undo_stack_destroy(stack);
    printf("PASSED\n");
}

// ============================================================================
// Validation Tests
// ============================================================================

LLE_TEST(undo_stack_validate_null) {
    printf("Testing undo stack validation with NULL... ");
    
    LLE_ASSERT(!lle_undo_stack_validate(NULL));
    
    printf("PASSED\n");
}

LLE_TEST(undo_stack_validate_valid) {
    printf("Testing undo stack validation with valid stack... ");
    
    lle_undo_stack_t *stack = lle_undo_stack_create();
    LLE_ASSERT_NOT_NULL(stack);
    
    LLE_ASSERT(lle_undo_stack_validate(stack));
    
    lle_undo_stack_destroy(stack);
    printf("PASSED\n");
}

// ============================================================================
// State Query Tests
// ============================================================================

LLE_TEST(undo_stack_can_undo_empty) {
    printf("Testing can_undo on empty stack... ");
    
    lle_undo_stack_t *stack = lle_undo_stack_create();
    LLE_ASSERT_NOT_NULL(stack);
    
    LLE_ASSERT(!lle_undo_stack_can_undo(stack));
    LLE_ASSERT_EQ(lle_undo_stack_undo_count(stack), 0);
    
    lle_undo_stack_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_stack_can_redo_empty) {
    printf("Testing can_redo on empty stack... ");
    
    lle_undo_stack_t *stack = lle_undo_stack_create();
    LLE_ASSERT_NOT_NULL(stack);
    
    LLE_ASSERT(!lle_undo_stack_can_redo(stack));
    LLE_ASSERT_EQ(lle_undo_stack_redo_count(stack), 0);
    
    lle_undo_stack_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_stack_can_undo_null) {
    printf("Testing can_undo with NULL stack... ");
    
    LLE_ASSERT(!lle_undo_stack_can_undo(NULL));
    
    printf("PASSED\n");
}

LLE_TEST(undo_stack_can_redo_null) {
    printf("Testing can_redo with NULL stack... ");
    
    LLE_ASSERT(!lle_undo_stack_can_redo(NULL));
    
    printf("PASSED\n");
}

LLE_TEST(undo_stack_counts_null) {
    printf("Testing count functions with NULL stack... ");
    
    LLE_ASSERT_EQ(lle_undo_stack_undo_count(NULL), 0);
    LLE_ASSERT_EQ(lle_undo_stack_redo_count(NULL), 0);
    
    printf("PASSED\n");
}

// ============================================================================
// Memory Management Tests
// ============================================================================

LLE_TEST(undo_stack_memory_usage_empty) {
    printf("Testing memory usage on empty stack... ");
    
    lle_undo_stack_t *stack = lle_undo_stack_create();
    LLE_ASSERT_NOT_NULL(stack);
    
    size_t memory = lle_undo_stack_memory_usage(stack);
    LLE_ASSERT(memory > 0); // At least the actions array
    
    size_t peak = lle_undo_stack_peak_memory(stack);
    LLE_ASSERT(peak >= memory);
    
    lle_undo_stack_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_stack_memory_usage_null) {
    printf("Testing memory usage with NULL stack... ");
    
    LLE_ASSERT_EQ(lle_undo_stack_memory_usage(NULL), 0);
    LLE_ASSERT_EQ(lle_undo_stack_peak_memory(NULL), 0);
    
    printf("PASSED\n");
}

LLE_TEST(undo_stack_clear_empty) {
    printf("Testing clear on empty stack... ");
    
    lle_undo_stack_t *stack = lle_undo_stack_create();
    LLE_ASSERT_NOT_NULL(stack);
    
    LLE_ASSERT(lle_undo_stack_clear(stack));
    
    // Should still be empty and valid
    LLE_ASSERT_EQ(lle_undo_stack_undo_count(stack), 0);
    LLE_ASSERT_EQ(lle_undo_stack_redo_count(stack), 0);
    LLE_ASSERT(!lle_undo_stack_can_undo(stack));
    LLE_ASSERT(!lle_undo_stack_can_redo(stack));
    LLE_ASSERT(lle_undo_stack_validate(stack));
    
    lle_undo_stack_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_stack_clear_null) {
    printf("Testing clear with NULL stack... ");
    
    LLE_ASSERT(!lle_undo_stack_clear(NULL));
    
    printf("PASSED\n");
}

// ============================================================================
// Configuration Validation Tests
// ============================================================================

LLE_TEST(undo_stack_init_invalid_params) {
    printf("Testing undo stack initialization with invalid parameters... ");
    
    lle_undo_stack_t stack;
    
    // Test NULL stack
    LLE_ASSERT(!lle_undo_stack_init(NULL));
    
    // Test invalid max_actions (too small)
    LLE_ASSERT(!lle_undo_stack_init_custom(&stack, LLE_MIN_UNDO_ACTIONS - 1, false, false));
    
    // Test invalid max_actions (too large)
    LLE_ASSERT(!lle_undo_stack_init_custom(&stack, LLE_MAX_UNDO_ACTIONS + 1, false, false));
    
    // Test valid boundary values
    LLE_ASSERT(lle_undo_stack_init_custom(&stack, LLE_MIN_UNDO_ACTIONS, false, false));
    lle_undo_stack_cleanup(&stack);
    
    LLE_ASSERT(lle_undo_stack_init_custom(&stack, LLE_MAX_UNDO_ACTIONS, false, false));
    lle_undo_stack_cleanup(&stack);
    
    printf("PASSED\n");
}

LLE_TEST(undo_stack_create_invalid_params) {
    printf("Testing undo stack creation with invalid parameters... ");
    
    // Test invalid max_actions (too small)
    lle_undo_stack_t *stack = lle_undo_stack_create_custom(LLE_MIN_UNDO_ACTIONS - 1, false, false);
    LLE_ASSERT_NULL(stack);
    
    // Test invalid max_actions (too large)
    stack = lle_undo_stack_create_custom(LLE_MAX_UNDO_ACTIONS + 1, false, false);
    LLE_ASSERT_NULL(stack);
    
    // Test valid boundary values
    stack = lle_undo_stack_create_custom(LLE_MIN_UNDO_ACTIONS, false, false);
    LLE_ASSERT_NOT_NULL(stack);
    lle_undo_stack_destroy(stack);
    
    stack = lle_undo_stack_create_custom(LLE_MAX_UNDO_ACTIONS, false, false);
    LLE_ASSERT_NOT_NULL(stack);
    lle_undo_stack_destroy(stack);
    
    printf("PASSED\n");
}

// ============================================================================
// Cleanup Safety Tests
// ============================================================================

LLE_TEST(undo_stack_cleanup_null) {
    printf("Testing cleanup with NULL stack... ");
    
    // Should not crash
    lle_undo_stack_cleanup(NULL);
    
    printf("PASSED\n");
}

LLE_TEST(undo_stack_destroy_null) {
    printf("Testing destroy with NULL stack... ");
    
    // Should not crash
    lle_undo_stack_destroy(NULL);
    
    printf("PASSED\n");
}

LLE_TEST(undo_stack_double_cleanup) {
    printf("Testing double cleanup safety... ");
    
    lle_undo_stack_t stack;
    LLE_ASSERT(lle_undo_stack_init(&stack));
    
    // First cleanup
    lle_undo_stack_cleanup(&stack);
    
    // Second cleanup should be safe
    lle_undo_stack_cleanup(&stack);
    
    printf("PASSED\n");
}

// ============================================================================
// Action Type Tests
// ============================================================================

LLE_TEST(undo_action_types) {
    printf("Testing undo action type enumeration... ");
    
    // Verify action types are defined
    lle_undo_action_type_t insert = LLE_UNDO_INSERT;
    lle_undo_action_type_t delete = LLE_UNDO_DELETE;
    lle_undo_action_type_t move = LLE_UNDO_MOVE_CURSOR;
    lle_undo_action_type_t replace = LLE_UNDO_REPLACE;
    
    // Basic sanity check - types should be different
    LLE_ASSERT(insert != delete);
    LLE_ASSERT(delete != move);
    LLE_ASSERT(move != replace);
    LLE_ASSERT(replace != insert);
    
    printf("PASSED\n");
}

// ============================================================================
// Test Helper Function Tests
// ============================================================================

LLE_TEST(test_action_creation) {
    printf("Testing test helper action creation... ");
    
    const char *test_text = "hello";
    lle_undo_action_t *action = create_test_action(LLE_UNDO_INSERT, 5, test_text, 0, 5);
    LLE_ASSERT_NOT_NULL(action);
    
    LLE_ASSERT(verify_undo_action(action, LLE_UNDO_INSERT, 5, test_text, 0));
    LLE_ASSERT_EQ(action->new_cursor, 5);
    LLE_ASSERT_EQ(action->length, strlen(test_text));
    LLE_ASSERT(action->owns_text);
    
    free_test_action(action);
    printf("PASSED\n");
}

LLE_TEST(test_action_creation_no_text) {
    printf("Testing test helper action creation without text... ");
    
    lle_undo_action_t *action = create_test_action(LLE_UNDO_MOVE_CURSOR, 10, NULL, 5, 10);
    LLE_ASSERT_NOT_NULL(action);
    
    LLE_ASSERT(verify_undo_action(action, LLE_UNDO_MOVE_CURSOR, 10, NULL, 5));
    LLE_ASSERT_EQ(action->new_cursor, 10);
    LLE_ASSERT_EQ(action->length, 0);
    LLE_ASSERT(!action->owns_text);
    LLE_ASSERT_NULL(action->text);
    
    free_test_action(action);
    printf("PASSED\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("Running LLE-032 Undo Stack Structure Tests...\n\n");
    
    // Basic structure tests
    test_undo_stack_init_default();
    test_undo_stack_init_custom();
    test_undo_stack_create_default();
    test_undo_stack_create_custom();
    
    // Validation tests
    test_undo_stack_validate_null();
    test_undo_stack_validate_valid();
    
    // State query tests
    test_undo_stack_can_undo_empty();
    test_undo_stack_can_redo_empty();
    test_undo_stack_can_undo_null();
    test_undo_stack_can_redo_null();
    test_undo_stack_counts_null();
    
    // Memory management tests
    test_undo_stack_memory_usage_empty();
    test_undo_stack_memory_usage_null();
    test_undo_stack_clear_empty();
    test_undo_stack_clear_null();
    
    // Configuration validation tests
    test_undo_stack_init_invalid_params();
    test_undo_stack_create_invalid_params();
    
    // Cleanup safety tests
    test_undo_stack_cleanup_null();
    test_undo_stack_destroy_null();
    test_undo_stack_double_cleanup();
    
    // Action type tests
    test_undo_action_types();
    
    // Test helper function tests
    test_test_action_creation();
    test_test_action_creation_no_text();
    
    printf("\nAll LLE-032 Undo Stack Structure tests passed!\n");
    return 0;
}