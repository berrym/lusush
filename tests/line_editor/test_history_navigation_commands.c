/**
 * @file test_history_navigation_commands.c
 * @brief Tests for history navigation command functions
 * 
 * This file contains tests for the new history navigation command functions
 * lle_cmd_history_up() and lle_cmd_history_down() that integrate with the
 * line editor's edit command system.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2025
 */

#include "test_framework.h"
#include "../src/line_editor/edit_commands.h"
#include "../src/line_editor/command_history.h"
#include "../src/line_editor/display.h"
#include "../src/line_editor/text_buffer.h"
#include "../src/line_editor/terminal_manager.h"
#include "../src/line_editor/prompt.h"
#include "../src/line_editor/display_state_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to create a minimal test display state
static lle_display_state_t *create_test_display_state(void) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(1024);
    if (!buffer) return NULL;
    
    // Create minimal display state - just allocate and initialize basic fields
    lle_display_state_t *state = calloc(1, sizeof(lle_display_state_t));
    if (!state) {
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    
    state->buffer = buffer;
    state->display_state_valid = true;
    
    return state;
}

// Helper function to destroy test display state
static void destroy_test_display_state(lle_display_state_t *state) {
    if (!state) return;
    
    if (state->buffer) {
        lle_text_buffer_destroy(state->buffer);
    }
    
    free(state);
}

// Helper function to create history with test data
static lle_history_t *create_test_history(void) {
    lle_history_t *history = lle_history_create(20, false);
    if (!history) return NULL;
    
    // Add test commands
    lle_history_add(history, "echo hello", false);      // index 0 (oldest)
    lle_history_add(history, "ls -la", false);          // index 1
    lle_history_add(history, "cd /tmp", false);         // index 2
    lle_history_add(history, "make clean", false);      // index 3
    lle_history_add(history, "git status", false);      // index 4 (newest)
    
    return history;
}

// Test basic history up command
LLE_TEST(cmd_history_up_basic) {
    printf("Testing basic history up command... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Initially buffer should be empty
    LLE_ASSERT(state->buffer->length == 0);
    
    // Execute history up command
    lle_command_result_t result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    
    // Buffer should now contain the newest history entry
    LLE_ASSERT(state->buffer->length == strlen("git status"));
    LLE_ASSERT(strncmp(state->buffer->buffer, "git status", state->buffer->length) == 0);
    LLE_ASSERT(state->buffer->cursor_pos == state->buffer->length);
    
    // Execute another history up command
    result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    
    // Buffer should now contain the previous history entry
    LLE_ASSERT(state->buffer->length == strlen("make clean"));
    LLE_ASSERT(strncmp(state->buffer->buffer, "make clean", state->buffer->length) == 0);
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test basic history down command
LLE_TEST(cmd_history_down_basic) {
    printf("Testing basic history down command... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Navigate up twice to get to a position where down makes sense
    lle_command_result_t result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    
    // Buffer should contain "make clean"
    LLE_ASSERT(strncmp(state->buffer->buffer, "make clean", state->buffer->length) == 0);
    
    // Execute history down command
    result = lle_cmd_history_down(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    
    // Buffer should now contain the next newer entry "git status"
    LLE_ASSERT(state->buffer->length == strlen("git status"));
    LLE_ASSERT(strncmp(state->buffer->buffer, "git status", state->buffer->length) == 0);
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test history down at end of history
LLE_TEST(cmd_history_down_at_end) {
    printf("Testing history down at end of history... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Navigate up once to get the newest entry
    lle_command_result_t result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "git status", state->buffer->length) == 0);
    
    // Execute history down command (should go beyond newest entry)
    result = lle_cmd_history_down(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    
    // Buffer should be cleared (no next entry)
    LLE_ASSERT(state->buffer->length == 0);
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test navigation sequence up and down
LLE_TEST(cmd_history_navigation_sequence) {
    printf("Testing history navigation sequence... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Add some initial content to buffer
    const char *initial_content = "initial text";
    for (size_t i = 0; i < strlen(initial_content); i++) {
        lle_text_insert_char(state->buffer, initial_content[i]);
    }
    LLE_ASSERT(state->buffer->length == strlen(initial_content));
    
    // Navigate up through history
    lle_command_result_t result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "git status", state->buffer->length) == 0);
    
    result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "make clean", state->buffer->length) == 0);
    
    result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "cd /tmp", state->buffer->length) == 0);
    
    // Navigate back down
    result = lle_cmd_history_down(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "make clean", state->buffer->length) == 0);
    
    result = lle_cmd_history_down(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "git status", state->buffer->length) == 0);
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test error conditions
LLE_TEST(cmd_history_error_conditions) {
    printf("Testing history command error conditions... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Test NULL parameters
    LLE_ASSERT(lle_cmd_history_up(NULL, history) == LLE_CMD_ERROR_INVALID_STATE);
    LLE_ASSERT(lle_cmd_history_up(state, NULL) == LLE_CMD_ERROR_INVALID_PARAM);
    LLE_ASSERT(lle_cmd_history_down(NULL, history) == LLE_CMD_ERROR_INVALID_STATE);
    LLE_ASSERT(lle_cmd_history_down(state, NULL) == LLE_CMD_ERROR_INVALID_PARAM);
    
    // Test with NULL buffer in state
    lle_text_buffer_t *original_buffer = state->buffer;
    state->buffer = NULL;
    LLE_ASSERT(lle_cmd_history_up(state, history) == LLE_CMD_ERROR_INVALID_STATE);
    LLE_ASSERT(lle_cmd_history_down(state, history) == LLE_CMD_ERROR_INVALID_STATE);
    state->buffer = original_buffer;
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test with empty history
LLE_TEST(cmd_history_empty_history) {
    printf("Testing history commands with empty history... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = lle_history_create(20, false);
    LLE_ASSERT(history != NULL);
    LLE_ASSERT(lle_history_size(history) == 0);
    
    // Commands should succeed but not change buffer
    size_t original_length = state->buffer->length;
    
    lle_command_result_t result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(state->buffer->length == original_length);
    
    result = lle_cmd_history_down(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(state->buffer->length == original_length);
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test with single history entry
LLE_TEST(cmd_history_single_entry) {
    printf("Testing history commands with single entry... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = lle_history_create(20, false);
    LLE_ASSERT(history != NULL);
    LLE_ASSERT(lle_history_add(history, "single command", false));
    LLE_ASSERT(lle_history_size(history) == 1);
    
    // First up should get the single entry
    lle_command_result_t result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(state->buffer->length == strlen("single command"));
    LLE_ASSERT(strncmp(state->buffer->buffer, "single command", state->buffer->length) == 0);
    
    // Second up should not change anything (already at oldest)
    result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(state->buffer->length == strlen("single command"));
    
    // Down should clear the buffer (go past newest)
    result = lle_cmd_history_down(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(state->buffer->length == 0);
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test cursor positioning after navigation
LLE_TEST(cmd_history_cursor_positioning) {
    printf("Testing cursor positioning after history navigation... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Navigate up to get a history entry
    lle_command_result_t result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "git status", state->buffer->length) == 0);
    
    // Cursor should be at the end of the text
    LLE_ASSERT(state->buffer->cursor_pos == state->buffer->length);
    LLE_ASSERT(state->buffer->cursor_pos == strlen("git status"));
    
    // Navigate to a longer entry
    result = lle_cmd_history_up(state, history);
    LLE_ASSERT(result == LLE_CMD_SUCCESS);
    LLE_ASSERT(strncmp(state->buffer->buffer, "make clean", state->buffer->length) == 0);
    
    // Cursor should still be at the end
    LLE_ASSERT(state->buffer->cursor_pos == state->buffer->length);
    LLE_ASSERT(state->buffer->cursor_pos == strlen("make clean"));
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Test buffer state consistency
LLE_TEST(cmd_history_buffer_consistency) {
    printf("Testing buffer state consistency... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT(state != NULL);
    
    lle_history_t *history = create_test_history();
    LLE_ASSERT(history != NULL);
    
    // Navigate through history and verify buffer is always null-terminated
    for (int i = 0; i < 3; i++) {
        lle_command_result_t result = lle_cmd_history_up(state, history);
        LLE_ASSERT(result == LLE_CMD_SUCCESS);
        
        // Verify buffer is null-terminated
        LLE_ASSERT(state->buffer->buffer[state->buffer->length] == '\0');
        
        // Verify cursor position is valid
        LLE_ASSERT(state->buffer->cursor_pos <= state->buffer->length);
        LLE_ASSERT(state->buffer->cursor_pos <= state->buffer->capacity);
    }
    
    lle_history_destroy(history);
    destroy_test_display_state(state);
    printf("PASSED\n");
}

// Main test function
int main(void) {
    printf("=== History Navigation Command Tests ===\n\n");
    
    // Run all tests
    test_cmd_history_up_basic();
    test_cmd_history_down_basic();
    test_cmd_history_down_at_end();
    test_cmd_history_navigation_sequence();
    test_cmd_history_error_conditions();
    test_cmd_history_empty_history();
    test_cmd_history_single_entry();
    test_cmd_history_cursor_positioning();
    test_cmd_history_buffer_consistency();
    
    printf("\n=== All History Navigation Command Tests Passed! ===\n");
    return 0;
}