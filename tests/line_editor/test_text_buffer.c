/**
 * @file test_text_buffer.c
 * @brief Test suite for LLE text buffer functionality
 *
 * Comprehensive tests for text buffer creation, initialization, and destruction.
 * Tests cover normal operations, edge cases, and error conditions.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../src/line_editor/text_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test framework macros
#define LLE_TEST(name) \
    static void test_##name(void); \
    static void test_##name(void)

#define LLE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", \
                    #condition, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "ASSERTION FAILED: %s (%zu) != %s (%zu) at %s:%d\n", \
                    #actual, (size_t)(actual), #expected, (size_t)(expected), \
                    __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "ASSERTION FAILED: %s is NULL at %s:%d\n", \
                    #ptr, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            fprintf(stderr, "ASSERTION FAILED: %s is not NULL at %s:%d\n", \
                    #ptr, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test) \
    do { \
        printf("Running test_%s...", #test); \
        fflush(stdout); \
        test_##test(); \
        tests_run++; \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

// Test: Create buffer with default capacity
LLE_TEST(buffer_create_default) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    
    LLE_ASSERT_NOT_NULL(buffer);
    LLE_ASSERT_NOT_NULL(buffer->buffer);
    LLE_ASSERT_EQ(buffer->capacity, LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_EQ(buffer->length, 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    LLE_ASSERT_EQ(buffer->char_count, 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Create buffer with custom capacity
LLE_TEST(buffer_create_custom_capacity) {
    size_t custom_capacity = 512;
    lle_text_buffer_t *buffer = lle_text_buffer_create(custom_capacity);
    
    LLE_ASSERT_NOT_NULL(buffer);
    LLE_ASSERT_NOT_NULL(buffer->buffer);
    LLE_ASSERT_EQ(buffer->capacity, custom_capacity);
    LLE_ASSERT_EQ(buffer->length, 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    LLE_ASSERT_EQ(buffer->char_count, 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Create buffer with minimum capacity enforcement
LLE_TEST(buffer_create_minimum_capacity) {
    size_t too_small = 32; // Less than LLE_MIN_BUFFER_CAPACITY
    lle_text_buffer_t *buffer = lle_text_buffer_create(too_small);
    
    LLE_ASSERT_NOT_NULL(buffer);
    LLE_ASSERT_NOT_NULL(buffer->buffer);
    LLE_ASSERT_EQ(buffer->capacity, LLE_MIN_BUFFER_CAPACITY);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Create buffer with maximum capacity enforcement
LLE_TEST(buffer_create_maximum_capacity) {
    size_t too_large = LLE_MAX_BUFFER_CAPACITY + 1000;
    lle_text_buffer_t *buffer = lle_text_buffer_create(too_large);
    
    LLE_ASSERT_NOT_NULL(buffer);
    LLE_ASSERT_NOT_NULL(buffer->buffer);
    LLE_ASSERT_EQ(buffer->capacity, LLE_MAX_BUFFER_CAPACITY);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Initialize existing buffer structure
LLE_TEST(buffer_init_existing) {
    lle_text_buffer_t buffer; // Stack allocated
    bool result = lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY);
    
    LLE_ASSERT(result);
    LLE_ASSERT_NOT_NULL(buffer.buffer);
    LLE_ASSERT_EQ(buffer.capacity, LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_EQ(buffer.length, 0);
    LLE_ASSERT_EQ(buffer.cursor_pos, 0);
    LLE_ASSERT_EQ(buffer.char_count, 0);
    LLE_ASSERT(lle_text_buffer_is_valid(&buffer));
    
    // Clean up manually since this is stack allocated
    free(buffer.buffer);
}

// Test: Initialize with NULL pointer
LLE_TEST(buffer_init_null_pointer) {
    bool result = lle_text_buffer_init(NULL, LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT(!result);
}

// Test: Clear buffer contents
LLE_TEST(buffer_clear) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Manually set some values to simulate usage
    buffer->length = 10;
    buffer->cursor_pos = 5;
    buffer->char_count = 8;
    strcpy(buffer->buffer, "test data");
    
    // Clear the buffer
    lle_text_buffer_clear(buffer);
    
    // Verify it's cleared but capacity remains
    LLE_ASSERT_EQ(buffer->length, 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    LLE_ASSERT_EQ(buffer->char_count, 0);
    LLE_ASSERT_EQ(buffer->capacity, LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_EQ(buffer->buffer[0], '\0'); // Should be zeroed
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Clear with NULL pointer
LLE_TEST(buffer_clear_null_pointer) {
    // Should not crash
    lle_text_buffer_clear(NULL);
}

// Test: Destroy with NULL pointer
LLE_TEST(buffer_destroy_null_pointer) {
    // Should not crash
    lle_text_buffer_destroy(NULL);
}

// Test: Buffer validation with invalid states
LLE_TEST(buffer_validation) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    // Test invalid states
    lle_text_buffer_t invalid_buffer;
    
    // NULL buffer
    LLE_ASSERT(!lle_text_buffer_is_valid(NULL));
    
    // Buffer with NULL internal buffer
    invalid_buffer = *buffer;
    invalid_buffer.buffer = NULL;
    LLE_ASSERT(!lle_text_buffer_is_valid(&invalid_buffer));
    
    // Buffer with length > capacity
    invalid_buffer = *buffer;
    invalid_buffer.length = buffer->capacity + 1;
    LLE_ASSERT(!lle_text_buffer_is_valid(&invalid_buffer));
    
    // Buffer with cursor_pos > length
    invalid_buffer = *buffer;
    invalid_buffer.length = 10;
    invalid_buffer.cursor_pos = 15;
    LLE_ASSERT(!lle_text_buffer_is_valid(&invalid_buffer));
    
    // Buffer with capacity too small
    invalid_buffer = *buffer;
    invalid_buffer.capacity = LLE_MIN_BUFFER_CAPACITY - 1;
    LLE_ASSERT(!lle_text_buffer_is_valid(&invalid_buffer));
    
    // Buffer with capacity too large
    invalid_buffer = *buffer;
    invalid_buffer.capacity = LLE_MAX_BUFFER_CAPACITY + 1;
    LLE_ASSERT(!lle_text_buffer_is_valid(&invalid_buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Zero initialization of buffer contents
LLE_TEST(buffer_zero_initialization) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Check that buffer is zero-initialized
    for (size_t i = 0; i < buffer->capacity; i++) {
        LLE_ASSERT_EQ(buffer->buffer[i], 0);
    }
    
    lle_text_buffer_destroy(buffer);
}

// Test: Multiple create/destroy cycles
LLE_TEST(buffer_multiple_cycles) {
    for (int i = 0; i < 100; i++) {
        lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
        LLE_ASSERT_NOT_NULL(buffer);
        LLE_ASSERT(lle_text_buffer_is_valid(buffer));
        lle_text_buffer_destroy(buffer);
    }
}

// =====================================
// LLE-003: Text Insertion Tests
// =====================================

// Test: Insert character basic functionality
LLE_TEST(insert_char_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert a character
    LLE_ASSERT(lle_text_insert_char(buffer, 'a'));
    
    // Verify state
    LLE_ASSERT_EQ(buffer->length, 1);
    LLE_ASSERT_EQ(buffer->cursor_pos, 1);
    LLE_ASSERT_EQ(buffer->char_count, 1);
    LLE_ASSERT_EQ(buffer->buffer[0], 'a');
    LLE_ASSERT_EQ(buffer->buffer[1], '\0');
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert character with NULL pointer
LLE_TEST(insert_char_null_pointer) {
    LLE_ASSERT(!lle_text_insert_char(NULL, 'a'));
}

// Test: Insert multiple characters
LLE_TEST(insert_char_multiple) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert multiple characters
    const char *text = "hello";
    for (size_t i = 0; i < strlen(text); i++) {
        LLE_ASSERT(lle_text_insert_char(buffer, text[i]));
    }
    
    // Verify final state
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    LLE_ASSERT_EQ(buffer->char_count, 5);
    LLE_ASSERT(strncmp(buffer->buffer, "hello", 5) == 0);
    LLE_ASSERT_EQ(buffer->buffer[5], '\0');
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert character with buffer resize
LLE_TEST(insert_char_buffer_resize) {
    // Create small buffer to force resize
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_MIN_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    size_t original_capacity = buffer->capacity;
    
    // Fill buffer to capacity - 2 (leaving space for 1 char + null terminator)
    for (size_t i = 0; i < original_capacity - 2; i++) {
        LLE_ASSERT(lle_text_insert_char(buffer, 'x'));
    }
    
    // Insert one more - this should still fit (uses last available space)
    LLE_ASSERT(lle_text_insert_char(buffer, 'y'));
    LLE_ASSERT_EQ(buffer->capacity, original_capacity); // No resize yet
    
    // Insert one more to trigger resize (no space left for char + null terminator)
    LLE_ASSERT(lle_text_insert_char(buffer, 'z'));
    
    // Verify resize occurred
    LLE_ASSERT(buffer->capacity > original_capacity);
    LLE_ASSERT_EQ(buffer->length, original_capacity);
    LLE_ASSERT_EQ(buffer->buffer[buffer->length - 1], 'z');
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert string basic functionality
LLE_TEST(insert_string_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert a string
    const char *test_str = "hello world";
    LLE_ASSERT(lle_text_insert_string(buffer, test_str));
    
    // Verify state
    size_t expected_len = strlen(test_str);
    LLE_ASSERT_EQ(buffer->length, expected_len);
    LLE_ASSERT_EQ(buffer->cursor_pos, expected_len);
    LLE_ASSERT_EQ(buffer->char_count, expected_len);
    LLE_ASSERT(strncmp(buffer->buffer, test_str, expected_len) == 0);
    LLE_ASSERT_EQ(buffer->buffer[expected_len], '\0');
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert string with NULL pointers
LLE_TEST(insert_string_null_pointer) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Test NULL buffer
    LLE_ASSERT(!lle_text_insert_string(NULL, "test"));
    
    // Test NULL string
    LLE_ASSERT(!lle_text_insert_string(buffer, NULL));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert empty string
LLE_TEST(insert_string_empty) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert empty string (should succeed but do nothing)
    LLE_ASSERT(lle_text_insert_string(buffer, ""));
    
    // Verify no change
    LLE_ASSERT_EQ(buffer->length, 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    LLE_ASSERT_EQ(buffer->char_count, 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert string with buffer resize
LLE_TEST(insert_string_buffer_resize) {
    // Create small buffer
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_MIN_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    size_t original_capacity = buffer->capacity;
    
    // Insert large string to force resize
    char large_string[200];
    memset(large_string, 'x', sizeof(large_string) - 1);
    large_string[sizeof(large_string) - 1] = '\0';
    
    LLE_ASSERT(lle_text_insert_string(buffer, large_string));
    
    // Verify resize occurred
    LLE_ASSERT(buffer->capacity > original_capacity);
    LLE_ASSERT_EQ(buffer->length, strlen(large_string));
    LLE_ASSERT(strncmp(buffer->buffer, large_string, strlen(large_string)) == 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert at arbitrary position basic functionality
LLE_TEST(insert_at_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // First, insert some initial text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 11);
    
    // Insert at position 6 (after "hello ")
    LLE_ASSERT(lle_text_insert_at(buffer, 6, "beautiful "));
    
    // Verify result: "hello beautiful world"
    const char *expected = "hello beautiful world";
    LLE_ASSERT_EQ(buffer->length, strlen(expected));
    LLE_ASSERT(strncmp(buffer->buffer, expected, strlen(expected)) == 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 21); // Cursor should have moved
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert at beginning
LLE_TEST(insert_at_beginning) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert initial text
    LLE_ASSERT(lle_text_insert_string(buffer, "world"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    // Insert at beginning
    LLE_ASSERT(lle_text_insert_at(buffer, 0, "hello "));
    
    // Verify result: "hello world"
    const char *expected = "hello world";
    LLE_ASSERT_EQ(buffer->length, strlen(expected));
    LLE_ASSERT(strncmp(buffer->buffer, expected, strlen(expected)) == 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 11); // Cursor should have moved
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert at middle position
LLE_TEST(insert_at_middle) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert initial text
    LLE_ASSERT(lle_text_insert_string(buffer, "abcdef"));
    
    // Set cursor to middle
    buffer->cursor_pos = 3;
    
    // Insert at position 3
    LLE_ASSERT(lle_text_insert_at(buffer, 3, "XYZ"));
    
    // Verify result: "abcXYZdef"
    const char *expected = "abcXYZdef";
    LLE_ASSERT_EQ(buffer->length, strlen(expected));
    LLE_ASSERT(strncmp(buffer->buffer, expected, strlen(expected)) == 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 6); // Cursor should have moved
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert at end position
LLE_TEST(insert_at_end) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert initial text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Set cursor to beginning
    buffer->cursor_pos = 0;
    
    // Insert at end (position 5)
    LLE_ASSERT(lle_text_insert_at(buffer, 5, " world"));
    
    // Verify result: "hello world"
    const char *expected = "hello world";
    LLE_ASSERT_EQ(buffer->length, strlen(expected));
    LLE_ASSERT(strncmp(buffer->buffer, expected, strlen(expected)) == 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0); // Cursor should not have moved
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Insert at invalid position
LLE_TEST(insert_at_invalid_position) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert some text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Try to insert beyond buffer length
    LLE_ASSERT(!lle_text_insert_at(buffer, 10, "test"));
    
    // Buffer should be unchanged
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT(strncmp(buffer->buffer, "hello", 5) == 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Cursor position updates with various insertions
LLE_TEST(insert_cursor_position_updates) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert char - cursor should advance
    LLE_ASSERT(lle_text_insert_char(buffer, 'a'));
    LLE_ASSERT_EQ(buffer->cursor_pos, 1);
    
    // Insert string - cursor should advance by string length
    LLE_ASSERT(lle_text_insert_string(buffer, "bcde"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    // Reset cursor to beginning
    buffer->cursor_pos = 0;
    
    // Insert at cursor position - cursor should advance
    LLE_ASSERT(lle_text_insert_char(buffer, 'X'));
    LLE_ASSERT_EQ(buffer->cursor_pos, 1);
    
    // Insert before cursor position - cursor should advance
    LLE_ASSERT(lle_text_insert_at(buffer, 0, "Y"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 2);
    
    // Insert after cursor position - cursor should not change
    LLE_ASSERT(lle_text_insert_at(buffer, 7, "Z"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 2);
    
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// =====================================
// LLE-004: Text Deletion Tests
// =====================================

// Test: Delete character basic functionality
LLE_TEST(delete_char_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert some text: "hello"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Set cursor to position 1 (between 'h' and 'e')
    buffer->cursor_pos = 1;
    
    // Delete character at cursor ('e')
    LLE_ASSERT(lle_text_delete_char(buffer));
    
    // Verify result: "hllo"
    LLE_ASSERT_EQ(buffer->length, 4);
    LLE_ASSERT_EQ(buffer->cursor_pos, 1); // Cursor should remain at position 1
    LLE_ASSERT_EQ(buffer->char_count, 4);
    LLE_ASSERT(strncmp(buffer->buffer, "hllo", 4) == 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete character with NULL pointer
LLE_TEST(delete_char_null_pointer) {
    LLE_ASSERT(!lle_text_delete_char(NULL));
}

// Test: Delete character at end of buffer
LLE_TEST(delete_char_at_end) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Cursor is at end (position 5)
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    // Try to delete - should fail (nothing to delete)
    LLE_ASSERT(!lle_text_delete_char(buffer));
    
    // Buffer should be unchanged
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT(strncmp(buffer->buffer, "hello", 5) == 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete character in empty buffer
LLE_TEST(delete_char_empty_buffer) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Try to delete from empty buffer
    LLE_ASSERT(!lle_text_delete_char(buffer));
    
    // Buffer should remain empty
    LLE_ASSERT_EQ(buffer->length, 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Backspace basic functionality
LLE_TEST(backspace_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert some text: "hello"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Set cursor to position 2 (after 'he')
    buffer->cursor_pos = 2;
    
    // Backspace (delete 'e')
    LLE_ASSERT(lle_text_backspace(buffer));
    
    // Verify result: "hllo" with cursor at position 1
    LLE_ASSERT_EQ(buffer->length, 4);
    LLE_ASSERT_EQ(buffer->cursor_pos, 1);
    LLE_ASSERT_EQ(buffer->char_count, 4);
    LLE_ASSERT(strncmp(buffer->buffer, "hllo", 4) == 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Backspace with NULL pointer
LLE_TEST(backspace_null_pointer) {
    LLE_ASSERT(!lle_text_backspace(NULL));
}

// Test: Backspace at beginning of buffer
LLE_TEST(backspace_at_beginning) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Set cursor to beginning
    buffer->cursor_pos = 0;
    
    // Try to backspace - should fail (nothing before cursor)
    LLE_ASSERT(!lle_text_backspace(buffer));
    
    // Buffer should be unchanged
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    LLE_ASSERT(strncmp(buffer->buffer, "hello", 5) == 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Backspace in empty buffer
LLE_TEST(backspace_empty_buffer) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Try to backspace from empty buffer
    LLE_ASSERT(!lle_text_backspace(buffer));
    
    // Buffer should remain empty
    LLE_ASSERT_EQ(buffer->length, 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete range basic functionality
LLE_TEST(delete_range_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text: "hello world"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Delete range [6, 11) - delete "world"
    LLE_ASSERT(lle_text_delete_range(buffer, 6, 11));
    
    // Verify result: "hello "
    LLE_ASSERT_EQ(buffer->length, 6);
    LLE_ASSERT(strncmp(buffer->buffer, "hello ", 6) == 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete range with NULL pointer
LLE_TEST(delete_range_null_pointer) {
    LLE_ASSERT(!lle_text_delete_range(NULL, 0, 5));
}

// Test: Delete range with invalid range
LLE_TEST(delete_range_invalid_range) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Test start > end
    LLE_ASSERT(!lle_text_delete_range(buffer, 3, 1));
    
    // Test start > length
    LLE_ASSERT(!lle_text_delete_range(buffer, 10, 15));
    
    // Test end > length
    LLE_ASSERT(!lle_text_delete_range(buffer, 2, 10));
    
    // Buffer should be unchanged
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT(strncmp(buffer->buffer, "hello", 5) == 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete range empty range
LLE_TEST(delete_range_empty_range) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Delete empty range [2, 2)
    LLE_ASSERT(lle_text_delete_range(buffer, 2, 2));
    
    // Buffer should be unchanged
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT(strncmp(buffer->buffer, "hello", 5) == 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete range cursor position updates
LLE_TEST(delete_range_cursor_updates) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text: "hello world"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Test cursor before deleted range
    buffer->cursor_pos = 2;
    LLE_ASSERT(lle_text_delete_range(buffer, 6, 11)); // Delete "world"
    LLE_ASSERT_EQ(buffer->cursor_pos, 2); // Should remain unchanged
    
    // Reset buffer
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Test cursor within deleted range
    buffer->cursor_pos = 8;
    LLE_ASSERT(lle_text_delete_range(buffer, 6, 11)); // Delete "world"
    LLE_ASSERT_EQ(buffer->cursor_pos, 6); // Should move to start of range
    
    // Reset buffer
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Test cursor after deleted range
    buffer->cursor_pos = 11;
    LLE_ASSERT(lle_text_delete_range(buffer, 3, 6)); // Delete "lo "
    LLE_ASSERT_EQ(buffer->cursor_pos, 8); // Should move back by deleted length
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete range at beginning
LLE_TEST(delete_range_at_beginning) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text: "hello world"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Delete range [0, 6) - delete "hello "
    LLE_ASSERT(lle_text_delete_range(buffer, 0, 6));
    
    // Verify result: "world"
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT(strncmp(buffer->buffer, "world", 5) == 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete range at end
LLE_TEST(delete_range_at_end) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text: "hello world"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Delete range [5, 11) - delete " world"
    LLE_ASSERT(lle_text_delete_range(buffer, 5, 11));
    
    // Verify result: "hello"
    LLE_ASSERT_EQ(buffer->length, 5);
    LLE_ASSERT(strncmp(buffer->buffer, "hello", 5) == 0);
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// Test: Delete entire buffer with range
LLE_TEST(delete_range_entire_buffer) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Delete entire range [0, 5)
    LLE_ASSERT(lle_text_delete_range(buffer, 0, 5));
    
    // Verify result: empty buffer
    LLE_ASSERT_EQ(buffer->length, 0);
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    LLE_ASSERT_EQ(buffer->char_count, 0);
    LLE_ASSERT_EQ(buffer->buffer[0], '\0');
    LLE_ASSERT(lle_text_buffer_is_valid(buffer));
    
    lle_text_buffer_destroy(buffer);
}

// =====================================
// LLE-005: Cursor Movement Tests
// =====================================

// Test: Move cursor left basic functionality
LLE_TEST(move_cursor_left_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text: "hello"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    // Move left
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 4);
    
    // Move left again
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 3);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Move cursor left at beginning
LLE_TEST(move_cursor_left_at_beginning) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text and move cursor to beginning
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    buffer->cursor_pos = 0;
    
    // Try to move left - should succeed (idempotent behavior: already at beginning)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Move cursor right basic functionality
LLE_TEST(move_cursor_right_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text and move cursor to beginning
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    buffer->cursor_pos = 0;
    
    // Move right
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 1);
    
    // Move right again
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 2);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Move cursor right at end
LLE_TEST(move_cursor_right_at_end) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text - cursor should be at end
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    // Try to move right - should succeed (idempotent behavior: already at end)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Move cursor home
LLE_TEST(move_cursor_home) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text - cursor at end
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 11);
    
    // Move to home
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_HOME));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    // Try to move home again - should succeed (idempotent behavior: already at home)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_HOME));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Move cursor end
LLE_TEST(move_cursor_end) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text and move cursor to beginning
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    buffer->cursor_pos = 0;
    
    // Move to end
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_END));
    LLE_ASSERT_EQ(buffer->cursor_pos, 11);
    
    // Try to move end again - should succeed (idempotent behavior: already at end)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_END));
    LLE_ASSERT_EQ(buffer->cursor_pos, 11);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Move cursor word left
LLE_TEST(move_cursor_word_left) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text: "hello beautiful world"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello beautiful world"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 21);
    
    // Move word left - should go to start of "world" (position 16)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 16);
    
    // Move word left again - should go to start of "beautiful" (position 6)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 6);
    
    // Move word left again - should go to start of "hello" (position 0)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    // Try to move word left again - should succeed (idempotent behavior: already at beginning)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Move cursor word right
LLE_TEST(move_cursor_word_right) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text and move cursor to beginning
    LLE_ASSERT(lle_text_insert_string(buffer, "hello beautiful world"));
    buffer->cursor_pos = 0;
    
    // Move word right - should go to start of "beautiful" (position 6)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 6);
    
    // Move word right again - should go to start of "world" (position 16)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 16);
    
    // Move word right again - should go to end (position 21)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 21);
    
    // Try to move word right again - should succeed (idempotent behavior: already at end)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 21);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Word movement with punctuation
LLE_TEST(move_cursor_word_with_punctuation) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text with punctuation: "hello, world!"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello, world!"));
    LLE_ASSERT_EQ(buffer->cursor_pos, 13);
    
    // Move word left - should go to start of "world" (position 7)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 7);
    
    // Move word left again - should go to start of "hello" (position 0)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Set cursor position basic functionality
LLE_TEST(set_cursor_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Set cursor to various positions
    LLE_ASSERT(lle_text_set_cursor(buffer, 0));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    LLE_ASSERT(lle_text_set_cursor(buffer, 5));
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    LLE_ASSERT(lle_text_set_cursor(buffer, 11));
    LLE_ASSERT_EQ(buffer->cursor_pos, 11);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Set cursor position with invalid position
LLE_TEST(set_cursor_invalid_position) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Insert text (length 5)
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    
    // Try to set cursor beyond buffer length
    LLE_ASSERT(!lle_text_set_cursor(buffer, 10));
    
    // Cursor should remain unchanged
    LLE_ASSERT_EQ(buffer->cursor_pos, 5);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Set cursor with NULL pointer
LLE_TEST(set_cursor_null_pointer) {
    LLE_ASSERT(!lle_text_set_cursor(NULL, 0));
}

// Test: Move cursor with NULL pointer
LLE_TEST(move_cursor_null_pointer) {
    LLE_ASSERT(!lle_text_move_cursor(NULL, LLE_MOVE_LEFT));
}

// Test: Move cursor in empty buffer
LLE_TEST(move_cursor_empty_buffer) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Try various movements in empty buffer
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));   // Idempotent: already at position 0
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));  // Idempotent: already at position 0
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_HOME));   // Idempotent: already at home (0)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_END));    // Idempotent: already at end (0)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));  // Idempotent: already at beginning
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT)); // Idempotent: already at end
    
    // Cursor should remain at 0
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Word movement edge cases
LLE_TEST(move_cursor_word_edge_cases) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(LLE_DEFAULT_BUFFER_CAPACITY);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Test with only spaces
    LLE_ASSERT(lle_text_insert_string(buffer, "   "));
    LLE_ASSERT_EQ(buffer->cursor_pos, 3);
    
    // Word left should go to beginning
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    // Word right should go to end
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 3);
    
    lle_text_buffer_destroy(buffer);
}

// Main test runner
int main(void) {
    printf("Running LLE Text Buffer Tests\n");
    printf("============================\n\n");
    
    RUN_TEST(buffer_create_default);
    RUN_TEST(buffer_create_custom_capacity);
    RUN_TEST(buffer_create_minimum_capacity);
    RUN_TEST(buffer_create_maximum_capacity);
    RUN_TEST(buffer_init_existing);
    RUN_TEST(buffer_init_null_pointer);
    RUN_TEST(buffer_clear);
    RUN_TEST(buffer_clear_null_pointer);
    RUN_TEST(buffer_destroy_null_pointer);
    RUN_TEST(buffer_validation);
    RUN_TEST(buffer_zero_initialization);
    RUN_TEST(buffer_multiple_cycles);
    
    // LLE-003: Text insertion tests
    RUN_TEST(insert_char_basic);
    RUN_TEST(insert_char_null_pointer);
    RUN_TEST(insert_char_multiple);
    RUN_TEST(insert_char_buffer_resize);
    RUN_TEST(insert_string_basic);
    RUN_TEST(insert_string_null_pointer);
    RUN_TEST(insert_string_empty);
    RUN_TEST(insert_string_buffer_resize);
    RUN_TEST(insert_at_basic);
    RUN_TEST(insert_at_beginning);
    RUN_TEST(insert_at_middle);
    RUN_TEST(insert_at_end);
    RUN_TEST(insert_at_invalid_position);
    RUN_TEST(insert_cursor_position_updates);
    
    // LLE-004: Text deletion tests
    RUN_TEST(delete_char_basic);
    RUN_TEST(delete_char_null_pointer);
    RUN_TEST(delete_char_at_end);
    RUN_TEST(delete_char_empty_buffer);
    RUN_TEST(backspace_basic);
    RUN_TEST(backspace_null_pointer);
    RUN_TEST(backspace_at_beginning);
    RUN_TEST(backspace_empty_buffer);
    RUN_TEST(delete_range_basic);
    RUN_TEST(delete_range_null_pointer);
    RUN_TEST(delete_range_invalid_range);
    RUN_TEST(delete_range_empty_range);
    RUN_TEST(delete_range_cursor_updates);
    RUN_TEST(delete_range_at_beginning);
    RUN_TEST(delete_range_at_end);
    RUN_TEST(delete_range_entire_buffer);
    
    // LLE-005: Cursor movement tests
    RUN_TEST(move_cursor_left_basic);
    RUN_TEST(move_cursor_left_at_beginning);
    RUN_TEST(move_cursor_right_basic);
    RUN_TEST(move_cursor_right_at_end);
    RUN_TEST(move_cursor_home);
    RUN_TEST(move_cursor_end);
    RUN_TEST(move_cursor_word_left);
    RUN_TEST(move_cursor_word_right);
    RUN_TEST(move_cursor_word_with_punctuation);
    RUN_TEST(set_cursor_basic);
    RUN_TEST(set_cursor_invalid_position);
    RUN_TEST(set_cursor_null_pointer);
    RUN_TEST(move_cursor_null_pointer);
    RUN_TEST(move_cursor_empty_buffer);
    RUN_TEST(move_cursor_word_edge_cases);
    
    printf("\n============================\n");
    printf("Tests completed: %d/%d passed\n", tests_passed, tests_run);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}