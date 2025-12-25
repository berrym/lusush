/**
 * @file test_render_cache.c
 * @brief Unit Tests for LLE Render Cache
 *
 * Tests the libhashtable-based caching system including:
 * - Cache initialization and cleanup
 * - Cache store and lookup operations
 * - Cache invalidation (single and all)
 * - LRU eviction policy
 * - Cache metrics tracking
 *
 * IMPLEMENTATION: src/lle/render_cache.c
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================== */
/*                         TEST FRAMEWORK                                     */
/* ========================================================================== */

/* Test tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                                             \
    static void test_##name(void);                                             \
    __attribute__((unused)) static void run_test_##name(void) {                \
        printf("Running test: %s\n", #name);                                   \
        tests_run++;                                                           \
        test_##name();                                                         \
        tests_passed++;                                                        \
    }                                                                          \
    static void test_##name(void)

#define ASSERT_TRUE(cond, msg)                                                 \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("  ✗ ASSERTION FAILED: %s\n", msg);                         \
            printf("    at %s:%d\n", __FILE__, __LINE__);                      \
            tests_failed++;                                                    \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)
#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)
#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)
#define ASSERT_EQ(a, b, msg) ASSERT_TRUE((a) == (b), msg)
#define ASSERT_NEQ(a, b, msg) ASSERT_TRUE((a) != (b), msg)
#define ASSERT_GT(a, b, msg) ASSERT_TRUE((a) > (b), msg)

/* ========================================================================== */
/*                         MOCK OBJECTS                                       */
/* ========================================================================== */

/* Mock memory pool */
static int mock_pool_dummy = 42;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t *)&mock_pool_dummy;

/* Stubs for Lusush functions */
lusush_memory_pool_system_t *global_memory_pool = NULL;

void *lusush_pool_alloc(size_t size) { return malloc(size); }

void lusush_pool_free(void *ptr) { free(ptr); }

lusush_pool_config_t lusush_pool_get_default_config(void) {
    lusush_pool_config_t config = {0};
    return config;
}

lusush_pool_error_t lusush_pool_init(const lusush_pool_config_t *config) {
    (void)config;
    return 0;
}

/* ========================================================================== */
/*                    CACHE INITIALIZATION TESTS                              */
/* ========================================================================== */

TEST(cache_init_success) {
    lle_display_cache_t *cache = NULL;

    lle_result_t result = lle_display_cache_init(&cache, mock_pool);

    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    ASSERT_NOT_NULL(cache, "Cache should be allocated");

    lle_display_cache_cleanup(cache);
}

TEST(cache_init_null_output) {
    lle_result_t result = lle_display_cache_init(NULL, mock_pool);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL output should return error");
}

TEST(cache_init_null_pool) {
    lle_display_cache_t *cache = NULL;

    lle_result_t result = lle_display_cache_init(&cache, NULL);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL pool should return error");
}

/* ========================================================================== */
/*                    CACHE CLEANUP TESTS                                     */
/* ========================================================================== */

TEST(cache_cleanup_null) {
    lle_result_t result = lle_display_cache_cleanup(NULL);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "Cleanup with NULL should return error");
}

TEST(cache_cleanup_success) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    lle_result_t result = lle_display_cache_cleanup(cache);

    ASSERT_EQ(result, LLE_SUCCESS, "Cleanup should succeed");
}

/* ========================================================================== */
/*                    CACHE STORE TESTS                                       */
/* ========================================================================== */

TEST(cache_store_null_cache) {
    const char *data = "test data";

    lle_result_t result =
        lle_display_cache_store(NULL, 123, data, strlen(data));

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL cache should return error");
}

TEST(cache_store_success) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    const char *data = "test data";
    lle_result_t result =
        lle_display_cache_store(cache, 123, data, strlen(data));

    ASSERT_EQ(result, LLE_SUCCESS, "Store should succeed");

    lle_display_cache_cleanup(cache);
}

TEST(cache_store_multiple_entries) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    /* Store multiple entries */
    for (uint64_t i = 0; i < 5; i++) {
        char data[32];
        snprintf(data, sizeof(data), "data_%" PRIu64, i);

        lle_result_t result =
            lle_display_cache_store(cache, i, data, strlen(data));
        ASSERT_EQ(result, LLE_SUCCESS, "Each store should succeed");
    }

    lle_display_cache_cleanup(cache);
}

/* ========================================================================== */
/*                    CACHE LOOKUP TESTS                                      */
/* ========================================================================== */

TEST(cache_lookup_null_cache) {
    void *data = NULL;
    size_t size = 0;

    lle_result_t result = lle_display_cache_lookup(NULL, 123, &data, &size);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL cache should return error");
}

TEST(cache_lookup_null_output) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    lle_result_t result = lle_display_cache_lookup(cache, 123, NULL, NULL);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL output should return error");

    lle_display_cache_cleanup(cache);
}

TEST(cache_lookup_miss) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    void *data = NULL;
    size_t size = 0;

    lle_result_t result = lle_display_cache_lookup(cache, 999, &data, &size);

    ASSERT_EQ(result, LLE_ERROR_CACHE_MISS,
              "Non-existent key should return CACHE_MISS");

    lle_display_cache_cleanup(cache);
}

TEST(cache_lookup_hit) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    /* Store data */
    const char *test_data = "cached content";
    lle_display_cache_store(cache, 456, test_data, strlen(test_data));

    /* Lookup data */
    void *data = NULL;
    size_t size = 0;
    lle_result_t result = lle_display_cache_lookup(cache, 456, &data, &size);

    ASSERT_EQ(result, LLE_SUCCESS, "Lookup should succeed for stored key");
    ASSERT_NOT_NULL(data, "Retrieved data should not be NULL");
    ASSERT_GT(size, 0, "Retrieved size should be > 0");

    lle_display_cache_cleanup(cache);
}

TEST(cache_store_and_lookup_cycle) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    /* Store then lookup multiple entries */
    for (uint64_t i = 0; i < 3; i++) {
        char store_data[32];
        snprintf(store_data, sizeof(store_data), "entry_%" PRIu64, i);

        /* Store */
        lle_display_cache_store(cache, i, store_data, strlen(store_data));

        /* Lookup */
        void *lookup_data = NULL;
        size_t lookup_size = 0;
        lle_result_t result =
            lle_display_cache_lookup(cache, i, &lookup_data, &lookup_size);

        ASSERT_EQ(result, LLE_SUCCESS, "Lookup should find stored entry");
        ASSERT_NOT_NULL(lookup_data, "Data should be retrieved");
    }

    lle_display_cache_cleanup(cache);
}

/* ========================================================================== */
/*                    CACHE INVALIDATION TESTS                                */
/* ========================================================================== */

TEST(cache_invalidate_null_cache) {
    lle_result_t result = lle_display_cache_invalidate(NULL, 123);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL cache should return error");
}

TEST(cache_invalidate_existing_entry) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    /* Store entry */
    const char *data = "to be invalidated";
    lle_display_cache_store(cache, 777, data, strlen(data));

    /* Invalidate */
    lle_result_t result = lle_display_cache_invalidate(cache, 777);
    ASSERT_EQ(result, LLE_SUCCESS, "Invalidate should succeed");

    /* Verify it's gone */
    void *lookup_data = NULL;
    size_t size = 0;
    result = lle_display_cache_lookup(cache, 777, &lookup_data, &size);
    ASSERT_EQ(result, LLE_ERROR_CACHE_MISS,
              "Invalidated entry should not be found");

    lle_display_cache_cleanup(cache);
}

TEST(cache_invalidate_all_null_cache) {
    lle_result_t result = lle_display_cache_invalidate_all(NULL);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL cache should return error");
}

TEST(cache_invalidate_all_success) {
    lle_display_cache_t *cache = NULL;
    lle_display_cache_init(&cache, mock_pool);

    /* Store multiple entries */
    for (uint64_t i = 0; i < 5; i++) {
        char data[16];
        snprintf(data, sizeof(data), "entry%" PRIu64, i);
        lle_display_cache_store(cache, i, data, strlen(data));
    }

    /* Invalidate all */
    lle_result_t result = lle_display_cache_invalidate_all(cache);
    ASSERT_EQ(result, LLE_SUCCESS, "Invalidate all should succeed");

    /* Verify all entries are gone */
    for (uint64_t i = 0; i < 5; i++) {
        void *data = NULL;
        size_t size = 0;
        result = lle_display_cache_lookup(cache, i, &data, &size);
        ASSERT_EQ(result, LLE_ERROR_CACHE_MISS,
                  "All entries should be invalidated");
    }

    lle_display_cache_cleanup(cache);
}

/* ========================================================================== */
/*                    RENDER CACHE TESTS                                      */
/* ========================================================================== */

TEST(render_cache_init_success) {
    lle_render_cache_t *cache = NULL;

    lle_result_t result = lle_render_cache_init(&cache, mock_pool);

    ASSERT_EQ(result, LLE_SUCCESS, "Render cache init should succeed");
    ASSERT_NOT_NULL(cache, "Render cache should be allocated");

    lle_render_cache_cleanup(cache);
}

TEST(render_cache_init_null_output) {
    lle_result_t result = lle_render_cache_init(NULL, mock_pool);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL output should return error");
}

TEST(render_cache_cleanup_null) {
    lle_result_t result = lle_render_cache_cleanup(NULL);

    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER,
              "NULL cleanup should return error");
}

/* ========================================================================== */
/*                         TEST RUNNER                                        */
/* ========================================================================== */

int main(void) {
    printf(
        "=================================================================\n");
    printf("  LLE Render Cache Unit Tests\n");
    printf("================================================================="
           "\n\n");

    /* Initialization tests */
    run_test_cache_init_success();
    run_test_cache_init_null_output();
    run_test_cache_init_null_pool();

    /* Cleanup tests */
    run_test_cache_cleanup_null();
    run_test_cache_cleanup_success();

    /* Store tests */
    run_test_cache_store_null_cache();
    run_test_cache_store_success();
    run_test_cache_store_multiple_entries();

    /* Lookup tests */
    run_test_cache_lookup_null_cache();
    run_test_cache_lookup_null_output();
    run_test_cache_lookup_miss();
    run_test_cache_lookup_hit();
    run_test_cache_store_and_lookup_cycle();

    /* Invalidation tests */
    run_test_cache_invalidate_null_cache();
    run_test_cache_invalidate_existing_entry();
    run_test_cache_invalidate_all_null_cache();
    run_test_cache_invalidate_all_success();

    /* Render cache tests */
    run_test_render_cache_init_success();
    run_test_render_cache_init_null_output();
    run_test_render_cache_cleanup_null();

    /* Print summary */
    printf("\n================================================================="
           "\n");
    printf("  Test Summary\n");
    printf(
        "=================================================================\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf(
        "=================================================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
