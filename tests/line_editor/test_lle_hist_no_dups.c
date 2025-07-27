/**
 * @file test_lle_hist_no_dups.c
 * @brief Comprehensive tests for LLE hist_no_dups functionality
 * 
 * This module tests the runtime-toggleable unique history system that
 * implements the Lusush shell's hist_no_dups feature with move-to-end
 * behavior and order preservation.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "../src/line_editor/command_history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// Test Fixtures and Helpers
// ============================================================================

/**
 * @brief Create test history with sample commands
 * @param max_entries Maximum entries
 * @param no_duplicates Enable unique history mode
 * @return Created history or NULL on failure
 */
static lle_history_t *create_sample_history(size_t max_entries, bool no_duplicates) __attribute__((unused));
static lle_history_t *create_sample_history(size_t max_entries, bool no_duplicates) {
    lle_history_t *history = lle_history_create(max_entries, no_duplicates);
    if (!history) return NULL;
    
    // Add sample commands (some will be duplicates)
    lle_history_add(history, "ls -la", false);          // index 0
    lle_history_add(history, "cd /home", false);        // index 1
    lle_history_add(history, "pwd", false);             // index 2
    lle_history_add(history, "ls -la", false);          // duplicate (should be handled)
    lle_history_add(history, "echo hello", false);      // index 3/4
    lle_history_add(history, "cd /home", false);        // duplicate (should be handled)
    lle_history_add(history, "git status", false);      // index 4/5/6
    
    return history;
}

/**
 * @brief Verify history contents match expected commands
 * @param history History to check
 * @param expected Array of expected commands
 * @param count Number of expected commands
 * @return true if match, false otherwise
 */
static bool verify_history_contents(lle_history_t *history, const char **expected, size_t count) {
    if (lle_history_size(history) != count) {
        return false;
    }
    
    for (size_t i = 0; i < count; i++) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        if (!entry || !entry->command || strcmp(entry->command, expected[i]) != 0) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// Basic No Duplicates Mode Tests
// ============================================================================

LLE_TEST(hist_no_dups_create_enabled) {
    printf("Testing history creation with no_duplicates enabled... ");
    
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    
    LLE_ASSERT(lle_history_get_no_duplicates(history));
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    LLE_ASSERT(lle_history_is_empty(history));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_create_disabled) {
    printf("Testing history creation with no_duplicates disabled... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    LLE_ASSERT(!lle_history_get_no_duplicates(history));
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_add_unique_commands) {
    printf("Testing adding unique commands with no_duplicates enabled... ");
    
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add unique commands
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "echo hello", false));
    
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    
    // Verify order is preserved
    const char *expected[] = {"ls", "pwd", "echo hello"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_add_duplicate_move_to_end) {
    printf("Testing duplicate command moves to end with updated timestamp... ");
    
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add initial commands
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "echo hello", false));
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    
    // Get initial timestamp of "ls" command
    const lle_history_entry_t *entry = lle_history_get(history, 0);
    LLE_ASSERT_NOT_NULL(entry);
    uint64_t initial_timestamp = entry->timestamp;
    
    // Add duplicate "ls" - should move to end
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT_EQ(lle_history_size(history), 3); // Size unchanged
    
    // Verify new order: "pwd", "echo hello", "ls"
    const char *expected[] = {"pwd", "echo hello", "ls"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    // Verify "ls" has updated timestamp (now at end)
    const lle_history_entry_t *moved_entry = lle_history_get(history, 2);
    LLE_ASSERT_NOT_NULL(moved_entry);
    LLE_ASSERT(moved_entry->timestamp >= initial_timestamp);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_multiple_duplicates) {
    printf("Testing multiple duplicate commands... ");
    
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add commands with multiple duplicates
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "ls", false));      // duplicate
    LLE_ASSERT(lle_history_add(history, "echo test", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));     // duplicate
    LLE_ASSERT(lle_history_add(history, "ls", false));      // duplicate again
    
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    
    // Final order should be: "echo test", "pwd", "ls"
    const char *expected[] = {"echo test", "pwd", "ls"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_force_add_override) {
    printf("Testing force_add overrides no_duplicates behavior... ");
    
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add initial command
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT_EQ(lle_history_size(history), 1);
    
    // Force add duplicate (should create two entries)
    LLE_ASSERT(lle_history_add(history, "ls", true));
    LLE_ASSERT_EQ(lle_history_size(history), 2);
    
    // Both entries should be "ls"
    const char *expected[] = {"ls", "ls"};
    LLE_ASSERT(verify_history_contents(history, expected, 2));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Runtime Toggle Tests
// ============================================================================

LLE_TEST(hist_no_dups_runtime_enable_cleans_history) {
    printf("Testing runtime enable cleans existing duplicates... ");
    
    // Start with no_duplicates disabled
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    LLE_ASSERT(!lle_history_get_no_duplicates(history));
    
    // Add commands with duplicates (allowed when disabled)
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "ls", false));      // duplicate allowed
    LLE_ASSERT(lle_history_add(history, "echo test", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));     // duplicate allowed
    
    LLE_ASSERT_EQ(lle_history_size(history), 5);
    
    // Enable no_duplicates - should clean duplicates
    LLE_ASSERT(lle_history_set_no_duplicates(history, true));
    LLE_ASSERT(lle_history_get_no_duplicates(history));
    
    // Should now have only unique entries, keeping latest occurrences
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    
    // Expected order: "ls", "echo test", "pwd" (chronological order of latest occurrences)
    const char *expected[] = {"ls", "echo test", "pwd"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_runtime_disable_allows_duplicates) {
    printf("Testing runtime disable allows future duplicates... ");
    
    // Start with no_duplicates enabled
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    LLE_ASSERT(lle_history_get_no_duplicates(history));
    
    // Add unique commands
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT_EQ(lle_history_size(history), 2);
    
    // Disable no_duplicates
    LLE_ASSERT(lle_history_set_no_duplicates(history, false));
    LLE_ASSERT(!lle_history_get_no_duplicates(history));
    
    // Should now allow consecutive duplicates only
    LLE_ASSERT(lle_history_add(history, "pwd", false));  // consecutive duplicate - blocked
    LLE_ASSERT_EQ(lle_history_size(history), 2);        // no change
    
    LLE_ASSERT(lle_history_add(history, "echo test", false)); // different command
    LLE_ASSERT(lle_history_add(history, "ls", false));        // non-consecutive duplicate - allowed
    LLE_ASSERT_EQ(lle_history_size(history), 4);
    
    // Final order: "ls", "pwd", "echo test", "ls"
    const char *expected[] = {"ls", "pwd", "echo test", "ls"};
    LLE_ASSERT(verify_history_contents(history, expected, 4));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_toggle_preserves_timestamps) {
    printf("Testing toggle preserves chronological order of timestamps... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add commands with some duplicates
    LLE_ASSERT(lle_history_add(history, "command1", false));
    LLE_ASSERT(lle_history_add(history, "command2", false));
    LLE_ASSERT(lle_history_add(history, "command1", false));  // duplicate
    LLE_ASSERT(lle_history_add(history, "command3", false));
    
    // Get timestamps before toggle
    const lle_history_entry_t *entry1 = lle_history_get(history, 0);  // command1 (first)
    const lle_history_entry_t *entry2 = lle_history_get(history, 1);  // command2
    const lle_history_entry_t *entry3 = lle_history_get(history, 2);  // command1 (second)
    const lle_history_entry_t *entry4 = lle_history_get(history, 3);  // command3
    
    uint64_t ts1_first __attribute__((unused)) = entry1->timestamp;
    uint64_t ts2 = entry2->timestamp;
    uint64_t ts1_second = entry3->timestamp;
    uint64_t ts3 = entry4->timestamp;
    
    // Enable no_duplicates
    LLE_ASSERT(lle_history_set_no_duplicates(history, true));
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    
    // Should keep latest occurrence of command1 (with ts1_second timestamp)
    // Chronological order: command2 (pos 1), command1 (pos 2), command3 (pos 3)
    const char *expected[] = {"command2", "command1", "command3"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    // Verify chronological order is preserved for remaining entries
    const lle_history_entry_t *final_cmd2 = lle_history_get(history, 0);
    const lle_history_entry_t *final_cmd1 = lle_history_get(history, 1);
    const lle_history_entry_t *final_cmd3 = lle_history_get(history, 2);
    
    LLE_ASSERT_EQ(final_cmd2->timestamp, ts2);
    LLE_ASSERT_EQ(final_cmd1->timestamp, ts1_second); // Latest occurrence
    LLE_ASSERT_EQ(final_cmd3->timestamp, ts3);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Remove Duplicates Function Tests
// ============================================================================

LLE_TEST(hist_no_dups_remove_duplicates_function) {
    printf("Testing lle_history_remove_duplicates function... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add commands with duplicates
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "echo test", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "ls", false));
    
    LLE_ASSERT_EQ(lle_history_size(history), 6);
    
    // Remove duplicates manually
    size_t removed = lle_history_remove_duplicates(history);
    LLE_ASSERT_EQ(removed, 3); // Removed 3 duplicate entries
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    
    // Should keep latest occurrences in chronological order
    const char *expected[] = {"echo test", "pwd", "ls"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_remove_duplicates_empty_history) {
    printf("Testing remove_duplicates on empty history... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    
    size_t removed = lle_history_remove_duplicates(history);
    LLE_ASSERT_EQ(removed, 0);
    LLE_ASSERT_EQ(lle_history_size(history), 0);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_remove_duplicates_no_duplicates) {
    printf("Testing remove_duplicates on history with no duplicates... ");
    
    lle_history_t *history = lle_history_create(10, false);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add unique commands
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "echo test", false));
    
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    
    size_t removed = lle_history_remove_duplicates(history);
    LLE_ASSERT_EQ(removed, 0); // No duplicates to remove
    LLE_ASSERT_EQ(lle_history_size(history), 3); // Size unchanged
    
    // Order should be preserved
    const char *expected[] = {"ls", "pwd", "echo test"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

LLE_TEST(hist_no_dups_error_handling) {
    printf("Testing error handling for no_duplicates functions... ");
    
    // Test NULL parameters
    LLE_ASSERT(!lle_history_set_no_duplicates(NULL, true));
    LLE_ASSERT(!lle_history_get_no_duplicates(NULL));
    LLE_ASSERT_EQ(lle_history_remove_duplicates(NULL), SIZE_MAX);
    
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_circular_buffer_behavior) {
    printf("Testing no_duplicates with circular buffer... ");
    
    // Create small history to trigger circular buffer
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add commands to fill part of buffer
    LLE_ASSERT(lle_history_add(history, "cmd1", false));
    LLE_ASSERT(lle_history_add(history, "cmd2", false));
    LLE_ASSERT(lle_history_add(history, "cmd3", false));
    LLE_ASSERT_EQ(lle_history_size(history), 3);
    LLE_ASSERT(!lle_history_is_full(history));
    
    // Add duplicate of first command (should move to end, causing overflow)
    LLE_ASSERT(lle_history_add(history, "cmd1", false));
    LLE_ASSERT_EQ(lle_history_size(history), 3); // Size unchanged due to duplicate removal
    
    // Should now have: "cmd2", "cmd3", "cmd1"
    const char *expected[] = {"cmd2", "cmd3", "cmd1"};
    LLE_ASSERT(verify_history_contents(history, expected, 3));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

LLE_TEST(hist_no_dups_integration_with_navigation) {
    printf("Testing no_duplicates integration with history navigation... ");
    
    lle_history_t *history = lle_history_create(10, true);
    LLE_ASSERT_NOT_NULL(history);
    
    // Add commands with duplicates
    LLE_ASSERT(lle_history_add(history, "ls", false));
    LLE_ASSERT(lle_history_add(history, "pwd", false));
    LLE_ASSERT(lle_history_add(history, "ls", false));  // moves to end
    
    LLE_ASSERT_EQ(lle_history_size(history), 2);
    
    // Test navigation works correctly
    const char *cmd1 = lle_history_prev(history);
    LLE_ASSERT_NOT_NULL(cmd1);
    LLE_ASSERT_STR_EQ(cmd1, "ls"); // Latest entry
    
    const char *cmd2 = lle_history_prev(history);
    LLE_ASSERT_NOT_NULL(cmd2);
    LLE_ASSERT_STR_EQ(cmd2, "pwd");
    
    // Should be at beginning
    const char *cmd3 = lle_history_prev(history);
    LLE_ASSERT_NULL(cmd3);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// ============================================================================
// Test Runner
// ============================================================================

int main(void) {
    printf("=== LLE History No Duplicates Tests ===\n\n");
    
    // Basic functionality tests
    test_hist_no_dups_create_enabled();
    test_hist_no_dups_create_disabled();
    test_hist_no_dups_add_unique_commands();
    test_hist_no_dups_add_duplicate_move_to_end();
    test_hist_no_dups_multiple_duplicates();
    test_hist_no_dups_force_add_override();
    
    // Runtime toggle tests
    test_hist_no_dups_runtime_enable_cleans_history();
    test_hist_no_dups_runtime_disable_allows_duplicates();
    test_hist_no_dups_toggle_preserves_timestamps();
    
    // Remove duplicates function tests
    test_hist_no_dups_remove_duplicates_function();
    test_hist_no_dups_remove_duplicates_empty_history();
    test_hist_no_dups_remove_duplicates_no_duplicates();
    
    // Edge cases and error handling
    test_hist_no_dups_error_handling();
    test_hist_no_dups_circular_buffer_behavior();
    test_hist_no_dups_integration_with_navigation();
    
    printf("\n=== All LLE History No Duplicates Tests Passed! ===\n");
    return 0;
}