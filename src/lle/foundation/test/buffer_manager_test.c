// src/lle/foundation/test/buffer_manager_test.c
//
// Test suite for LLE Buffer Manager

#include "../buffer/buffer_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

// Test: Buffer manager initialization
static int test_init_cleanup(void) {
    lle_buffer_manager_t manager;
    
    int result = lle_buffer_manager_init(&manager, 50, 256);
    if (result != LLE_BUFFER_MGR_OK) {
        return TEST_FAIL;
    }
    
    if (lle_buffer_manager_get_count(&manager) != 0) {
        lle_buffer_manager_cleanup(&manager);
        return TEST_FAIL;
    }
    
    if (lle_buffer_manager_get_current(&manager) != NULL) {
        lle_buffer_manager_cleanup(&manager);
        return TEST_FAIL;
    }
    
    lle_buffer_manager_cleanup(&manager);
    return TEST_PASS;
}

// Test: Create named buffer
static int test_create_named_buffer(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t buffer_id;
    int result = lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    int test_result = TEST_FAIL;
    
    if (result != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_count(&manager) != 1) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_named_count(&manager) != 1) {
        goto cleanup;
    }
    
    if (!lle_buffer_manager_has_buffer(&manager, buffer_id)) {
        goto cleanup;
    }
    
    if (!lle_buffer_manager_has_buffer_by_name(&manager, "test")) {
        goto cleanup;
    }
    
    // Should be current buffer since it's the first
    lle_managed_buffer_t *current = lle_buffer_manager_get_current(&manager);
    if (!current || current->buffer_id != buffer_id) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Create scratch buffer
static int test_create_scratch_buffer(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t buffer_id;
    int result = lle_buffer_manager_create_scratch(&manager, &buffer_id);
    
    int test_result = TEST_FAIL;
    
    if (result != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_count(&manager) != 1) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_scratch_count(&manager) != 1) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_named_count(&manager) != 0) {
        goto cleanup;
    }
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_buffer(&manager, buffer_id);
    if (!buf || buf->name != NULL) {
        goto cleanup;
    }
    
    if (!(buf->flags & LLE_BUFFER_FLAG_SCRATCH)) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Multiple buffers
static int test_multiple_buffers(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id1, id2, id3;
    int test_result = TEST_FAIL;
    
    // Create named buffers
    if (lle_buffer_manager_create_buffer(&manager, "buffer1", &id1) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_create_buffer(&manager, "buffer2", &id2) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    // Create scratch buffer
    if (lle_buffer_manager_create_scratch(&manager, &id3) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_count(&manager) != 3) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_named_count(&manager) != 2) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_scratch_count(&manager) != 1) {
        goto cleanup;
    }
    
    // Verify all exist
    if (!lle_buffer_manager_has_buffer(&manager, id1) ||
        !lle_buffer_manager_has_buffer(&manager, id2) ||
        !lle_buffer_manager_has_buffer(&manager, id3)) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Buffer switching
static int test_buffer_switching(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id1, id2;
    int test_result = TEST_FAIL;
    
    lle_buffer_manager_create_buffer(&manager, "first", &id1);
    lle_buffer_manager_create_buffer(&manager, "second", &id2);
    
    // Should start with first buffer current
    lle_managed_buffer_t *current = lle_buffer_manager_get_current(&manager);
    if (!current || current->buffer_id != id1) {
        goto cleanup;
    }
    
    // Switch to second
    if (lle_buffer_manager_switch_to_buffer(&manager, id2) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    current = lle_buffer_manager_get_current(&manager);
    if (!current || current->buffer_id != id2) {
        goto cleanup;
    }
    
    // Switch by name
    if (lle_buffer_manager_switch_to_buffer_by_name(&manager, "first") != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    current = lle_buffer_manager_get_current(&manager);
    if (!current || current->buffer_id != id1) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Delete buffer
static int test_delete_buffer(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id1, id2;
    int test_result = TEST_FAIL;
    
    lle_buffer_manager_create_buffer(&manager, "first", &id1);
    lle_buffer_manager_create_buffer(&manager, "second", &id2);
    
    if (lle_buffer_manager_get_count(&manager) != 2) {
        goto cleanup;
    }
    
    // Delete first buffer
    if (lle_buffer_manager_delete_buffer(&manager, id1) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_count(&manager) != 1) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_has_buffer(&manager, id1)) {
        goto cleanup;
    }
    
    if (!lle_buffer_manager_has_buffer(&manager, id2)) {
        goto cleanup;
    }
    
    // Current should have switched to second
    lle_managed_buffer_t *current = lle_buffer_manager_get_current(&manager);
    if (!current || current->buffer_id != id2) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Delete buffer by name
static int test_delete_buffer_by_name(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id;
    int test_result = TEST_FAIL;
    
    lle_buffer_manager_create_buffer(&manager, "test", &id);
    
    if (lle_buffer_manager_delete_buffer_by_name(&manager, "test") != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_count(&manager) != 0) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_has_buffer_by_name(&manager, "test")) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Rename buffer
static int test_rename_buffer(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id;
    int test_result = TEST_FAIL;
    
    lle_buffer_manager_create_buffer(&manager, "oldname", &id);
    
    if (lle_buffer_manager_rename_buffer(&manager, id, "newname") != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_has_buffer_by_name(&manager, "oldname")) {
        goto cleanup;
    }
    
    if (!lle_buffer_manager_has_buffer_by_name(&manager, "newname")) {
        goto cleanup;
    }
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_buffer(&manager, id);
    if (!buf || strcmp(buf->name, "newname") != 0) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Rename scratch buffer to named
static int test_rename_scratch_to_named(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id;
    int test_result = TEST_FAIL;
    
    lle_buffer_manager_create_scratch(&manager, &id);
    
    if (lle_buffer_manager_get_scratch_count(&manager) != 1) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_named_count(&manager) != 0) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_rename_buffer(&manager, id, "promoted") != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_scratch_count(&manager) != 0) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_named_count(&manager) != 1) {
        goto cleanup;
    }
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_buffer(&manager, id);
    if (!buf || !(buf->flags & LLE_BUFFER_FLAG_PERSISTENT)) {
        goto cleanup;
    }
    
    if (buf->flags & LLE_BUFFER_FLAG_SCRATCH) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: List buffers
static int test_list_buffers(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id1, id2, id3;
    int test_result = TEST_FAIL;
    
    lle_buffer_manager_create_buffer(&manager, "first", &id1);
    lle_buffer_manager_create_buffer(&manager, "second", &id2);
    lle_buffer_manager_create_scratch(&manager, &id3);
    
    size_t count;
    uint32_t *ids = lle_buffer_manager_list_buffers(&manager, &count);
    
    if (!ids || count != 3) {
        free(ids);
        goto cleanup;
    }
    
    // Verify all IDs are present
    bool found1 = false, found2 = false, found3 = false;
    for (size_t i = 0; i < count; i++) {
        if (ids[i] == id1) found1 = true;
        if (ids[i] == id2) found2 = true;
        if (ids[i] == id3) found3 = true;
    }
    
    free(ids);
    
    if (!found1 || !found2 || !found3) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Duplicate name rejection
static int test_duplicate_name_rejection(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id1, id2;
    int test_result = TEST_FAIL;
    
    if (lle_buffer_manager_create_buffer(&manager, "samename", &id1) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    // Try to create another with same name
    int result = lle_buffer_manager_create_buffer(&manager, "samename", &id2);
    if (result != LLE_BUFFER_MGR_ERR_BUFFER_EXISTS) {
        goto cleanup;
    }
    
    // Should still have only one buffer
    if (lle_buffer_manager_get_count(&manager) != 1) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Maximum buffers limit
static int test_max_buffers_limit(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 3, 256); // Max 3 buffers
    
    uint32_t id;
    int test_result = TEST_FAIL;
    
    // Create 3 buffers (should succeed)
    if (lle_buffer_manager_create_buffer(&manager, "buf1", &id) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    if (lle_buffer_manager_create_buffer(&manager, "buf2", &id) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    if (lle_buffer_manager_create_buffer(&manager, "buf3", &id) != LLE_BUFFER_MGR_OK) {
        goto cleanup;
    }
    
    // Try to create 4th buffer (should fail)
    int result = lle_buffer_manager_create_buffer(&manager, "buf4", &id);
    if (result != LLE_BUFFER_MGR_ERR_MAX_BUFFERS) {
        goto cleanup;
    }
    
    if (lle_buffer_manager_get_count(&manager) != 3) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Test: Buffer content operations
static int test_buffer_content_operations(void) {
    lle_buffer_manager_t manager;
    lle_buffer_manager_init(&manager, 50, 256);
    
    uint32_t id;
    int test_result = TEST_FAIL;
    
    lle_buffer_manager_create_buffer(&manager, "test", &id);
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_buffer(&manager, id);
    if (!buf) {
        goto cleanup;
    }
    
    // Insert text into buffer
    if (lle_buffer_insert_string(&buf->buffer, 0, "hello world", 11) != LLE_BUFFER_OK) {
        goto cleanup;
    }
    
    if (lle_buffer_size(&buf->buffer) != 11) {
        goto cleanup;
    }
    
    // Get contents
    char contents[32];
    if (lle_buffer_get_contents(&buf->buffer, contents, sizeof(contents)) != LLE_BUFFER_OK) {
        goto cleanup;
    }
    
    if (strcmp(contents, "hello world") != 0) {
        goto cleanup;
    }
    
    test_result = TEST_PASS;
    
cleanup:
    lle_buffer_manager_cleanup(&manager);
    return test_result;
}

// Main test runner
int main(void) {
    printf("\nLLE Buffer Manager Tests\n");
    printf("========================\n\n");
    
    test_result("Init and cleanup", test_init_cleanup());
    test_result("Create named buffer", test_create_named_buffer());
    test_result("Create scratch buffer", test_create_scratch_buffer());
    test_result("Multiple buffers", test_multiple_buffers());
    test_result("Buffer switching", test_buffer_switching());
    test_result("Delete buffer", test_delete_buffer());
    test_result("Delete buffer by name", test_delete_buffer_by_name());
    test_result("Rename buffer", test_rename_buffer());
    test_result("Rename scratch to named", test_rename_scratch_to_named());
    test_result("List buffers", test_list_buffers());
    test_result("Duplicate name rejection", test_duplicate_name_rejection());
    test_result("Maximum buffers limit", test_max_buffers_limit());
    test_result("Buffer content operations", test_buffer_content_operations());
    
    printf("\n========================\n");
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
