/**
 * @file test_lle_024_history_structure.c
 * @brief Tests for LLE-024: History Structure
 * 
 * Tests the command history data structures and basic functionality including
 * creation, initialization, entry management, navigation, and memory handling.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "command_history.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Create a test history with sample entries
 * @param max_entries Maximum entries for history
 * @param add_entries Number of sample entries to add
 * @return Pointer to created history
 */
static lle_history_t *create_test_history(size_t max_entries, size_t add_entries) {
    lle_history_t *history = lle_history_create(max_entries, false);
    if (!history) {
        return NULL;
    }
    
    // Add sample entries
    for (size_t i = 0; i < add_entries; i++) {
        char command[64];
        snprintf(command, sizeof(command), "command_%zu", i + 1);
        if (!lle_history_add(history, command, false)) {
            lle_history_destroy(history);
            return NULL;
        }
    }
    
    return history;
}

/**
 * @brief Verify history entry content
 * @param entry History entry to verify
 * @param expected_command Expected command text
 * @return true if entry matches expected content
 */
static bool verify_history_entry(const lle_history_entry_t *entry, const char *expected_command) {
    if (!entry || !expected_command) {
        return false;
    }
    
    if (!entry->command) {
        return false;
    }
    
    if (strcmp(entry->command, expected_command) != 0) {
        return false;
    }
    
    if (entry->length != strlen(expected_command)) {
        return false;
    }
    
    if (entry->timestamp == 0) {
        return false;
    }
    
    return true;
}

// ============================================================================
// Basic Structure Tests
// ============================================================================

LLE_TEST(history_create_default) {
    printf("Testing history creation with default size... ");
    
    lle_history_t *history = lle_history_create(0, false);
    LLE_ASSERT_NOT_NULL(history);
    
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    LLE_ASSERT_EQ(lle_history_max_size(history), LLE_HISTORY_DEFAULT_MAX_ENTRIES);
    LLE_ASSERT(lle_history_is_empty(history));
    LLE_ASSERT(!lle_history_is_full(history));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_create_custom_size) {
    printf("Testing history creation with custom size... ");
    
    lle_history_t *history = lle_history_create(100, false);
    LLE_ASSERT_NOT_NULL(history);
    
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    LLE_ASSERT_EQ(lle_history_max_size(history), 100);
    LLE_ASSERT(lle_history_is_empty(history));
    LLE_ASSERT(!lle_history_is_full(history));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_create_invalid_size) {
    printf("Testing history creation with invalid sizes... ");
    
    // Too small
    lle_history_t *history1 = lle_history_create(2, false);
    LLE_ASSERT_NULL(history1);
    
    // Too large
    lle_history_t *history2 = lle_history_create(100000, false);
    LLE_ASSERT_NULL(history2);
    
    printf("PASSED\n");
}

LLE_TEST(history_init_stack) {
    printf("Testing history initialization on stack... ");
    
    lle_history_t history;
    bool result = lle_history_init(&history, 50, false);
    LLE_ASSERT(result);
    
    LLE_ASSERT_EQ(lle_history_size(&history), 0);
    LLE_ASSERT_EQ(lle_history_max_size(&history), 50);
    LLE_ASSERT(lle_history_is_empty(&history));
    
    // Clean up manually for stack allocation
    lle_history_cleanup(&history);
    
    printf("PASSED\n");
}

LLE_TEST(history_clear) {
    printf("Testing history clear functionality... ");
    
    lle_history_t *history = create_test_history(20, 5);
    LLE_ASSERT_NOT_NULL(history);
    LLE_ASSERT_EQ(lle_history_size(history), 5);
    
    bool result = lle_history_clear(history);
    LLE_ASSERT(result);
    
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    LLE_ASSERT(lle_history_is_empty(history));
    LLE_ASSERT(!lle_history_is_full(history));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Entry Management Tests
// ============================================================================

LLE_TEST(history_add_basic) {
    printf("Testing basic history entry addition... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    bool result = lle_history_add(history, "first command", false);
    LLE_ASSERT(result);
    LLE_ASSERT_EQ(lle_history_size(history), 1);
    LLE_ASSERT(!lle_history_is_empty(history));
    
    const lle_history_entry_t *entry = lle_history_get(history, 0);
    LLE_ASSERT_NOT_NULL(entry);
    LLE_ASSERT(verify_history_entry(entry, "first command"));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_add_multiple) {
    printf("Testing multiple history entry addition... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    const char *commands[] = {"ls", "cd /home", "pwd", "echo hello"};
    size_t num_commands = sizeof(commands) / sizeof(commands[0]);
    
    for (size_t i = 0; i < num_commands; i++) {
        bool result = lle_history_add(history, commands[i], false);
        LLE_ASSERT(result);
        LLE_ASSERT_EQ(lle_history_size(history), i + 1);
    }
    
    // Verify all entries
    for (size_t i = 0; i < num_commands; i++) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        LLE_ASSERT_NOT_NULL(entry);
        LLE_ASSERT(verify_history_entry(entry, commands[i]));
    }
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_add_duplicates) {
    printf("Testing duplicate entry handling... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add first command
    bool result1 = lle_history_add(history, "ls -la", false);
    LLE_ASSERT(result1);
    LLE_ASSERT_EQ(lle_history_size(history), 1);
    
    // Add same command again (should be ignored)
    bool result2 = lle_history_add(history, "ls -la", false);
    LLE_ASSERT(result2);
    LLE_ASSERT_EQ(lle_history_size(history), 1);
    
    // Force add duplicate
    bool result3 = lle_history_add(history, "ls -la", true);
    LLE_ASSERT(result3);
    LLE_ASSERT_EQ(lle_history_size(history), 2);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_add_empty_command) {
    printf("Testing empty command addition... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Empty string should be rejected
    bool result = lle_history_add(history, "", false);
    LLE_ASSERT(!result);
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    
    // NULL command should be rejected
    bool result2 = lle_history_add(history, NULL, false);
    LLE_ASSERT(!result2);
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_circular_buffer) {
    printf("Testing circular buffer behavior... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add entries up to capacity
    for (int i = 1; i <= 10; i++) {
        char cmd[16];
        snprintf(cmd, sizeof(cmd), "cmd%d", i);
        lle_history_add(history, cmd, false);
    }
    
    LLE_ASSERT_EQ(lle_history_size(history), 10);
    LLE_ASSERT(lle_history_is_full(history));
    
    // Add one more (should replace oldest)
    lle_history_add(history, "cmd11", false);
    LLE_ASSERT_EQ(lle_history_size(history), 10);
    LLE_ASSERT(lle_history_is_full(history));
    
    // Verify entries: cmd2 through cmd11 (cmd1 should be replaced)
    const lle_history_entry_t *entry0 = lle_history_get(history, 0);
    const lle_history_entry_t *entry1 = lle_history_get(history, 1);
    const lle_history_entry_t *entry9 = lle_history_get(history, 9);
    
    LLE_ASSERT(verify_history_entry(entry0, "cmd2"));
    LLE_ASSERT(verify_history_entry(entry1, "cmd3"));
    LLE_ASSERT(verify_history_entry(entry9, "cmd11"));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Navigation Tests
// ============================================================================

LLE_TEST(history_navigation_basic) {
    printf("Testing basic history navigation... ");
    
    lle_history_t *history = create_test_history(10, 3);
    LLE_ASSERT_NOT_NULL(history);
    
    // Navigate to previous (newest)
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_PREV);
    LLE_ASSERT_NOT_NULL(entry);
    LLE_ASSERT(verify_history_entry(entry, "command_3"));
    
    // Navigate to previous again
    entry = lle_history_navigate(history, LLE_HISTORY_PREV);
    LLE_ASSERT_NOT_NULL(entry);
    LLE_ASSERT(verify_history_entry(entry, "command_2"));
    
    // Navigate to next
    entry = lle_history_navigate(history, LLE_HISTORY_NEXT);
    LLE_ASSERT_NOT_NULL(entry);
    LLE_ASSERT(verify_history_entry(entry, "command_3"));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_navigation_boundaries) {
    printf("Testing navigation boundary conditions... ");
    
    lle_history_t *history = create_test_history(10, 2);
    LLE_ASSERT_NOT_NULL(history);
    
    // Navigate to first
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_FIRST);
    LLE_ASSERT_NOT_NULL(entry);
    LLE_ASSERT(verify_history_entry(entry, "command_1"));
    
    // Try to go before first (should return NULL)
    entry = lle_history_navigate(history, LLE_HISTORY_PREV);
    LLE_ASSERT_NULL(entry);
    
    // Navigate to last
    entry = lle_history_navigate(history, LLE_HISTORY_LAST);
    LLE_ASSERT_NOT_NULL(entry);
    LLE_ASSERT(verify_history_entry(entry, "command_2"));
    
    // Try to go past last (should reset navigation)
    entry = lle_history_navigate(history, LLE_HISTORY_NEXT);
    LLE_ASSERT_NULL(entry);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_navigation_reset) {
    printf("Testing navigation reset... ");
    
    lle_history_t *history = create_test_history(10, 3);
    LLE_ASSERT_NOT_NULL(history);
    
    // Start navigation
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_PREV);
    LLE_ASSERT_NOT_NULL(entry);
    
    // Current should work
    entry = lle_history_current(history);
    LLE_ASSERT_NOT_NULL(entry);
    
    // Reset navigation
    lle_history_reset_navigation(history);
    
    // Current should return NULL
    entry = lle_history_current(history);
    LLE_ASSERT_NULL(entry);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Temporary Buffer Tests
// ============================================================================

LLE_TEST(history_temp_buffer) {
    printf("Testing temporary buffer functionality... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    const char *temp_content = "partial command";
    size_t temp_length = strlen(temp_content);
    
    // Set temporary buffer
    bool result = lle_history_set_temp_buffer(history, temp_content, temp_length);
    LLE_ASSERT(result);
    
    // Get temporary buffer
    size_t retrieved_length;
    const char *retrieved = lle_history_get_temp_buffer(history, &retrieved_length);
    LLE_ASSERT_NOT_NULL(retrieved);
    LLE_ASSERT_EQ(retrieved_length, temp_length);
    LLE_ASSERT_STR_EQ(retrieved, temp_content);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_temp_buffer_clear) {
    printf("Testing temporary buffer clearing... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Set temporary buffer
    lle_history_set_temp_buffer(history, "test", 4);
    
    // Clear history (should clear temp buffer)
    lle_history_clear(history);
    
    // Temp buffer should be gone
    const char *retrieved = lle_history_get_temp_buffer(history, NULL);
    LLE_ASSERT_NULL(retrieved);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Configuration Tests
// ============================================================================

LLE_TEST(history_max_size_change) {
    printf("Testing maximum size changes... ");
    
    lle_history_t *history = create_test_history(20, 15);
    LLE_ASSERT_NOT_NULL(history);
    LLE_ASSERT_EQ(lle_history_size(history), 15);
    
    // Reduce size (should remove oldest entries)
    bool result = lle_history_set_max_size(history, 12);
    LLE_ASSERT(result);
    LLE_ASSERT_EQ(lle_history_max_size(history), 12);
    LLE_ASSERT_EQ(lle_history_size(history), 12);
    
    // Verify remaining entries are the newest (command_4 through command_15)
    const lle_history_entry_t *entry0 = lle_history_get(history, 0);
    const lle_history_entry_t *entry1 = lle_history_get(history, 1);
    const lle_history_entry_t *entry11 = lle_history_get(history, 11);
    
    LLE_ASSERT(verify_history_entry(entry0, "command_4"));
    LLE_ASSERT(verify_history_entry(entry1, "command_5"));
    LLE_ASSERT(verify_history_entry(entry11, "command_15"));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_max_size_invalid) {
    printf("Testing invalid maximum size changes... ");
    
    lle_history_t *history = lle_history_create(100, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Too small
    bool result1 = lle_history_set_max_size(history, 5);
    LLE_ASSERT(!result1);
    LLE_ASSERT_EQ(lle_history_max_size(history), 100);
    
    // Too large
    bool result2 = lle_history_set_max_size(history, 100000);
    LLE_ASSERT(!result2);
    LLE_ASSERT_EQ(lle_history_max_size(history), 100);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Statistics Tests
// ============================================================================

LLE_TEST(history_statistics) {
    printf("Testing history statistics... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add some entries with varying lengths
    lle_history_add(history, "ls", false);
    lle_history_add(history, "cd /home/user", false);
    lle_history_add(history, "pwd", false);
    
    lle_history_stats_t stats;
    bool result = lle_history_get_stats(history, &stats);
    LLE_ASSERT(result);
    
    LLE_ASSERT_EQ(stats.current_entries, 3);
    LLE_ASSERT_EQ(stats.total_entries, 3);
    LLE_ASSERT_EQ(stats.longest_command, 13); // "cd /home/user"
    LLE_ASSERT_EQ(stats.average_length, (2 + 13 + 3) / 3); // 6
    LLE_ASSERT(stats.memory_usage > 0);
    LLE_ASSERT(stats.newest_timestamp >= stats.oldest_timestamp);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(history_statistics_empty) {
    printf("Testing statistics on empty history... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    lle_history_stats_t stats;
    bool result = lle_history_get_stats(history, &stats);
    LLE_ASSERT(result);
    
    LLE_ASSERT_EQ(stats.current_entries, 0);
    LLE_ASSERT_EQ(stats.total_entries, 0);
    LLE_ASSERT_EQ(stats.longest_command, 0);
    LLE_ASSERT_EQ(stats.average_length, 0);
    LLE_ASSERT_EQ(stats.oldest_timestamp, 0);
    LLE_ASSERT_EQ(stats.newest_timestamp, 0);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

LLE_TEST(history_null_parameters) {
    printf("Testing NULL parameter handling... ");
    
    // Test create with NULL-like conditions already covered
    
    // Test operations on NULL history
    LLE_ASSERT(!lle_history_add(NULL, "test", false));
    LLE_ASSERT_NULL(lle_history_get(NULL, 0));
    LLE_ASSERT_NULL(lle_history_current(NULL));
    LLE_ASSERT_NULL(lle_history_navigate(NULL, LLE_HISTORY_PREV));
    LLE_ASSERT(!lle_history_set_temp_buffer(NULL, "test", 4));
    LLE_ASSERT_NULL(lle_history_get_temp_buffer(NULL, NULL));
    LLE_ASSERT(lle_history_is_empty(NULL)); // Should return true for NULL
    LLE_ASSERT(!lle_history_is_full(NULL));
    LLE_ASSERT_EQ(lle_history_size(NULL), 0);
    LLE_ASSERT_EQ(lle_history_max_size(NULL), 0);
    LLE_ASSERT(!lle_history_set_max_size(NULL, 100));
    LLE_ASSERT(!lle_history_get_stats(NULL, NULL));
    
    printf("PASSED\n");
}

LLE_TEST(history_invalid_operations) {
    printf("Testing invalid operations... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Test invalid get index
    LLE_ASSERT_NULL(lle_history_get(history, 0)); // Empty history
    LLE_ASSERT_NULL(lle_history_get(history, 100)); // Out of range
    
    // Add one entry
    lle_history_add(history, "test", false);
    
    // Test invalid get index
    LLE_ASSERT_NULL(lle_history_get(history, 1)); // Out of range
    LLE_ASSERT_NULL(lle_history_get(history, SIZE_MAX)); // Way out of range
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("=== LLE-024 History Structure Tests ===\n\n");
    
    // Basic Structure Tests
    test_history_create_default();
    test_history_create_custom_size();
    test_history_create_invalid_size();
    test_history_init_stack();
    test_history_clear();
    
    // Entry Management Tests
    test_history_add_basic();
    test_history_add_multiple();
    test_history_add_duplicates();
    test_history_add_empty_command();
    test_history_circular_buffer();
    
    // Navigation Tests
    test_history_navigation_basic();
    test_history_navigation_boundaries();
    test_history_navigation_reset();
    
    // Temporary Buffer Tests
    test_history_temp_buffer();
    test_history_temp_buffer_clear();
    
    // Configuration Tests
    test_history_max_size_change();
    test_history_max_size_invalid();
    
    // Statistics Tests
    test_history_statistics();
    test_history_statistics_empty();
    
    // Error Handling Tests
    test_history_null_parameters();
    test_history_invalid_operations();
    
    printf("\n=== All LLE-024 History Structure Tests Passed! ===\n");
    return 0;
}