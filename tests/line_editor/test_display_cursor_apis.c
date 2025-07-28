/**
 * @file test_display_cursor_apis.c
 * @brief Test new display cursor movement APIs for keybinding integration
 *
 * Tests the new convenience APIs added to the display system for proper
 * keybinding implementation without manual terminal operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "display.h"
#include "text_buffer.h"
#include "prompt.h"
#include "terminal_manager.h"

// Test framework macro
#define LLE_TEST(name) \
    static bool test_##name(void); \
    static bool test_##name(void)

/**
 * Test lle_display_move_cursor_home functionality
 */
LLE_TEST(display_move_cursor_home) {
    // Create test components
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    // Initialize components
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        fprintf(stderr, "Failed to initialize text buffer\n");
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        fprintf(stderr, "Failed to initialize prompt\n");
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        fprintf(stderr, "Failed to initialize terminal\n");
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    // Create display state
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        fprintf(stderr, "Failed to create display state\n");
        goto cleanup;
    }
    
    // Add some text to buffer
    const char *test_text = "Hello, World!";
    for (size_t i = 0; i < strlen(test_text); i++) {
        if (!lle_text_insert_char(&buffer, test_text[i])) {
            fprintf(stderr, "Failed to insert character\n");
            goto cleanup;
        }
    }
    
    // Verify cursor is at end
    if (buffer.cursor_pos != strlen(test_text)) {
        fprintf(stderr, "Cursor not at expected position after text insertion\n");
        goto cleanup;
    }
    
    // Test moving cursor to home
    if (!lle_display_move_cursor_home(display)) {
        fprintf(stderr, "Failed to move cursor to home\n");
        goto cleanup;
    }
    
    // Verify cursor is at beginning
    if (buffer.cursor_pos != 0) {
        fprintf(stderr, "Cursor not at beginning after move_cursor_home\n");
        goto cleanup;
    }
    
    // Verify display state is still valid
    if (!lle_display_validate(display)) {
        fprintf(stderr, "Display state invalid after move_cursor_home\n");
        goto cleanup;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;
    
cleanup:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test lle_display_move_cursor_end functionality
 */
LLE_TEST(display_move_cursor_end) {
    // Create test components
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    // Initialize components
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        fprintf(stderr, "Failed to initialize text buffer\n");
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        fprintf(stderr, "Failed to initialize prompt\n");
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        fprintf(stderr, "Failed to initialize terminal\n");
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    // Create display state
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        fprintf(stderr, "Failed to create display state\n");
        goto cleanup;
    }
    
    // Add some text to buffer
    const char *test_text = "Test text here";
    for (size_t i = 0; i < strlen(test_text); i++) {
        if (!lle_text_insert_char(&buffer, test_text[i])) {
            fprintf(stderr, "Failed to insert character\n");
            goto cleanup;
        }
    }
    
    // Move cursor to beginning first
    if (!lle_text_move_cursor(&buffer, LLE_MOVE_HOME)) {
        fprintf(stderr, "Failed to move cursor to home initially\n");
        goto cleanup;
    }
    
    // Verify cursor is at beginning
    if (buffer.cursor_pos != 0) {
        fprintf(stderr, "Cursor not at beginning after initial move\n");
        goto cleanup;
    }
    
    // Test moving cursor to end
    if (!lle_display_move_cursor_end(display)) {
        fprintf(stderr, "Failed to move cursor to end\n");
        goto cleanup;
    }
    
    // Verify cursor is at end
    if (buffer.cursor_pos != strlen(test_text)) {
        fprintf(stderr, "Cursor not at end after move_cursor_end\n");
        goto cleanup;
    }
    
    // Verify display state is still valid
    if (!lle_display_validate(display)) {
        fprintf(stderr, "Display state invalid after move_cursor_end\n");
        goto cleanup;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;
    
cleanup:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test lle_display_clear_line functionality
 */
LLE_TEST(display_clear_line) {
    // Create test components
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    // Initialize components
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        fprintf(stderr, "Failed to initialize text buffer\n");
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        fprintf(stderr, "Failed to initialize prompt\n");
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        fprintf(stderr, "Failed to initialize terminal\n");
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    // Create display state
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        fprintf(stderr, "Failed to create display state\n");
        goto cleanup;
    }
    
    // Add some text to buffer
    const char *test_text = "This text will be cleared";
    for (size_t i = 0; i < strlen(test_text); i++) {
        if (!lle_text_insert_char(&buffer, test_text[i])) {
            fprintf(stderr, "Failed to insert character\n");
            goto cleanup;
        }
    }
    
    // Verify buffer has content
    if (buffer.length == 0) {
        fprintf(stderr, "Buffer empty after text insertion\n");
        goto cleanup;
    }
    
    // Test clearing the line
    if (!lle_display_clear_line(display)) {
        fprintf(stderr, "Failed to clear line\n");
        goto cleanup;
    }
    
    // Verify buffer is now empty
    if (buffer.length != 0) {
        fprintf(stderr, "Buffer not empty after clear_line\n");
        goto cleanup;
    }
    
    // Verify cursor is at beginning
    if (buffer.cursor_pos != 0) {
        fprintf(stderr, "Cursor not at beginning after clear_line\n");
        goto cleanup;
    }
    
    // Verify display state is still valid
    if (!lle_display_validate(display)) {
        fprintf(stderr, "Display state invalid after clear_line\n");
        goto cleanup;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;
    
cleanup:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test parameter validation for new APIs
 */
LLE_TEST(api_parameter_validation) {
    // Test NULL parameter handling
    if (lle_display_move_cursor_home(NULL)) {
        fprintf(stderr, "move_cursor_home should fail with NULL parameter\n");
        return false;
    }
    
    if (lle_display_move_cursor_end(NULL)) {
        fprintf(stderr, "move_cursor_end should fail with NULL parameter\n");
        return false;
    }
    
    if (lle_display_clear_line(NULL)) {
        fprintf(stderr, "clear_line should fail with NULL parameter\n");
        return false;
    }
    
    if (lle_display_enter_search_mode(NULL)) {
        fprintf(stderr, "enter_search_mode should fail with NULL parameter\n");
        return false;
    }
    
    if (lle_display_exit_search_mode(NULL)) {
        fprintf(stderr, "exit_search_mode should fail with NULL parameter\n");
        return false;
    }
    
    if (lle_display_update_search_prompt(NULL, "test", 4, "match", 5)) {
        fprintf(stderr, "update_search_prompt should fail with NULL state\n");
        return false;
    }
    
    return true;
}

/**
 * Test search mode APIs
 */
LLE_TEST(search_mode_apis) {
    // Create test components
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    // Initialize components
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        fprintf(stderr, "Failed to initialize text buffer\n");
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        fprintf(stderr, "Failed to initialize prompt\n");
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        fprintf(stderr, "Failed to initialize terminal\n");
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    // Create display state
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        fprintf(stderr, "Failed to create display state\n");
        goto cleanup;
    }
    
    // Test entering search mode
    if (!lle_display_enter_search_mode(display)) {
        fprintf(stderr, "Failed to enter search mode\n");
        goto cleanup;
    }
    
    // Verify display needs refresh flag is set
    if (!display->needs_refresh) {
        fprintf(stderr, "Display refresh flag not set after entering search mode\n");
        goto cleanup;
    }
    
    // Test updating search prompt
    const char *search_term = "test";
    const char *match_text = "test command from history";
    if (!lle_display_update_search_prompt(display, search_term, strlen(search_term), 
                                          match_text, strlen(match_text))) {
        fprintf(stderr, "Failed to update search prompt\n");
        goto cleanup;
    }
    
    // Test exiting search mode
    if (!lle_display_exit_search_mode(display)) {
        fprintf(stderr, "Failed to exit search mode\n");
        goto cleanup;
    }
    
    // Verify display state is still valid
    if (!lle_display_validate(display)) {
        fprintf(stderr, "Display state invalid after search mode operations\n");
        goto cleanup;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;
    
cleanup:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test cursor movement with complex text content
 */
LLE_TEST(cursor_movement_complex_content) {
    // Create test components
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    // Initialize components
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        fprintf(stderr, "Failed to initialize text buffer\n");
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        fprintf(stderr, "Failed to initialize prompt\n");
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        fprintf(stderr, "Failed to initialize terminal\n");
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    // Create display state
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        fprintf(stderr, "Failed to create display state\n");
        goto cleanup;
    }
    
    // Add complex text with Unicode characters
    const char *complex_text = "Hello 世界 with émojis 🌍 and symbols!";
    for (size_t i = 0; i < strlen(complex_text); i++) {
        if (!lle_text_insert_char(&buffer, complex_text[i])) {
            fprintf(stderr, "Failed to insert character at position %zu\n", i);
            goto cleanup;
        }
    }
    
    // Test multiple cursor movements
    for (int iteration = 0; iteration < 3; iteration++) {
        // Move to home
        if (!lle_display_move_cursor_home(display)) {
            fprintf(stderr, "Failed to move cursor to home in iteration %d\n", iteration);
            goto cleanup;
        }
        
        if (buffer.cursor_pos != 0) {
            fprintf(stderr, "Cursor not at home in iteration %d\n", iteration);
            goto cleanup;
        }
        
        // Move to end
        if (!lle_display_move_cursor_end(display)) {
            fprintf(stderr, "Failed to move cursor to end in iteration %d\n", iteration);
            goto cleanup;
        }
        
        if (buffer.cursor_pos != strlen(complex_text)) {
            fprintf(stderr, "Cursor not at end in iteration %d\n", iteration);
            goto cleanup;
        }
    }
    
    // Verify display state remains valid
    if (!lle_display_validate(display)) {
        fprintf(stderr, "Display state invalid after complex cursor movements\n");
        goto cleanup;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;
    
cleanup:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    // Clean up stack-allocated structures manually
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

int main(void) {
    printf("Testing display cursor movement APIs...\n");
    
    int tests_passed = 0;
    int tests_total = 0;
    
    tests_total++;
    if (test_display_move_cursor_home()) {
        printf("✅ test_display_move_cursor_home PASSED\n");
        tests_passed++;
    } else {
        printf("❌ test_display_move_cursor_home FAILED\n");
    }
    
    tests_total++;
    if (test_display_move_cursor_end()) {
        printf("✅ test_display_move_cursor_end PASSED\n");
        tests_passed++;
    } else {
        printf("❌ test_display_move_cursor_end FAILED\n");
    }
    
    tests_total++;
    if (test_display_clear_line()) {
        printf("✅ test_display_clear_line PASSED\n");
        tests_passed++;
    } else {
        printf("❌ test_display_clear_line FAILED\n");
    }
    
    tests_total++;
    if (test_api_parameter_validation()) {
        printf("✅ test_api_parameter_validation PASSED\n");
        tests_passed++;
    } else {
        printf("❌ test_api_parameter_validation FAILED\n");
    }
    
    tests_total++;
    if (test_search_mode_apis()) {
        printf("✅ test_search_mode_apis PASSED\n");
        tests_passed++;
    } else {
        printf("❌ test_search_mode_apis FAILED\n");
    }
    
    tests_total++;
    if (test_cursor_movement_complex_content()) {
        printf("✅ test_cursor_movement_complex_content PASSED\n");
        tests_passed++;
    } else {
        printf("❌ test_cursor_movement_complex_content FAILED\n");
    }
    
    printf("\n=== Display Cursor APIs Test Results ===\n");
    printf("Tests passed: %d/%d\n", tests_passed, tests_total);
    
    if (tests_passed == tests_total) {
        printf("🎉 All display cursor API tests PASSED!\n");
        return 0;
    } else {
        printf("💥 Some display cursor API tests FAILED!\n");
        return 1;
    }
}