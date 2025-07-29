/**
 * @file test_character_duplication_fix.c
 * @brief Simple test to validate the character duplication fix
 * 
 * This test validates that the true incremental character updates work correctly
 * and that character duplication is eliminated.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024-12-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "display.h"
#include "text_buffer.h"
#include "terminal_manager.h"
#include "prompt.h"

/**
 * @brief Simple test to validate incremental character updates
 */
static bool test_incremental_character_updates(void) {
    printf("Testing incremental character updates...\n");
    
    // Create text buffer
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    if (!buffer) {
        printf("Failed to create text buffer\n");
        return false;
    }
    
    // Create terminal manager
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    if (!terminal) {
        printf("Failed to create terminal manager\n");
        lle_text_buffer_destroy(buffer);
        return false;
    }
    
    // Initialize terminal
    if (!lle_terminal_init(terminal)) {
        printf("Failed to initialize terminal\n");
        free(terminal);
        lle_text_buffer_destroy(buffer);
        return false;
    }
    
    // Create simple prompt
    lle_prompt_t *prompt = lle_prompt_create(256);
    if (!prompt) {
        printf("Failed to create prompt\n");
        lle_terminal_cleanup(terminal);
        free(terminal);
        lle_text_buffer_destroy(buffer);
        return false;
    }
    
    if (!lle_prompt_parse(prompt, "$ ")) {
        printf("Failed to parse prompt text\n");
        lle_prompt_destroy(prompt);
        lle_terminal_cleanup(terminal);
        free(terminal);
        lle_text_buffer_destroy(buffer);
        return false;
    }
    
    // Create display state
    lle_display_state_t *display = lle_display_create(prompt, buffer, terminal);
    if (!display) {
        printf("Failed to create display state\n");
        lle_prompt_destroy(prompt);
        lle_terminal_cleanup(terminal);
        free(terminal);
        lle_text_buffer_destroy(buffer);
        return false;
    }
    
    printf("Display state created successfully\n");
    
    // Test 1: Initial state should have empty tracking
    if (display->last_displayed_length != 0) {
        printf("FAIL: Initial displayed length should be 0, got %zu\n", display->last_displayed_length);
        goto cleanup;
    }
    
    if (display->display_state_valid != false) {
        printf("FAIL: Initial display state should be invalid\n");
        goto cleanup;
    }
    
    printf("PASS: Initial state correct\n");
    
    // Test 2: Add single character 'h'
    if (!lle_text_insert_char(buffer, 'h')) {
        printf("FAIL: Failed to insert character 'h'\n");
        goto cleanup;
    }
    
    printf("Calling incremental update after adding 'h'...\n");
    if (!lle_display_update_incremental(display)) {
        printf("FAIL: First incremental update failed\n");
        goto cleanup;
    }
    
    // After first update, tracking should be valid and length should be 1
    if (!display->display_state_valid) {
        printf("FAIL: Display state should be valid after first update\n");
        goto cleanup;
    }
    
    if (display->last_displayed_length != 1) {
        printf("FAIL: Displayed length should be 1, got %zu\n", display->last_displayed_length);
        goto cleanup;
    }
    
    if (display->last_displayed_content[0] != 'h') {
        printf("FAIL: Displayed content should be 'h', got '%c'\n", display->last_displayed_content[0]);
        goto cleanup;
    }
    
    printf("PASS: Single character addition works\n");
    
    // Test 3: Add second character 'e' (should trigger incremental path)
    if (!lle_text_insert_char(buffer, 'e')) {
        printf("FAIL: Failed to insert character 'e'\n");
        goto cleanup;
    }
    
    printf("Calling incremental update after adding 'e'...\n");
    if (!lle_display_update_incremental(display)) {
        printf("FAIL: Second incremental update failed\n");
        goto cleanup;
    }
    
    // Check that incremental update worked
    if (display->last_displayed_length != 2) {
        printf("FAIL: Displayed length should be 2, got %zu\n", display->last_displayed_length);
        goto cleanup;
    }
    
    if (strncmp(display->last_displayed_content, "he", 2) != 0) {
        printf("FAIL: Displayed content should be 'he', got '%.2s'\n", display->last_displayed_content);
        goto cleanup;
    }
    
    printf("PASS: Second character addition works\n");
    
    // Test 4: Add third character 'l' 
    if (!lle_text_insert_char(buffer, 'l')) {
        printf("FAIL: Failed to insert character 'l'\n");
        goto cleanup;
    }
    
    printf("Calling incremental update after adding 'l'...\n");
    if (!lle_display_update_incremental(display)) {
        printf("FAIL: Third incremental update failed\n");
        goto cleanup;
    }
    
    if (display->last_displayed_length != 3) {
        printf("FAIL: Displayed length should be 3, got %zu\n", display->last_displayed_length);
        goto cleanup;
    }
    
    if (strncmp(display->last_displayed_content, "hel", 3) != 0) {
        printf("FAIL: Displayed content should be 'hel', got '%.3s'\n", display->last_displayed_content);
        goto cleanup;
    }
    
    printf("PASS: Third character addition works\n");
    
    // Test 5: Backspace (should trigger incremental deletion)
    if (!lle_text_backspace(buffer)) {
        printf("FAIL: Failed to backspace\n");
        goto cleanup;
    }
    
    printf("Calling incremental update after backspace...\n");
    if (!lle_display_update_incremental(display)) {
        printf("FAIL: Backspace incremental update failed\n");
        goto cleanup;
    }
    
    if (display->last_displayed_length != 2) {
        printf("FAIL: After backspace, displayed length should be 2, got %zu\n", display->last_displayed_length);
        goto cleanup;
    }
    
    if (strncmp(display->last_displayed_content, "he", 2) != 0) {
        printf("FAIL: After backspace, displayed content should be 'he', got '%.2s'\n", display->last_displayed_content);
        goto cleanup;
    }
    
    printf("PASS: Backspace works\n");
    
    // Test 6: Complex change (paste operation) - should trigger controlled rewrite
    lle_text_buffer_clear(buffer);
    if (!lle_text_insert_string(buffer, "hello world")) {
        printf("FAIL: Failed to insert complex string\n");
        goto cleanup;
    }
    
    printf("Calling incremental update after complex change...\n");
    if (!lle_display_update_incremental(display)) {
        printf("FAIL: Complex change incremental update failed\n");
        goto cleanup;
    }
    
    if (display->last_displayed_length != 11) {
        printf("FAIL: After complex change, displayed length should be 11, got %zu\n", display->last_displayed_length);
        goto cleanup;
    }
    
    if (strncmp(display->last_displayed_content, "hello world", 11) != 0) {
        printf("FAIL: After complex change, displayed content should be 'hello world', got '%.11s'\n", display->last_displayed_content);
        goto cleanup;
    }
    
    printf("PASS: Complex change works\n");
    
    printf("All incremental update tests passed!\n");
    
    // Cleanup
    lle_display_destroy(display);
    lle_prompt_destroy(prompt);
    lle_terminal_cleanup(terminal);
    free(terminal);
    lle_text_buffer_destroy(buffer);
    
    return true;
    
cleanup:
    lle_display_destroy(display);
    lle_prompt_destroy(prompt);
    lle_terminal_cleanup(terminal);
    free(terminal);
    lle_text_buffer_destroy(buffer);
    return false;
}

/**
 * @brief Main test function
 */
int main(void) {
    printf("=== Character Duplication Fix Test ===\n\n");
    
    // Enable debug mode for detailed output
    setenv("LLE_DEBUG", "1", 1);
    
    bool success = test_incremental_character_updates();
    
    printf("\n=== Test Results ===\n");
    if (success) {
        printf("✓ All tests PASSED\n");
        printf("✓ Character duplication fix is working correctly\n");
        printf("✓ True incremental updates implemented successfully\n");
        return 0;
    } else {
        printf("✗ Tests FAILED\n");
        printf("✗ Character duplication fix needs more work\n");
        return 1;
    }
}