/**
 * @file test_buffer_behavior.c
 * @brief Behavioral tests for LLE Week 3 buffer management
 *
 * These tests validate the gap buffer implementation's correctness
 * following the same validation pattern as Week 1 and Week 2.
 *
 * Test Philosophy:
 * - Test behaviors, not implementation details
 * - Validate public API contracts
 * - Cover edge cases and boundary conditions
 * - Ensure gap buffer algorithm correctness
 */

#include "lle/buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Test helper macros
#define TEST(name) \
    static bool test_##name(void); \
    static void run_test_##name(void) { \
        tests_run++; \
        printf("  Testing: %s ... ", #name); \
        fflush(stdout); \
        if (test_##name()) { \
            printf("PASS\n"); \
            tests_passed++; \
        } else { \
            printf("FAIL\n"); \
            tests_failed++; \
        } \
    } \
    static bool test_##name(void)

#define RUN_TEST(name) run_test_##name()

// Helper function to get buffer content as string
static bool get_buffer_as_string(lle_buffer_t *buffer, char *output, size_t output_size) {
    lle_buffer_result_t result = lle_buffer_get_content(buffer, output, output_size);
    return result == LLE_BUFFER_SUCCESS;
}

// Helper function to verify buffer content
static bool verify_content(lle_buffer_t *buffer, const char *expected) {
    char actual[1024];
    if (!get_buffer_as_string(buffer, actual, sizeof(actual))) {
        return false;
    }
    return strcmp(actual, expected) == 0;
}

// Helper function to verify cursor position
static bool verify_cursor(lle_buffer_t *buffer, size_t expected_pos) {
    size_t actual_pos;
    lle_buffer_result_t result = lle_buffer_get_cursor(buffer, &actual_pos);
    return result == LLE_BUFFER_SUCCESS && actual_pos == expected_pos;
}

// Helper function to verify buffer length
static bool verify_length(lle_buffer_t *buffer, size_t expected_len) {
    size_t actual_len;
    lle_buffer_result_t result = lle_buffer_get_length(buffer, &actual_len);
    return result == LLE_BUFFER_SUCCESS && actual_len == expected_len;
}

//=============================================================================
// Test Suite
//=============================================================================

TEST(buffer_init_and_destroy) {
    lle_buffer_t *buffer = NULL;
    
    // Initialize buffer
    lle_buffer_result_t result = lle_buffer_init(&buffer, 64);
    if (result != LLE_BUFFER_SUCCESS) return false;
    if (buffer == NULL) return false;
    
    // Verify initial state: empty buffer
    if (!verify_content(buffer, "")) return false;
    if (!verify_cursor(buffer, 0)) return false;
    if (!verify_length(buffer, 0)) return false;
    
    // Cleanup
    lle_buffer_destroy(buffer);
    return true;
}

TEST(buffer_insert_text) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text at start
    lle_buffer_result_t result = lle_buffer_insert(buffer, "Hello");
    if (result != LLE_BUFFER_SUCCESS) return false;
    
    // Verify content and cursor position
    if (!verify_content(buffer, "Hello")) return false;
    if (!verify_cursor(buffer, 5)) return false;  // Cursor after "Hello"
    if (!verify_length(buffer, 5)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(buffer_insert_char) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert individual characters
    lle_buffer_insert_char(buffer, 'H');
    lle_buffer_insert_char(buffer, 'i');
    lle_buffer_insert_char(buffer, '!');
    
    // Verify
    if (!verify_content(buffer, "Hi!")) return false;
    if (!verify_cursor(buffer, 3)) return false;
    if (!verify_length(buffer, 3)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(buffer_insert_at_middle) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert initial text
    lle_buffer_insert(buffer, "Hello");
    
    // Move cursor to middle (after 'l', before 'l')
    lle_buffer_set_cursor(buffer, 3);
    
    // Insert text in middle
    lle_buffer_insert(buffer, "X");
    
    // Should be "HelXlo"
    if (!verify_content(buffer, "HelXlo")) return false;
    if (!verify_cursor(buffer, 4)) return false;
    if (!verify_length(buffer, 6)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(buffer_delete_before_cursor) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text
    lle_buffer_insert(buffer, "Hello");
    
    // Delete one character (backspace) - should delete 'o'
    lle_buffer_result_t result = lle_buffer_delete_before_cursor(buffer);
    if (result != LLE_BUFFER_SUCCESS) return false;
    
    // Verify
    if (!verify_content(buffer, "Hell")) return false;
    if (!verify_cursor(buffer, 4)) return false;
    if (!verify_length(buffer, 4)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(buffer_delete_at_cursor) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text
    lle_buffer_insert(buffer, "Hello");
    
    // Move cursor to start
    lle_buffer_set_cursor(buffer, 0);
    
    // Delete at cursor (should delete 'H')
    lle_buffer_result_t result = lle_buffer_delete_at_cursor(buffer);
    if (result != LLE_BUFFER_SUCCESS) return false;
    
    // Verify
    if (!verify_content(buffer, "ello")) return false;
    if (!verify_cursor(buffer, 0)) return false;
    if (!verify_length(buffer, 4)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(cursor_movement_left_right) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text
    lle_buffer_insert(buffer, "ABC");
    
    // Cursor should be at 3
    if (!verify_cursor(buffer, 3)) return false;
    
    // Move left twice
    lle_buffer_move_cursor_left(buffer);
    if (!verify_cursor(buffer, 2)) return false;
    
    lle_buffer_move_cursor_left(buffer);
    if (!verify_cursor(buffer, 1)) return false;
    
    // Move right once
    lle_buffer_move_cursor_right(buffer);
    if (!verify_cursor(buffer, 2)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(cursor_movement_home_end) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text
    lle_buffer_insert(buffer, "Hello World");
    
    // Cursor at end (11)
    if (!verify_cursor(buffer, 11)) return false;
    
    // Move to home
    lle_buffer_move_cursor_home(buffer);
    if (!verify_cursor(buffer, 0)) return false;
    
    // Move to end
    lle_buffer_move_cursor_end(buffer);
    if (!verify_cursor(buffer, 11)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(cursor_set_position) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text
    lle_buffer_insert(buffer, "0123456789");
    
    // Set cursor to various positions
    lle_buffer_set_cursor(buffer, 0);
    if (!verify_cursor(buffer, 0)) return false;
    
    lle_buffer_set_cursor(buffer, 5);
    if (!verify_cursor(buffer, 5)) return false;
    
    lle_buffer_set_cursor(buffer, 10);
    if (!verify_cursor(buffer, 10)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(buffer_clear) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text
    lle_buffer_insert(buffer, "Hello World");
    
    // Clear buffer
    lle_buffer_result_t result = lle_buffer_clear(buffer);
    if (result != LLE_BUFFER_SUCCESS) return false;
    
    // Verify empty state
    if (!verify_content(buffer, "")) return false;
    if (!verify_cursor(buffer, 0)) return false;
    if (!verify_length(buffer, 0)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(empty_buffer_operations) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Try delete on empty buffer - should succeed gracefully (no-op)
    lle_buffer_result_t result1 = lle_buffer_delete_before_cursor(buffer);
    if (result1 != LLE_BUFFER_SUCCESS) return false;
    
    lle_buffer_result_t result2 = lle_buffer_delete_at_cursor(buffer);
    if (result2 != LLE_BUFFER_SUCCESS) return false;
    
    // Try cursor movement left on empty buffer - should succeed gracefully (no-op)
    lle_buffer_result_t result3 = lle_buffer_move_cursor_left(buffer);
    if (result3 != LLE_BUFFER_SUCCESS) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(boundary_cursor_movement) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Insert text
    lle_buffer_insert(buffer, "ABC");
    
    // Move cursor to start
    lle_buffer_move_cursor_home(buffer);
    
    // Try to move left past start - should succeed gracefully (no-op)
    lle_buffer_result_t result1 = lle_buffer_move_cursor_left(buffer);
    if (result1 != LLE_BUFFER_SUCCESS) return false;
    if (!verify_cursor(buffer, 0)) return false;  // Cursor unchanged
    
    // Move to end
    lle_buffer_move_cursor_end(buffer);
    
    // Try to move right past end - should succeed gracefully (no-op)
    lle_buffer_result_t result2 = lle_buffer_move_cursor_right(buffer);
    if (result2 != LLE_BUFFER_SUCCESS) return false;
    if (!verify_cursor(buffer, 3)) return false;  // Cursor unchanged
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(gap_buffer_expansion) {
    lle_buffer_t *buffer = NULL;
    
    // Start with small buffer
    lle_buffer_init(&buffer, 8);
    
    // Insert text that exceeds initial capacity
    lle_buffer_insert(buffer, "This is a long text that will cause the buffer to expand");
    
    // Verify content is intact
    if (!verify_content(buffer, "This is a long text that will cause the buffer to expand")) return false;
    if (!verify_length(buffer, 56)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(complex_editing_sequence) {
    lle_buffer_t *buffer = NULL;
    lle_buffer_init(&buffer, 64);
    
    // Build "Hello World" through complex operations
    lle_buffer_insert(buffer, "Hello");           // "Hello|"
    lle_buffer_insert_char(buffer, ' ');          // "Hello |"
    lle_buffer_insert(buffer, "World");           // "Hello World|"
    
    // Move to middle and insert
    lle_buffer_set_cursor(buffer, 6);             // "Hello |World"
    lle_buffer_insert(buffer, "Beautiful ");      // "Hello Beautiful |World"
    
    // Move to end and delete some chars
    lle_buffer_move_cursor_end(buffer);           // "Hello Beautiful World|"
    lle_buffer_delete_before_cursor(buffer);      // "Hello Beautiful Worl|"
    lle_buffer_delete_before_cursor(buffer);      // "Hello Beautiful Wor|"
    lle_buffer_insert(buffer, "ld!");             // "Hello Beautiful World!|"
    
    // Verify final state
    if (!verify_content(buffer, "Hello Beautiful World!")) return false;
    if (!verify_cursor(buffer, 22)) return false;
    
    lle_buffer_destroy(buffer);
    return true;
}

TEST(null_pointer_handling) {
    // Test null buffer pointer handling
    lle_buffer_result_t result;
    
    result = lle_buffer_insert(NULL, "test");
    if (result != LLE_BUFFER_ERROR_NOT_INITIALIZED) return false;
    
    result = lle_buffer_insert_char(NULL, 'x');
    if (result != LLE_BUFFER_ERROR_NOT_INITIALIZED) return false;
    
    result = lle_buffer_delete_before_cursor(NULL);
    if (result != LLE_BUFFER_ERROR_NOT_INITIALIZED) return false;
    
    result = lle_buffer_move_cursor_left(NULL);
    if (result != LLE_BUFFER_ERROR_NOT_INITIALIZED) return false;
    
    size_t pos;
    result = lle_buffer_get_cursor(NULL, &pos);
    if (result != LLE_BUFFER_ERROR_NOT_INITIALIZED) return false;
    
    return true;
}

//=============================================================================
// Test Runner
//=============================================================================

int main(void) {
    printf("LLE Week 3 Buffer Behavioral Tests\n");
    printf("===================================\n\n");
    
    // Run all tests
    RUN_TEST(buffer_init_and_destroy);
    RUN_TEST(buffer_insert_text);
    RUN_TEST(buffer_insert_char);
    RUN_TEST(buffer_insert_at_middle);
    RUN_TEST(buffer_delete_before_cursor);
    RUN_TEST(buffer_delete_at_cursor);
    RUN_TEST(cursor_movement_left_right);
    RUN_TEST(cursor_movement_home_end);
    RUN_TEST(cursor_set_position);
    RUN_TEST(buffer_clear);
    RUN_TEST(empty_buffer_operations);
    RUN_TEST(boundary_cursor_movement);
    RUN_TEST(gap_buffer_expansion);
    RUN_TEST(complex_editing_sequence);
    RUN_TEST(null_pointer_handling);
    
    // Print summary
    printf("\n");
    printf("===================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("===================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
