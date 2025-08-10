/**
 * @file test_reverse_search.c
 * @brief Comprehensive test suite for reverse search functionality
 * 
 * Tests the reliable Ctrl+R reverse search implementation using the proven
 * exact backspace replication pattern. Validates all search operations,
 * navigation, and state management.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// Include LLE components
#include "reverse_search.h"
#include "text_buffer.h"
#include "command_history.h"
#include "display.h"
#include "terminal_manager.h"

// Test framework macros
#define LLE_TEST(name) \
    static bool test_##name(void)

#define LLE_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define LLE_ASSERT_STR_EQ(actual, expected, message) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            fprintf(stderr, "STRING ASSERTION FAILED: %s\n", message); \
            fprintf(stderr, "  Expected: '%s'\n", expected); \
            fprintf(stderr, "  Actual:   '%s'\n", actual); \
            return false; \
        } \
    } while (0)

#define LLE_ASSERT_INT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "INTEGER ASSERTION FAILED: %s\n", message); \
            fprintf(stderr, "  Expected: %d\n", expected); \
            fprintf(stderr, "  Actual:   %d\n", actual); \
            return false; \
        } \
    } while (0)

// ============================================================================
// Test Fixtures and Setup
// ============================================================================

/**
 * @brief Create test history with sample commands
 * 
 * @param history Pointer to history structure to populate
 * @return true on success, false on error
 */
static bool setup_test_history(lle_history_t *history) {
    const char *test_commands[] = {
        "echo hello world",
        "ls -la /home",
        "grep pattern file.txt",
        "echo test command",
        "cd /var/log",
        "tail -f syslog",
        "echo another test",
        "find . -name '*.c'",
        "echo final command"
    };
    
    size_t num_commands = sizeof(test_commands) / sizeof(test_commands[0]);
    
    for (size_t i = 0; i < num_commands; i++) {
        if (!lle_history_add(history, test_commands[i], false)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Create minimal display state for testing
 * 
 * @return Pointer to display state, or NULL on error
 */
static lle_display_state_t *create_test_display(void) {
    // Create minimal display state structure
    lle_display_state_t *display = calloc(1, sizeof(lle_display_state_t));
    if (!display) return NULL;
    
    // Initialize basic display state
    // Note: This is a simplified test setup
    return display;
}

/**
 * @brief Clean up test display state
 * 
 * @param display Display state to clean up
 */
static void cleanup_test_display(lle_display_state_t *display) {
    if (display) {
        free(display);
    }
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

LLE_TEST(init_and_cleanup) {
    // Test system initialization
    LLE_ASSERT(lle_reverse_search_init(), "Failed to initialize reverse search system");
    LLE_ASSERT(!lle_reverse_search_is_active(), "Search should not be active after init");
    
    // Test system cleanup
    lle_reverse_search_cleanup();
    LLE_ASSERT(!lle_reverse_search_is_active(), "Search should not be active after cleanup");
    
    return true;
}

LLE_TEST(enter_exit_search_mode) {
    lle_reverse_search_init();
    
    // Create test fixtures
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    
    // Add test content to buffer
    lle_text_buffer_clear(buffer);
    const char *original_text = "original command";
    for (size_t i = 0; i < strlen(original_text); i++) {
        lle_text_insert_char(buffer, original_text[i]);
    }
    
    // Test entering search mode
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    LLE_ASSERT(lle_reverse_search_is_active(), "Search should be active after entering");
    
    // Test exiting search mode (cancel)
    LLE_ASSERT(lle_reverse_search_exit(display, false), "Failed to exit search mode");
    LLE_ASSERT(!lle_reverse_search_is_active(), "Search should not be active after exit");
    
    // Clean up
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

LLE_TEST(basic_search_functionality) {
    lle_reverse_search_init();
    
    // Create test fixtures
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    LLE_ASSERT(setup_test_history(history), "Failed to setup test history");
    
    // Enter search mode
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    
    // Test adding characters to search query
    LLE_ASSERT(lle_reverse_search_add_char(display, buffer, history, 'e'), "Failed to add 'e'");
    LLE_ASSERT(lle_reverse_search_add_char(display, buffer, history, 'c'), "Failed to add 'c'");
    LLE_ASSERT(lle_reverse_search_add_char(display, buffer, history, 'h'), "Failed to add 'h'");
    LLE_ASSERT(lle_reverse_search_add_char(display, buffer, history, 'o'), "Failed to add 'o'");
    
    // Verify search query
    const char *query = lle_reverse_search_get_query();
    LLE_ASSERT(query != NULL, "Query should not be NULL");
    LLE_ASSERT_STR_EQ(query, "echo", "Query should be 'echo'");
    
    // Verify match was found
    int match_index = lle_reverse_search_get_match_index();
    LLE_ASSERT(match_index >= 0, "Should have found a match");
    
    // Test accepting match
    LLE_ASSERT(lle_reverse_search_exit(display, true), "Failed to accept match");
    
    // Clean up
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

LLE_TEST(search_query_editing) {
    lle_reverse_search_init();
    
    // Create test fixtures
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    LLE_ASSERT(setup_test_history(history), "Failed to setup test history");
    
    // Enter search mode
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    
    // Build search query
    lle_reverse_search_add_char(display, buffer, history, 'g');
    lle_reverse_search_add_char(display, buffer, history, 'r');
    lle_reverse_search_add_char(display, buffer, history, 'e');
    lle_reverse_search_add_char(display, buffer, history, 'p');
    
    // Verify query
    const char *query = lle_reverse_search_get_query();
    LLE_ASSERT_STR_EQ(query, "grep", "Query should be 'grep'");
    
    // Test backspace functionality
    LLE_ASSERT(lle_reverse_search_backspace(display, buffer, history), "Failed to backspace");
    query = lle_reverse_search_get_query();
    LLE_ASSERT_STR_EQ(query, "gre", "Query should be 'gre' after backspace");
    
    // Test multiple backspaces
    lle_reverse_search_backspace(display, buffer, history);
    lle_reverse_search_backspace(display, buffer, history);
    query = lle_reverse_search_get_query();
    LLE_ASSERT_STR_EQ(query, "g", "Query should be 'g' after multiple backspaces");
    
    // Test backspace on empty query
    lle_reverse_search_backspace(display, buffer, history);
    lle_reverse_search_backspace(display, buffer, history); // Should handle gracefully
    query = lle_reverse_search_get_query();
    LLE_ASSERT_STR_EQ(query, "", "Query should be empty after clearing");
    
    // Clean up
    lle_reverse_search_exit(display, false);
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

// ============================================================================
// Navigation Tests
// ============================================================================

LLE_TEST(backward_search_navigation) {
    lle_reverse_search_init();
    
    // Create test fixtures
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    LLE_ASSERT(setup_test_history(history), "Failed to setup test history");
    
    // Enter search mode and search for "echo"
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    
    lle_reverse_search_add_char(display, buffer, history, 'e');
    lle_reverse_search_add_char(display, buffer, history, 'c');
    lle_reverse_search_add_char(display, buffer, history, 'h');
    lle_reverse_search_add_char(display, buffer, history, 'o');
    
    // Should find most recent "echo" command first
    int first_match = lle_reverse_search_get_match_index();
    LLE_ASSERT(first_match >= 0, "Should find first match");
    
    // Navigate to next older match
    LLE_ASSERT(lle_reverse_search_next_match(display, buffer, history, LLE_SEARCH_BACKWARD), 
               "Should find older match");
    
    int second_match = lle_reverse_search_get_match_index();
    LLE_ASSERT(second_match >= 0, "Should find second match");
    LLE_ASSERT(second_match < first_match, "Second match should be older (lower index)");
    
    // Try to find another older match
    bool found_third = lle_reverse_search_next_match(display, buffer, history, LLE_SEARCH_BACKWARD);
    if (found_third) {
        int third_match = lle_reverse_search_get_match_index();
        LLE_ASSERT(third_match < second_match, "Third match should be older than second");
    }
    
    // Clean up
    lle_reverse_search_exit(display, false);
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

LLE_TEST(forward_search_navigation) {
    lle_reverse_search_init();
    
    // Create test fixtures
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    LLE_ASSERT(setup_test_history(history), "Failed to setup test history");
    
    // Enter search mode and search for "echo"
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    
    lle_reverse_search_add_char(display, buffer, history, 'e');
    lle_reverse_search_add_char(display, buffer, history, 'c');
    lle_reverse_search_add_char(display, buffer, history, 'h');
    lle_reverse_search_add_char(display, buffer, history, 'o');
    
    // Navigate to older matches first
    lle_reverse_search_next_match(display, buffer, history, LLE_SEARCH_BACKWARD);
    lle_reverse_search_next_match(display, buffer, history, LLE_SEARCH_BACKWARD);
    
    int older_match = lle_reverse_search_get_match_index();
    
    // Now navigate forward (toward newer entries)
    bool found_newer = lle_reverse_search_next_match(display, buffer, history, LLE_SEARCH_FORWARD);
    if (found_newer) {
        int newer_match = lle_reverse_search_get_match_index();
        LLE_ASSERT(newer_match > older_match, "Forward search should find newer match");
    }
    
    // Clean up
    lle_reverse_search_exit(display, false);
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

// ============================================================================
// Edge Case Tests
// ============================================================================

LLE_TEST(no_match_scenarios) {
    lle_reverse_search_init();
    
    // Create test fixtures
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    LLE_ASSERT(setup_test_history(history), "Failed to setup test history");
    
    // Enter search mode
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    
    // Search for non-existent pattern
    lle_reverse_search_add_char(display, buffer, history, 'x');
    lle_reverse_search_add_char(display, buffer, history, 'y');
    lle_reverse_search_add_char(display, buffer, history, 'z');
    
    // Should not find any match
    int match_index = lle_reverse_search_get_match_index();
    LLE_ASSERT(match_index == -1, "Should not find match for 'xyz'");
    
    // Test navigation with no matches
    bool found = lle_reverse_search_next_match(display, buffer, history, LLE_SEARCH_BACKWARD);
    LLE_ASSERT(!found, "Should not find any matches for navigation");
    
    // Clean up
    lle_reverse_search_exit(display, false);
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

LLE_TEST(empty_history_search) {
    lle_reverse_search_init();
    
    // Create test fixtures with empty history
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    
    // Enter search mode with empty history
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    
    // Try to search
    lle_reverse_search_add_char(display, buffer, history, 'a');
    
    // Should not find any match
    int match_index = lle_reverse_search_get_match_index();
    LLE_ASSERT(match_index == -1, "Should not find match in empty history");
    
    // Clean up
    lle_reverse_search_exit(display, false);
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

LLE_TEST(error_handling) {
    lle_reverse_search_init();
    
    // Test with NULL parameters
    LLE_ASSERT(!lle_reverse_search_enter(NULL, NULL, NULL), "Should handle NULL parameters");
    LLE_ASSERT(!lle_reverse_search_add_char(NULL, NULL, NULL, 'a'), "Should handle NULL parameters");
    LLE_ASSERT(!lle_reverse_search_backspace(NULL, NULL, NULL), "Should handle NULL parameters");
    LLE_ASSERT(!lle_reverse_search_next_match(NULL, NULL, NULL, LLE_SEARCH_BACKWARD), 
               "Should handle NULL parameters");
    
    // Test operations when not in search mode
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(!lle_reverse_search_add_char(display, buffer, history, 'a'), 
               "Should fail when not in search mode");
    LLE_ASSERT(!lle_reverse_search_backspace(display, buffer, history), 
               "Should fail when not in search mode");
    LLE_ASSERT(!lle_reverse_search_next_match(display, buffer, history, LLE_SEARCH_BACKWARD), 
               "Should fail when not in search mode");
    
    // Clean up
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

// ============================================================================
// State Management Tests
// ============================================================================

LLE_TEST(state_consistency) {
    lle_reverse_search_init();
    
    // Create test fixtures
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_history_t *history = lle_history_create(100, false);
    lle_display_state_t *display = create_test_display();
    
    LLE_ASSERT(buffer && history && display, "Failed to create test fixtures");
    LLE_ASSERT(setup_test_history(history), "Failed to setup test history");
    
    // Test state before entering search
    LLE_ASSERT(!lle_reverse_search_is_active(), "Should not be active initially");
    LLE_ASSERT(lle_reverse_search_get_query() == NULL, "Query should be NULL when inactive");
    LLE_ASSERT(lle_reverse_search_get_match_index() == -1, "Match index should be -1 when inactive");
    
    // Enter search mode
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_reverse_search_enter(display, buffer, history), "Failed to enter search mode");
    
    // Test state after entering search
    LLE_ASSERT(lle_reverse_search_is_active(), "Should be active after entering");
    LLE_ASSERT(lle_reverse_search_get_query() != NULL, "Query should not be NULL when active");
    LLE_ASSERT_STR_EQ(lle_reverse_search_get_query(), "", "Query should be empty initially");
    LLE_ASSERT_INT_EQ(lle_reverse_search_get_direction(), LLE_SEARCH_BACKWARD, 
                     "Direction should be backward initially");
    
    // Add search characters and test state
    lle_reverse_search_add_char(display, buffer, history, 't');
    lle_reverse_search_add_char(display, buffer, history, 'e');
    lle_reverse_search_add_char(display, buffer, history, 's');
    lle_reverse_search_add_char(display, buffer, history, 't');
    
    LLE_ASSERT_STR_EQ(lle_reverse_search_get_query(), "test", "Query should be 'test'");
    
    // Exit search and test final state
    lle_reverse_search_exit(display, false);
    LLE_ASSERT(!lle_reverse_search_is_active(), "Should not be active after exit");
    LLE_ASSERT(lle_reverse_search_get_query() == NULL, "Query should be NULL after exit");
    
    // Clean up
    lle_text_buffer_destroy(buffer);
    lle_history_destroy(history);
    cleanup_test_display(display);
    lle_reverse_search_cleanup();
    
    return true;
}

// ============================================================================
// Test Runner
// ============================================================================

/**
 * @brief Run all reverse search tests
 * 
 * @return 0 on success, 1 on failure
 */
int main(void) {
    printf("Running reverse search tests...\n\n");
    
    struct {
        const char *name;
        bool (*test_func)(void);
    } tests[] = {
        {"init_and_cleanup", test_init_and_cleanup},
        {"enter_exit_search_mode", test_enter_exit_search_mode},
        {"basic_search_functionality", test_basic_search_functionality},
        {"search_query_editing", test_search_query_editing},
        {"backward_search_navigation", test_backward_search_navigation},
        {"forward_search_navigation", test_forward_search_navigation},
        {"no_match_scenarios", test_no_match_scenarios},
        {"empty_history_search", test_empty_history_search},
        {"error_handling", test_error_handling},
        {"state_consistency", test_state_consistency}
    };
    
    size_t num_tests = sizeof(tests) / sizeof(tests[0]);
    size_t passed = 0;
    size_t failed = 0;
    
    for (size_t i = 0; i < num_tests; i++) {
        printf("Running test: %s... ", tests[i].name);
        fflush(stdout);
        
        if (tests[i].test_func()) {
            printf("PASSED\n");
            passed++;
        } else {
            printf("FAILED\n");
            failed++;
        }
    }
    
    printf("\n=== Test Results ===\n");
    printf("Total tests: %zu\n", num_tests);
    printf("Passed: %zu\n", passed);
    printf("Failed: %zu\n", failed);
    printf("Success rate: %.1f%%\n", (double)passed / num_tests * 100.0);
    
    return (failed == 0) ? 0 : 1;
}