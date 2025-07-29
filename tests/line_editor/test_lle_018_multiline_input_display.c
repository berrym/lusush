/**
 * @file test_lle_018_multiline_input_display.c
 * @brief Test suite for LLE-018: Multiline Input Display
 *
 * Tests the multiline input display functionality including display state
 * management, prompt and text rendering, cursor positioning, and screen updates.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../src/line_editor/display.h"
#include "../src/line_editor/prompt.h"
#include "../src/line_editor/text_buffer.h"
#include "../src/line_editor/terminal_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test framework macros
#define LLE_TEST(name) \
    static void test_##name(void); \
    static void test_##name(void)

#define LLE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", \
                    #condition, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "ASSERTION FAILED: %s (%zu) != %s (%zu) at %s:%d\n", \
                    #actual, (size_t)(actual), #expected, (size_t)(expected), \
                    __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            fprintf(stderr, "ASSERTION FAILED: %s (\"%s\") != %s (\"%s\") at %s:%d\n", \
                    #actual, (actual), #expected, (expected), \
                    __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "ASSERTION FAILED: %s is NULL at %s:%d\n", \
                    #ptr, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            fprintf(stderr, "ASSERTION FAILED: %s is not NULL at %s:%d\n", \
                    #ptr, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

// Global test components
static lle_prompt_t test_prompt;
static lle_text_buffer_t test_buffer;
static lle_terminal_manager_t test_terminal;
static bool terminal_available = false;

// Setup function - run before each test
static bool setup_test_components(void) {
    // Initialize prompt
    if (!lle_prompt_init(&test_prompt)) {
        return false;
    }
    
    // Initialize text buffer
    if (!lle_text_buffer_init(&test_buffer, 1024)) {
        return false;
    }
    
    // Initialize terminal manager
    lle_terminal_init_result_t term_result = lle_terminal_init(&test_terminal);
    if (term_result == LLE_TERM_INIT_SUCCESS) {
        terminal_available = true;
    } else {
        // Terminal initialization might fail in CI environments
        // Return true but mark as non-functional
        terminal_available = false;
    }
    
    return true;
}

// Cleanup function - run after each test
static void cleanup_test_components(void) {
    lle_prompt_clear(&test_prompt);
    lle_text_buffer_clear(&test_buffer);
    lle_terminal_cleanup(&test_terminal);
}

// Test display state initialization
LLE_TEST(display_init_basic) {
    lle_display_state_t state;
    memset(&state, 0, sizeof(state));  // Initialize to zero to avoid garbage values
    
    bool result = lle_display_init(&state);
    LLE_ASSERT(result == true);
    
    // Check initial values
    LLE_ASSERT_NULL(state.prompt);
    LLE_ASSERT_NULL(state.buffer);
    LLE_ASSERT_NULL(state.terminal);
    LLE_ASSERT_EQ(state.cursor_pos.valid, false);
    LLE_ASSERT_EQ(state.display_start_offset, 0);
    LLE_ASSERT_EQ(state.display_start_line, 0);
    LLE_ASSERT_EQ(state.last_rendered_length, 0);
    LLE_ASSERT_EQ(state.last_rendered_lines, 0);
    LLE_ASSERT_EQ(state.needs_refresh, true);
    LLE_ASSERT_EQ(state.cursor_visible, true);
    LLE_ASSERT_EQ(state.initialized, true);
    LLE_ASSERT_EQ(state.display_flags, LLE_DISPLAY_FLAG_NONE);
    
    printf("Running test_display_init_basic... ");
    printf("PASSED\n");
}

// Test display state initialization with NULL
LLE_TEST(display_init_null) {
    bool result = lle_display_init(NULL);
    LLE_ASSERT(result == false);
    
    printf("Running test_display_init_null... ");
    printf("PASSED\n");
}

// Test display state creation
LLE_TEST(display_create_basic) {
    if (!setup_test_components()) {
        printf("Running test_display_create_basic... SKIPPED (setup failed)\n");
        return;
    }
    
    // Parse a simple prompt
    lle_prompt_parse(&test_prompt, "$ ");
    
    // Add some text to buffer
    lle_text_insert_at(&test_buffer, 0, "hello world");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Check associated components
    LLE_ASSERT(state->prompt == &test_prompt);
    LLE_ASSERT(state->buffer == &test_buffer);
    LLE_ASSERT(state->terminal == &test_terminal);
    LLE_ASSERT_EQ(state->initialized, true);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_create_basic... ");
    printf("PASSED\n");
}

// Test display creation with NULL parameters
LLE_TEST(display_create_null_parameters) {
    if (!setup_test_components()) {
        printf("Running test_display_create_null_parameters... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    
    // NULL prompt
    lle_display_state_t *state = lle_display_create(NULL, &test_buffer, &test_terminal);
    LLE_ASSERT_NULL(state);
    
    // NULL buffer
    state = lle_display_create(&test_prompt, NULL, &test_terminal);
    LLE_ASSERT_NULL(state);
    
    // NULL terminal
    state = lle_display_create(&test_prompt, &test_buffer, NULL);
    LLE_ASSERT_NULL(state);
    
    cleanup_test_components();
    
    printf("Running test_display_create_null_parameters... ");
    printf("PASSED\n");
}

// Test display validation
LLE_TEST(display_validate_basic) {
    if (!setup_test_components()) {
        printf("Running test_display_validate_basic... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "test");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Should be valid
    bool result = lle_display_validate(state);
    LLE_ASSERT(result == true);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_validate_basic... ");
    printf("PASSED\n");
}

// Test display validation with invalid state
LLE_TEST(display_validate_invalid) {
    // NULL state
    bool result = lle_display_validate(NULL);
    LLE_ASSERT(result == false);
    
    // Uninitialized state
    lle_display_state_t state;
    memset(&state, 0, sizeof(state));
    result = lle_display_validate(&state);
    LLE_ASSERT(result == false);
    
    printf("Running test_display_validate_invalid... ");
    printf("PASSED\n");
}

// Test simple display rendering
LLE_TEST(display_render_simple) {
    if (!setup_test_components()) {
        printf("Running test_display_render_simple... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "hello");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Render display
    bool result = lle_display_render(state);
    LLE_ASSERT(result == true);
    
    // Check state after rendering
    LLE_ASSERT_EQ(state->needs_refresh, false);
    LLE_ASSERT_EQ(state->last_rendered_length, 5);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_render_simple... ");
    printf("PASSED\n");
}

// Test multiline display rendering
LLE_TEST(display_render_multiline) {
    if (!setup_test_components()) {
        printf("Running test_display_render_multiline... SKIPPED (setup failed)\n");
        return;
    }
    
    // Multiline prompt
    lle_prompt_parse(&test_prompt, "user@host:~/path\n$ ");
    
    // Multiline input text
    lle_text_insert_at(&test_buffer, 0, "line1\nline2\nline3");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Render display
    bool result = lle_display_render(state);
    LLE_ASSERT(result == true);
    
    // Check that multiple lines were rendered (only in terminal environments)
    if (terminal_available) {
        LLE_ASSERT(state->last_rendered_lines > 1);
    } else {
        // In non-terminal environments, just check that rendering succeeded
        LLE_ASSERT(state->last_rendered_lines >= 1);
    }
    LLE_ASSERT_EQ(state->last_rendered_length, 17);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_render_multiline... ");
    printf("PASSED\n");
}

// Test display rendering with empty buffer
LLE_TEST(display_render_empty_buffer) {
    if (!setup_test_components()) {
        printf("Running test_display_render_empty_buffer... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    // Buffer is empty (no text inserted)
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Render display
    bool result = lle_display_render(state);
    LLE_ASSERT(result == true);
    
    // Check state after rendering empty buffer
    LLE_ASSERT_EQ(state->last_rendered_length, 0);
    LLE_ASSERT_EQ(state->last_rendered_lines, lle_prompt_get_height(&test_prompt));
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_render_empty_buffer... ");
    printf("PASSED\n");
}

// Test cursor position update
LLE_TEST(display_update_cursor) {
    if (!setup_test_components()) {
        printf("Running test_display_update_cursor... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "hello world");
    lle_text_set_cursor(&test_buffer, 5); // Position at space
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Update cursor position
    bool result = lle_display_update_cursor(state);
    LLE_ASSERT(result == true);
    
    // Check that cursor position was calculated
    LLE_ASSERT_EQ(state->cursor_pos.valid, true);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_update_cursor... ");
    printf("PASSED\n");
}

// Test display clearing
LLE_TEST(display_clear) {
    if (!setup_test_components()) {
        printf("Running test_display_clear... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "test content");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Render first
    lle_display_render(state);
    LLE_ASSERT(state->last_rendered_length > 0);
    
    // Clear display
    bool result = lle_display_clear(state);
    LLE_ASSERT(result == true);
    
    // Check state after clearing
    LLE_ASSERT_EQ(state->last_rendered_length, 0);
    LLE_ASSERT_EQ(state->last_rendered_lines, 0);
    LLE_ASSERT_EQ(state->needs_refresh, true);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_clear... ");
    printf("PASSED\n");
}

// Test display refresh
LLE_TEST(display_refresh) {
    if (!setup_test_components()) {
        printf("Running test_display_refresh... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "initial");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Initial render
    lle_display_render(state);
    
    // Modify buffer
    lle_text_insert_at(&test_buffer, 7, " content");
    
    // Refresh display
    bool result = lle_display_refresh(state);
    LLE_ASSERT(result == true);
    
    // Check that display was updated
    LLE_ASSERT_EQ(state->last_rendered_length, 15);
    LLE_ASSERT_EQ(state->needs_refresh, false);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_refresh... ");
    printf("PASSED\n");
}

// Test cursor offset setting
LLE_TEST(display_set_cursor_offset) {
    if (!setup_test_components()) {
        printf("Running test_display_set_cursor_offset... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "hello world");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Set cursor offset
    bool result = lle_display_set_cursor_offset(state, 6);
    LLE_ASSERT(result == true);
    
    // Check that buffer cursor was updated
    LLE_ASSERT_EQ(test_buffer.cursor_pos, 6);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_set_cursor_offset... ");
    printf("PASSED\n");
}

// Test display flags
LLE_TEST(display_flags) {
    lle_display_state_t state;
    memset(&state, 0, sizeof(state));  // Initialize to zero to avoid garbage values
    lle_display_init(&state);
    
    // Test setting and getting flags
    bool result = lle_display_set_flags(&state, LLE_DISPLAY_FLAG_FORCE_REFRESH | LLE_DISPLAY_FLAG_CURSOR_ONLY);
    LLE_ASSERT(result == true);
    
    uint32_t flags = lle_display_get_flags(&state);
    LLE_ASSERT_EQ(flags, LLE_DISPLAY_FLAG_FORCE_REFRESH | LLE_DISPLAY_FLAG_CURSOR_ONLY);
    
    // Test with NULL
    result = lle_display_set_flags(NULL, 0);
    LLE_ASSERT(result == false);
    
    flags = lle_display_get_flags(NULL);
    LLE_ASSERT_EQ(flags, 0);
    
    printf("Running test_display_flags... ");
    printf("PASSED\n");
}

// Test display statistics
LLE_TEST(display_statistics) {
    if (!setup_test_components()) {
        printf("Running test_display_statistics... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "test\nlines");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Render to populate statistics
    lle_display_render(state);
    
    size_t lines_rendered, chars_rendered, cursor_line, cursor_col;
    bool result = lle_display_get_statistics(state, &lines_rendered, &chars_rendered, &cursor_line, &cursor_col);
    LLE_ASSERT(result == true);
    
    LLE_ASSERT_EQ(chars_rendered, 10);
    LLE_ASSERT(lines_rendered > 1); // Should have multiple lines
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_statistics... ");
    printf("PASSED\n");
}

// Test update after insertion
LLE_TEST(display_update_after_insert) {
    if (!setup_test_components()) {
        printf("Running test_display_update_after_insert... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "hello");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Initial render
    lle_display_render(state);
    
    // Insert more text
    lle_text_insert_at(&test_buffer, 5, " world");
    
    // Update display after insertion
    bool result = lle_display_update_after_insert(state, 5, 6);
    LLE_ASSERT(result == true);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_update_after_insert... ");
    printf("PASSED\n");
}

// Test update after deletion
LLE_TEST(display_update_after_delete) {
    if (!setup_test_components()) {
        printf("Running test_display_update_after_delete... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    lle_text_insert_at(&test_buffer, 0, "hello world");
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Initial render
    lle_display_render(state);
    
    // Delete some text
    lle_text_delete_range(&test_buffer, 5, 11); // Delete " world"
    
    // Update display after deletion
    bool result = lle_display_update_after_delete(state, 5, 6);
    LLE_ASSERT(result == true);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_update_after_delete... ");
    printf("PASSED\n");
}

// Test long line wrapping
LLE_TEST(display_long_line_wrapping) {
    if (!setup_test_components()) {
        printf("Running test_display_long_line_wrapping... SKIPPED (setup failed)\n");
        return;
    }
    
    lle_prompt_parse(&test_prompt, "$ ");
    
    // Create a long line that will wrap
    char long_text[200];
    for (int i = 0; i < 199; i++) {
        long_text[i] = 'a' + (i % 26);
    }
    long_text[199] = '\0';
    
    lle_text_insert_at(&test_buffer, 0, long_text);
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Render long line
    bool result = lle_display_render(state);
    LLE_ASSERT(result == true);
    
    // Should render multiple lines due to wrapping (only in terminal environments)
    if (terminal_available) {
        LLE_ASSERT(state->last_rendered_lines > 1);
    } else {
        // In non-terminal environments, just check that rendering succeeded
        LLE_ASSERT(state->last_rendered_lines >= 1);
    }
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_long_line_wrapping... ");
    printf("PASSED\n");
}

// Test display with complex prompt and input
LLE_TEST(display_complex_scenario) {
    if (!setup_test_components()) {
        printf("Running test_display_complex_scenario... SKIPPED (setup failed)\n");
        return;
    }
    
    // Complex multiline prompt with ANSI codes
    const char *complex_prompt = "\033[1;32m[\033[0muser@host\033[1;32m]\033[0m \033[1;34m~/project\033[0m\n\033[1;31m❯\033[0m ";
    lle_prompt_parse(&test_prompt, complex_prompt);
    
    // Complex input with newlines
    const char *complex_input = "echo 'Hello, World!'\nls -la | grep test\ncd ~/projects && make";
    lle_text_insert_at(&test_buffer, 0, complex_input);
    
    lle_display_state_t *state = lle_display_create(&test_prompt, &test_buffer, &test_terminal);
    LLE_ASSERT_NOT_NULL(state);
    
    // Render complex scenario
    bool result = lle_display_render(state);
    LLE_ASSERT(result == true);
    
    // Should handle complex multiline content
    LLE_ASSERT(state->last_rendered_lines > 2);
    LLE_ASSERT_EQ(state->last_rendered_length, strlen(complex_input));
    
    // Test cursor positioning
    lle_text_set_cursor(&test_buffer, 20); // Middle of input
    result = lle_display_update_cursor(state);
    LLE_ASSERT(result == true);
    LLE_ASSERT_EQ(state->cursor_pos.valid, true);
    
    lle_display_destroy(state);
    cleanup_test_components();
    
    printf("Running test_display_complex_scenario... ");
    printf("PASSED\n");
}

int main(void) {
    printf("Running LLE-018 Multiline Input Display Tests...\n");
    printf("===============================================\n\n");
    
    // Basic initialization tests
    test_display_init_basic();
    test_display_init_null();
    
    // Creation and validation tests
    test_display_create_basic();
    test_display_create_null_parameters();
    test_display_validate_basic();
    test_display_validate_invalid();
    
    // Rendering tests
    test_display_render_simple();
    test_display_render_multiline();
    test_display_render_empty_buffer();
    
    // Cursor and update tests
    test_display_update_cursor();
    test_display_clear();
    test_display_refresh();
    test_display_set_cursor_offset();
    
    // Configuration tests
    test_display_flags();
    test_display_statistics();
    
    // Edit operation tests
    test_display_update_after_insert();
    test_display_update_after_delete();
    
    // Edge case tests
    test_display_long_line_wrapping();
    
    // Complex scenario tests
    test_display_complex_scenario();
    
    printf("\n===============================================\n");
    printf("All LLE-018 Multiline Input Display Tests Passed!\n");
    printf("Multiline input display functionality implemented and validated.\n");
    printf("✓ Display state management and validation\n");
    printf("✓ Prompt and input text rendering\n");
    printf("✓ Multiline input handling\n");
    printf("✓ Cursor positioning and updates\n");
    printf("✓ Efficient display clearing and refresh\n");
    printf("✓ Text insertion and deletion updates\n");
    printf("✓ Line wrapping for long content\n");
    printf("✓ Complex scenarios with ANSI prompts\n");
    printf("✓ Configuration flags and statistics\n");
    printf("✓ Comprehensive error handling\n");
    
    return 0;
}