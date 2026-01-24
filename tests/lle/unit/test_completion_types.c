/*
 * Lush Shell - LLE Completion Types Unit Tests
 * Copyright (C) 2021-2026  Michael Berry
 *
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "lle/completion/completion_types.h"
#include "lle/memory_management.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// TEST: Type Information Queries
// ============================================================================

static void test_type_info_queries(void) {
    printf("test_type_info_queries... ");

    // Test getting type info for builtin
    const lle_completion_type_info_t *info =
        lle_completion_type_get_info(LLE_COMPLETION_TYPE_BUILTIN);
    assert(info != NULL);
    assert(info->type == LLE_COMPLETION_TYPE_BUILTIN);
    assert(strcmp(info->type_name, "Built-in") == 0);
    assert(strcmp(info->category_name, "BUILT-INS") == 0);
    assert(info->default_priority == 900);

    // Test category name getter
    const char *category =
        lle_completion_type_get_category(LLE_COMPLETION_TYPE_COMMAND);
    assert(strcmp(category, "COMMANDS") == 0);

    // Test indicator getter
    const char *indicator =
        lle_completion_type_get_indicator(LLE_COMPLETION_TYPE_FILE);
    assert(indicator != NULL);

    // Test invalid type (should return UNKNOWN)
    info = lle_completion_type_get_info((lle_completion_type_t)999);
    assert(info != NULL);
    assert(info->type == LLE_COMPLETION_TYPE_UNKNOWN);

    printf("PASS\n");
}

// ============================================================================
// TEST: Completion Item Creation and Cleanup
// ============================================================================

static void test_completion_item_lifecycle(void) {
    printf("test_completion_item_lifecycle... ");

    // Note: LLE uses global memory pool, so we pass a dummy non-NULL pool
    // pointer
    lle_memory_pool_t *pool = (lle_memory_pool_t *)1;

    // Create item
    lle_completion_item_t *item = NULL;
    lle_result_t result = lle_completion_item_create(
        pool, "test_command", " ", LLE_COMPLETION_TYPE_COMMAND, 800, &item);

    assert(result == LLE_SUCCESS);
    assert(item != NULL);
    assert(item->text != NULL);
    assert(strcmp(item->text, "test_command") == 0);
    assert(item->suffix != NULL);
    assert(strcmp(item->suffix, " ") == 0);
    assert(item->type == LLE_COMPLETION_TYPE_COMMAND);
    assert(item->relevance_score == 800);
    assert(item->owns_text == true);
    assert(item->owns_suffix == true);

    // Free item
    result = lle_completion_item_free(pool, item);
    assert(result == LLE_SUCCESS);

    printf("PASS\n");
}

// ============================================================================
// TEST: Completion Item with Description
// ============================================================================

static void test_completion_item_with_description(void) {
    printf("test_completion_item_with_description... ");

    lle_memory_pool_t *pool = (lle_memory_pool_t *)1;

    lle_completion_item_t *item = NULL;
    lle_result_t result = lle_completion_item_create_with_description(
        pool, "ls", " ", LLE_COMPLETION_TYPE_BUILTIN, 900,
        "List directory contents", &item);

    assert(result == LLE_SUCCESS);
    assert(item != NULL);
    assert(item->description != NULL);
    assert(strcmp(item->description, "List directory contents") == 0);
    assert(item->owns_description == true);

    lle_completion_item_free(pool, item);

    printf("PASS\n");
}

// ============================================================================
// TEST: Completion Result Creation and Management
// ============================================================================

static void test_completion_result_lifecycle(void) {
    printf("test_completion_result_lifecycle... ");

    lle_memory_pool_t *pool = (lle_memory_pool_t *)1;

    // Create result
    lle_completion_result_t *result = NULL;
    lle_result_t res = lle_completion_result_create(pool, 4, &result);

    assert(res == LLE_SUCCESS);
    assert(result != NULL);
    assert(result->count == 0);
    assert(result->capacity == 4);
    assert(result->items != NULL);
    assert(result->memory_pool == pool);

    // Add items
    res = lle_completion_result_add(result, "cd", " ",
                                    LLE_COMPLETION_TYPE_BUILTIN, 900);
    assert(res == LLE_SUCCESS);
    assert(result->count == 1);
    assert(result->builtin_count == 1);

    res = lle_completion_result_add(result, "ls", " ",
                                    LLE_COMPLETION_TYPE_COMMAND, 800);
    assert(res == LLE_SUCCESS);
    assert(result->count == 2);
    assert(result->command_count == 1);

    res = lle_completion_result_add(result, "file.txt", "",
                                    LLE_COMPLETION_TYPE_FILE, 600);
    assert(res == LLE_SUCCESS);
    assert(result->count == 3);
    assert(result->file_count == 1);

    // Test capacity growth
    res = lle_completion_result_add(result, "dir/", "/",
                                    LLE_COMPLETION_TYPE_DIRECTORY, 700);
    assert(res == LLE_SUCCESS);
    assert(result->count == 4);

    // Adding 5th item should trigger growth (capacity was 4)
    res = lle_completion_result_add(result, "$HOME", "",
                                    LLE_COMPLETION_TYPE_VARIABLE, 500);
    assert(res == LLE_SUCCESS);
    assert(result->count == 5);
    assert(result->capacity >= 5); // Should have grown

    // Test item retrieval
    const lle_completion_item_t *item =
        lle_completion_result_get_item(result, 0);
    assert(item != NULL);
    assert(strcmp(item->text, "cd") == 0);

    // Test count by type
    size_t builtin_count = lle_completion_result_count_by_type(
        result, LLE_COMPLETION_TYPE_BUILTIN);
    assert(builtin_count == 1);

    // Free result
    res = lle_completion_result_free(result);
    assert(res == LLE_SUCCESS);

    printf("PASS\n");
}

// ============================================================================
// TEST: Completion Result Sorting
// ============================================================================

static void test_completion_result_sorting(void) {
    printf("test_completion_result_sorting... ");

    lle_memory_pool_t *pool = (lle_memory_pool_t *)1;

    lle_completion_result_t *result = NULL;
    lle_completion_result_create(pool, 8, &result);

    // Add items in mixed order
    lle_completion_result_add(result, "file1.txt", "", LLE_COMPLETION_TYPE_FILE,
                              500);
    lle_completion_result_add(result, "cd", " ", LLE_COMPLETION_TYPE_BUILTIN,
                              950);
    lle_completion_result_add(result, "ls", " ", LLE_COMPLETION_TYPE_COMMAND,
                              800);
    lle_completion_result_add(result, "echo", " ", LLE_COMPLETION_TYPE_BUILTIN,
                              900);

    // Sort
    lle_result_t res = lle_completion_result_sort(result);
    assert(res == LLE_SUCCESS);

    // After sorting:
    // - Builtins first (type 0)
    // - Then commands (type 1)
    // - Then files (type 2)
    // Within each type, higher relevance first

    const lle_completion_item_t *item0 =
        lle_completion_result_get_item(result, 0);
    const lle_completion_item_t *item1 =
        lle_completion_result_get_item(result, 1);
    const lle_completion_item_t *item2 =
        lle_completion_result_get_item(result, 2);
    const lle_completion_item_t *item3 =
        lle_completion_result_get_item(result, 3);

    // First two should be builtins
    assert(item0->type == LLE_COMPLETION_TYPE_BUILTIN);
    assert(item1->type == LLE_COMPLETION_TYPE_BUILTIN);

    // Within builtins, higher relevance first
    assert(item0->relevance_score >= item1->relevance_score);

    // Next should be command
    assert(item2->type == LLE_COMPLETION_TYPE_COMMAND);

    // Last should be file
    assert(item3->type == LLE_COMPLETION_TYPE_FILE);

    lle_completion_result_free(result);

    printf("PASS\n");
}

// ============================================================================
// TEST: Classification Functions
// ============================================================================

static void test_classification(void) {
    printf("test_classification... ");

    // Test variable detection
    lle_completion_type_t type = lle_completion_classify_text("$HOME", false);
    assert(type == LLE_COMPLETION_TYPE_VARIABLE);

    // Test path with slash (not at command position)
    type = lle_completion_classify_text("dir/file", false);
    // Should classify as FILE since it's not at command position
    // and directory check will fail for non-existent path
    assert(type == LLE_COMPLETION_TYPE_FILE ||
           type == LLE_COMPLETION_TYPE_DIRECTORY);

    // Test command position without slash
    type = lle_completion_classify_text("somecommand", true);
    // Without actual alias/builtin check, should default to COMMAND
    assert(type == LLE_COMPLETION_TYPE_COMMAND);

    printf("PASS\n");
}

// ============================================================================
// TEST: Error Handling
// ============================================================================

static void test_error_handling(void) {
    printf("test_error_handling... ");

    lle_memory_pool_t *pool = (lle_memory_pool_t *)1;

    // Test NULL parameters
    lle_completion_item_t *item = NULL;
    lle_result_t result = lle_completion_item_create(
        NULL, "test", " ", LLE_COMPLETION_TYPE_COMMAND, 800, &item);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    result = lle_completion_item_create(
        pool, NULL, " ", LLE_COMPLETION_TYPE_COMMAND, 800, &item);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    result = lle_completion_item_create(pool, "test", " ",
                                        LLE_COMPLETION_TYPE_COMMAND, 800, NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    // Test NULL result parameter
    lle_completion_result_t *comp_result = NULL;
    result = lle_completion_result_create(NULL, 16, &comp_result);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    result = lle_completion_result_create(pool, 16, NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    printf("PASS\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("Running LLE Completion Types Unit Tests\n");
    printf("=========================================\n\n");

    test_type_info_queries();
    test_completion_item_lifecycle();
    test_completion_item_with_description();
    test_completion_result_lifecycle();
    test_completion_result_sorting();
    test_classification();
    test_error_handling();

    printf("\n=========================================\n");
    printf("All tests PASSED!\n");

    return 0;
}
