/**
 * @file test_lle_026_history_navigation.c
 * @brief Tests for LLE-026 History Navigation (convenience functions)
 * 
 * This file contains comprehensive tests for the history navigation convenience
 * functions including prev/next navigation, position management, and integration
 * with the existing navigation system.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "../src/line_editor/lle_history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to create history with test data
static lle_history_t *create_test_history(void) {
    lle_history_t *history = lle_history_create(20);
    if (!history) return NULL;
    
    // Add test commands
    lle_history_add(history, "echo hello", false);      // index 0 (oldest)
    lle_history_add(history, "ls -la", false);          // index 1
    lle_history_add(history, "cd /tmp", false);         // index 2
    lle_history_add(history, "make clean", false);      // index 3
    lle_history_add(history, "git status", false);      // index 4 (newest)
    
    return history;
}

// Test basic previous navigation
LLE_TEST(history_prev_basic) {
    printf("Testing basic previous navigation... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    LLE_ASSERT(lle_history_size(history) == 5);
    
    // Start navigation - should get newest entry first
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "git status") == 0);
    
    // Continue navigating backwards
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "make clean") == 0);
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "cd /tmp") == 0);
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "ls -la") == 0);
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "echo hello") == 0);
    
    // Try to go beyond oldest - should return NULL
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd == NULL);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test basic next navigation
LLE_TEST(history_next_basic) {
    printf("Testing basic next navigation... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Navigate to oldest entry first
    LLE_ASSERT(lle_history_set_position(history, 0));
    
    // Navigate forward
    const char *cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "ls -la") == 0);
    
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "cd /tmp") == 0);
    
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "make clean") == 0);
    
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "git status") == 0);
    
    // Try to go beyond newest - should return NULL and exit navigation
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd == NULL);
    
    // Verify navigation mode was exited
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test position management functions
LLE_TEST(history_position_management) {
    printf("Testing position management functions... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Initially not in navigation mode
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    
    // Set position directly
    LLE_ASSERT(lle_history_set_position(history, 2));
    LLE_ASSERT(lle_history_get_position(history) == 2);
    
    // Verify we can get the command at that position
    const lle_history_entry_t *entry = lle_history_current(history);
    LLE_ASSERT(entry != NULL);
    LLE_ASSERT(strcmp(entry->command, "cd /tmp") == 0);
    
    // Set to different positions
    LLE_ASSERT(lle_history_set_position(history, 0));
    LLE_ASSERT(lle_history_get_position(history) == 0);
    
    LLE_ASSERT(lle_history_set_position(history, 4));
    LLE_ASSERT(lle_history_get_position(history) == 4);
    
    // Try invalid position
    LLE_ASSERT(!lle_history_set_position(history, 5));
    LLE_ASSERT(!lle_history_set_position(history, 100));
    
    // Position should remain unchanged after failed set
    LLE_ASSERT(lle_history_get_position(history) == 4);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test position reset functionality
LLE_TEST(history_position_reset) {
    printf("Testing position reset functionality... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Enter navigation mode
    LLE_ASSERT(lle_history_set_position(history, 2));
    LLE_ASSERT(lle_history_get_position(history) == 2);
    
    // Reset position
    LLE_ASSERT(lle_history_reset_position(history));
    
    // Should no longer be in navigation mode
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    
    // Should be able to start navigation again
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "git status") == 0);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test mixed navigation (prev/next combination)
LLE_TEST(history_mixed_navigation) {
    printf("Testing mixed prev/next navigation... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Start with prev to go to newest
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "git status") == 0);
    LLE_ASSERT(lle_history_get_position(history) == 4);
    
    // Go back two more
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "make clean") == 0);
    
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "cd /tmp") == 0);
    LLE_ASSERT(lle_history_get_position(history) == 2);
    
    // Now go forward one
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "make clean") == 0);
    LLE_ASSERT(lle_history_get_position(history) == 3);
    
    // Back one again
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "cd /tmp") == 0);
    LLE_ASSERT(lle_history_get_position(history) == 2);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test navigation with empty history
LLE_TEST(history_navigation_empty) {
    printf("Testing navigation with empty history... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    LLE_ASSERT(lle_history_size(history) == 0);
    
    // All navigation should return NULL on empty history
    LLE_ASSERT(lle_history_prev(history) == NULL);
    LLE_ASSERT(lle_history_next(history) == NULL);
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    LLE_ASSERT(!lle_history_set_position(history, 0));
    
    // Reset should still work
    LLE_ASSERT(lle_history_reset_position(history));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test navigation with single entry
LLE_TEST(history_navigation_single_entry) {
    printf("Testing navigation with single entry... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    LLE_ASSERT(lle_history_add(history, "single command", false));
    LLE_ASSERT(lle_history_size(history) == 1);
    
    // Should be able to navigate to the single entry
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "single command") == 0);
    LLE_ASSERT(lle_history_get_position(history) == 0);
    
    // Trying to go prev again should return NULL
    LLE_ASSERT(lle_history_prev(history) == NULL);
    
    // Going next should exit navigation mode
    LLE_ASSERT(lle_history_next(history) == NULL);
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    
    // Position 0 should be valid, position 1 should not
    LLE_ASSERT(lle_history_set_position(history, 0));
    LLE_ASSERT(!lle_history_set_position(history, 1));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test error conditions
LLE_TEST(history_navigation_error_conditions) {
    printf("Testing navigation error conditions... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Test NULL parameters
    LLE_ASSERT(lle_history_prev(NULL) == NULL);
    LLE_ASSERT(lle_history_next(NULL) == NULL);
    LLE_ASSERT(lle_history_get_position(NULL) == LLE_HISTORY_INVALID_POSITION);
    LLE_ASSERT(!lle_history_set_position(NULL, 0));
    LLE_ASSERT(!lle_history_reset_position(NULL));
    
    // Test with valid history
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test integration with existing navigation system
LLE_TEST(history_navigation_integration) {
    printf("Testing integration with existing navigation system... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Use convenience function to start navigation
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "git status") == 0);
    
    // Mix with existing navigation API
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_PREV);
    LLE_ASSERT(entry != NULL);
    LLE_ASSERT(strcmp(entry->command, "make clean") == 0);
    
    // Use convenience function again
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "cd /tmp") == 0);
    
    // Check current position
    LLE_ASSERT(lle_history_get_position(history) == 2);
    
    // Use existing current function
    entry = lle_history_current(history);
    LLE_ASSERT(entry != NULL);
    LLE_ASSERT(strcmp(entry->command, "cd /tmp") == 0);
    
    // Reset using existing function
    lle_history_reset_navigation(history);
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test boundary conditions thoroughly
LLE_TEST(history_navigation_boundaries) {
    printf("Testing navigation boundary conditions... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Navigate to oldest entry
    LLE_ASSERT(lle_history_set_position(history, 0));
    LLE_ASSERT(lle_history_get_position(history) == 0);
    
    // Try to go further back
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd == NULL);
    
    // Should still be at position 0
    LLE_ASSERT(lle_history_get_position(history) == 0);
    
    // Navigate to newest entry
    LLE_ASSERT(lle_history_set_position(history, 4));
    LLE_ASSERT(lle_history_get_position(history) == 4);
    
    // Go forward past newest - should exit navigation
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd == NULL);
    LLE_ASSERT(lle_history_get_position(history) == LLE_HISTORY_INVALID_POSITION);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test navigation state consistency
LLE_TEST(history_navigation_state_consistency) {
    printf("Testing navigation state consistency... ");
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Start navigation with prev
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(lle_history_get_position(history) == 4);
    
    // Set position directly
    LLE_ASSERT(lle_history_set_position(history, 1));
    LLE_ASSERT(lle_history_get_position(history) == 1);
    
    // Continue with prev from new position
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "echo hello") == 0);
    LLE_ASSERT(lle_history_get_position(history) == 0);
    
    // Continue with next
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "ls -la") == 0);
    LLE_ASSERT(lle_history_get_position(history) == 1);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test large history navigation performance
LLE_TEST(history_navigation_large) {
    printf("Testing navigation with large history... ");
    
    lle_history_t *history = lle_history_create(1000);
    LLE_ASSERT(history != NULL);
    
    // Add many entries
    char command[32];
    for (int i = 0; i < 100; i++) {
        snprintf(command, sizeof(command), "command_%d", i);
        LLE_ASSERT(lle_history_add(history, command, false));
    }
    
    LLE_ASSERT(lle_history_size(history) == 100);
    
    // Navigate to middle
    LLE_ASSERT(lle_history_set_position(history, 50));
    LLE_ASSERT(lle_history_get_position(history) == 50);
    
    // Navigate back and forth
    const char *cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "command_49") == 0);
    
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "command_50") == 0);
    
    // Jump to boundaries
    LLE_ASSERT(lle_history_set_position(history, 0));
    cmd = lle_history_next(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "command_1") == 0);
    
    LLE_ASSERT(lle_history_set_position(history, 99));
    cmd = lle_history_prev(history);
    LLE_ASSERT(cmd != NULL);
    LLE_ASSERT(strcmp(cmd, "command_98") == 0);
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Main test function
int main(void) {
    printf("=== LLE-026 History Navigation Tests ===\n\n");
    
    // Run all tests
    test_history_prev_basic();
    test_history_next_basic();
    test_history_position_management();
    test_history_position_reset();
    test_history_mixed_navigation();
    test_history_navigation_empty();
    test_history_navigation_single_entry();
    test_history_navigation_error_conditions();
    test_history_navigation_integration();
    test_history_navigation_boundaries();
    test_history_navigation_state_consistency();
    test_history_navigation_large();
    
    printf("\n=== All LLE-026 History Navigation Tests Passed! ===\n");
    return 0;
}