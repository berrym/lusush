// src/lle/foundation/test/editor_kill_ring_test.c
//
// Tests for LLE Kill Ring Operations (Emacs-style)

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
static void test_result(const char *test_name, int passed) {
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("[PASS] %s\n", test_name);
    } else {
        printf("[FAIL] %s\n", test_name);
    }
}

// Test: Kill line and yank
static int test_kill_line_yank(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed with code %d\n", result);
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Insert text and kill from middle
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 6);  // After "hello "
    
    lle_editor_kill_line(&editor);
    
    // Verify text was deleted
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After kill_line", buffer, "hello ");
    
    // Move to end and yank
    lle_editor_set_cursor_pos(&editor, 6);
    lle_editor_yank(&editor);
    
    // Verify text was yanked
    memset(buffer, 0, sizeof(buffer));
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After yank", buffer, "hello world");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Kill whole line and yank
static int test_kill_whole_line_yank(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Insert multiline text
    lle_editor_insert_string(&editor, "first\nsecond\nthird", 18);
    lle_editor_set_cursor_pos(&editor, 8);  // In "second"
    
    lle_editor_kill_whole_line(&editor);
    
    // Verify line was deleted
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After kill_whole_line", buffer, "first\nthird");
    
    // Yank it back
    lle_editor_yank(&editor);
    
    memset(buffer, 0, sizeof(buffer));
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After yank", buffer, "first\nsecond\nthird");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Multiple kills accumulate in ring
static int test_multiple_kills_ring(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Kill multiple pieces of text
    lle_editor_insert_string(&editor, "first line", 10);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    lle_editor_insert_string(&editor, "second line", 11);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    lle_editor_insert_string(&editor, "third line", 10);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    // Yank should give us most recent kill
    lle_editor_yank(&editor);
    
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Most recent kill", buffer, "third line");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Yank-pop cycles through ring
static int test_yank_pop_cycling(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Kill three different texts
    lle_editor_insert_string(&editor, "AAA", 3);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    lle_editor_insert_string(&editor, "BBB", 3);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    lle_editor_insert_string(&editor, "CCC", 3);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    // Yank gives us "CCC" (most recent)
    lle_editor_yank(&editor);
    
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("First yank", buffer, "CCC");
    
    // Yank-pop should replace with "BBB"
    lle_editor_yank_pop(&editor);
    
    memset(buffer, 0, sizeof(buffer));
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After yank-pop 1", buffer, "BBB");
    
    // Another yank-pop should give "AAA"
    lle_editor_yank_pop(&editor);
    
    memset(buffer, 0, sizeof(buffer));
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After yank-pop 2", buffer, "AAA");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Kill region operation
static int test_kill_region(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Insert text and kill a region
    lle_editor_insert_string(&editor, "hello beautiful world", 21);
    
    // Kill "beautiful " (positions 6-16)
    lle_editor_kill_region(&editor, 6, 16);
    
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After kill_region", buffer, "hello world");
    
    // Yank it back
    lle_editor_set_cursor_pos(&editor, 6);
    lle_editor_yank(&editor);
    
    memset(buffer, 0, sizeof(buffer));
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("After yank region", buffer, "hello beautiful world");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Yank with no kills (edge case)
static int test_yank_empty_ring(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Try to yank with nothing in kill ring
    lle_editor_insert_string(&editor, "test", 4);
    lle_editor_yank(&editor);
    
    // Should be no-op
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Yank with empty ring", buffer, "test");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Yank-pop without preceding yank (edge case)
static int test_yank_pop_without_yank(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Kill some text
    lle_editor_insert_string(&editor, "test", 4);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    // Try yank-pop without yank first (should be no-op)
    lle_editor_yank_pop(&editor);
    
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Yank-pop without yank", buffer, "");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Kill ring with only one entry
static int test_yank_pop_single_entry(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Kill one piece of text
    lle_editor_insert_string(&editor, "only one", 8);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    // Yank it
    lle_editor_yank(&editor);
    
    // Yank-pop with only one entry should be no-op or cycle back to same
    lle_editor_yank_pop(&editor);
    
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Yank-pop single entry", buffer, "only one");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Kill line at end of line (kills newline)
static int test_kill_line_at_eol(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Insert multiline text
    lle_editor_insert_string(&editor, "line1\nline2\nline3", 17);
    lle_editor_set_cursor_pos(&editor, 5);  // At end of "line1"
    
    // Kill should remove newline
    lle_editor_kill_line(&editor);
    
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Kill at EOL", buffer, "line1line2\nline3");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Consecutive yanks at different positions
static int test_yank_multiple_positions(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        fflush(stderr);
        return TEST_FAIL;
    }
    
    // Kill text
    lle_editor_insert_string(&editor, "COPY", 4);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_kill_line(&editor);
    
    // Yank at position 0
    lle_editor_insert_string(&editor, "___", 3);
    lle_editor_set_cursor_pos(&editor, 0);
    lle_editor_yank(&editor);
    
    char buffer[256] = {0};
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("First yank position", buffer, "COPY___");
    
    // Yank at different position
    lle_editor_set_cursor_pos(&editor, 7);
    lle_editor_yank(&editor);
    
    memset(buffer, 0, sizeof(buffer));
    lle_buffer_get_contents(&editor.buffer, buffer, sizeof(buffer));
    ASSERT_STR_EQ("Second yank position", buffer, "COPY___COPY");
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

int main(void) {
    printf("=== LLE Kill Ring Tests ===\n\n");
    
    test_result("Kill line and yank", test_kill_line_yank());
    test_result("Kill whole line and yank", test_kill_whole_line_yank());
    test_result("Multiple kills accumulate", test_multiple_kills_ring());
    test_result("Yank-pop cycling", test_yank_pop_cycling());
    test_result("Kill region", test_kill_region());
    test_result("Yank with empty ring", test_yank_empty_ring());
    test_result("Yank-pop without yank", test_yank_pop_without_yank());
    test_result("Yank-pop single entry", test_yank_pop_single_entry());
    test_result("Kill line at EOL", test_kill_line_at_eol());
    test_result("Yank at multiple positions", test_yank_multiple_positions());
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\nAll tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed\n");
        return 1;
    }
}
