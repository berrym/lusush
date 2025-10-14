// src/lle/foundation/test/editor_advanced_test.c
//
// Tests for LLE Advanced Editor Operations

#include "../editor/editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_PASS 0
#define TEST_FAIL 1

// ANSI color codes
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

// Print test result
static void test_result(const char *test_name, int result) {
    tests_run++;
    if (result == TEST_PASS) {
        tests_passed++;
        printf("%s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, test_name);
    } else {
        printf("%s[FAIL]%s %s\n", COLOR_RED, COLOR_RESET, test_name);
    }
}

// Test: Word forward movement
static int test_word_forward(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert test text: "hello world test"
    lle_editor_insert_string(&editor, "hello world test", 16);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Move forward one word - should go to position 5 (after "hello")
    lle_editor_move_word_forward(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 6) {  // Position after space
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Move forward again - should go to position 11 (after "world")
    lle_editor_move_word_forward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 12) {  // Position after space
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Move forward again - should go to end (position 16)
    lle_editor_move_word_forward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 16) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Word backward movement
static int test_word_backward(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert test text: "hello world test"
    lle_editor_insert_string(&editor, "hello world test", 16);
    // Cursor is at end (position 16)
    
    // Move backward one word - should go to position 12 (start of "test")
    lle_editor_move_word_backward(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 12) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Move backward again - should go to position 6 (start of "world")
    lle_editor_move_word_backward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 6) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Move backward again - should go to position 0 (start of "hello")
    lle_editor_move_word_backward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Move to line start
static int test_line_start(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert multiline text
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    
    // Position in middle of second line
    lle_editor_set_cursor_pos(&editor, 18);  // "second l|ine"
    
    lle_editor_move_to_line_start(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 11) {  // Start of "second"
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Move to line end
static int test_line_end(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert multiline text
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    
    // Position in middle of second line
    lle_editor_set_cursor_pos(&editor, 15);  // "second| line"
    
    lle_editor_move_to_line_end(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 22) {  // End of "second line"
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Delete word before cursor
static int test_delete_word_before(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert test text: "hello world test"
    lle_editor_insert_string(&editor, "hello world test", 16);
    
    // Position at end of "world" (before space before "test")
    lle_editor_set_cursor_pos(&editor, 11);
    
    // Delete word before cursor - should delete "world"
    lle_editor_delete_word_before_cursor(&editor);
    
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    
    if (strcmp(buffer, "hello  test") != 0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Cursor should now be at position 6
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 6) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Delete word at cursor
static int test_delete_word_at_cursor(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert test text: "hello world test"
    lle_editor_insert_string(&editor, "hello world test", 16);
    
    // Position at start of "world"
    lle_editor_set_cursor_pos(&editor, 6);
    
    // Delete word at cursor - should delete "world"
    lle_editor_delete_word_at_cursor(&editor);
    
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    
    if (strcmp(buffer, "hello  test") != 0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Cursor should still be at position 6
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 6) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Kill line from cursor to end
static int test_kill_line(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert multiline text
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    
    // Position after "second " (before "line")
    lle_editor_set_cursor_pos(&editor, 18);  // "second |line"
    
    // Kill to end of line
    lle_editor_kill_line(&editor);
    
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    
    if (strcmp(buffer, "first line\nsecond \nthird") != 0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Cursor should stay at position 18
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 18) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Kill whole line
static int test_kill_whole_line(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert multiline text
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    
    // Position in middle of second line
    lle_editor_set_cursor_pos(&editor, 15);  // "second| line"
    
    // Kill whole line
    lle_editor_kill_whole_line(&editor);
    
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    
    if (strcmp(buffer, "first line\n\nthird") != 0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Cursor should be at start of line (position 11)
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 11) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Word movement with punctuation
static int test_word_punctuation(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert text with punctuation: "hello, world!"
    lle_editor_insert_string(&editor, "hello, world!", 13);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Move forward - should stop at punctuation
    lle_editor_move_word_forward(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 5) {  // After "hello"
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    // Move forward again - should skip punctuation and space
    lle_editor_move_word_forward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    if (pos != 7) {  // After ", "
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Delete word with multiple spaces
static int test_delete_word_spaces(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert text with multiple spaces: "hello    world"
    lle_editor_insert_string(&editor, "hello    world", 14);
    
    // Position after spaces
    lle_editor_set_cursor_pos(&editor, 9);  // Before "world"
    
    // Delete word before cursor - should delete all trailing spaces and "hello"
    lle_editor_delete_word_before_cursor(&editor);
    
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    
    if (strcmp(buffer, "world") != 0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Performance of word operations
static int test_word_performance(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Insert large text with many words
    const char *text = "The quick brown fox jumps over the lazy dog multiple times ";
    for (int i = 0; i < 100; i++) {
        lle_editor_insert_string(&editor, text, strlen(text));
    }
    
    // Perform many word movements
    lle_editor_set_cursor_pos(&editor, 0);
    for (int i = 0; i < 100; i++) {
        lle_editor_move_word_forward(&editor);
    }
    
    // Get performance metrics
    uint64_t op_count;
    double avg_time;
    lle_editor_get_metrics(&editor, &op_count, &avg_time);
    
    // Check that average time is reasonable (< 10 μs per operation)
    if (avg_time > 10.0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Edge case - empty buffer operations
static int test_empty_buffer_operations(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) return TEST_FAIL;
    
    // Try all operations on empty buffer
    lle_editor_move_word_forward(&editor);
    lle_editor_move_word_backward(&editor);
    lle_editor_delete_word_before_cursor(&editor);
    lle_editor_delete_word_at_cursor(&editor);
    lle_editor_kill_line(&editor);
    lle_editor_kill_whole_line(&editor);
    
    // All should succeed without crashing
    size_t size = lle_editor_get_size(&editor);
    if (size != 0) {
        lle_editor_cleanup(&editor);
        return TEST_FAIL;
    }
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

int main(void) {
    // Check if we have a TTY
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "This test requires a TTY (interactive terminal)\n");
        fprintf(stderr, "Run this test directly in a terminal, not through a pipe\n");
        return 1;
    }
    
    printf("Running LLE Advanced Editor Tests...\n\n");
    
    // Run all tests
    test_result("Word forward movement", test_word_forward());
    test_result("Word backward movement", test_word_backward());
    test_result("Move to line start", test_line_start());
    test_result("Move to line end", test_line_end());
    test_result("Delete word before cursor", test_delete_word_before());
    test_result("Delete word at cursor", test_delete_word_at_cursor());
    test_result("Kill line from cursor", test_kill_line());
    test_result("Kill whole line", test_kill_whole_line());
    test_result("Word movement with punctuation", test_word_punctuation());
    test_result("Delete word with multiple spaces", test_delete_word_spaces());
    test_result("Word operation performance", test_word_performance());
    test_result("Empty buffer edge cases", test_empty_buffer_operations());
    
    // Print summary
    printf("\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n%sALL TESTS PASSED%s\n", COLOR_GREEN, COLOR_RESET);
        return 0;
    } else {
        printf("\n%sSOME TESTS FAILED%s\n", COLOR_RED, COLOR_RESET);
        return 1;
    }
}
