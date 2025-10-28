/**
 * @file test_dirty_tracker.c
 * @brief Unit Tests for LLE Dirty Region Tracker
 * 
 * Tests the dirty region tracking system including:
 * - Tracker initialization and cleanup
 * - Region marking (single region, range, full)
 * - Region querying and iteration
 * - Region clearing
 * - Automatic region merging
 * 
 * IMPLEMENTATION: src/lle/dirty_tracker.c
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ========================================================================== */
/*                         TEST FRAMEWORK                                     */
/* ========================================================================== */

/* Test tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    __attribute__((unused)) static void run_test_##name(void) { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
    } \
    static void test_##name(void)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  ✗ ASSERTION FAILED: %s\n", msg); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)
#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)
#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)
#define ASSERT_EQ(a, b, msg) ASSERT_TRUE((a) == (b), msg)
#define ASSERT_NEQ(a, b, msg) ASSERT_TRUE((a) != (b), msg)

/* ========================================================================== */
/*                         MOCK OBJECTS                                       */
/* ========================================================================== */

/* Mock memory pool */
static int mock_pool_dummy = 42;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t*)&mock_pool_dummy;

/* Stubs for Lusush functions */
lusush_memory_pool_system_t *global_memory_pool = NULL;

void *lusush_pool_alloc(size_t size) {
    return malloc(size);
}

void lusush_pool_free(void *ptr) {
    free(ptr);
}

lusush_pool_config_t lusush_pool_get_default_config(void) {
    lusush_pool_config_t config = {0};
    return config;
}

lusush_pool_error_t lusush_pool_init(const lusush_pool_config_t *config) {
    (void)config;
    return 0;
}

/* ========================================================================== */
/*                    TRACKER INITIALIZATION TESTS                            */
/* ========================================================================== */

TEST(tracker_init_success) {
    lle_dirty_tracker_t *tracker = NULL;
    
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    ASSERT_NOT_NULL(tracker, "Tracker should be allocated");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(tracker_init_null_output) {
    lle_result_t result = lle_dirty_tracker_init(NULL, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "NULL output should return error");
}

TEST(tracker_init_null_pool) {
    lle_dirty_tracker_t *tracker = NULL;
    
    lle_result_t result = lle_dirty_tracker_init(&tracker, NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "NULL pool should return error");
}

/* ========================================================================== */
/*                    TRACKER CLEANUP TESTS                                   */
/* ========================================================================== */

TEST(tracker_cleanup_null) {
    lle_result_t result = lle_dirty_tracker_cleanup(NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Cleanup with NULL should return error");
}

TEST(tracker_cleanup_success) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    lle_result_t result = lle_dirty_tracker_cleanup(tracker);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Cleanup should succeed");
}

/* ========================================================================== */
/*                    REGION MARKING TESTS                                    */
/* ========================================================================== */

TEST(tracker_mark_full_null) {
    lle_result_t result = lle_dirty_tracker_mark_full(NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Mark full with NULL should return error");
}

TEST(tracker_mark_full_success) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    lle_result_t result = lle_dirty_tracker_mark_full(tracker);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Mark full should succeed");
    ASSERT_TRUE(lle_dirty_tracker_needs_full_redraw(tracker), "Should need full redraw");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(tracker_mark_region_null) {
    lle_result_t result = lle_dirty_tracker_mark_region(NULL, 100);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Mark region with NULL should return error");
}

TEST(tracker_mark_region_success) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    lle_result_t result = lle_dirty_tracker_mark_region(tracker, 100);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Mark region should succeed");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 100), "Region should be dirty");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(tracker_mark_multiple_regions) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    /* Mark several regions */
    lle_dirty_tracker_mark_region(tracker, 50);
    lle_dirty_tracker_mark_region(tracker, 100);
    lle_dirty_tracker_mark_region(tracker, 200);
    
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 50), "Region 50 should be dirty");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 100), "Region 100 should be dirty");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 200), "Region 200 should be dirty");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(tracker_mark_range_null) {
    lle_result_t result = lle_dirty_tracker_mark_range(NULL, 100, 50);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Mark range with NULL should return error");
}

TEST(tracker_mark_range_success) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    lle_result_t result = lle_dirty_tracker_mark_range(tracker, 100, 50);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Mark range should succeed");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 100), "Start of range should be dirty");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 125), "Middle of range should be dirty");
    
    lle_dirty_tracker_cleanup(tracker);
}

/* ========================================================================== */
/*                    REGION QUERY TESTS                                      */
/* ========================================================================== */

TEST(tracker_is_region_dirty_null) {
    bool result = lle_dirty_tracker_is_region_dirty(NULL, 100);
    
    ASSERT_TRUE(result, "NULL tracker should return true (safe default)");
}

TEST(tracker_is_region_dirty_clean) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    /* Clear initial full redraw flag to test clean state */
    lle_dirty_tracker_clear(tracker);
    
    bool result = lle_dirty_tracker_is_region_dirty(tracker, 100);
    
    ASSERT_FALSE(result, "Unmarked region should not be dirty");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(tracker_needs_full_redraw_null) {
    bool result = lle_dirty_tracker_needs_full_redraw(NULL);
    
    ASSERT_TRUE(result, "NULL tracker should return true (safe default)");
}

TEST(tracker_needs_full_redraw_clean) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    /* Clear initial full redraw flag to test clean state */
    lle_dirty_tracker_clear(tracker);
    
    bool result = lle_dirty_tracker_needs_full_redraw(tracker);
    
    ASSERT_FALSE(result, "Clean tracker should not need full redraw");
    
    lle_dirty_tracker_cleanup(tracker);
}

/* ========================================================================== */
/*                    REGION CLEARING TESTS                                   */
/* ========================================================================== */

TEST(tracker_clear_null) {
    lle_result_t result = lle_dirty_tracker_clear(NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Clear with NULL should return error");
}

TEST(tracker_clear_success) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    /* Mark some regions */
    lle_dirty_tracker_mark_region(tracker, 100);
    lle_dirty_tracker_mark_region(tracker, 200);
    
    /* Clear */
    lle_result_t result = lle_dirty_tracker_clear(tracker);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Clear should succeed");
    ASSERT_FALSE(lle_dirty_tracker_is_region_dirty(tracker, 100), "Region should be clean after clear");
    ASSERT_FALSE(lle_dirty_tracker_is_region_dirty(tracker, 200), "Region should be clean after clear");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(tracker_clear_after_full) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    /* Mark full */
    lle_dirty_tracker_mark_full(tracker);
    ASSERT_TRUE(lle_dirty_tracker_needs_full_redraw(tracker), "Should need full redraw");
    
    /* Clear */
    lle_dirty_tracker_clear(tracker);
    ASSERT_FALSE(lle_dirty_tracker_needs_full_redraw(tracker), "Should not need full redraw after clear");
    
    lle_dirty_tracker_cleanup(tracker);
}

/* ========================================================================== */
/*                    REGION LIFECYCLE TESTS                                  */
/* ========================================================================== */

TEST(tracker_mark_clear_cycle) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    /* Mark, clear, mark again */
    lle_dirty_tracker_mark_region(tracker, 100);
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 100), "Region should be dirty");
    
    lle_dirty_tracker_clear(tracker);
    ASSERT_FALSE(lle_dirty_tracker_is_region_dirty(tracker, 100), "Region should be clean");
    
    lle_dirty_tracker_mark_region(tracker, 100);
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 100), "Region should be dirty again");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(tracker_overlapping_ranges) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_dirty_tracker_init(&tracker, mock_pool);
    
    /* Mark overlapping ranges */
    lle_dirty_tracker_mark_range(tracker, 100, 50);  /* 100-150 */
    lle_dirty_tracker_mark_range(tracker, 140, 30);  /* 140-170 (overlaps) */
    
    /* All overlapping regions should be dirty */
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 100), "First range start should be dirty");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 145), "Overlap region should be dirty");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 165), "Second range end should be dirty");
    
    lle_dirty_tracker_cleanup(tracker);
}

/* ========================================================================== */
/*                         TEST RUNNER                                        */
/* ========================================================================== */

int main(void) {
    printf("=================================================================\n");
    printf("  LLE Dirty Tracker Unit Tests\n");
    printf("=================================================================\n\n");
    
    /* Initialization tests */
    run_test_tracker_init_success();
    run_test_tracker_init_null_output();
    run_test_tracker_init_null_pool();
    
    /* Cleanup tests */
    run_test_tracker_cleanup_null();
    run_test_tracker_cleanup_success();
    
    /* Region marking tests */
    run_test_tracker_mark_full_null();
    run_test_tracker_mark_full_success();
    run_test_tracker_mark_region_null();
    run_test_tracker_mark_region_success();
    run_test_tracker_mark_multiple_regions();
    run_test_tracker_mark_range_null();
    run_test_tracker_mark_range_success();
    
    /* Region query tests */
    run_test_tracker_is_region_dirty_null();
    run_test_tracker_is_region_dirty_clean();
    run_test_tracker_needs_full_redraw_null();
    run_test_tracker_needs_full_redraw_clean();
    
    /* Region clearing tests */
    run_test_tracker_clear_null();
    run_test_tracker_clear_success();
    run_test_tracker_clear_after_full();
    
    /* Lifecycle tests */
    run_test_tracker_mark_clear_cycle();
    run_test_tracker_overlapping_ranges();
    
    /* Print summary */
    printf("\n=================================================================\n");
    printf("  Test Summary\n");
    printf("=================================================================\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("=================================================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
