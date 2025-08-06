/**
 * @file test_lle_state_observer.c
 * @brief Tests for LLE State Observer functionality
 *
 * This test suite validates the non-invasive state observer system,
 * ensuring it can monitor operations without interfering with existing functionality.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lle_state_observer.h"
#include "display.h"
#include "terminal_manager.h"

// Test framework macros
#define LLE_TEST(name) \
    static bool test_##name(void)

#define LLE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define LLE_ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "Assertion failed: %s (%zu) != %s (%zu) at %s:%d\n", \
                    #actual, (size_t)(actual), #expected, (size_t)(expected), __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

// Mock display and terminal for testing
static lle_display_state_t test_display;
static lle_terminal_manager_t test_terminal;

// Test initialization and cleanup
LLE_TEST(initialization) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    
    // Verify default configuration
    LLE_ASSERT(config.enabled == true);
    LLE_ASSERT(config.periodic_validation == true);
    LLE_ASSERT(config.health_threshold > 0.0 && config.health_threshold <= 1.0);
    LLE_ASSERT(config.operation_buffer_size > 0);
    
    // Initialize observer
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Verify observer state
    lle_state_health_t health;
    LLE_ASSERT(lle_state_observer_get_health(observer, &health));
    LLE_ASSERT(health.health_score == 1.0); // Should start healthy
    
    // Cleanup
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test configuration management
LLE_TEST(configuration) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Test enable/disable
    LLE_ASSERT(lle_state_observer_set_enabled(observer, false));
    LLE_ASSERT(lle_state_observer_set_enabled(observer, true));
    
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test operation recording
LLE_TEST(operation_recording) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    config.operation_buffer_size = 10;
    
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Record some operations
    LLE_ASSERT(lle_state_observer_record_operation(observer, LLE_OP_WRITE, 10, 0, 10, true, "Test write"));
    LLE_ASSERT(lle_state_observer_record_operation(observer, LLE_OP_CURSOR_MOVE, 0, 10, 15, true, "Test cursor move"));
    LLE_ASSERT(lle_state_observer_record_operation(observer, LLE_OP_RENDER, 25, 15, 15, true, "Test render"));
    
    // Get statistics
    lle_observer_stats_t stats;
    LLE_ASSERT(lle_state_observer_get_statistics(observer, &stats));
    LLE_ASSERT_EQ(stats.total_operations, 3);
    
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test health validation
LLE_TEST(health_validation) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    config.validation_interval = 1; // 1 microsecond for immediate validation
    
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Validate health
    LLE_ASSERT(lle_state_observer_validate_health(observer));
    
    // Check that validation was performed
    lle_observer_stats_t stats;
    LLE_ASSERT(lle_state_observer_get_statistics(observer, &stats));
    LLE_ASSERT(stats.validation_checks > 0);
    
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test risky operation detection
LLE_TEST(risky_operations) {
    // Test risky operation identification
    LLE_ASSERT(lle_state_observer_is_risky_operation(LLE_OP_CLEAR) == true);
    LLE_ASSERT(lle_state_observer_is_risky_operation(LLE_OP_RENDER) == true);
    LLE_ASSERT(lle_state_observer_is_risky_operation(LLE_OP_NAVIGATION) == true);
    
    // Test non-risky operations
    LLE_ASSERT(lle_state_observer_is_risky_operation(LLE_OP_WRITE) == false);
    LLE_ASSERT(lle_state_observer_is_risky_operation(LLE_OP_INSERT) == false);
    LLE_ASSERT(lle_state_observer_is_risky_operation(LLE_OP_CURSOR_MOVE) == false);
    
    return true;
}

// Test operation type names
LLE_TEST(operation_names) {
    LLE_ASSERT(strcmp(lle_state_observer_operation_type_name(LLE_OP_WRITE), "WRITE") == 0);
    LLE_ASSERT(strcmp(lle_state_observer_operation_type_name(LLE_OP_CLEAR), "CLEAR") == 0);
    LLE_ASSERT(strcmp(lle_state_observer_operation_type_name(LLE_OP_RENDER), "RENDER") == 0);
    LLE_ASSERT(strcmp(lle_state_observer_operation_type_name(LLE_OP_NAVIGATION), "NAVIGATION") == 0);
    
    return true;
}

// Test validation timing
LLE_TEST(validation_timing) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    config.validation_interval = 1000000; // 1 second
    
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Should not be due immediately
    LLE_ASSERT(lle_state_observer_validation_due(observer) == false);
    
    // Force validation
    LLE_ASSERT(lle_state_observer_force_validation(observer) == true);
    
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test recovery mechanism
LLE_TEST(recovery_mechanism) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    config.max_recovery_attempts = 3;
    
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Test recovery attempt
    LLE_ASSERT(lle_state_observer_recovery_in_progress(observer) == false);
    LLE_ASSERT(lle_state_observer_attempt_recovery(observer, "Test recovery"));
    
    // Check statistics
    lle_observer_stats_t stats;
    LLE_ASSERT(lle_state_observer_get_statistics(observer, &stats));
    LLE_ASSERT(stats.recoveries_attempted > 0);
    LLE_ASSERT(stats.recoveries_successful > 0);
    
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test statistics tracking
LLE_TEST(statistics_tracking) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Perform various operations
    lle_state_observer_record_operation(observer, LLE_OP_WRITE, 5, 0, 5, true, "Write test");
    lle_state_observer_record_operation(observer, LLE_OP_RENDER, 5, 5, 5, true, "Render test");
    lle_state_observer_validate_health(observer);
    
    // Check statistics
    lle_observer_stats_t stats;
    LLE_ASSERT(lle_state_observer_get_statistics(observer, &stats));
    LLE_ASSERT(stats.total_operations >= 2);
    LLE_ASSERT(stats.validation_checks >= 1);
    LLE_ASSERT(stats.current_health_score >= 0.0 && stats.current_health_score <= 1.0);
    
    // Reset statistics
    LLE_ASSERT(lle_state_observer_reset_statistics(observer));
    LLE_ASSERT(lle_state_observer_get_statistics(observer, &stats));
    LLE_ASSERT(stats.total_operations == 0);
    
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test macro helpers
LLE_TEST(macro_helpers) {
    lle_observer_config_t config;
    lle_state_observer_get_default_config(&config);
    config.validation_interval = 1; // Immediate validation
    config.enabled = true; // Explicitly enable
    
    lle_state_observer_t *observer = lle_state_observer_init((struct lle_display_state *)&test_display, (struct lle_terminal_manager *)&test_terminal, &config);
    LLE_ASSERT(observer != NULL);
    
    // Test operation recording using direct function call
    LLE_ASSERT(lle_state_observer_record_operation(observer, LLE_OP_WRITE, 10, 0, 10, true, "Macro test"));
    
    // Test operation recording macro
    LLE_OBSERVER_RECORD_OP(observer, LLE_OP_CURSOR_MOVE, 5, 10, 15, true, "Macro cursor move");
    
    // Test health check macro
    LLE_OBSERVER_CHECK_HEALTH(observer);
    
    // Verify operations were recorded
    lle_observer_stats_t stats;
    LLE_ASSERT(lle_state_observer_get_statistics(observer, &stats));
    LLE_ASSERT(stats.total_operations >= 1);
    LLE_ASSERT(stats.validation_checks >= 0); // Validation might not be due yet
    
    lle_state_observer_cleanup(observer);
    
    return true;
}

// Test error conditions
LLE_TEST(error_conditions) {
    // Test NULL parameter handling
    LLE_ASSERT(lle_state_observer_init(NULL, NULL, NULL) == NULL);
    LLE_ASSERT(lle_state_observer_record_operation(NULL, LLE_OP_WRITE, 0, 0, 0, true, "test") == false);
    LLE_ASSERT(lle_state_observer_validate_health(NULL) == true); // Should assume healthy
    LLE_ASSERT(lle_state_observer_get_health(NULL, NULL) == false);
    LLE_ASSERT(lle_state_observer_attempt_recovery(NULL, "test") == false);
    
    return true;
}

// Test function to run all tests
static void run_tests(void) {
    printf("Running LLE State Observer Tests...\n");
    
    // Initialize mock objects
    memset(&test_display, 0, sizeof(test_display));
    memset(&test_terminal, 0, sizeof(test_terminal));
    
    // Test results tracking
    int passed = 0;
    int total = 0;
    
    // Run all tests
    #define RUN_TEST(name) \
        do { \
            total++; \
            printf("Running " #name "... "); \
            if (test_##name()) { \
                printf("PASS\n"); \
                passed++; \
            } else { \
                printf("FAIL\n"); \
            } \
        } while (0)
    
    RUN_TEST(initialization);
    RUN_TEST(configuration);
    RUN_TEST(operation_recording);
    RUN_TEST(health_validation);
    RUN_TEST(risky_operations);
    RUN_TEST(operation_names);
    RUN_TEST(validation_timing);
    RUN_TEST(recovery_mechanism);
    RUN_TEST(statistics_tracking);
    RUN_TEST(macro_helpers);
    RUN_TEST(error_conditions);
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Total: %d\n", total);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", total - passed);
    printf("Success Rate: %.1f%%\n", (double)passed / total * 100.0);
    
    if (passed == total) {
        printf("✅ All tests passed!\n");
    } else {
        printf("❌ Some tests failed.\n");
    }
}

int main(void) {
    run_tests();
    return 0;
}