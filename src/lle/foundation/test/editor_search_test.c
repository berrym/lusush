// src/lle/foundation/test/editor_search_test.c
//
// Tests for LLE Incremental Search Operations

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

#define ASSERT_TRUE(name, condition) \
    if (!(condition)) { \
        fprintf(stderr, "  FAIL: %s - condition was false\n", name); \
        fflush(stderr); \
        lle_editor_cleanup(&editor); \
        return TEST_FAIL; \
    }

#define ASSERT_FALSE(name, condition) \
    if (condition) { \
        fprintf(stderr, "  FAIL: %s - condition was true\n", name); \
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

// Test: Start forward search
static int test_search_forward_start(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Start search
    lle_editor_search_forward(&editor);
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_TRUE("Search active", search->active);
    ASSERT_TRUE("Search forward", search->forward);
    ASSERT_EQ("Pattern length", search->pattern_len, 0);
    ASSERT_EQ("Start position", search->search_start_pos, 0);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Start backward search
static int test_search_backward_start(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 11);
    
    // Start backward search
    lle_editor_search_backward(&editor);
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_TRUE("Search active", search->active);
    ASSERT_FALSE("Search backward", search->forward);
    ASSERT_EQ("Pattern length", search->pattern_len, 0);
    ASSERT_EQ("Start position", search->search_start_pos, 11);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Forward search finds match
static int test_search_forward_find(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Search for "world"
    lle_editor_search_forward(&editor);
    lle_editor_search_add_char(&editor, 'w');
    lle_editor_search_add_char(&editor, 'o');
    lle_editor_search_add_char(&editor, 'r');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'd');
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_TRUE("Match found", search->found);
    ASSERT_EQ("Match start", search->match_start, 6);
    ASSERT_EQ("Match end", search->match_end, 11);
    ASSERT_EQ("Cursor moved", lle_editor_get_cursor_pos(&editor), 6);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Backward search finds match
static int test_search_backward_find(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 11);
    
    // Search backward for "hello"
    lle_editor_search_backward(&editor);
    lle_editor_search_add_char(&editor, 'h');
    lle_editor_search_add_char(&editor, 'e');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'o');
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_TRUE("Match found", search->found);
    ASSERT_EQ("Match start", search->match_start, 0);
    ASSERT_EQ("Match end", search->match_end, 5);
    ASSERT_EQ("Cursor moved", lle_editor_get_cursor_pos(&editor), 0);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Search not found
static int test_search_not_found(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Search for non-existent text
    lle_editor_search_forward(&editor);
    lle_editor_search_add_char(&editor, 'x');
    lle_editor_search_add_char(&editor, 'y');
    lle_editor_search_add_char(&editor, 'z');
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_FALSE("Match not found", search->found);
    ASSERT_EQ("Match count", search->match_count, 0);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Search backspace
static int test_search_backspace(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Search for "world" then backspace to "wor"
    lle_editor_search_forward(&editor);
    lle_editor_search_add_char(&editor, 'w');
    lle_editor_search_add_char(&editor, 'o');
    lle_editor_search_add_char(&editor, 'r');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'd');
    lle_editor_search_backspace(&editor);
    lle_editor_search_backspace(&editor);
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("Pattern length", search->pattern_len, 3);
    ASSERT_STR_EQ("Pattern", search->pattern, "wor");
    ASSERT_TRUE("Still found", search->found);
    ASSERT_EQ("Match start", search->match_start, 6);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Search next (multiple matches)
static int test_search_next(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "the cat and the dog", 19);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Search for "the" (appears twice)
    lle_editor_search_forward(&editor);
    lle_editor_search_add_char(&editor, 't');
    lle_editor_search_add_char(&editor, 'h');
    lle_editor_search_add_char(&editor, 'e');
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("First match", search->match_start, 0);
    
    // Find next
    lle_editor_search_next(&editor);
    search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("Second match", search->match_start, 12);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Search cancel
static int test_search_cancel(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Search for "world"
    lle_editor_search_forward(&editor);
    lle_editor_search_add_char(&editor, 'w');
    lle_editor_search_add_char(&editor, 'o');
    lle_editor_search_add_char(&editor, 'r');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'd');
    
    ASSERT_EQ("Cursor at match", lle_editor_get_cursor_pos(&editor), 6);
    
    // Cancel search
    lle_editor_search_cancel(&editor);
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_FALSE("Search not active", search->active);
    ASSERT_EQ("Cursor restored", lle_editor_get_cursor_pos(&editor), 0);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Search accept
static int test_search_accept(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world", 11);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Search for "world"
    lle_editor_search_forward(&editor);
    lle_editor_search_add_char(&editor, 'w');
    lle_editor_search_add_char(&editor, 'o');
    lle_editor_search_add_char(&editor, 'r');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'd');
    
    ASSERT_EQ("Cursor at match", lle_editor_get_cursor_pos(&editor), 6);
    
    // Accept search
    lle_editor_search_accept(&editor);
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_FALSE("Search not active", search->active);
    ASSERT_EQ("Cursor stays", lle_editor_get_cursor_pos(&editor), 6);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Incremental search (char by char)
static int test_incremental_search(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world wonderful", 21);
    lle_editor_set_cursor_pos(&editor, 0);
    
    // Incrementally search for "wor"
    lle_editor_search_forward(&editor);
    
    // Add 'w' - should find "world"
    lle_editor_search_add_char(&editor, 'w');
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("Found 'w' at world", search->match_start, 6);
    
    // Add 'o' - should still match "world"
    lle_editor_search_add_char(&editor, 'o');
    search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("Found 'wo' at world", search->match_start, 6);
    
    // Add 'r' - should still match "world"
    lle_editor_search_add_char(&editor, 'r');
    search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("Found 'wor' at world", search->match_start, 6);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test: Search wrap around
static int test_search_wrap(void) {
    lle_editor_t editor;
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "  FAIL: Editor init failed\n");
        return TEST_FAIL;
    }
    
    lle_editor_insert_string(&editor, "hello world hello", 17);
    lle_editor_set_cursor_pos(&editor, 8);  // Start in middle
    
    // Search forward for "hello" (should find second one first)
    lle_editor_search_forward(&editor);
    lle_editor_search_add_char(&editor, 'h');
    lle_editor_search_add_char(&editor, 'e');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'l');
    lle_editor_search_add_char(&editor, 'o');
    
    const lle_search_state_t *search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("Found second hello", search->match_start, 12);
    
    // Search next should wrap to first
    lle_editor_search_next(&editor);
    search = lle_editor_get_search_state(&editor);
    ASSERT_EQ("Wrapped to first hello", search->match_start, 0);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

int main(void) {
    printf("=== LLE Incremental Search Tests ===\n\n");
    
    test_result("Start forward search", test_search_forward_start());
    test_result("Start backward search", test_search_backward_start());
    test_result("Forward search finds match", test_search_forward_find());
    test_result("Backward search finds match", test_search_backward_find());
    test_result("Search not found", test_search_not_found());
    test_result("Search backspace", test_search_backspace());
    test_result("Search next", test_search_next());
    test_result("Search cancel", test_search_cancel());
    test_result("Search accept", test_search_accept());
    test_result("Incremental search", test_incremental_search());
    test_result("Search wrap around", test_search_wrap());
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\nAll tests passed!\n");
        return 0;
    } else {
        printf("\nSome tests failed\n");
        return 1;
    }
}
