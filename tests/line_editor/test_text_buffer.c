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