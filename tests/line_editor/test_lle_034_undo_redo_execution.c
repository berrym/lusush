/**
 * @file test_lle_034_undo_redo_execution.c
 * @brief Tests for LLE-034: Undo/Redo Execution
 * 
 * Tests the undo/redo execution functionality including operation reversal,
 * redo capability maintenance, cursor position updates, and edge case handling
 * for all action types (INSERT, DELETE, MOVE_CURSOR, REPLACE).
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "undo.h"
#include "text_buffer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Verify text buffer content and cursor position
 */
static bool verify_buffer_state(const lle_text_buffer_t *buffer,
                               const char *expected_text,
                               size_t expected_cursor) {
    if (!buffer) {
        return false;
    }
    
    // Check text content
    if (expected_text) {
        if (!buffer->buffer || strcmp(buffer->buffer, expected_text) != 0) {
            return false;
        }
        if (buffer->length != strlen(expected_text)) {
            return false;
        }
    } else {
        if (buffer->buffer && buffer->length > 0) {
            return false;
        }
    }
    
    // Check cursor position
    if (buffer->cursor_pos != expected_cursor) {
        return false;
    }
    
    return true;
}

/**
 * @brief Setup a text buffer with initial content and cursor position
 */
static lle_text_buffer_t *setup_buffer_with_text(const char *text, size_t cursor_pos) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    if (!buffer) {
        return NULL;
    }
    
    if (text && strlen(text) > 0) {
        if (!lle_text_insert_string(buffer, text)) {
            lle_text_buffer_destroy(buffer);
            return NULL;
        }
    }
    
    if (!lle_text_set_cursor(buffer, cursor_pos)) {
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    
    return buffer;
}

// ============================================================================
// Basic Undo/Redo Tests
// ============================================================================

LLE_TEST(basic_undo_insert) {
    printf("Testing basic undo of insert operation... ");
    
    // Create stack and buffer
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello", 5);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record insert action
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, " world", 5));
    
    // Apply the insertion to buffer
    LLE_ASSERT(lle_text_insert_string(buffer, " world"));
    
    // Verify buffer state after insertion
    LLE_ASSERT(verify_buffer_state(buffer, "hello world", 11));
    
    // Execute undo
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    
    // Verify undo restored original state
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    // Verify stack state
    LLE_ASSERT(!lle_undo_can_undo(stack));
    LLE_ASSERT(lle_redo_can_redo(stack));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(basic_redo_insert) {
    printf("Testing basic redo of insert operation... ");
    
    // Create stack and buffer
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello", 5);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record and apply insert
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, " world", 5));
    LLE_ASSERT(lle_text_insert_string(buffer, " world"));
    
    // Undo the operation
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    // Execute redo
    LLE_ASSERT(lle_redo_execute(stack, buffer));
    
    // Verify redo restored the insertion
    LLE_ASSERT(verify_buffer_state(buffer, "hello world", 11));
    
    // Verify stack state
    LLE_ASSERT(lle_undo_can_undo(stack));
    LLE_ASSERT(!lle_redo_can_redo(stack));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_delete_operation) {
    printf("Testing undo of delete operation... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello world", 6);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record delete action (deleting "world")
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_DELETE, 6, "world", 11));
    
    // Apply the deletion to buffer
    LLE_ASSERT(lle_text_delete_range(buffer, 6, 11));
    
    // Verify buffer state after deletion
    LLE_ASSERT(verify_buffer_state(buffer, "hello ", 6));
    
    // Execute undo
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    
    // Verify undo restored deleted text
    LLE_ASSERT(verify_buffer_state(buffer, "hello world", 11));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_cursor_movement) {
    printf("Testing undo of cursor movement... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello", 0);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record cursor move action (from 0 to 5)
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_MOVE_CURSOR, 0, NULL, 0));
    
    // Apply the cursor movement
    LLE_ASSERT(lle_text_set_cursor(buffer, 5));
    
    // Verify cursor moved
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    // Execute undo
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    
    // Verify undo restored cursor position
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 0));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_replace_operation) {
    printf("Testing undo of replace operation... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello world", 0);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record replace action (replacing "hello" with "hi")
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_REPLACE, 0, "hello", 0));
    
    // Apply the replacement (delete "hello", insert "hi")
    LLE_ASSERT(lle_text_delete_range(buffer, 0, 5));
    LLE_ASSERT(lle_text_insert_at(buffer, 0, "hi"));
    
    // Verify buffer state after replacement
    LLE_ASSERT(verify_buffer_state(buffer, "hi world", 2));
    
    // Execute undo
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    
    // Verify undo restored original text
    LLE_ASSERT(verify_buffer_state(buffer, "hello world", 0));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

// ============================================================================
// Multiple Operations Tests
// ============================================================================

LLE_TEST(multiple_undo_redo_cycle) {
    printf("Testing multiple undo/redo cycles... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("", 0);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record and apply multiple operations
    // Operation 1: Insert "hello"
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, "hello", 0));
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Operation 2: Insert " world"
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, " world", 5));
    LLE_ASSERT(lle_text_insert_string(buffer, " world"));
    
    // Verify final state
    LLE_ASSERT(verify_buffer_state(buffer, "hello world", 11));
    
    // Undo both operations
    LLE_ASSERT(lle_undo_execute(stack, buffer));  // Undo " world"
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    LLE_ASSERT(lle_undo_execute(stack, buffer));  // Undo "hello"
    LLE_ASSERT(verify_buffer_state(buffer, "", 0));
    
    // Redo both operations
    LLE_ASSERT(lle_redo_execute(stack, buffer));  // Redo "hello"
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    LLE_ASSERT(lle_redo_execute(stack, buffer));  // Redo " world"
    LLE_ASSERT(verify_buffer_state(buffer, "hello world", 11));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(mixed_operation_types) {
    printf("Testing undo/redo with mixed operation types... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello world", 11);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Operation 1: Delete " world"
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_DELETE, 5, " world", 11));
    LLE_ASSERT(lle_text_delete_range(buffer, 5, 11));
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    // Operation 2: Move cursor to beginning
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_MOVE_CURSOR, 0, NULL, 5));
    LLE_ASSERT(lle_text_set_cursor(buffer, 0));
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 0));
    
    // Operation 3: Insert "hi "
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 0, "hi ", 0));
    LLE_ASSERT(lle_text_insert_string(buffer, "hi "));
    LLE_ASSERT(verify_buffer_state(buffer, "hi hello", 3));
    
    // Undo all operations in reverse order
    LLE_ASSERT(lle_undo_execute(stack, buffer));  // Undo insert "hi "
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 0));
    
    LLE_ASSERT(lle_undo_execute(stack, buffer));  // Undo cursor move
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    LLE_ASSERT(lle_undo_execute(stack, buffer));  // Undo delete " world"
    LLE_ASSERT(verify_buffer_state(buffer, "hello world", 11));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

LLE_TEST(parameter_validation) {
    printf("Testing parameter validation... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("test", 4);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Test NULL parameters
    LLE_ASSERT(!lle_undo_execute(NULL, buffer));
    LLE_ASSERT(!lle_undo_execute(stack, NULL));
    LLE_ASSERT(!lle_redo_execute(NULL, buffer));
    LLE_ASSERT(!lle_redo_execute(stack, NULL));
    
    // Test can_undo/can_redo with NULL
    LLE_ASSERT(!lle_undo_can_undo(NULL));
    LLE_ASSERT(!lle_redo_can_redo(NULL));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(empty_stack_operations) {
    printf("Testing operations on empty stack... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("test", 4);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Try undo/redo on empty stack
    LLE_ASSERT(!lle_undo_execute(stack, buffer));
    LLE_ASSERT(!lle_redo_execute(stack, buffer));
    
    // Check availability
    LLE_ASSERT(!lle_undo_can_undo(stack));
    LLE_ASSERT(!lle_redo_can_redo(stack));
    
    // Buffer should be unchanged
    LLE_ASSERT(verify_buffer_state(buffer, "test", 4));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(undo_without_redo_available) {
    printf("Testing undo when no redo is available initially... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello", 5);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record action
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, " world", 5));
    LLE_ASSERT(lle_text_insert_string(buffer, " world"));
    
    // Initially no redo available
    LLE_ASSERT(!lle_redo_can_redo(stack));
    LLE_ASSERT(lle_undo_can_undo(stack));
    
    // Execute undo
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    
    // Now redo should be available, undo should not
    LLE_ASSERT(lle_redo_can_redo(stack));
    LLE_ASSERT(!lle_undo_can_undo(stack));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(empty_text_operations) {
    printf("Testing undo/redo with empty text operations... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("hello", 5);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record insertion of empty string
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, "", 5));
    LLE_ASSERT(lle_text_insert_string(buffer, ""));
    
    // Buffer should be unchanged
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    // Execute undo (should handle empty string gracefully)
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    
    // Buffer should still be unchanged
    LLE_ASSERT(verify_buffer_state(buffer, "hello", 5));
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

LLE_TEST(stack_state_consistency) {
    printf("Testing stack state consistency after operations... ");
    
    lle_undo_stack_t *stack = lle_undo_create(10);
    LLE_ASSERT_NOT_NULL(stack);
    
    lle_text_buffer_t *buffer = setup_buffer_with_text("test", 4);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Record multiple actions
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 4, "1", 4));
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 5, "2", 5));
    LLE_ASSERT(lle_undo_record_action(stack, LLE_UNDO_INSERT, 6, "3", 6));
    
    // Apply operations
    LLE_ASSERT(lle_text_insert_string(buffer, "1"));
    LLE_ASSERT(lle_text_insert_string(buffer, "2"));
    LLE_ASSERT(lle_text_insert_string(buffer, "3"));
    
    // Check initial state
    LLE_ASSERT_EQ(stack->count, 3);
    LLE_ASSERT_EQ(stack->current, 3);
    LLE_ASSERT_EQ(stack->undo_count, 3);
    LLE_ASSERT_EQ(stack->redo_count, 0);
    
    // Undo one operation
    LLE_ASSERT(lle_undo_execute(stack, buffer));
    LLE_ASSERT_EQ(stack->current, 2);
    LLE_ASSERT_EQ(stack->undo_count, 2);
    LLE_ASSERT_EQ(stack->redo_count, 1);
    
    // Redo one operation
    LLE_ASSERT(lle_redo_execute(stack, buffer));
    LLE_ASSERT_EQ(stack->current, 3);
    LLE_ASSERT_EQ(stack->undo_count, 3);
    LLE_ASSERT_EQ(stack->redo_count, 0);
    
    // Cleanup
    lle_text_buffer_destroy(buffer);
    lle_undo_destroy(stack);
    printf("PASSED\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("Running LLE-034 Undo/Redo Execution Tests...\n\n");
    
    // Basic undo/redo tests
    test_basic_undo_insert();
    test_basic_redo_insert();
    test_undo_delete_operation();
    test_undo_cursor_movement();
    test_undo_replace_operation();
    
    // Multiple operations tests
    test_multiple_undo_redo_cycle();
    test_mixed_operation_types();
    
    // Edge cases and error handling
    test_parameter_validation();
    test_empty_stack_operations();
    test_undo_without_redo_available();
    test_empty_text_operations();
    test_stack_state_consistency();
    
    printf("\n✅ All LLE-034 Undo/Redo Execution tests passed!\n");
    printf("📊 Tests completed: 12 comprehensive tests\n");
    printf("🔧 Features tested: Undo execution, redo execution, state management, edge cases\n");
    printf("📈 Action types covered: INSERT, DELETE, MOVE_CURSOR, REPLACE\n");
    
    return 0;
}