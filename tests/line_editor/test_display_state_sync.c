/**
 * @file test_display_state_sync.c
 * @brief Lusush Line Editor - Display State Synchronization System Tests
 *
 * This module tests the unified bidirectional terminal-display state synchronization
 * system to ensure proper state tracking, validation, and recovery functionality.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include "../src/line_editor/display_state_sync.h"
#include "../src/line_editor/display_state_integration.h"
#include "../src/line_editor/display.h"
#include "../src/line_editor/terminal_manager.h"
#include "../src/line_editor/text_buffer.h"

// Test framework macros
#define LLE_TEST(name) \
    static bool test_##name(void)

#define LLE_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED: %s\n", message); \
            return false; \
        } \
    } while(0)

#define LLE_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            fprintf(stderr, "ASSERTION FAILED: %s (expected: %ld, actual: %ld)\n", \
                    message, (long)(expected), (long)(actual)); \
            return false; \
        } \
    } while(0)

#define LLE_ASSERT_STR_EQ(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            fprintf(stderr, "ASSERTION FAILED: %s (expected: '%s', actual: '%s')\n", \
                    message, (expected), (actual)); \
            return false; \
        } \
    } while(0)

// Test helper functions
static lle_display_state_t *create_test_display(void) {
    lle_display_state_t *display = calloc(1, sizeof(lle_display_state_t));
    if (!display) return NULL;
    
    // Initialize basic display state
    display->geometry.cols = 80;
    display->geometry.rows = 24;
    display->cursor_pos.absolute_row = 0;
    display->cursor_pos.absolute_col = 0;
    display->initialized = true;
    
    // Create a test buffer
    display->buffer = calloc(1, sizeof(lle_text_buffer_t));
    if (!display->buffer) {
        free(display);
        return NULL;
    }
    
    display->buffer->buffer = malloc(256);
    if (!display->buffer->buffer) {
        free(display->buffer);
        free(display);
        return NULL;
    }
    
    display->buffer->buffer[0] = '\0';
    display->buffer->length = 0;
    display->buffer->capacity = 256;
    display->buffer->cursor_pos = 0;
    
    return display;
}

static lle_terminal_manager_t *create_test_terminal(void) {
    lle_terminal_manager_t *terminal = calloc(1, sizeof(lle_terminal_manager_t));
    if (!terminal) return NULL;
    
    // Initialize basic terminal manager
    terminal->termcap_initialized = true;
    terminal->stdout_fd = STDOUT_FILENO;
    terminal->stdin_fd = STDIN_FILENO;
    
    return terminal;
}

static void cleanup_test_display(lle_display_state_t *display) {
    if (display) {
        if (display->buffer) {
            free(display->buffer->buffer);
            free(display->buffer);
        }
        free(display);
    }
}

static void cleanup_test_terminal(lle_terminal_manager_t *terminal) {
    free(terminal);
}

// ============================================================================
// Core Synchronization Tests
// ============================================================================

LLE_TEST(sync_context_initialization) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    
    LLE_ASSERT(display != NULL, "Failed to create test display");
    LLE_ASSERT(terminal != NULL, "Failed to create test terminal");
    
    // Test sync context initialization
    lle_state_sync_context_t *sync_ctx = lle_state_sync_init(terminal, display);
    LLE_ASSERT(sync_ctx != NULL, "Failed to initialize sync context");
    
    // Verify context properties
    LLE_ASSERT(sync_ctx->terminal == terminal, "Terminal reference not set");
    LLE_ASSERT(sync_ctx->display == display, "Display reference not set");
    LLE_ASSERT(sync_ctx->bidirectional_sync_enabled == true, "Bidirectional sync not enabled");
    LLE_ASSERT(sync_ctx->ansi_sequence_tracking == true, "ANSI tracking not enabled");
    
    // Test terminal state
    LLE_ASSERT(sync_ctx->terminal_state != NULL, "Terminal state not initialized");
    LLE_ASSERT_EQ(80, sync_ctx->terminal_state->width, "Terminal width incorrect");
    LLE_ASSERT_EQ(24, sync_ctx->terminal_state->height, "Terminal height incorrect");
    
    // Test display sync state
    LLE_ASSERT(sync_ctx->display_sync != NULL, "Display sync state not initialized");
    LLE_ASSERT(sync_ctx->display_sync->expected_content != NULL, "Expected content buffer not allocated");
    
    lle_state_sync_cleanup(sync_ctx);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

LLE_TEST(terminal_state_tracking) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_state_sync_context_t *sync_ctx = lle_state_sync_init(terminal, display);
    
    LLE_ASSERT(sync_ctx != NULL, "Failed to initialize sync context");
    
    // Test terminal write tracking
    const char *test_data = "hello";
    bool success = lle_terminal_state_update_write(sync_ctx, test_data, 5, 0, 5);
    LLE_ASSERT(success, "Terminal write tracking failed");
    
    // Verify cursor position updated
    LLE_ASSERT_EQ(0, sync_ctx->terminal_state->cursor_row, "Cursor row incorrect after write");
    LLE_ASSERT_EQ(5, sync_ctx->terminal_state->cursor_col, "Cursor column incorrect after write");
    
    // Test terminal clear tracking
    success = lle_terminal_state_update_clear(sync_ctx, "eol", 0, 5, 0, 79);
    LLE_ASSERT(success, "Terminal clear tracking failed");
    
    // Test cursor movement tracking
    success = lle_terminal_state_update_cursor(sync_ctx, 1, 10);
    LLE_ASSERT(success, "Terminal cursor tracking failed");
    LLE_ASSERT_EQ(1, sync_ctx->terminal_state->cursor_row, "Cursor row not updated");
    LLE_ASSERT_EQ(10, sync_ctx->terminal_state->cursor_col, "Cursor column not updated");
    
    lle_state_sync_cleanup(sync_ctx);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

LLE_TEST(display_sync_state_management) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_state_sync_context_t *sync_ctx = lle_state_sync_init(terminal, display);
    
    LLE_ASSERT(sync_ctx != NULL, "Failed to initialize sync context");
    
    // Test expected content update
    const char *expected = "test content";
    bool success = lle_display_sync_update_expected(sync_ctx, expected, 12, 12, 1);
    LLE_ASSERT(success, "Failed to update expected content");
    
    // Verify expected content stored
    LLE_ASSERT_EQ(12, sync_ctx->display_sync->expected_length, "Expected length incorrect");
    LLE_ASSERT_EQ(12, sync_ctx->display_sync->expected_cursor_pos, "Expected cursor position incorrect");
    LLE_ASSERT_STR_EQ(expected, sync_ctx->display_sync->expected_content, "Expected content incorrect");
    
    // Test dirty marking
    success = lle_display_sync_mark_dirty(sync_ctx, "test reason");
    LLE_ASSERT(success, "Failed to mark display dirty");
    LLE_ASSERT(sync_ctx->display_sync->sync_required, "Sync not marked as required");
    
    // Test consistency checking
    bool consistent = lle_display_sync_check_consistency(sync_ctx, expected, 12, 12);
    LLE_ASSERT(consistent, "Consistency check failed for matching content");
    
    consistent = lle_display_sync_check_consistency(sync_ctx, "different", 9, 9);
    LLE_ASSERT(!consistent, "Consistency check should fail for different content");
    
    lle_state_sync_cleanup(sync_ctx);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

LLE_TEST(state_validation_and_sync) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_state_sync_context_t *sync_ctx = lle_state_sync_init(terminal, display);
    
    LLE_ASSERT(sync_ctx != NULL, "Failed to initialize sync context");
    
    // Test initial validation (should pass)
    bool valid = lle_state_sync_validate(sync_ctx);
    LLE_ASSERT(valid, "Initial state validation failed");
    
    // Create state divergence
    sync_ctx->terminal_state->cursor_row = 5;
    sync_ctx->terminal_state->cursor_col = 20;
    display->cursor_pos.absolute_row = 0;
    display->cursor_pos.absolute_col = 0;
    
    // Test validation with divergence
    valid = lle_state_sync_validate(sync_ctx);
    LLE_ASSERT(!valid, "Validation should fail with divergent state");
    
    // Test synchronization
    bool success = lle_state_sync_perform(sync_ctx);
    LLE_ASSERT(success, "State synchronization failed");
    
    // Test force resynchronization
    success = lle_state_sync_force_resync(sync_ctx);
    LLE_ASSERT(success, "Force resynchronization failed");
    
    lle_state_sync_cleanup(sync_ctx);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

// ============================================================================
// Integration Layer Tests
// ============================================================================

LLE_TEST(integration_initialization) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    
    // Test integration initialization
    lle_display_integration_t *integration = lle_display_integration_init(display, terminal);
    LLE_ASSERT(integration != NULL, "Failed to initialize integration");
    
    // Verify integration properties
    LLE_ASSERT(integration->sync_enabled, "Sync not enabled by default");
    LLE_ASSERT(integration->validation_enabled, "Validation not enabled by default");
    LLE_ASSERT_EQ(5, integration->sync_frequency, "Default sync frequency incorrect");
    
    // Test configuration changes
    lle_display_integration_set_sync_enabled(integration, false);
    LLE_ASSERT(!integration->sync_enabled, "Sync enable setting failed");
    
    lle_display_integration_set_sync_frequency(integration, 10);
    LLE_ASSERT_EQ(10, integration->sync_frequency, "Sync frequency setting failed");
    
    lle_display_integration_set_debug_mode(integration, true);
    LLE_ASSERT(integration->debug_mode, "Debug mode setting failed");
    
    lle_display_integration_cleanup(integration);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

LLE_TEST(integration_operations) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_display_integration_t *integration = lle_display_integration_init(display, terminal);
    
    LLE_ASSERT(integration != NULL, "Failed to initialize integration");
    
    // Test basic operations (note: these will actually write to stdout in test)
    bool success;
    
    // Test cursor movement
    success = lle_display_integration_move_cursor_home(integration);
    LLE_ASSERT(success, "Move cursor home failed");
    
    // Test character insertion
    success = lle_display_integration_insert_char(integration, 'A');
    LLE_ASSERT(success, "Character insertion failed");
    
    // Test backspace
    success = lle_display_integration_exact_backspace(integration, 1);
    LLE_ASSERT(success, "Exact backspace failed");
    
    // Test state validation
    success = lle_display_integration_validate_state(integration);
    LLE_ASSERT(success, "State validation failed");
    
    // Test force sync
    success = lle_display_integration_force_sync(integration);
    LLE_ASSERT(success, "Force sync failed");
    
    lle_display_integration_cleanup(integration);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

LLE_TEST(content_replacement) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_display_integration_t *integration = lle_display_integration_init(display, terminal);
    
    LLE_ASSERT(integration != NULL, "Failed to initialize integration");
    
    // Test content replacement
    const char *old_content = "old text";
    const char *new_content = "new text content";
    
    bool success = lle_display_integration_replace_content(integration,
                                                           old_content, 8,
                                                           new_content, 16);
    LLE_ASSERT(success, "Content replacement failed");
    
    // Test state consistency after replacement
    success = lle_display_integration_validate_state(integration);
    LLE_ASSERT(success, "State validation failed after content replacement");
    
    // Test state reset
    success = lle_display_integration_reset_tracking(integration);
    LLE_ASSERT(success, "State reset failed");
    
    lle_display_integration_cleanup(integration);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

LLE_TEST(performance_and_statistics) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_display_integration_t *integration = lle_display_integration_init(display, terminal);
    
    LLE_ASSERT(integration != NULL, "Failed to initialize integration");
    
    // Perform some operations to generate statistics
    for (int i = 0; i < 10; i++) {
        lle_display_integration_insert_char(integration, 'A' + i);
    }
    
    // Test statistics retrieval
    uint64_t total_ops;
    size_t total_syncs;
    double avg_sync_time;
    double sync_skip_rate;
    
    bool success = lle_display_integration_get_stats(integration, 
                                                     &total_ops, &total_syncs,
                                                     &avg_sync_time, &sync_skip_rate);
    LLE_ASSERT(success, "Failed to get integration statistics");
    LLE_ASSERT(total_ops >= 10, "Operation count incorrect");
    
    // Test state sync statistics
    size_t sync_failures;
    double divergence_rate;
    
    success = lle_state_sync_get_statistics(integration->sync_ctx,
                                            &total_syncs, &sync_failures,
                                            &avg_sync_time, &divergence_rate);
    LLE_ASSERT(success, "Failed to get sync statistics");
    
    lle_display_integration_cleanup(integration);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

LLE_TEST(ansi_sequence_processing) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_state_sync_context_t *sync_ctx = lle_state_sync_init(terminal, display);
    
    LLE_ASSERT(sync_ctx != NULL, "Failed to initialize sync context");
    
    // Test ANSI sequence processing
    const char *cursor_up = "\x1b[A";
    bool success = lle_terminal_state_process_ansi(sync_ctx, cursor_up, 3);
    LLE_ASSERT(success, "ANSI cursor up processing failed");
    
    const char *clear_line = "\x1b[K";
    success = lle_terminal_state_process_ansi(sync_ctx, clear_line, 3);
    LLE_ASSERT(success, "ANSI clear line processing failed");
    
    const char *cursor_pos = "\x1b[5;10H";
    success = lle_terminal_state_process_ansi(sync_ctx, cursor_pos, 7);
    LLE_ASSERT(success, "ANSI cursor position processing failed");
    
    // Verify ANSI sequence count
    LLE_ASSERT(sync_ctx->ansi_sequences_processed >= 3, "ANSI sequence count incorrect");
    
    lle_state_sync_cleanup(sync_ctx);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

// ============================================================================
// Edge Case and Error Handling Tests
// ============================================================================

LLE_TEST(null_parameter_handling) {
    // Test NULL parameter handling
    lle_state_sync_context_t *sync_ctx = lle_state_sync_init(NULL, NULL);
    LLE_ASSERT(sync_ctx == NULL, "Sync init should fail with NULL parameters");
    
    lle_display_integration_t *integration = lle_display_integration_init(NULL, NULL);
    LLE_ASSERT(integration == NULL, "Integration init should fail with NULL parameters");
    
    // Test NULL context operations
    bool success = lle_state_sync_perform(NULL);
    LLE_ASSERT(!success, "Sync perform should fail with NULL context");
    
    success = lle_display_integration_validate_state(NULL);
    LLE_ASSERT(!success, "Validation should fail with NULL integration");
    
    return true;
}

LLE_TEST(state_divergence_recovery) {
    lle_display_state_t *display = create_test_display();
    lle_terminal_manager_t *terminal = create_test_terminal();
    lle_state_sync_context_t *sync_ctx = lle_state_sync_init(terminal, display);
    
    LLE_ASSERT(sync_ctx != NULL, "Failed to initialize sync context");
    
    // Create significant state divergence
    sync_ctx->terminal_state->cursor_row = 10;
    sync_ctx->terminal_state->cursor_col = 50;
    display->cursor_pos.absolute_row = 0;
    display->cursor_pos.absolute_col = 0;
    
    // Mark as having content mismatch
    lle_display_sync_update_expected(sync_ctx->display_sync, "expected", 8, 8, 1);
    sync_ctx->display_sync->content_validated = false;
    sync_ctx->display_sync->cursor_validated = false;
    
    // Test recovery
    bool success = lle_state_sync_recover_divergence(sync_ctx);
    LLE_ASSERT(success, "State divergence recovery failed");
    
    // Verify recovery restored consistency
    LLE_ASSERT(sync_ctx->display_sync->content_validated, "Content not validated after recovery");
    LLE_ASSERT(sync_ctx->display_sync->cursor_validated, "Cursor not validated after recovery");
    
    lle_state_sync_cleanup(sync_ctx);
    cleanup_test_display(display);
    cleanup_test_terminal(terminal);
    
    return true;
}

// ============================================================================
// Test Runner
// ============================================================================

int main(void) {
    printf("Running Display State Synchronization Tests...\n\n");
    
    struct {
        const char *name;
        bool (*test_func)(void);
    } tests[] = {
        {"Sync Context Initialization", test_sync_context_initialization},
        {"Terminal State Tracking", test_terminal_state_tracking},
        {"Display Sync State Management", test_display_sync_state_management},
        {"State Validation and Sync", test_state_validation_and_sync},
        {"Integration Initialization", test_integration_initialization},
        {"Integration Operations", test_integration_operations},
        {"Content Replacement", test_content_replacement},
        {"Performance and Statistics", test_performance_and_statistics},
        {"ANSI Sequence Processing", test_ansi_sequence_processing},
        {"NULL Parameter Handling", test_null_parameter_handling},
        {"State Divergence Recovery", test_state_divergence_recovery}
    };
    
    int total_tests = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    int failed = 0;
    
    for (int i = 0; i < total_tests; i++) {
        printf("Running: %s... ", tests[i].name);
        fflush(stdout);
        
        bool result = tests[i].test_func();
        if (result) {
            printf("PASS\n");
            passed++;
        } else {
            printf("FAIL\n");
            failed++;
        }
    }
    
    printf("\n=== Test Results ===\n");
    printf("Total: %d\n", total_tests);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Success Rate: %.1f%%\n", (double)passed / total_tests * 100);
    
    if (failed == 0) {
        printf("\n🎉 All tests passed! Display state synchronization system is working correctly.\n");
        return 0;
    } else {
        printf("\n❌ %d test(s) failed. Please check the implementation.\n", failed);
        return 1;
    }
}