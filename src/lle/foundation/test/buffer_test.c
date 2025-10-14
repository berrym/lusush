// src/lle/foundation/test/buffer_test.c
//
// Comprehensive tests for gap buffer implementation

#define _POSIX_C_SOURCE 200809L
#include "../buffer/buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        tests_run++; \
        printf("Running test: %s...", #name); \
        fflush(stdout); \
        test_##name(); \
        tests_passed++; \
        printf(" PASS\n"); \
    } \
    static void test_##name(void)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("\n  FAIL: %s\n", message); \
            tests_failed++; \
            return; \
        } \
    } while (0)

// Test 1: Basic initialization and cleanup
TEST(init_cleanup) {
    lle_buffer_t buffer;
    
    int result = lle_buffer_init(&buffer, 1024);
    ASSERT(result == LLE_BUFFER_OK, "Init should succeed");
    ASSERT(buffer.data != NULL, "Data should be allocated");
    ASSERT(buffer.capacity == 1024, "Capacity should be 1024");
    ASSERT(lle_buffer_is_empty(&buffer), "Buffer should be empty");
    
    lle_buffer_cleanup(&buffer);
    ASSERT(buffer.data == NULL, "Data should be freed");
}

// Test 2: Init from string
TEST(init_from_string) {
    lle_buffer_t buffer;
    const char *str = "Hello, World!";
    
    int result = lle_buffer_init_from_string(&buffer, str, strlen(str));
    ASSERT(result == LLE_BUFFER_OK, "Init from string should succeed");
    ASSERT(lle_buffer_size(&buffer) == strlen(str), "Size should match string length");
    ASSERT(!lle_buffer_is_empty(&buffer), "Buffer should not be empty");
    
    char contents[64];
    result = lle_buffer_get_contents(&buffer, contents, sizeof(contents));
    ASSERT(result == LLE_BUFFER_OK, "Get contents should succeed");
    ASSERT(strcmp(contents, str) == 0, "Contents should match");
    
    lle_buffer_cleanup(&buffer);
}

// Test 3: Single character operations
TEST(single_char_operations) {
    lle_buffer_t buffer;
    lle_buffer_init(&buffer, 256);
    
    // Insert characters
    int result = lle_buffer_insert_char(&buffer, 0, 'A');
    ASSERT(result == LLE_BUFFER_OK, "Insert should succeed");
    ASSERT(lle_buffer_size(&buffer) == 1, "Size should be 1");
    
    lle_buffer_insert_char(&buffer, 1, 'B');
    lle_buffer_insert_char(&buffer, 2, 'C');
    ASSERT(lle_buffer_size(&buffer) == 3, "Size should be 3");
    
    // Get characters
    char ch;
    result = lle_buffer_get_char(&buffer, 0, &ch);
    ASSERT(result == LLE_BUFFER_OK && ch == 'A', "First char should be A");
    
    result = lle_buffer_get_char(&buffer, 1, &ch);
    ASSERT(result == LLE_BUFFER_OK && ch == 'B', "Second char should be B");
    
    result = lle_buffer_get_char(&buffer, 2, &ch);
    ASSERT(result == LLE_BUFFER_OK && ch == 'C', "Third char should be C");
    
    // Delete character
    result = lle_buffer_delete_char(&buffer, 1);
    ASSERT(result == LLE_BUFFER_OK, "Delete should succeed");
    ASSERT(lle_buffer_size(&buffer) == 2, "Size should be 2");
    
    result = lle_buffer_get_char(&buffer, 1, &ch);
    ASSERT(result == LLE_BUFFER_OK && ch == 'C', "Second char should now be C");
    
    lle_buffer_cleanup(&buffer);
}

// Test 4: String operations
TEST(string_operations) {
    lle_buffer_t buffer;
    lle_buffer_init(&buffer, 256);
    
    // Insert string
    const char *str1 = "Hello";
    int result = lle_buffer_insert_string(&buffer, 0, str1, strlen(str1));
    ASSERT(result == LLE_BUFFER_OK, "Insert string should succeed");
    ASSERT(lle_buffer_size(&buffer) == 5, "Size should be 5");
    
    // Insert at end
    const char *str2 = ", World!";
    result = lle_buffer_insert_string(&buffer, 5, str2, strlen(str2));
    ASSERT(result == LLE_BUFFER_OK, "Insert at end should succeed");
    ASSERT(lle_buffer_size(&buffer) == 13, "Size should be 13");
    
    // Get contents
    char contents[64];
    result = lle_buffer_get_contents(&buffer, contents, sizeof(contents));
    ASSERT(result == LLE_BUFFER_OK, "Get contents should succeed");
    ASSERT(strcmp(contents, "Hello, World!") == 0, "Contents should match");
    
    lle_buffer_cleanup(&buffer);
}

// Test 5: Insert in middle
TEST(insert_in_middle) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "HelloWorld", 10);
    
    // Insert in middle
    int result = lle_buffer_insert_string(&buffer, 5, ", ", 2);
    ASSERT(result == LLE_BUFFER_OK, "Insert in middle should succeed");
    ASSERT(lle_buffer_size(&buffer) == 12, "Size should be 12");
    
    char contents[64];
    lle_buffer_get_contents(&buffer, contents, sizeof(contents));
    ASSERT(strcmp(contents, "Hello, World") == 0, "Contents should match");
    
    lle_buffer_cleanup(&buffer);
}

// Test 6: Delete range
TEST(delete_range) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "Hello, World!", 13);
    
    // Delete ", World"
    int result = lle_buffer_delete_range(&buffer, 5, 12);
    ASSERT(result == LLE_BUFFER_OK, "Delete range should succeed");
    ASSERT(lle_buffer_size(&buffer) == 6, "Size should be 6");
    
    char contents[64];
    lle_buffer_get_contents(&buffer, contents, sizeof(contents));
    ASSERT(strcmp(contents, "Hello!") == 0, "Contents should match");
    
    lle_buffer_cleanup(&buffer);
}

// Test 7: Buffer growth
TEST(buffer_growth) {
    lle_buffer_t buffer;
    lle_buffer_init(&buffer, 16);  // Small initial capacity
    
    size_t initial_capacity = buffer.capacity;
    
    // Insert more than initial capacity
    const char *str = "This is a long string that exceeds initial capacity";
    int result = lle_buffer_insert_string(&buffer, 0, str, strlen(str));
    ASSERT(result == LLE_BUFFER_OK, "Insert should succeed");
    ASSERT(buffer.capacity > initial_capacity, "Buffer should have grown");
    ASSERT(buffer.grow_count > 0, "Grow count should be non-zero");
    
    char contents[128];
    lle_buffer_get_contents(&buffer, contents, sizeof(contents));
    ASSERT(strcmp(contents, str) == 0, "Contents should match after growth");
    
    lle_buffer_cleanup(&buffer);
}

// Test 8: Read-only buffer
TEST(read_only_buffer) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "Test", 4);
    
    lle_buffer_set_read_only(&buffer, true);
    
    int result = lle_buffer_insert_char(&buffer, 0, 'X');
    ASSERT(result == LLE_BUFFER_ERR_READ_ONLY, "Insert should fail on read-only");
    
    result = lle_buffer_delete_char(&buffer, 0);
    ASSERT(result == LLE_BUFFER_ERR_READ_ONLY, "Delete should fail on read-only");
    
    lle_buffer_cleanup(&buffer);
}

// Test 9: Modified flag
TEST(modified_flag) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "Test", 4);
    
    ASSERT(!lle_buffer_is_modified(&buffer), "Should not be modified initially");
    
    lle_buffer_insert_char(&buffer, 0, 'X');
    ASSERT(lle_buffer_is_modified(&buffer), "Should be modified after insert");
    
    lle_buffer_clear_modified(&buffer);
    ASSERT(!lle_buffer_is_modified(&buffer), "Should not be modified after clear");
    
    lle_buffer_cleanup(&buffer);
}

// Test 10: Search operations
TEST(search_operations) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "Hello, World! Hello!", 20);
    
    // Find character
    lle_buffer_pos_t pos = lle_buffer_find_char(&buffer, 0, 'W');
    ASSERT(pos == 7, "Should find 'W' at position 7");
    
    pos = lle_buffer_find_char(&buffer, 0, 'X');
    ASSERT(pos == LLE_BUFFER_NPOS, "Should not find 'X'");
    
    // Find string
    pos = lle_buffer_find_string(&buffer, 0, "Hello", 5);
    ASSERT(pos == 0, "Should find 'Hello' at position 0");
    
    pos = lle_buffer_find_string(&buffer, 1, "Hello", 5);
    ASSERT(pos == 14, "Should find second 'Hello' at position 14");
    
    lle_buffer_cleanup(&buffer);
}

// Test 11: Line operations
TEST(line_operations) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "Line 1\nLine 2\nLine 3", 20);
    
    // Line count
    size_t count = lle_buffer_line_count(&buffer);
    ASSERT(count == 3, "Should have 3 lines");
    
    // Line start
    lle_buffer_pos_t pos = lle_buffer_line_start(&buffer, 10);
    ASSERT(pos == 7, "Line start should be at position 7");
    
    // Line end
    pos = lle_buffer_line_end(&buffer, 10);
    ASSERT(pos == 13, "Line end should be at position 13");
    
    // Line number
    size_t line = lle_buffer_get_line_number(&buffer, 10);
    ASSERT(line == 1, "Position 10 should be on line 1 (0-indexed)");
    
    lle_buffer_cleanup(&buffer);
}

// Test 12: Performance
TEST(performance) {
    lle_buffer_t buffer;
    lle_buffer_init(&buffer, 1024);
    
    // Perform many insertions
    const int iterations = 1000;
    for (int i = 0; i < iterations; i++) {
        lle_buffer_insert_char(&buffer, 0, 'A');
    }
    
    ASSERT(lle_buffer_size(&buffer) == iterations, "Size should match iterations");
    
    // Get metrics
    uint64_t insert_count, delete_count, move_count;
    double avg_time_us;
    lle_buffer_get_metrics(&buffer, &insert_count, &delete_count, &move_count, &avg_time_us);
    
    ASSERT(insert_count == iterations, "Insert count should match");
    ASSERT(avg_time_us < 10.0, "Average time should be <10μs");
    
    printf("\n  Performance: %lu inserts, avg %.3f μs",
           (unsigned long)insert_count, avg_time_us);
    
    lle_buffer_cleanup(&buffer);
}

// Test 13: Clear buffer
TEST(clear_buffer) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "Test content", 12);
    
    ASSERT(lle_buffer_size(&buffer) == 12, "Should have content");
    
    int result = lle_buffer_clear(&buffer);
    ASSERT(result == LLE_BUFFER_OK, "Clear should succeed");
    ASSERT(lle_buffer_is_empty(&buffer), "Buffer should be empty");
    ASSERT(lle_buffer_size(&buffer) == 0, "Size should be 0");
    
    lle_buffer_cleanup(&buffer);
}

// Test 14: Error handling
TEST(error_handling) {
    lle_buffer_t buffer;
    lle_buffer_init(&buffer, 256);
    
    // Out of bounds access
    char ch;
    int result = lle_buffer_get_char(&buffer, 1000, &ch);
    ASSERT(result == LLE_BUFFER_ERR_OUT_OF_BOUNDS, "Should return out of bounds");
    
    // Invalid position
    result = lle_buffer_insert_char(&buffer, 1000, 'X');
    ASSERT(result == LLE_BUFFER_ERR_INVALID_POS, "Should return invalid position");
    
    lle_buffer_cleanup(&buffer);
}

int main(void) {
    printf("LLE Gap Buffer Tests\n");
    printf("====================\n\n");
    
    // Run all tests
    run_test_init_cleanup();
    run_test_init_from_string();
    run_test_single_char_operations();
    run_test_string_operations();
    run_test_insert_in_middle();
    run_test_delete_range();
    run_test_buffer_growth();
    run_test_read_only_buffer();
    run_test_modified_flag();
    run_test_search_operations();
    run_test_line_operations();
    run_test_performance();
    run_test_clear_buffer();
    run_test_error_handling();
    
    // Summary
    printf("\n====================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed > 0) {
        printf("\nFAILURE: %d test(s) failed\n", tests_failed);
        return 1;
    }
    
    printf("\nSUCCESS: All tests passed\n");
    return 0;
}
