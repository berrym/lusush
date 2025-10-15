// src/lle/foundation/test/undo_test.c
//
// Test suite for LLE Undo/Redo System

#include "../buffer/undo.h"
#include "../buffer/buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Test result codes
#define TEST_PASS 0
#define TEST_FAIL 1

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

// Helper: Print test result
static void test_result(const char *test_name, int result) {
    tests_run++;
    if (result == TEST_PASS) {
        tests_passed++;
        printf("[PASS] %s\n", test_name);
    } else {
        printf("[FAIL] %s\n", test_name);
    }
}

// Helper: Get buffer contents
static char* get_buffer_contents(lle_buffer_t *buffer) {
    size_t size = lle_buffer_size(buffer);
    char *contents = malloc(size + 1);
    if (!contents) return NULL;
    
    lle_buffer_get_contents(buffer, contents, size + 1);
    return contents;
}

// Test: Change tracker initialization
static int test_init_cleanup(void) {
    lle_change_tracker_t tracker;
    
    int result = lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    if (result != LLE_UNDO_OK) {
        return TEST_FAIL;
    }
    
    if (!lle_change_tracker_is_enabled(&tracker)) {
        lle_change_tracker_cleanup(&tracker);
        return TEST_FAIL;
    }
    
    if (lle_change_tracker_can_undo(&tracker)) {
        lle_change_tracker_cleanup(&tracker);
        return TEST_FAIL;
    }
    
    if (lle_change_tracker_can_redo(&tracker)) {
        lle_change_tracker_cleanup(&tracker);
        return TEST_FAIL;
    }
    
    lle_change_tracker_cleanup(&tracker);
    return TEST_PASS;
}

// Test: Record and undo single insert
static int test_undo_single_insert(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Begin sequence
    lle_change_tracker_begin_sequence(&tracker);
    
    // Insert "hello"
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    
    // End sequence
    lle_change_tracker_end_sequence(&tracker);
    
    // Verify buffer has "hello"
    char *contents = get_buffer_contents(&buffer);
    int result = TEST_FAIL;
    
    if (strcmp(contents, "hello") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    // Undo
    if (lle_change_tracker_undo(&tracker, &buffer) != LLE_UNDO_OK) {
        goto cleanup;
    }
    
    // Verify buffer is empty
    if (lle_buffer_size(&buffer) != 0) {
        goto cleanup;
    }
    
    // Can't undo anymore
    if (lle_change_tracker_can_undo(&tracker)) {
        goto cleanup;
    }
    
    // Can redo
    if (!lle_change_tracker_can_redo(&tracker)) {
        goto cleanup;
    }
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: Redo after undo
static int test_redo_after_undo(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Insert "hello"
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    // Undo
    lle_change_tracker_undo(&tracker, &buffer);
    
    // Redo
    int result = TEST_FAIL;
    if (lle_change_tracker_redo(&tracker, &buffer) != LLE_UNDO_OK) {
        goto cleanup;
    }
    
    // Verify buffer has "hello" again
    char *contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "hello") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    // Can undo but not redo
    if (!lle_change_tracker_can_undo(&tracker)) {
        goto cleanup;
    }
    
    if (lle_change_tracker_can_redo(&tracker)) {
        goto cleanup;
    }
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: Undo single delete
static int test_undo_single_delete(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init_from_string(&buffer, "hello", 5);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Delete "hello"
    lle_change_tracker_begin_sequence(&tracker);
    lle_change_tracker_record_delete(&tracker, 0, "hello", 5, 5, 0);
    lle_buffer_delete_range(&buffer, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    // Verify buffer is empty
    int result = TEST_FAIL;
    if (lle_buffer_size(&buffer) != 0) {
        goto cleanup;
    }
    
    // Undo delete
    if (lle_change_tracker_undo(&tracker, &buffer) != LLE_UNDO_OK) {
        goto cleanup;
    }
    
    // Verify buffer has "hello"
    char *contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "hello") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: Multiple operations in single sequence
static int test_multiple_operations_sequence(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Begin sequence with multiple operations
    lle_change_tracker_begin_sequence(&tracker);
    
    // Insert "hello"
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    
    // Insert " world"
    lle_buffer_insert_string(&buffer, 5, " world", 6);
    lle_change_tracker_record_insert(&tracker, 5, " world", 6, 5, 11);
    
    lle_change_tracker_end_sequence(&tracker);
    
    // Verify buffer has "hello world"
    char *contents = get_buffer_contents(&buffer);
    int result = TEST_FAIL;
    
    if (strcmp(contents, "hello world") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    // Undo entire sequence
    if (lle_change_tracker_undo(&tracker, &buffer) != LLE_UNDO_OK) {
        goto cleanup;
    }
    
    // Verify buffer is empty (both operations undone)
    if (lle_buffer_size(&buffer) != 0) {
        goto cleanup;
    }
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: Multiple sequences
static int test_multiple_sequences(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Sequence 1: Insert "hello"
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    // Sequence 2: Insert " world"
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 5, " world", 6);
    lle_change_tracker_record_insert(&tracker, 5, " world", 6, 5, 11);
    lle_change_tracker_end_sequence(&tracker);
    
    int result = TEST_FAIL;
    char *contents;
    
    // Verify "hello world"
    contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "hello world") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    // Undo sequence 2
    lle_change_tracker_undo(&tracker, &buffer);
    
    // Verify "hello"
    contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "hello") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    // Undo sequence 1
    lle_change_tracker_undo(&tracker, &buffer);
    
    // Verify empty
    if (lle_buffer_size(&buffer) != 0) {
        goto cleanup;
    }
    
    // Redo sequence 1
    lle_change_tracker_redo(&tracker, &buffer);
    
    // Verify "hello"
    contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "hello") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    // Redo sequence 2
    lle_change_tracker_redo(&tracker, &buffer);
    
    // Verify "hello world"
    contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "hello world") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: New change invalidates redo history
static int test_new_change_invalidates_redo(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Insert "hello"
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    // Undo
    lle_change_tracker_undo(&tracker, &buffer);
    
    int result = TEST_FAIL;
    
    // Can redo at this point
    if (!lle_change_tracker_can_redo(&tracker)) {
        goto cleanup;
    }
    
    // Insert "world" (new change)
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 0, "world", 5);
    lle_change_tracker_record_insert(&tracker, 0, "world", 5, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    // Redo should no longer be available
    if (lle_change_tracker_can_redo(&tracker)) {
        goto cleanup;
    }
    
    // Buffer should have "world"
    char *contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "world") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: Clear undo/redo history
static int test_clear_history(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Insert "hello"
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    int result = TEST_FAIL;
    
    // Should be able to undo
    if (!lle_change_tracker_can_undo(&tracker)) {
        goto cleanup;
    }
    
    // Clear history
    lle_change_tracker_clear(&tracker);
    
    // Should not be able to undo or redo
    if (lle_change_tracker_can_undo(&tracker)) {
        goto cleanup;
    }
    
    if (lle_change_tracker_can_redo(&tracker)) {
        goto cleanup;
    }
    
    // Buffer should still have "hello" (clear doesn't affect buffer)
    char *contents = get_buffer_contents(&buffer);
    if (strcmp(contents, "hello") != 0) {
        free(contents);
        goto cleanup;
    }
    free(contents);
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: Enable/disable tracking
static int test_enable_disable_tracking(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    // Disable tracking
    lle_change_tracker_set_enabled(&tracker, false);
    
    int result = TEST_FAIL;
    
    if (lle_change_tracker_is_enabled(&tracker)) {
        goto cleanup;
    }
    
    // Record operation while disabled
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    // Should not be able to undo (tracking was disabled)
    if (lle_change_tracker_can_undo(&tracker)) {
        goto cleanup;
    }
    
    // Re-enable tracking
    lle_change_tracker_set_enabled(&tracker, true);
    
    if (!lle_change_tracker_is_enabled(&tracker)) {
        goto cleanup;
    }
    
    // Now record operation with tracking enabled
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 5, " world", 6);
    lle_change_tracker_record_insert(&tracker, 5, " world", 6, 5, 11);
    lle_change_tracker_end_sequence(&tracker);
    
    // Should be able to undo this one
    if (!lle_change_tracker_can_undo(&tracker)) {
        goto cleanup;
    }
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Test: Statistics
static int test_statistics(void) {
    lle_buffer_t buffer;
    lle_change_tracker_t tracker;
    
    lle_buffer_init(&buffer, 256);
    lle_change_tracker_init(&tracker, 100, 1024 * 1024);
    
    size_t total_sequences, total_operations, memory_used;
    uint32_t undo_count, redo_count;
    
    // Initial stats
    lle_change_tracker_get_stats(&tracker, &total_sequences, &total_operations,
                                 &memory_used, &undo_count, &redo_count);
    
    int result = TEST_FAIL;
    
    if (total_sequences != 0 || total_operations != 0) {
        goto cleanup;
    }
    
    // Add a sequence
    lle_change_tracker_begin_sequence(&tracker);
    lle_buffer_insert_string(&buffer, 0, "hello", 5);
    lle_change_tracker_record_insert(&tracker, 0, "hello", 5, 0, 5);
    lle_change_tracker_end_sequence(&tracker);
    
    // Check stats
    lle_change_tracker_get_stats(&tracker, &total_sequences, &total_operations,
                                 &memory_used, &undo_count, &redo_count);
    
    if (total_sequences != 1 || total_operations != 1) {
        goto cleanup;
    }
    
    if (memory_used == 0) {
        goto cleanup;
    }
    
    // Undo
    lle_change_tracker_undo(&tracker, &buffer);
    
    // Check undo count incremented
    lle_change_tracker_get_stats(&tracker, &total_sequences, &total_operations,
                                 &memory_used, &undo_count, &redo_count);
    
    if (undo_count != 1) {
        goto cleanup;
    }
    
    // Redo
    lle_change_tracker_redo(&tracker, &buffer);
    
    // Check redo count incremented
    lle_change_tracker_get_stats(&tracker, &total_sequences, &total_operations,
                                 &memory_used, &undo_count, &redo_count);
    
    if (redo_count != 1) {
        goto cleanup;
    }
    
    result = TEST_PASS;
    
cleanup:
    lle_buffer_cleanup(&buffer);
    lle_change_tracker_cleanup(&tracker);
    return result;
}

// Main test runner
int main(void) {
    printf("\nLLE Undo/Redo System Tests\n");
    printf("===========================\n\n");
    
    test_result("Init and cleanup", test_init_cleanup());
    test_result("Undo single insert", test_undo_single_insert());
    test_result("Redo after undo", test_redo_after_undo());
    test_result("Undo single delete", test_undo_single_delete());
    test_result("Multiple operations in sequence", test_multiple_operations_sequence());
    test_result("Multiple sequences", test_multiple_sequences());
    test_result("New change invalidates redo", test_new_change_invalidates_redo());
    test_result("Clear history", test_clear_history());
    test_result("Enable/disable tracking", test_enable_disable_tracking());
    test_result("Statistics", test_statistics());
    
    printf("\n===========================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\nAll tests passed!\n");
        return 0;
    } else {
        printf("\nSome tests failed\n");
        return 1;
    }
}
