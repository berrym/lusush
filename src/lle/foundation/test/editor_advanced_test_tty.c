// src/lle/foundation/test/editor_advanced_test_v2.c
//
// Tests for LLE Advanced Editor Operations - with detailed error reporting

#include "../editor/editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_PASS 0
#define TEST_FAIL 1

#define ASSERT_EQ(name, actual, expected) \
    if ((actual) != (expected)) { \
        fprintf(stderr, "  FAIL: %s - got %zu, expected %zu\n", name, (size_t)(actual), (size_t)(expected)); \
        fflush(stderr); \
        lle_editor_cleanup(&editor); \
        return TEST_FAIL; \
    }

#define ASSERT_STR_EQ(name, actual, expected) \
    if (strcmp((actual), (expected)) != 0) { \
        fprintf(stderr, "  FAIL: %s - got '%s', expected '%s'\n", name, actual, expected); \
        fflush(stderr); \
        lle_editor_cleanup(&editor); \
        return TEST_FAIL; \
    }

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

// Print test result
static void test_result(const char *test_name, int result) {
    tests_run++;
    if (result == TEST_PASS) {  // TEST_PASS is 0, so check explicitly
        tests_passed++;
        printf("[PASS] %s\n", test_name);
    } else {
        printf("[FAIL] %s\n", test_name);
    }
}

// Test: Word backward movement
static int test_word_backward(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed with code %d (LLE_EDITOR_OK=%d)\n", result, LLE_EDITOR_OK);
        fflush(stderr);
        return TEST_FAIL;
    }
    
    printf("  Inserting 'hello world test' (16 chars)\n");
    lle_editor_insert_string(&editor, "hello world test", 16);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    printf("  After insert, cursor at: %zu\n", pos);
    
    printf("  Moving word backward (1st)...\n");
    lle_editor_move_word_backward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    printf("  Cursor at: %zu (expected 12)\n", pos);
    ASSERT_EQ("First backward", pos, 12);
    
    printf("  Moving word backward (2nd)...\n");
    lle_editor_move_word_backward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    printf("  Cursor at: %zu (expected 6)\n", pos);
    ASSERT_EQ("Second backward", pos, 6);
    
    printf("  Moving word backward (3rd)...\n");
    lle_editor_move_word_backward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    printf("  Cursor at: %zu (expected 0)\n", pos);
    ASSERT_EQ("Third backward", pos, 0);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Move to line start
static int test_line_start(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        printf("  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    lle_editor_set_cursor_pos(&editor, 18);
    
    lle_editor_move_to_line_start(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    ASSERT_EQ("Line start", pos, 11);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Delete word before cursor
static int test_delete_word_before(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        printf("  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world test", 16);
    lle_editor_set_cursor_pos(&editor, 11);
    
    lle_editor_delete_word_before_cursor(&editor);
    
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Content after delete", buffer, "hello  test");
    
    size_t pos = lle_editor_get_cursor_pos(&editor);
    ASSERT_EQ("Cursor position", pos, 6);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Kill whole line
static int test_kill_whole_line(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        printf("  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    lle_editor_set_cursor_pos(&editor, 15);
    
    lle_editor_kill_whole_line(&editor);
    
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Content after kill", buffer, "first line\n\nthird");
    
    size_t pos = lle_editor_get_cursor_pos(&editor);
    ASSERT_EQ("Cursor position", pos, 11);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Empty buffer operations
static int test_empty_buffer(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        printf("  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    // All should succeed without crashing
    lle_editor_move_word_forward(&editor);
    lle_editor_move_word_backward(&editor);
    lle_editor_delete_word_before_cursor(&editor);
    lle_editor_delete_word_at_cursor(&editor);
    lle_editor_kill_line(&editor);
    lle_editor_kill_whole_line(&editor);
    
    size_t size = lle_editor_get_size(&editor);
    ASSERT_EQ("Buffer size", size, 0);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

int main(void) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "This test requires a TTY (interactive terminal)\n");
        return 1;
    }
    
    printf("Running LLE Advanced Editor Tests (v2)...\n\n");
    
    // Run subset of tests with detailed error reporting
    test_result("Word backward movement", test_word_backward());
    test_result("Move to line start", test_line_start());
    test_result("Delete word before cursor", test_delete_word_before());
    test_result("Kill whole line", test_kill_whole_line());
    test_result("Empty buffer edge cases", test_empty_buffer());
    
    printf("\nTests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
