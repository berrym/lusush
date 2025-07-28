/**
 * @file test_lle_phase3_search.c
 * @brief Test Phase 3 Ctrl+R reverse search functionality
 *
 * Tests the new Ctrl+R search implementation using display system APIs
 * to ensure proper search functionality without display corruption.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "line_editor.h"
#include "display.h"
#include "history.h"
#include "text_buffer.h"
#include "prompt.h"
#include "terminal_manager.h"
#include "input_handler.h"

// Test framework macro
#define LLE_TEST(name) \
    static bool test_##name(void); \
    static bool test_##name(void)

/**
 * Test search mode display APIs functionality
 */
LLE_TEST(search_mode_display_apis) {
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
    
    // Test updating search prompt
    const char *search_term = "test";
    const char *match_text = "test command example";
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
        fprintf(stderr, "Display state invalid after search operations\n");
        goto cleanup;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;

cleanup:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test search prompt update with various inputs
 */
LLE_TEST(search_prompt_updates) {
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        goto cleanup_prompt_updates;
    }
    
    // Enter search mode
    if (!lle_display_enter_search_mode(display)) {
        goto cleanup_prompt_updates;
    }
    
    // Test empty search term
    if (!lle_display_update_search_prompt(display, NULL, 0, NULL, 0)) {
        fprintf(stderr, "Failed to update search prompt with empty term\n");
        goto cleanup_prompt_updates;
    }
    
    // Test search term with no match
    const char *no_match_term = "nonexistent";
    if (!lle_display_update_search_prompt(display, no_match_term, strlen(no_match_term), NULL, 0)) {
        fprintf(stderr, "Failed to update search prompt with no match\n");
        goto cleanup_prompt_updates;
    }
    
    // Test search term with match
    const char *search_term = "ls";
    const char *match_text = "ls -la /home/user";
    if (!lle_display_update_search_prompt(display, search_term, strlen(search_term),
                                          match_text, strlen(match_text))) {
        fprintf(stderr, "Failed to update search prompt with match\n");
        goto cleanup_prompt_updates;
    }
    
    // Test long search term
    const char *long_term = "this_is_a_very_long_search_term_that_tests_buffer_handling";
    const char *long_match = "this_is_a_very_long_search_term_that_tests_buffer_handling and more text";
    if (!lle_display_update_search_prompt(display, long_term, strlen(long_term),
                                          long_match, strlen(long_match))) {
        fprintf(stderr, "Failed to update search prompt with long term\n");
        goto cleanup_prompt_updates;
    }
    
    // Exit search mode
    if (!lle_display_exit_search_mode(display)) {
        goto cleanup_prompt_updates;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;

cleanup_prompt_updates:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test search mode state management
 */
LLE_TEST(search_mode_state_management) {
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        goto cleanup_state_mgmt;
    }
    
    // Add initial text to buffer
    const char *initial_text = "initial command";
    for (size_t i = 0; i < strlen(initial_text); i++) {
        if (!lle_text_insert_char(&buffer, initial_text[i])) {
            fprintf(stderr, "Failed to insert initial text\n");
            goto cleanup_state_mgmt;
        }
    }
    
    // Verify initial state
    if (buffer.length != strlen(initial_text)) {
        fprintf(stderr, "Buffer length incorrect after initial text\n");
        goto cleanup_state_mgmt;
    }
    
    // Enter search mode
    if (!lle_display_enter_search_mode(display)) {
        fprintf(stderr, "Failed to enter search mode\n");
        goto cleanup_state_mgmt;
    }
    
    // Verify display state consistency during search
    if (!lle_display_validate(display)) {
        fprintf(stderr, "Display state invalid during search mode\n");
        goto cleanup_state_mgmt;
    }
    
    // Update search multiple times
    for (int i = 0; i < 5; i++) {
        char search_term[32];
        snprintf(search_term, sizeof(search_term), "search%d", i);
        
        if (!lle_display_update_search_prompt(display, search_term, strlen(search_term), NULL, 0)) {
            fprintf(stderr, "Failed to update search prompt iteration %d\n", i);
            goto cleanup_state_mgmt;
        }
        
        // Verify state remains valid
        if (!lle_display_validate(display)) {
            fprintf(stderr, "Display state invalid after search update %d\n", i);
            goto cleanup_state_mgmt;
        }
    }
    
    // Exit search mode
    if (!lle_display_exit_search_mode(display)) {
        fprintf(stderr, "Failed to exit search mode\n");
        goto cleanup_state_mgmt;
    }
    
    // Verify state restored correctly
    if (!lle_display_validate(display)) {
        fprintf(stderr, "Display state invalid after exiting search mode\n");
        goto cleanup_state_mgmt;
    }
    
    // Verify buffer content preserved
    if (buffer.length != strlen(initial_text)) {
        fprintf(stderr, "Buffer length changed after search mode\n");
        goto cleanup_state_mgmt;
    }
    
    if (strncmp(buffer.buffer, initial_text, strlen(initial_text)) != 0) {
        fprintf(stderr, "Buffer content changed after search mode\n");
        goto cleanup_state_mgmt;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;

cleanup_state_mgmt:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test search mode with NULL parameter validation
 */
LLE_TEST(search_mode_null_validation) {
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        goto cleanup_null_validation;
    }
    
    // Test NULL display state
    if (lle_display_enter_search_mode(NULL)) {
        fprintf(stderr, "Enter search mode should fail with NULL display\n");
        goto cleanup_null_validation;
    }
    
    if (lle_display_exit_search_mode(NULL)) {
        fprintf(stderr, "Exit search mode should fail with NULL display\n");
        goto cleanup_null_validation;
    }
    
    if (lle_display_update_search_prompt(NULL, "test", 4, "match", 5)) {
        fprintf(stderr, "Update search prompt should fail with NULL display\n");
        goto cleanup_null_validation;
    }
    
    // Test with valid display but invalid state
    // Temporarily corrupt display state for testing
    lle_display_state_t *original_display = display;
    bool original_initialized = display->initialized;
    display->initialized = false;
    
    if (lle_display_enter_search_mode(display)) {
        fprintf(stderr, "Enter search mode should fail with invalid display state\n");
        display->initialized = original_initialized;
        goto cleanup_null_validation;
    }
    
    if (lle_display_exit_search_mode(display)) {
        fprintf(stderr, "Exit search mode should fail with invalid display state\n");
        display->initialized = original_initialized;
        goto cleanup_null_validation;
    }
    
    if (lle_display_update_search_prompt(display, "test", 4, "match", 5)) {
        fprintf(stderr, "Update search prompt should fail with invalid display state\n");
        display->initialized = original_initialized;
        goto cleanup_null_validation;
    }
    
    // Restore display state
    display->initialized = original_initialized;
    display = original_display;
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;

cleanup_null_validation:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

/**
 * Test search mode edge cases
 */
LLE_TEST(search_mode_edge_cases) {
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        return false;
    }
    
    if (!lle_prompt_init(&prompt)) {
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        if (prompt.text) free(prompt.text);
        if (prompt.lines) free(prompt.lines);
        if (buffer.buffer) free(buffer.buffer);
        return false;
    }
    
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        goto cleanup_edge_cases;
    }
    
    // Test multiple enter/exit cycles
    for (int i = 0; i < 3; i++) {
        if (!lle_display_enter_search_mode(display)) {
            fprintf(stderr, "Failed to enter search mode in cycle %d\n", i);
            goto cleanup_edge_cases;
        }
        
        if (!lle_display_exit_search_mode(display)) {
            fprintf(stderr, "Failed to exit search mode in cycle %d\n", i);
            goto cleanup_edge_cases;
        }
    }
    
    // Test nested search mode calls (should handle gracefully)
    if (!lle_display_enter_search_mode(display)) {
        fprintf(stderr, "Failed to enter search mode for nesting test\n");
        goto cleanup_edge_cases;
    }
    
    // Second enter should either succeed or fail gracefully
    lle_display_enter_search_mode(display);
    
    // Exit should work regardless
    if (!lle_display_exit_search_mode(display)) {
        fprintf(stderr, "Failed to exit search mode after nesting test\n");
        goto cleanup_edge_cases;
    }
    
    // Test zero-length updates
    if (!lle_display_enter_search_mode(display)) {
        fprintf(stderr, "Failed to enter search mode for zero-length test\n");
        goto cleanup_edge_cases;
    }
    
    if (!lle_display_update_search_prompt(display, "", 0, "", 0)) {
        fprintf(stderr, "Failed to update search prompt with zero-length strings\n");
        goto cleanup_edge_cases;
    }
    
    if (!lle_display_exit_search_mode(display)) {
        fprintf(stderr, "Failed to exit search mode after zero-length test\n");
        goto cleanup_edge_cases;
    }
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return true;

cleanup_edge_cases:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return false;
}

int main(void) {
    printf("Testing Phase 3 Ctrl+R search functionality...\n");
    
    bool all_passed = true;
    
    // Test search mode display APIs
    printf("Running test_search_mode_display_apis... ");
    if (test_search_mode_display_apis()) {
        printf("✅ PASSED\n");
    } else {
        printf("❌ FAILED\n");
        all_passed = false;
    }
    
    // Test search prompt updates
    printf("Running test_search_prompt_updates... ");
    if (test_search_prompt_updates()) {
        printf("✅ PASSED\n");
    } else {
        printf("❌ FAILED\n");
        all_passed = false;
    }
    
    // Test search mode state management
    printf("Running test_search_mode_state_management... ");
    if (test_search_mode_state_management()) {
        printf("✅ PASSED\n");
    } else {
        printf("❌ FAILED\n");
        all_passed = false;
    }
    
    // Test NULL parameter validation
    printf("Running test_search_mode_null_validation... ");
    if (test_search_mode_null_validation()) {
        printf("✅ PASSED\n");
    } else {
        printf("❌ FAILED\n");
        all_passed = false;
    }
    
    // Test edge cases
    printf("Running test_search_mode_edge_cases... ");
    if (test_search_mode_edge_cases()) {
        printf("✅ PASSED\n");
    } else {
        printf("❌ FAILED\n");
        all_passed = false;
    }
    
    printf("\n");
    if (all_passed) {
        printf("🎉 All Phase 3 search functionality tests PASSED!\n");
        printf("✅ Search mode display APIs working correctly\n");
        printf("✅ Search prompt updates functioning properly\n");
        printf("✅ State management maintains consistency\n");
        printf("✅ NULL parameter validation working\n");
        printf("✅ Edge cases handled gracefully\n");
        return 0;
    } else {
        printf("❌ Some Phase 3 search functionality tests FAILED!\n");
        return 1;
    }
}