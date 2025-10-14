// src/lle/foundation/test/editor_test.c
//
// Tests for editor integration layer

#define _POSIX_C_SOURCE 200809L
#include "../editor/editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        tests_run++; \
        printf("Running test: %s...", #name); \
        fflush(stdout); \
        test_##name(); \
        tests_passed++; \
        printf(" PASS\n"); \
    } \
    static void test_##name(void)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("\n  FAIL: %s\n", message); \
            tests_failed++; \
            return; \
        } \
    } while (0)

// Test 1: Basic initialization
TEST(init_cleanup) {
    lle_editor_t editor;
    
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    ASSERT(result == LLE_EDITOR_OK, "Init should succeed");
    ASSERT(editor.initialized, "Should be initialized");
    ASSERT(editor.state.cursor_pos == 0, "Cursor should be at 0");
    
    lle_editor_cleanup(&editor);
    ASSERT(!editor.initialized, "Should not be initialized after cleanup");
}

// Test 2: Init with buffer
TEST(init_with_buffer) {
    lle_editor_t editor;
    const char *text = "Hello, World!";
    
    int result = lle_editor_init_with_buffer(&editor, STDIN_FILENO, STDOUT_FILENO,
                                             text, strlen(text));
    ASSERT(result == LLE_EDITOR_OK, "Init with buffer should succeed");
    ASSERT(lle_editor_get_size(&editor) == strlen(text), "Size should match");
    ASSERT(lle_editor_get_cursor_pos(&editor) == strlen(text), 
           "Cursor should be at end");
    
    char content[64];
    lle_editor_get_content(&editor, content, sizeof(content));
    ASSERT(strcmp(content, text) == 0, "Content should match");
    
    lle_editor_cleanup(&editor);
}

// Test 3: Insert character
TEST(insert_char) {
    lle_editor_t editor;
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    int result = lle_editor_insert_char(&editor, 'A');
    ASSERT(result == LLE_EDITOR_OK, "Insert should succeed");
    ASSERT(lle_editor_get_size(&editor) == 1, "Size should be 1");
    ASSERT(lle_editor_get_cursor_pos(&editor) == 1, "Cursor should be at 1");
    
    lle_editor_insert_char(&editor, 'B');
    lle_editor_insert_char(&editor, 'C');
    ASSERT(lle_editor_get_size(&editor) == 3, "Size should be 3");
    
    char content[64];
    lle_editor_get_content(&editor, content, sizeof(content));
    ASSERT(strcmp(content, "ABC") == 0, "Content should be ABC");
    
    lle_editor_cleanup(&editor);
}

// Test 4: Insert string
TEST(insert_string) {
    lle_editor_t editor;
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    const char *str = "Hello, World!";
    int result = lle_editor_insert_string(&editor, str, strlen(str));
    ASSERT(result == LLE_EDITOR_OK, "Insert string should succeed");
    ASSERT(lle_editor_get_size(&editor) == strlen(str), "Size should match");
    
    char content[64];
    lle_editor_get_content(&editor, content, sizeof(content));
    ASSERT(strcmp(content, str) == 0, "Content should match");
    
    lle_editor_cleanup(&editor);
}

// Test 5: Delete operations
TEST(delete_operations) {
    lle_editor_t editor;
    lle_editor_init_with_buffer(&editor, STDIN_FILENO, STDOUT_FILENO,
                                 "ABCDEF", 6);
    
    // Delete char before cursor (backspace from end)
    int result = lle_editor_delete_char_before_cursor(&editor);
    ASSERT(result == LLE_EDITOR_OK, "Delete should succeed");
    ASSERT(lle_editor_get_size(&editor) == 5, "Size should be 5");
    
    char content[64];
    lle_editor_get_content(&editor, content, sizeof(content));
    ASSERT(strcmp(content, "ABCDE") == 0, "Content should be ABCDE");
    
    // Move cursor to position 2, delete at cursor
    lle_editor_set_cursor_pos(&editor, 2);
    result = lle_editor_delete_char_at_cursor(&editor);
    ASSERT(result == LLE_EDITOR_OK, "Delete at cursor should succeed");
    ASSERT(lle_editor_get_size(&editor) == 4, "Size should be 4");
    
    lle_editor_get_content(&editor, content, sizeof(content));
    ASSERT(strcmp(content, "ABDE") == 0, "Content should be ABDE");
    
    lle_editor_cleanup(&editor);
}

// Test 6: Cursor movement
TEST(cursor_movement) {
    lle_editor_t editor;
    lle_editor_init_with_buffer(&editor, STDIN_FILENO, STDOUT_FILENO,
                                 "ABCDEF", 6);
    
    // Cursor should start at end
    ASSERT(lle_editor_get_cursor_pos(&editor) == 6, "Cursor should be at 6");
    
    // Move left
    lle_editor_move_cursor_left(&editor);
    ASSERT(lle_editor_get_cursor_pos(&editor) == 5, "Cursor should be at 5");
    
    lle_editor_move_cursor_left(&editor);
    lle_editor_move_cursor_left(&editor);
    ASSERT(lle_editor_get_cursor_pos(&editor) == 3, "Cursor should be at 3");
    
    // Move right
    lle_editor_move_cursor_right(&editor);
    ASSERT(lle_editor_get_cursor_pos(&editor) == 4, "Cursor should be at 4");
    
    // Move to start
    lle_editor_move_cursor_to_start(&editor);
    ASSERT(lle_editor_get_cursor_pos(&editor) == 0, "Cursor should be at 0");
    
    // Move to end
    lle_editor_move_cursor_to_end(&editor);
    ASSERT(lle_editor_get_cursor_pos(&editor) == 6, "Cursor should be at 6");
    
    lle_editor_cleanup(&editor);
}

// Test 7: Insert at various positions
TEST(insert_at_positions) {
    lle_editor_t editor;
    lle_editor_init_with_buffer(&editor, STDIN_FILENO, STDOUT_FILENO,
                                 "AC", 2);
    
    // Insert B in middle
    lle_editor_set_cursor_pos(&editor, 1);
    lle_editor_insert_char(&editor, 'B');
    
    char content[64];
    lle_editor_get_content(&editor, content, sizeof(content));
    ASSERT(strcmp(content, "ABC") == 0, "Content should be ABC");
    ASSERT(lle_editor_get_cursor_pos(&editor) == 2, "Cursor should be at 2");
    
    // Insert at start
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_insert_char(&editor, 'X');
    lle_editor_get_content(&editor, content, sizeof(content));
    ASSERT(strcmp(content, "XABC") == 0, "Content should be XABC");
    
    lle_editor_cleanup(&editor);
}

// Test 8: Clear editor
TEST(clear_editor) {
    lle_editor_t editor;
    lle_editor_init_with_buffer(&editor, STDIN_FILENO, STDOUT_FILENO,
                                 "Test content", 12);
    
    ASSERT(lle_editor_get_size(&editor) > 0, "Should have content");
    
    int result = lle_editor_clear(&editor);
    ASSERT(result == LLE_EDITOR_OK, "Clear should succeed");
    ASSERT(lle_editor_get_size(&editor) == 0, "Size should be 0");
    ASSERT(lle_editor_get_cursor_pos(&editor) == 0, "Cursor should be at 0");
    
    lle_editor_cleanup(&editor);
}

// Test 9: Modified flag
TEST(modified_flag) {
    lle_editor_t editor;
    lle_editor_init_with_buffer(&editor, STDIN_FILENO, STDOUT_FILENO,
                                 "Test", 4);
    
    ASSERT(!lle_editor_is_modified(&editor), "Should not be modified initially");
    
    lle_editor_insert_char(&editor, 'X');
    ASSERT(lle_editor_is_modified(&editor), "Should be modified after insert");
    
    lle_editor_cleanup(&editor);
}

// Test 10: Performance
TEST(performance) {
    lle_editor_t editor;
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    // Perform many operations
    const int iterations = 100;
    for (int i = 0; i < iterations; i++) {
        lle_editor_insert_char(&editor, 'A' + (i % 26));
    }
    
    ASSERT(lle_editor_get_size(&editor) == iterations, "Size should match");
    
    // Get metrics
    uint64_t op_count;
    double avg_time_us;
    lle_editor_get_metrics(&editor, &op_count, &avg_time_us);
    
    ASSERT(op_count == iterations, "Operation count should match");
    ASSERT(avg_time_us < 100.0, "Average time should be <100μs");
    
    printf("\n  Performance: %lu ops, avg %.3f μs",
           (unsigned long)op_count, avg_time_us);
    
    lle_editor_cleanup(&editor);
}

int main(void) {
    printf("LLE Editor Integration Tests\n");
    printf("============================\n\n");
    
    // Check if running in a terminal
    if (!isatty(STDIN_FILENO)) {
        printf("ERROR: Not running in a terminal (stdin not a tty)\n");
        printf("Please run this test in an interactive terminal.\n");
        return 1;
    }
    
    // Run all tests
    run_test_init_cleanup();
    run_test_init_with_buffer();
    run_test_insert_char();
    run_test_insert_string();
    run_test_delete_operations();
    run_test_cursor_movement();
    run_test_insert_at_positions();
    run_test_clear_editor();
    run_test_modified_flag();
    run_test_performance();
    
    // Summary
    printf("\n============================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed > 0) {
        printf("\nFAILURE: %d test(s) failed\n", tests_failed);
        return 1;
    }
    
    printf("\nSUCCESS: All tests passed\n");
    return 0;
}
