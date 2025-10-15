// src/lle/foundation/test/editor_history_integration_test.c
//
// Editor-History Integration Test Suite
//
// Tests the integration of the history system with the editor.

#include "../editor/editor.h"
#include "../history/history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Test result codes
#define TEST_PASS 0
#define TEST_FAIL 1

// Test macros
#define ASSERT_TRUE(msg, cond) \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s\n", msg); \
        return TEST_FAIL; \
    }

#define ASSERT_EQ(msg, val, expected) \
    if ((val) != (expected)) { \
        fprintf(stderr, "FAIL: %s (expected %d, got %d)\n", msg, (int)(expected), (int)(val)); \
        return TEST_FAIL; \
    }

#define ASSERT_STR_EQ(msg, val, expected) \
    if (strcmp((val), (expected)) != 0) { \
        fprintf(stderr, "FAIL: %s (expected \"%s\", got \"%s\")\n", msg, expected, val); \
        return TEST_FAIL; \
    }

// Test set history
static int test_set_history(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    if (result != LLE_EDITOR_OK) {
        fprintf(stderr, "Editor init failed: %d\n", result);
        return TEST_FAIL;
    }
    lle_history_init(&history, 100, NULL);
    
    lle_editor_set_history(&editor, &history);
    ASSERT_TRUE("History set", editor.history == &history);
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test accept line adds to history
static int test_accept_line(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Add some text
    lle_editor_insert_string(&editor, "test command", 12);
    
    // Accept line (add to history)
    int result = lle_editor_history_accept_line(&editor);
    ASSERT_EQ("Accept line result", result, LLE_EDITOR_OK);
    
    // Check history
    ASSERT_EQ("History count", lle_history_count(&history), 1);
    const lle_history_entry_t *entry = lle_history_get_recent(&history);
    ASSERT_STR_EQ("History entry", entry->line, "test command");
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test navigate to previous history
static int test_history_previous(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Add some history
    lle_history_add(&history, "first command");
    lle_history_add(&history, "second command");
    lle_history_add(&history, "third command");
    
    // Start with current line
    lle_editor_insert_string(&editor, "current", 7);
    
    // Navigate to previous (most recent history)
    int result = lle_editor_history_previous(&editor);
    ASSERT_EQ("History previous result", result, LLE_EDITOR_OK);
    
    // Check buffer has history entry
    char buf[256];
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("Buffer after previous", buf, "third command");
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test navigate through multiple history entries
static int test_history_navigation(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Add history
    lle_history_add(&history, "cmd1");
    lle_history_add(&history, "cmd2");
    lle_history_add(&history, "cmd3");
    
    char buf[256];
    
    // Navigate back
    lle_editor_history_previous(&editor);
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("First previous", buf, "cmd3");
    
    lle_editor_history_previous(&editor);
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("Second previous", buf, "cmd2");
    
    lle_editor_history_previous(&editor);
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("Third previous", buf, "cmd1");
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test navigate back and forward
static int test_history_back_and_forward(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Add history
    lle_history_add(&history, "cmd1");
    lle_history_add(&history, "cmd2");
    
    // Start with current line
    lle_editor_insert_string(&editor, "current", 7);
    
    char buf[256];
    
    // Go back
    lle_editor_history_previous(&editor);
    lle_editor_history_previous(&editor);
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("After going back", buf, "cmd1");
    
    // Go forward
    lle_editor_history_next(&editor);
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("After going forward", buf, "cmd2");
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test restore original line
static int test_restore_original(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Add history
    lle_history_add(&history, "old command");
    
    // Start with current line
    lle_editor_insert_string(&editor, "new text", 8);
    
    // Navigate to history
    lle_editor_history_previous(&editor);
    
    char buf[256];
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("Buffer after previous", buf, "old command");
    
    // Go forward to restore original
    lle_editor_history_next(&editor);
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("Restored original", buf, "new text");
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test cancel history navigation
static int test_cancel_navigation(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Add history
    lle_history_add(&history, "history entry");
    
    // Start with current line
    lle_editor_insert_string(&editor, "original", 8);
    
    // Navigate to history
    lle_editor_history_previous(&editor);
    
    // Cancel navigation
    int result = lle_editor_history_cancel(&editor);
    ASSERT_EQ("Cancel result", result, LLE_EDITOR_OK);
    
    // Should restore original
    char buf[256];
    lle_buffer_get_contents(&editor.buffer, buf, sizeof(buf));
    ASSERT_STR_EQ("Restored after cancel", buf, "original");
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test empty history
static int test_empty_history(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Try to navigate with empty history
    int result = lle_editor_history_previous(&editor);
    ASSERT_EQ("Previous on empty history", result, LLE_EDITOR_OK);
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test no history system set
static int test_no_history_system(void) {
    lle_editor_t editor;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    // Try operations without history system
    int result = lle_editor_history_previous(&editor);
    ASSERT_EQ("Previous without history", result, LLE_EDITOR_OK);
    
    result = lle_editor_history_accept_line(&editor);
    ASSERT_EQ("Accept without history", result, LLE_EDITOR_OK);
    
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test accept after navigation
static int test_accept_after_navigation(void) {
    lle_editor_t editor;
    lle_history_t history;
    
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_history_init(&history, 100, NULL);
    lle_editor_set_history(&editor, &history);
    
    // Add history
    lle_history_add(&history, "old command");
    
    // Navigate to history
    lle_editor_history_previous(&editor);
    
    // Accept (should add to history again)
    lle_editor_history_accept_line(&editor);
    
    // History should have 2 entries (duplicate allowed in this case)
    ASSERT_EQ("History count after accept", lle_history_count(&history), 2);
    
    lle_history_cleanup(&history);
    lle_editor_cleanup(&editor);
    return TEST_PASS;
}

// Test structure
typedef struct {
    const char *name;
    int (*func)(void);
} test_case_t;

// Test suite
static test_case_t tests[] = {
    {"Set history system", test_set_history},
    {"Accept line adds to history", test_accept_line},
    {"Navigate to previous history", test_history_previous},
    {"Navigate through history entries", test_history_navigation},
    {"Navigate back and forward", test_history_back_and_forward},
    {"Restore original line", test_restore_original},
    {"Cancel history navigation", test_cancel_navigation},
    {"Empty history", test_empty_history},
    {"No history system set", test_no_history_system},
    {"Accept after navigation", test_accept_after_navigation},
};

int main(void) {
    int total = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    
    printf("Running Editor-History Integration Test Suite (%d tests)\n", total);
    printf("========================================================\n\n");
    
    for (int i = 0; i < total; i++) {
        printf("Test %d: %s... ", i + 1, tests[i].name);
        fflush(stdout);
        
        int result = tests[i].func();
        if (result == TEST_PASS) {
            printf("PASS\n");
            passed++;
        } else {
            printf("FAIL\n");
        }
    }
    
    printf("\n========================================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    
    return (passed == total) ? 0 : 1;
}
