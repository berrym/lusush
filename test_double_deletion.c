/**
 * @file test_double_deletion.c
 * @brief Test program to investigate double-deletion bug in backspace operations
 * 
 * This program creates controlled test scenarios to identify the root cause
 * of the double-deletion bug where single backspace operations delete two
 * characters instead of one during line wrap boundary crossing.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024-12-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

// LLE includes
#include "src/line_editor/text_buffer.h"
#include "src/line_editor/edit_commands.h"
#include "src/line_editor/display.h"
#include "src/line_editor/prompt.h"
#include "src/line_editor/terminal_manager.h"
#include "src/line_editor/geometry.h"

// Debug functionality
#include "debug_double_deletion.c"

// ============================================================================
// Test State Management
// ============================================================================

typedef struct {
    lle_text_buffer_t *buffer;
    lle_display_state_t *display;
    lle_prompt_t *prompt;
    lle_terminal_manager_t *terminal;
    lle_geometry_t geometry;
    bool initialized;
} test_state_t;

/**
 * @brief Create test state with controlled environment
 *
 * @param terminal_width Width to simulate
 * @param terminal_height Height to simulate
 * @return Test state or NULL on failure
 */
static test_state_t *create_test_state(size_t terminal_width, size_t terminal_height) {
    test_state_t *state = calloc(1, sizeof(test_state_t));
    if (!state) return NULL;
    
    // Create buffer
    state->buffer = lle_text_buffer_create(512);
    if (!state->buffer) {
        free(state);
        return NULL;
    }
    
    // Create terminal manager (NULL for testing)
    state->terminal = NULL; // Use NULL to avoid actual terminal I/O
    
    // Create prompt
    state->prompt = lle_prompt_create();
    if (state->prompt) {
        lle_prompt_set_text(state->prompt, "lusush$ ");
        lle_prompt_set_theme(state->prompt, LLE_THEME_DEFAULT);
    }
    
    // Set geometry
    state->geometry.width = terminal_width;
    state->geometry.height = terminal_height;
    
    // Create display state
    state->display = lle_display_create(state->buffer, state->prompt, state->terminal, &state->geometry);
    
    state->initialized = (state->display != NULL);
    
    return state;
}

/**
 * @brief Destroy test state and cleanup resources
 *
 * @param state Test state to destroy
 */
static void destroy_test_state(test_state_t *state) {
    if (!state) return;
    
    if (state->display) {
        lle_display_destroy(state->display);
    }
    
    if (state->prompt) {
        lle_prompt_destroy(state->prompt);
    }
    
    if (state->buffer) {
        lle_text_buffer_destroy(state->buffer);
    }
    
    free(state);
}

// ============================================================================
// Individual Test Cases
// ============================================================================

/**
 * @brief Test 1: Basic backspace operation (no boundary crossing)
 *
 * @return true if test passes
 */
static bool test_basic_backspace(void) {
    printf("\n=== TEST 1: Basic Backspace ===\n");
    
    test_state_t *state = create_test_state(80, 24);
    if (!state || !state->initialized) {
        printf("FAIL: Could not create test state\n");
        destroy_test_state(state);
        return false;
    }
    
    // Add some text
    const char *test_text = "hello world";
    for (size_t i = 0; i < strlen(test_text); i++) {
        lle_text_insert_char(state->buffer, test_text[i]);
    }
    
    size_t initial_length = state->buffer->length;
    printf("Initial: length=%zu, content='%s'\n", initial_length, state->buffer->buffer);
    
    // Perform backspace
    lle_command_result_t result = lle_cmd_backspace(state->display);
    
    size_t final_length = state->buffer->length;
    printf("Final: length=%zu, content='%s'\n", final_length, state->buffer->buffer);
    
    bool success = (result == LLE_CMD_SUCCESS) && (final_length == initial_length - 1);
    printf("Result: %s\n", success ? "PASS" : "FAIL");
    
    if (!success) {
        printf("Expected length: %zu, Actual length: %zu\n", initial_length - 1, final_length);
    }
    
    destroy_test_state(state);
    return success;
}

/**
 * @brief Test 2: Backspace at exact line wrap boundary
 *
 * @return true if test passes
 */
static bool test_boundary_backspace(void) {
    printf("\n=== TEST 2: Boundary Backspace ===\n");
    
    size_t terminal_width = 40; // Narrow terminal for easier boundary testing
    test_state_t *state = create_test_state(terminal_width, 24);
    if (!state || !state->initialized) {
        printf("FAIL: Could not create test state\n");
        destroy_test_state(state);
        return false;
    }
    
    // Calculate prompt width
    size_t prompt_width = state->prompt ? lle_prompt_get_last_line_width(state->prompt) : 8;
    size_t content_width = terminal_width - prompt_width;
    
    printf("Terminal width: %zu, Prompt width: %zu, Content width: %zu\n", 
           terminal_width, prompt_width, content_width);
    
    // Fill exactly to boundary
    for (size_t i = 0; i < content_width + 5; i++) { // Go 5 chars past boundary
        char c = 'a' + (i % 26);
        lle_text_insert_char(state->buffer, c);
    }
    
    size_t initial_length = state->buffer->length;
    printf("Initial: length=%zu, content='%s'\n", initial_length, state->buffer->buffer);
    printf("Cursor position: %zu (should cause wrapping)\n", state->buffer->cursor_pos);
    
    // Enable debug tracking
    lle_debug_enable();
    
    // Perform backspace at boundary
    lle_command_result_t result = lle_cmd_backspace(state->display);
    
    size_t final_length = state->buffer->length;
    printf("Final: length=%zu, content='%s'\n", final_length, state->buffer->buffer);
    
    // Check for double deletion
    size_t expected_length = initial_length > 0 ? initial_length - 1 : 0;
    size_t actual_deletion = initial_length - final_length;
    
    bool success = (result == LLE_CMD_SUCCESS) && (actual_deletion == 1);
    printf("Expected deletion: 1, Actual deletion: %zu\n", actual_deletion);
    printf("Result: %s\n", success ? "PASS" : "FAIL");
    
    if (!success) {
        printf("DOUBLE DELETION DETECTED!\n");
        lle_debug_print_summary();
    }
    
    lle_debug_disable();
    destroy_test_state(state);
    return success;
}

/**
 * @brief Test 3: Multiple backspaces across boundary
 *
 * @return true if test passes
 */
static bool test_multiple_boundary_backspaces(void) {
    printf("\n=== TEST 3: Multiple Boundary Backspaces ===\n");
    
    size_t terminal_width = 30;
    test_state_t *state = create_test_state(terminal_width, 24);
    if (!state || !state->initialized) {
        printf("FAIL: Could not create test state\n");
        destroy_test_state(state);
        return false;
    }
    
    // Create content that spans multiple lines
    const char *long_text = "This is a very long line that should definitely wrap around multiple times in our narrow terminal window for testing purposes";
    for (size_t i = 0; i < strlen(long_text) && i < 100; i++) {
        lle_text_insert_char(state->buffer, long_text[i]);
    }
    
    printf("Initial: length=%zu\n", state->buffer->length);
    printf("Content: '%s'\n", state->buffer->buffer);
    
    lle_debug_enable();
    
    bool all_successful = true;
    size_t expected_total_deletion = 0;
    size_t initial_total_length = state->buffer->length;
    
    // Perform 10 backspaces
    for (int i = 0; i < 10 && state->buffer->length > 0; i++) {
        size_t length_before = state->buffer->length;
        
        printf("\nBackspace %d: length_before=%zu\n", i + 1, length_before);
        
        lle_command_result_t result = lle_cmd_backspace(state->display);
        
        size_t length_after = state->buffer->length;
        size_t chars_deleted = length_before - length_after;
        
        printf("Backspace %d: length_after=%zu, deleted=%zu\n", i + 1, length_after, chars_deleted);
        
        if (result != LLE_CMD_SUCCESS || chars_deleted != 1) {
            printf("FAIL: Backspace %d deleted %zu chars (expected 1)\n", i + 1, chars_deleted);
            all_successful = false;
        }
        
        expected_total_deletion += 1;
    }
    
    size_t final_total_length = state->buffer->length;
    size_t actual_total_deletion = initial_total_length - final_total_length;
    
    printf("\nSummary:\n");
    printf("Expected total deletion: %zu\n", expected_total_deletion);
    printf("Actual total deletion: %zu\n", actual_total_deletion);
    printf("Difference: %zu\n", actual_total_deletion > expected_total_deletion ? 
           actual_total_deletion - expected_total_deletion : 0);
    
    bool success = all_successful && (actual_total_deletion == expected_total_deletion);
    printf("Result: %s\n", success ? "PASS" : "FAIL");
    
    if (!success) {
        lle_debug_print_summary();
    }
    
    lle_debug_disable();
    destroy_test_state(state);
    return success;
}

/**
 * @brief Test 4: Direct buffer function testing
 *
 * @return true if test passes
 */
static bool test_buffer_function_direct(void) {
    printf("\n=== TEST 4: Direct Buffer Function Testing ===\n");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(128);
    if (!buffer) {
        printf("FAIL: Could not create buffer\n");
        return false;
    }
    
    // Add test content
    const char *test_content = "0123456789abcdefghijklmnopqrstuvwxyz";
    for (size_t i = 0; i < strlen(test_content); i++) {
        lle_text_insert_char(buffer, test_content[i]);
    }
    
    printf("Initial buffer: length=%zu, content='%s'\n", buffer->length, buffer->buffer);
    
    lle_debug_enable();
    
    bool success = true;
    
    // Test direct lle_text_backspace calls
    for (int i = 0; i < 5; i++) {
        size_t length_before = buffer->length;
        
        bool result = lle_debug_text_backspace(buffer);
        
        size_t length_after = buffer->length;
        size_t chars_deleted = length_before - length_after;
        
        printf("Direct backspace %d: %s, deleted=%zu\n", i + 1, result ? "SUCCESS" : "FAIL", chars_deleted);
        
        if (!result || chars_deleted != 1) {
            printf("FAIL: Direct backspace %d deleted %zu chars (expected 1)\n", i + 1, chars_deleted);
            success = false;
        }
    }
    
    printf("Final buffer: length=%zu, content='%s'\n", buffer->length, buffer->buffer);
    
    if (!success) {
        lle_debug_print_summary();
    }
    
    lle_debug_disable();
    lle_text_buffer_destroy(buffer);
    
    printf("Result: %s\n", success ? "PASS" : "FAIL");
    return success;
}

// ============================================================================
// Main Test Runner
// ============================================================================

/**
 * @brief Run all double deletion tests
 *
 * @return true if all tests pass
 */
static bool run_all_tests(void) {
    printf("=== DOUBLE DELETION BUG INVESTIGATION ===\n");
    printf("Testing backspace operations for character deletion accuracy\n\n");
    
    bool all_passed = true;
    
    // Run individual tests
    if (!test_basic_backspace()) all_passed = false;
    if (!test_boundary_backspace()) all_passed = false;
    if (!test_multiple_boundary_backspaces()) all_passed = false;
    if (!test_buffer_function_direct()) all_passed = false;
    
    printf("\n=== FINAL RESULTS ===\n");
    printf("All tests: %s\n", all_passed ? "PASS" : "FAIL");
    
    if (!all_passed) {
        printf("\nDOUBLE DELETION BUG CONFIRMED\n");
        printf("Recommendation: Review buffer modification logic and display update interactions\n");
    } else {
        printf("\nNo double deletion detected in controlled tests\n");
        printf("Issue may be specific to interactive terminal environment\n");
    }
    
    return all_passed;
}

/**
 * @brief Show usage information
 */
static void show_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --verbose  Enable verbose debug output\n");
    printf("  -t, --test N   Run specific test (1-4)\n");
    printf("  -w, --width N  Set terminal width for testing (default: 80)\n");
    printf("\nTests:\n");
    printf("  1: Basic backspace operation\n");
    printf("  2: Backspace at line wrap boundary\n");
    printf("  3: Multiple backspaces across boundaries\n");
    printf("  4: Direct buffer function testing\n");
}

/**
 * @brief Main entry point
 */
int main(int argc, char *argv[]) {
    bool verbose = false;
    int specific_test = 0;
    size_t terminal_width = 80;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            if (i + 1 < argc) {
                specific_test = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) {
                terminal_width = (size_t)atoi(argv[++i]);
            }
        }
    }
    
    if (verbose) {
        printf("Verbose mode enabled\n");
        printf("Terminal width: %zu\n", terminal_width);
    }
    
    // Run smoke test first
    if (!lle_debug_smoke_test()) {
        printf("FAIL: Debug system smoke test failed\n");
        return 1;
    }
    
    bool success = false;
    
    // Run specific test or all tests
    if (specific_test > 0) {
        printf("Running test %d only\n\n", specific_test);
        switch (specific_test) {
            case 1: success = test_basic_backspace(); break;
            case 2: success = test_boundary_backspace(); break;
            case 3: success = test_multiple_boundary_backspaces(); break;
            case 4: success = test_buffer_function_direct(); break;
            default:
                printf("Invalid test number: %d\n", specific_test);
                show_usage(argv[0]);
                return 1;
        }
    } else {
        success = run_all_tests();
    }
    
    return success ? 0 : 1;
}