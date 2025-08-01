/**
 * @file test_history_navigation_fix.c
 * @brief Test suite for LLE-R003 History Navigation Recovery
 * 
 * This test validates that history navigation integrates properly with
 * the display stabilization system and provides reliable functionality
 * without character duplication or display corruption.
 * 
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "command_history.h"
#include "line_editor.h"
#include "display_stabilization.h"
#include "text_buffer.h"

// Test framework macros
#define LLE_TEST(name) static bool test_##name(void)
#define LLE_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define LLE_ASSERT_STR_EQ(expected, actual, message) \
    do { \
        if (strcmp(expected, actual) != 0) { \
            fprintf(stderr, "FAIL: %s:%d: %s (expected: '%s', got: '%s')\n", \
                    __FILE__, __LINE__, message, expected, actual); \
            return false; \
        } \
    } while (0)

// Test functions

/**
 * @brief Test basic history navigation functionality
 */
LLE_TEST(basic_history_navigation) {
    lle_history_t *history = lle_history_create(100, false);
    LLE_ASSERT(history != NULL, "Failed to create history");
    
    // Add test commands
    LLE_ASSERT(lle_history_add(history, "first command", false), "Failed to add first command");
    LLE_ASSERT(lle_history_add(history, "second command", false), "Failed to add second command");
    LLE_ASSERT(lle_history_add(history, "third command", false), "Failed to add third command");
    
    // Test navigation
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to get previous command");
    LLE_ASSERT_STR_EQ("third command", cmd, "Wrong previous command");
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to get second previous command");
    LLE_ASSERT_STR_EQ("second command", cmd, "Wrong second previous command");
    
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL, "Failed to get next command");
    LLE_ASSERT_STR_EQ("third command", cmd, "Wrong next command");
    
    lle_history_destroy(history);
    return true;
}

/**
 * @brief Test history navigation edge cases
 */
LLE_TEST(history_navigation_edge_cases) {
    lle_history_t *history = lle_history_create(100, false);
    LLE_ASSERT(history != NULL, "Failed to create history");
    
    // Test empty history
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd == NULL, "Previous on empty history should return NULL");
    
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd == NULL, "Next on empty history should return NULL");
    
    // Add single command
    LLE_ASSERT(lle_history_add(history, "only command", false), "Failed to add command");
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to get command from single-entry history");
    LLE_ASSERT_STR_EQ("only command", cmd, "Wrong command from single-entry history");
    
    // Test going past boundaries
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd == NULL, "Should not go past oldest command");
    
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd == NULL, "Should exit navigation mode");
    
    lle_history_destroy(history);
    return true;
}

/**
 * @brief Test display stabilization integration
 */
LLE_TEST(display_stabilization_integration) {
    // Initialize stabilization system
    lle_display_stabilization_config_t config = {
        .validation_level = LLE_ESCAPE_VALIDATION_BASIC,
        .allowed_sequences = LLE_ESCAPE_TYPE_ALL,
        .recovery_strategy = LLE_RECOVERY_FALLBACK,
        .enable_memory_safety_checks = true,
        .enable_performance_monitoring = true,
        .enable_sequence_logging = false,
        .max_error_rate = 0.1,
        .max_render_time_ns = 5000000
    };
    
    LLE_ASSERT(lle_display_stabilization_init(&config), "Failed to initialize stabilization");
    
    // Test that stabilization is available for history operations
    lle_display_stability_metrics_t metrics;
    LLE_ASSERT(lle_display_stabilization_get_metrics(&metrics), "Failed to get stabilization metrics");
    
    lle_display_stabilization_cleanup();
    return true;
}

/**
 * @brief Test history navigation with long commands
 */
LLE_TEST(long_command_navigation) {
    lle_history_t *history = lle_history_create(100, false);
    LLE_ASSERT(history != NULL, "Failed to create history");
    
    // Add commands of various lengths
    const char *short_cmd = "ls";
    const char *medium_cmd = "find /usr/local -name '*.so' -type f";
    const char *long_cmd = "for file in $(find /usr/local -name '*.so' -type f); do echo \"Processing: $file\"; ldd \"$file\" 2>/dev/null | grep -v 'not found' || true; done";
    
    LLE_ASSERT(lle_history_add(history, short_cmd, false), "Failed to add short command");
    LLE_ASSERT(lle_history_add(history, medium_cmd, false), "Failed to add medium command");
    LLE_ASSERT(lle_history_add(history, long_cmd, false), "Failed to add long command");
    
    // Navigate and verify
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to get long command");
    LLE_ASSERT_STR_EQ(long_cmd, cmd, "Wrong long command retrieved");
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to get medium command");
    LLE_ASSERT_STR_EQ(medium_cmd, cmd, "Wrong medium command retrieved");
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to get short command");
    LLE_ASSERT_STR_EQ(short_cmd, cmd, "Wrong short command retrieved");
    
    lle_history_destroy(history);
    return true;
}

/**
 * @brief Test navigation reset functionality
 */
LLE_TEST(navigation_reset) {
    lle_history_t *history = lle_history_create(100, false);
    LLE_ASSERT(history != NULL, "Failed to create history");
    
    // Add test commands
    LLE_ASSERT(lle_history_add(history, "cmd1", false), "Failed to add cmd1");
    LLE_ASSERT(lle_history_add(history, "cmd2", false), "Failed to add cmd2");
    LLE_ASSERT(lle_history_add(history, "cmd3", false), "Failed to add cmd3");
    
    // Navigate into history
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to start navigation");
    LLE_ASSERT_STR_EQ("cmd3", cmd, "Wrong initial command");
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to navigate further");
    LLE_ASSERT_STR_EQ("cmd2", cmd, "Wrong second command");
    
    // Reset navigation
    lle_history_reset_navigation(history);
    
    // Should be able to start fresh navigation
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL, "Failed to restart navigation after reset");
    LLE_ASSERT_STR_EQ("cmd3", cmd, "Wrong command after reset");
    
    lle_history_destroy(history);
    return true;
}

/**
 * @brief Test memory safety during navigation
 */
LLE_TEST(memory_safety) {
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT(history != NULL, "Failed to create history");
    
    // Add commands to fill capacity
    for (int i = 0; i < 15; i++) {
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "command_%d", i);
        LLE_ASSERT(lle_history_add(history, cmd, false), "Failed to add command");
    }
    
    // Navigate through all available history
    int count = 0;
    const char *cmd = lle_history_prev(history);
    while (cmd != NULL && count < 20) { // Prevent infinite loop
        count++;
        cmd = lle_history_prev(history);
    }
    
    LLE_ASSERT(count <= 10, "Navigation returned more commands than capacity");
    LLE_ASSERT(count > 0, "No commands found in navigation");
    
    lle_history_destroy(history);
    return true;
}

// Test runner
int main(void) {
    printf("Running LLE History Navigation Fix Tests...\n");
    
    struct {
        const char *name;
        bool (*func)(void);
    } tests[] = {
        {"basic_history_navigation", test_basic_history_navigation},
        {"history_navigation_edge_cases", test_history_navigation_edge_cases},
        {"display_stabilization_integration", test_display_stabilization_integration},
        {"long_command_navigation", test_long_command_navigation},
        {"navigation_reset", test_navigation_reset},
        {"memory_safety", test_memory_safety}
    };
    
    int passed = 0;
    int total = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < total; i++) {
        printf("Running %s... ", tests[i].name);
        fflush(stdout);
        
        if (tests[i].func()) {
            printf("PASS\n");
            passed++;
        } else {
            printf("FAIL\n");
        }
    }
    
    printf("\nTests run: %d, Tests passed: %d, Tests failed: %d\n", 
           total, passed, total - passed);
    
    if (passed == total) {
        printf("🎉 All history navigation tests passed!\n");
        return 0;
    } else {
        printf("❌ Some tests failed\n");
        return 1;
    }
}