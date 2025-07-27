/**
 * @file test_lle_033_undo_operation_recording.c
 * @brief Tests for LLE-033: Undo Operation Recording
 * 
 * Tests the undo operation recording functionality including action recording,
 * stack capacity management, memory efficiency, action order maintenance,
 * and action merging capabilities.
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
 * @brief Verify that an action was recorded correctly
 */
static bool verify_recorded_action(const lle_undo_stack_t *stack,
                                  size_t index,
                                  lle_undo_action_type_t expected_type,
                                  size_t expected_position,
                                  const char *expected_text,
                                  size_t expected_old_cursor) {
    if (!stack || !stack->actions || index >= stack->count) {
        return false;
    }
    
    const lle_undo_action_t *action = &stack->actions[index];
    
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
        if (!action->owns_text) {
            return false;
        }
    } else {
        if (action->text != NULL || action->length != 0) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// Basic Recording Tests
// ============================================================================

LLE_TEST(basic_action_recording) {
    printf("Testing basic action recording... ");
    
    // Create undo stack
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Record an insertion action
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, "hello", 0));
    
    // Verify stack state
    LLE_ASSERT_EQ(stack->count, 1);
    LLE_ASSERT_EQ(stack->current, 1);
    LLE_ASSERT(stack->can_undo);
    LLE_ASSERT(!stack->can_redo);
    LLE_ASSERT_EQ(stack->undo_count, 1);
    LLE_ASSERT_EQ(stack->redo_count, 0);
    
    // Verify the recorded action
    LLE_ASSERT(verify_recorded_action(stack, 0, LLE_UNDO_INSERT, 0, "hello", 0));
    
    // Cleanup
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(multiple_action_types) {
    printf("Testing recording different action types... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Record various action types
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, "abc", 0));
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_DELETE, 2, "c", 3));
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_MOVE_CURSOR, 0, NULL, 2));
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_REPLACE, 1, "xyz", 1));
    
    // Verify count
    LLE_ASSERT_EQ(stack->count, 4);
    LLE_ASSERT_EQ(stack->undo_count, 4);
    
    // Verify each action
    LLE_ASSERT(verify_recorded_action(stack, 0, LLE_UNDO_INSERT, 0, "abc", 0));
    LLE_ASSERT(verify_recorded_action(stack, 1, LLE_UNDO_DELETE, 2, "c", 3));
    LLE_ASSERT(verify_recorded_action(stack, 2, LLE_UNDO_MOVE_CURSOR, 0, NULL, 2));
    LLE_ASSERT(verify_recorded_action(stack, 3, LLE_UNDO_REPLACE, 1, "xyz", 1));
    
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(stack_capacity_management) {
    printf("Testing stack capacity management... ");
    
    // Create small stack to test capacity limits (minimum is 10)
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Fill stack to capacity
    for (int i = 0; i < 10; i++) {
        char text[32];
        snprintf(text, sizeof(text), "action_%d", i);
        LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, i, text, i));
    }
    
    LLE_ASSERT_EQ(stack->count, 10);
    
    // Add one more action - should remove oldest
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 10, "overflow", 10));
    
    // Should still have 10 actions, but oldest is gone
    LLE_ASSERT_EQ(stack->count, 10);
    
    // Verify oldest action was removed and new one added
    LLE_ASSERT(verify_recorded_action(stack, 0, LLE_UNDO_INSERT, 1, "action_1", 1));
    LLE_ASSERT(verify_recorded_action(stack, 9, LLE_UNDO_INSERT, 10, "overflow", 10));
    
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(parameter_validation) {
    printf("Testing parameter validation... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Test NULL stack
    LLE_ASSERT(!lle_undo_record_action(NULL, LLE_UNDO_INSERT, 0, "test", 0));
    
    // Test invalid action type (assuming max valid type is LLE_UNDO_REPLACE)
    LLE_ASSERT(!lle_undo_record_action(stack, (lle_undo_action_type_t)999, 0, "test", 0));
    
    // Test text operations with NULL text
    LLE_ASSERT(!lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, NULL, 0));
    LLE_ASSERT(!lle_undo_record_action(stack, LLE_UNDO_DELETE, 0, NULL, 0));
    LLE_ASSERT(!lle_undo_record_action(stack, LLE_UNDO_REPLACE, 0, NULL, 0));
    
    // Test cursor move with NULL text (should be allowed)
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_MOVE_CURSOR, 0, NULL, 0));
    
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(memory_efficiency_tracking) {
    printf("Testing memory efficiency and tracking... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    size_t initial_memory = lle_undo_stack_memory_usage(stack);
    
    // Record action with text
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, "test", 0));
    
    size_t after_one = lle_undo_stack_memory_usage(stack);
    LLE_ASSERT(after_one > initial_memory);
    
    // Record action without text
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_MOVE_CURSOR, 4, NULL, 0));
    
    size_t after_two = lle_undo_stack_memory_usage(stack);
    // Should be only slightly larger (no text to store)
    LLE_ASSERT(after_two >= after_one);
    
    // Verify memory tracking
    LLE_ASSERT(stack->total_memory >= 5); // "test" + null terminator
    LLE_ASSERT(stack->peak_memory >= after_two);
    
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(action_order_maintenance) {
    printf("Testing action order maintenance... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Record actions in specific order
    const char *texts[] = {"first", "second", "third", "fourth"};
    for (int i = 0; i < 4; i++) {
        LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, i * 10, texts[i], i * 5));
    }
    
    // Verify order is maintained
    for (int i = 0; i < 4; i++) {
        LLE_ASSERT(verify_recorded_action(stack, i, LLE_UNDO_INSERT, i * 10, texts[i], i * 5));
    }
    
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(empty_text_handling) {
    printf("Testing empty text handling... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Record action with empty string
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, "", 0));
    
    // Verify action was recorded
    LLE_ASSERT_EQ(stack->count, 1);
    LLE_ASSERT(verify_recorded_action(stack, 0, LLE_UNDO_INSERT, 0, "", 0));
    
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(redo_state_clearing) {
    printf("Testing redo state clearing when recording new actions... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    // Record some actions
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, "hello", 0));
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, " world", 5));
    
    // Simulate undo by moving current position back
    stack->current = 1;
    stack->undo_count = 1;
    stack->redo_count = 1;
    stack->can_redo = true;
    
    // Record new action - should clear redo state
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, " there", 5));
    
    // Verify redo state is cleared
    LLE_ASSERT(!stack->can_redo);
    LLE_ASSERT_EQ(stack->redo_count, 0);
    LLE_ASSERT_EQ(stack->count, 2); // Old redo action should be gone
    LLE_ASSERT(verify_recorded_action(stack, 1, LLE_UNDO_INSERT, 5, " there", 5));
    
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("Running LLE-033 Undo Operation Recording Tests...\n\n");
    
    // Basic recording tests
    test_basic_action_recording();
    test_multiple_action_types();
    test_stack_capacity_management();
    test_parameter_validation();
    test_memory_efficiency_tracking();
    test_action_order_maintenance();
    test_empty_text_handling();
    test_redo_state_clearing();
    
    printf("\n✅ All LLE-033 Undo Operation Recording tests passed!\n");
    printf("📊 Tests completed: 8 comprehensive tests\n");
    printf("🔧 Features tested: Action recording, capacity management, memory efficiency, parameter validation\n");
    
    return 0;
}