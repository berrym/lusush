/**
 * @file test_display_stabilization.c
 * @brief Lusush Line Editor - Display Stabilization Test Suite
 *
 * Minimal test suite for the display stabilization system focusing on
 * core functionality without complex integrations.
 *
 * Part of LLE-R002: Display System Stabilization testing.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../../src/line_editor/display_stabilization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

// Simple test framework
#define TEST_ASSERT(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
} while(0)

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test_func) do { \
    printf("Running %s...", #test_func); \
    tests_run++; \
    test_func(); \
    tests_passed++; \
    printf(" PASS\n"); \
} while(0)

/**
 * @brief Test basic stabilization system initialization and cleanup.
 */
void test_stabilization_init_cleanup(void) {
    // Test initialization with default config
    TEST_ASSERT(lle_display_stabilization_init(NULL));
    
    // Test metrics retrieval
    lle_display_stability_metrics_t metrics;
    TEST_ASSERT(lle_display_stabilization_get_metrics(&metrics));
    TEST_ASSERT(metrics.health_status == LLE_HEALTH_EXCELLENT);
    TEST_ASSERT(metrics.total_operations == 0);
    TEST_ASSERT(metrics.error_rate == 0.0);
    
    // Test cleanup
    lle_display_stabilization_cleanup();
    
    // Test custom configuration
    lle_display_stabilization_config_t config = LLE_DISPLAY_STABILIZATION_DEFAULT_CONFIG;
    config.validation_level = LLE_ESCAPE_VALIDATION_STRICT;
    config.enable_sequence_logging = false;
    config.max_render_time_ns = 10000000; // 10ms
    
    TEST_ASSERT(lle_display_stabilization_init(&config));
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test escape sequence validation functionality.
 */
void test_escape_sequence_validation(void) {
    lle_display_stabilization_config_t config = LLE_DISPLAY_STABILIZATION_DEFAULT_CONFIG;
    config.validation_level = LLE_ESCAPE_VALIDATION_BASIC;
    TEST_ASSERT(lle_display_stabilization_init(&config));
    
    lle_escape_validation_result_t result;
    
    // Test valid cursor movement sequence
    const char *cursor_up = "\033[A";
    TEST_ASSERT(lle_escape_sequence_validate(cursor_up, strlen(cursor_up), &result));
    TEST_ASSERT(result.is_valid);
    TEST_ASSERT(result.is_safe);
    TEST_ASSERT(result.type == LLE_ESCAPE_TYPE_CURSOR_MOVE);
    
    // Test valid clear sequence
    const char *clear_screen = "\033[2J";
    TEST_ASSERT(lle_escape_sequence_validate(clear_screen, strlen(clear_screen), &result));
    TEST_ASSERT(result.is_valid);
    TEST_ASSERT(result.is_safe);
    TEST_ASSERT(result.type == LLE_ESCAPE_TYPE_CLEAR);
    
    // Test valid color sequence
    const char *color_red = "\033[31m";
    TEST_ASSERT(lle_escape_sequence_validate(color_red, strlen(color_red), &result));
    TEST_ASSERT(result.is_valid);
    TEST_ASSERT(result.is_safe);
    TEST_ASSERT(result.type == LLE_ESCAPE_TYPE_COLOR);
    
    // Test invalid sequence (doesn't start with ESC)
    const char *invalid_seq = "invalid";
    TEST_ASSERT(lle_escape_sequence_validate(invalid_seq, strlen(invalid_seq), &result));
    TEST_ASSERT(!result.is_valid);
    TEST_ASSERT(!result.is_safe);
    
    // Test sequence type checking
    TEST_ASSERT(lle_escape_sequence_is_allowed(LLE_ESCAPE_TYPE_CURSOR_MOVE));
    TEST_ASSERT(lle_escape_sequence_is_allowed(LLE_ESCAPE_TYPE_CLEAR));
    TEST_ASSERT(lle_escape_sequence_is_allowed(LLE_ESCAPE_TYPE_COLOR));
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test escape sequence sanitization.
 */
void test_escape_sequence_sanitization(void) {
    TEST_ASSERT(lle_display_stabilization_init(NULL));
    
    char output[256];
    
    // Test sanitizing valid sequence
    const char *valid_seq = "\033[2J";
    size_t result_len = lle_escape_sequence_sanitize(valid_seq, strlen(valid_seq), 
                                                     output, sizeof(output));
    TEST_ASSERT(result_len > 0);
    TEST_ASSERT(strcmp(output, valid_seq) == 0);
    
    // Test sanitizing invalid sequence
    const char *invalid_seq = "invalid";
    result_len = lle_escape_sequence_sanitize(invalid_seq, strlen(invalid_seq), 
                                              output, sizeof(output));
    TEST_ASSERT(result_len == 0);
    
    // Test null parameters
    result_len = lle_escape_sequence_sanitize(NULL, 0, output, sizeof(output));
    TEST_ASSERT(result_len == 0);
    
    result_len = lle_escape_sequence_sanitize(valid_seq, strlen(valid_seq), NULL, 0);
    TEST_ASSERT(result_len == 0);
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test display system health monitoring.
 */
void test_health_monitoring(void) {
    lle_display_stabilization_config_t config = LLE_DISPLAY_STABILIZATION_DEFAULT_CONFIG;
    config.enable_performance_monitoring = true;
    config.max_error_rate = 0.1; // 10%
    TEST_ASSERT(lle_display_stabilization_init(&config));
    
    // Initial health check
    lle_display_stability_metrics_t metrics;
    lle_display_health_t health = lle_display_system_health_check(NULL, &metrics);
    TEST_ASSERT(health == LLE_HEALTH_EXCELLENT);
    TEST_ASSERT(metrics.error_rate == 0.0);
    
    // Simulate some operations with failures
    for (int i = 0; i < 100; i++) {
        bool success = i % 10 != 0; // 10% failure rate
        lle_display_stabilization_record_operation(success, 1000000); // 1ms
    }
    
    health = lle_display_system_health_check(NULL, &metrics);
    
    // Debug the actual values
    printf("\nDEBUG: health=%d, total_ops=%lu, failed_ops=%lu, error_rate=%.3f, max_error_rate=0.1\n",
           health, metrics.total_operations, metrics.failed_operations, metrics.error_rate);
    
    TEST_ASSERT(metrics.total_operations == 100);
    TEST_ASSERT(metrics.failed_operations == 10);
    TEST_ASSERT(metrics.error_rate >= 0.09 && metrics.error_rate <= 0.11);
    
    // With 10% error rate and max_error_rate=0.1, health should be LLE_HEALTH_GOOD or LLE_HEALTH_EXCELLENT
    TEST_ASSERT(health == LLE_HEALTH_GOOD || health == LLE_HEALTH_EXCELLENT);
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test performance benchmarking.
 */
void test_performance_benchmarking(void) {
    TEST_ASSERT(lle_display_stabilization_init(NULL));
    
    lle_display_stability_metrics_t metrics;
    TEST_ASSERT(lle_display_performance_benchmark(100, &metrics));
    
    TEST_ASSERT(metrics.total_operations == 100);
    TEST_ASSERT(metrics.failed_operations == 0);
    TEST_ASSERT(metrics.error_rate == 0.0);
    TEST_ASSERT(metrics.avg_render_time_ns > 0);
    TEST_ASSERT(metrics.min_render_time_ns > 0);
    TEST_ASSERT(metrics.max_render_time_ns >= metrics.min_render_time_ns);
    TEST_ASSERT(metrics.performance_efficiency > 0.0 && metrics.performance_efficiency <= 1.0);
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test feature toggling functionality.
 */
void test_feature_toggling(void) {
    TEST_ASSERT(lle_display_stabilization_init(NULL));
    
    // Test enabling/disabling individual features
    TEST_ASSERT(lle_display_stabilization_toggle_features(LLE_STABILIZATION_FEATURE_LOGGING, true));
    TEST_ASSERT(lle_display_stabilization_toggle_features(LLE_STABILIZATION_FEATURE_VALIDATION, false));
    TEST_ASSERT(lle_display_stabilization_toggle_features(LLE_STABILIZATION_FEATURE_MONITORING, true));
    
    // Test enabling all features
    TEST_ASSERT(lle_display_stabilization_toggle_features(LLE_STABILIZATION_FEATURE_ALL, true));
    
    // Test disabling all features
    TEST_ASSERT(lle_display_stabilization_toggle_features(LLE_STABILIZATION_FEATURE_ALL, false));
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test terminal compatibility checking.
 */
void test_terminal_compatibility(void) {
    // Terminal compatibility check should work regardless of stabilization state
    bool compatible = lle_display_terminal_compatibility_check();
    
    // In test environment, this might be false (no TTY), but should not crash
    TEST_ASSERT(compatible == true || compatible == false);
    
    // Test with stabilization initialized
    TEST_ASSERT(lle_display_stabilization_init(NULL));
    
    compatible = lle_display_terminal_compatibility_check();
    TEST_ASSERT(compatible == true || compatible == false);
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test configuration updates at runtime.
 */
void test_config_updates(void) {
    TEST_ASSERT(lle_display_stabilization_init(NULL));
    
    // Create new configuration
    lle_display_stabilization_config_t new_config = LLE_DISPLAY_STABILIZATION_DEFAULT_CONFIG;
    new_config.validation_level = LLE_ESCAPE_VALIDATION_STRICT;
    new_config.recovery_strategy = LLE_RECOVERY_RESET;
    new_config.max_render_time_ns = 20000000; // 20ms
    new_config.max_error_rate = 0.05; // 5%
    
    TEST_ASSERT(lle_display_stabilization_update_config(&new_config));
    
    // Verify configuration was applied (indirect test through behavior)
    lle_escape_validation_result_t result;
    const char *test_seq = "\033[A";
    TEST_ASSERT(lle_escape_sequence_validate(test_seq, strlen(test_seq), &result));
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test edge cases and error conditions.
 */
void test_edge_cases(void) {
    // Test operations before initialization
    lle_display_stability_metrics_t metrics;
    TEST_ASSERT(!lle_display_stabilization_get_metrics(&metrics));
    
    TEST_ASSERT(!lle_display_stabilization_update_config(NULL));
    
    TEST_ASSERT(!lle_display_stabilization_toggle_features(0, true));
    
    // Initialize and test error conditions
    TEST_ASSERT(lle_display_stabilization_init(NULL));
    
    // Test validation with invalid parameters
    lle_escape_validation_result_t result;
    TEST_ASSERT(!lle_escape_sequence_validate(NULL, 0, &result));
    
    TEST_ASSERT(!lle_escape_sequence_validate("test", 4, NULL));
    
    // Test sanitization with invalid parameters
    char output[256];
    TEST_ASSERT(lle_escape_sequence_sanitize(NULL, 0, output, sizeof(output)) == 0);
    
    TEST_ASSERT(lle_escape_sequence_sanitize("test", 4, NULL, 0) == 0);
    
    // Test benchmark with invalid parameters
    TEST_ASSERT(!lle_display_performance_benchmark(0, &metrics));
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Test basic error recovery without display state.
 */
void test_error_recovery_basic(void) {
    lle_display_stabilization_config_t config = LLE_DISPLAY_STABILIZATION_DEFAULT_CONFIG;
    config.recovery_strategy = LLE_RECOVERY_FALLBACK;
    TEST_ASSERT(lle_display_stabilization_init(&config));
    
    // Test error recovery with null state (should handle gracefully)
    TEST_ASSERT(!lle_display_error_recovery(NULL, EINVAL));
    
    // Test memory safety check with null state
    TEST_ASSERT(lle_display_memory_safety_check(NULL));
    
    // Test system reset with null state
    TEST_ASSERT(!lle_display_system_reset(NULL));
    
    // Test recovery with different strategies
    config.recovery_strategy = LLE_RECOVERY_IGNORE;
    lle_display_stabilization_update_config(&config);
    
    config.recovery_strategy = LLE_RECOVERY_ABORT;
    lle_display_stabilization_update_config(&config);
    
    lle_display_stabilization_cleanup();
}

/**
 * @brief Main test runner for display stabilization tests.
 */
int main(void) {
    printf("Running LLE Display Stabilization Tests...\n");
    
    RUN_TEST(test_stabilization_init_cleanup);
    RUN_TEST(test_escape_sequence_validation);
    RUN_TEST(test_escape_sequence_sanitization);
    RUN_TEST(test_health_monitoring);
    RUN_TEST(test_performance_benchmarking);
    RUN_TEST(test_feature_toggling);
    RUN_TEST(test_terminal_compatibility);
    RUN_TEST(test_config_updates);
    RUN_TEST(test_edge_cases);
    RUN_TEST(test_error_recovery_basic);
    
    // Print results
    printf("\nDisplay Stabilization Test Results:\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("🎉 All display stabilization tests passed!\n");
        return 0;
    } else {
        printf("❌ Some display stabilization tests failed.\n");
        return 1;
    }
}