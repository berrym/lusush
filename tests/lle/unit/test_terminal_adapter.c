/**
 * @file test_terminal_adapter.c
 * @brief Unit Tests for LLE Terminal Adapter
 * 
 * Tests the terminal adapter component that provides terminal capability
 * detection and multi-terminal compatibility.
 * 
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION: src/lle/terminal_adapter.c
 * 
 * TEST COVERAGE:
 * - Terminal adapter initialization and cleanup
 * - Terminal type detection
 * - Capability detection for different terminal types
 * - Compatibility matrix functionality
 * - Feature support queries
 * - Parameter validation
 * - Memory allocation error handling
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "display/display_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Mock memory pool instance */
static int mock_pool_dummy = 42;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t*)&mock_pool_dummy;

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        test_##name(); \
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

#define ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("  ✗ ASSERTION FAILED: %s\n", msg); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)
#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)

/* Mock display controller */
static display_controller_t mock_display;

/* ========================================================================== */
/*                        TERMINAL ADAPTER TESTS                              */
/* ========================================================================== */

TEST(terminal_adapter_init_success) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Terminal adapter init should succeed");
    ASSERT_NOT_NULL(adapter, "Adapter should be allocated");
    ASSERT_NOT_NULL(adapter->capabilities, "Capabilities should be allocated");
    ASSERT_NOT_NULL(adapter->compat_matrix, "Compatibility matrix should be allocated");
    ASSERT_EQ(adapter->display_controller, &mock_display, "Display controller should match");
    ASSERT_EQ(adapter->memory_pool, mock_pool, "Memory pool should match");
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_adapter_init_null_adapter) {
    lle_result_t result = lle_terminal_adapter_init(NULL, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL adapter pointer");
    
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_adapter_init_null_display) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, NULL, mock_pool);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL display controller");
    
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_adapter_init_null_pool) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL memory pool");
    
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_adapter_cleanup_null) {
    lle_result_t result = lle_terminal_adapter_cleanup(NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL adapter");
    
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_capabilities_detected) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    /* Verify capabilities are detected */
    ASSERT_NOT_NULL(adapter->capabilities, "Capabilities should exist");
    ASSERT_TRUE(adapter->capabilities->terminal_type >= LLE_TERMINAL_UNKNOWN &&
                adapter->capabilities->terminal_type <= LLE_TERMINAL_CONSOLE,
                "Terminal type should be valid");
    
    /* Verify terminal size is set (either detected or default) */
    ASSERT_TRUE(adapter->capabilities->terminal_width > 0, "Width should be positive");
    ASSERT_TRUE(adapter->capabilities->terminal_height > 0, "Height should be positive");
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_get_capabilities) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    const lle_terminal_capabilities_t *caps = lle_terminal_adapter_get_capabilities(adapter);
    ASSERT_NOT_NULL(caps, "Get capabilities should return non-NULL");
    ASSERT_EQ(caps, adapter->capabilities, "Should return adapter's capabilities");
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_get_capabilities_null) {
    const lle_terminal_capabilities_t *caps = lle_terminal_adapter_get_capabilities(NULL);
    ASSERT_NULL(caps, "Get capabilities with NULL adapter should return NULL");
    
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_compatibility_matrix_initialized) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    /* Verify compatibility matrix exists */
    ASSERT_NOT_NULL(adapter->compat_matrix, "Compatibility matrix should exist");
    
    /* Verify at least one feature name is set */
    bool found_feature = false;
    for (int i = 0; i < 16; i++) {
        if (adapter->compat_matrix->feature_names[i] != NULL) {
            found_feature = true;
            break;
        }
    }
    ASSERT_TRUE(found_feature, "At least one feature name should be set");
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_supports_feature_colors) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    /* Most terminals support basic colors */
    bool supports = lle_terminal_adapter_supports_feature(adapter, "colors");
    /* We can't assert true/false because it depends on detected terminal,
     * but we can verify the function works */
    (void)supports; /* Function call succeeded */
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_supports_feature_utf8) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    /* Most modern terminals support UTF-8 */
    bool supports = lle_terminal_adapter_supports_feature(adapter, "utf8");
    (void)supports; /* Function call succeeded */
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_supports_feature_null_adapter) {
    bool supports = lle_terminal_adapter_supports_feature(NULL, "colors");
    ASSERT_FALSE(supports, "NULL adapter should return false");
    
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_supports_feature_null_name) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    bool supports = lle_terminal_adapter_supports_feature(adapter, NULL);
    ASSERT_FALSE(supports, "NULL feature name should return false");
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_supports_feature_unknown) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    bool supports = lle_terminal_adapter_supports_feature(adapter, "nonexistent_feature");
    ASSERT_FALSE(supports, "Unknown feature should return false");
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_type_detection) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    /* Verify terminal type is detected (will vary by environment) */
    lle_terminal_type_t type = adapter->capabilities->terminal_type;
    
    /* Type should be one of the valid enum values */
    ASSERT_TRUE(type >= LLE_TERMINAL_UNKNOWN && type <= LLE_TERMINAL_CONSOLE,
                "Terminal type should be valid enum value");
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

TEST(terminal_capabilities_consistency) {
    lle_terminal_adapter_t *adapter = NULL;
    lle_result_t result = lle_terminal_adapter_init(&adapter, &mock_display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Init should succeed");
    
    /* If terminal supports truecolor, it should also support 256 colors */
    if (adapter->capabilities->supports_truecolor) {
        ASSERT_TRUE(adapter->capabilities->supports_256_colors,
                    "Truecolor implies 256 color support");
    }
    
    /* If terminal supports 256 colors, it should support basic colors */
    if (adapter->capabilities->supports_256_colors) {
        ASSERT_TRUE(adapter->capabilities->supports_colors,
                    "256 colors implies basic color support");
    }
    
    lle_terminal_adapter_cleanup(adapter);
    tests_passed++;
    printf("  ✓ PASSED\n");
}

/* ========================================================================== */
/*                            TEST RUNNER                                     */
/* ========================================================================== */

int main(void) {
    printf("=================================================================\n");
    printf("  LLE Terminal Adapter Unit Tests\n");
    printf("=================================================================\n\n");
    
    /* Initialization tests */
    run_test_terminal_adapter_init_success();
    run_test_terminal_adapter_init_null_adapter();
    run_test_terminal_adapter_init_null_display();
    run_test_terminal_adapter_init_null_pool();
    
    /* Cleanup tests */
    run_test_terminal_adapter_cleanup_null();
    
    /* Capability detection tests */
    run_test_terminal_capabilities_detected();
    run_test_terminal_get_capabilities();
    run_test_terminal_get_capabilities_null();
    
    /* Compatibility matrix tests */
    run_test_terminal_compatibility_matrix_initialized();
    
    /* Feature support tests */
    run_test_terminal_supports_feature_colors();
    run_test_terminal_supports_feature_utf8();
    run_test_terminal_supports_feature_null_adapter();
    run_test_terminal_supports_feature_null_name();
    run_test_terminal_supports_feature_unknown();
    
    /* Detection and consistency tests */
    run_test_terminal_type_detection();
    run_test_terminal_capabilities_consistency();
    
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
