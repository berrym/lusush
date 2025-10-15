// src/lle/foundation/test/history_test.c
//
// History System Test Suite
//
// Tests command history storage, search, and persistence functionality.

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

// Test history initialization
static int test_history_init(void) {
    lle_history_t history;
    
    int result = lle_history_init(&history, 100, NULL);
    ASSERT_EQ("Init result", result, LLE_HISTORY_OK);
    ASSERT_EQ("Initial count", lle_history_count(&history), 0);
    ASSERT_EQ("Capacity", history.capacity, 100);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test adding entries
static int test_history_add(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    int result = lle_history_add(&history, "command1");
    ASSERT_EQ("Add result", result, LLE_HISTORY_OK);
    ASSERT_EQ("Count after add", lle_history_count(&history), 1);
    
    lle_history_add(&history, "command2");
    lle_history_add(&history, "command3");
    ASSERT_EQ("Count after 3 adds", lle_history_count(&history), 3);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test retrieving entries
static int test_history_get(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    lle_history_add(&history, "first");
    lle_history_add(&history, "second");
    lle_history_add(&history, "third");
    
    // Get most recent (index 0)
    const lle_history_entry_t *entry = lle_history_get(&history, 0);
    ASSERT_TRUE("Entry 0 exists", entry != NULL);
    ASSERT_STR_EQ("Entry 0 content", entry->line, "third");
    
    // Get middle (index 1)
    entry = lle_history_get(&history, 1);
    ASSERT_TRUE("Entry 1 exists", entry != NULL);
    ASSERT_STR_EQ("Entry 1 content", entry->line, "second");
    
    // Get oldest (index 2)
    entry = lle_history_get(&history, 2);
    ASSERT_TRUE("Entry 2 exists", entry != NULL);
    ASSERT_STR_EQ("Entry 2 content", entry->line, "first");
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test circular buffer wraparound
static int test_history_wraparound(void) {
    lle_history_t history;
    lle_history_init(&history, 3, NULL);  // Small capacity
    
    lle_history_add(&history, "cmd1");
    lle_history_add(&history, "cmd2");
    lle_history_add(&history, "cmd3");
    ASSERT_EQ("Count at capacity", lle_history_count(&history), 3);
    
    // Add one more, should overwrite oldest
    lle_history_add(&history, "cmd4");
    ASSERT_EQ("Count after wraparound", lle_history_count(&history), 3);
    
    // Most recent should be cmd4
    const lle_history_entry_t *entry = lle_history_get(&history, 0);
    ASSERT_STR_EQ("Most recent after wraparound", entry->line, "cmd4");
    
    // Oldest should be cmd2 (cmd1 was overwritten)
    entry = lle_history_get(&history, 2);
    ASSERT_STR_EQ("Oldest after wraparound", entry->line, "cmd2");
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test duplicate detection
static int test_history_duplicates(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    // Duplicates ignored by default
    lle_history_add(&history, "duplicate");
    lle_history_add(&history, "duplicate");
    ASSERT_EQ("Duplicate ignored", lle_history_count(&history), 1);
    
    // Different command should be added
    lle_history_add(&history, "different");
    ASSERT_EQ("Different command added", lle_history_count(&history), 2);
    
    // Disable duplicate detection
    lle_history_set_ignore_duplicates(&history, false);
    lle_history_add(&history, "repeat");
    lle_history_add(&history, "repeat");
    ASSERT_EQ("Duplicates allowed", lle_history_count(&history), 4);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test space-prefixed command ignoring
static int test_history_ignore_space(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    // Space-prefixed ignored by default
    lle_history_add(&history, " secret");
    ASSERT_EQ("Space-prefixed ignored", lle_history_count(&history), 0);
    
    // Normal command added
    lle_history_add(&history, "normal");
    ASSERT_EQ("Normal command added", lle_history_count(&history), 1);
    
    // Disable ignore space
    lle_history_set_ignore_space(&history, false);
    lle_history_add(&history, " visible");
    ASSERT_EQ("Space-prefixed allowed", lle_history_count(&history), 2);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test clear operation
static int test_history_clear(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    lle_history_add(&history, "cmd1");
    lle_history_add(&history, "cmd2");
    lle_history_add(&history, "cmd3");
    ASSERT_EQ("Count before clear", lle_history_count(&history), 3);
    
    int result = lle_history_clear(&history);
    ASSERT_EQ("Clear result", result, LLE_HISTORY_OK);
    ASSERT_EQ("Count after clear", lle_history_count(&history), 0);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test file persistence
static int test_history_persistence(void) {
    const char *temp_file = "/tmp/lle_history_test.txt";
    
    // Create history and save
    {
        lle_history_t history;
        lle_history_init(&history, 100, temp_file);
        
        lle_history_add(&history, "persistent1");
        lle_history_add(&history, "persistent2");
        lle_history_add(&history, "persistent3");
        
        int result = lle_history_save(&history);
        ASSERT_EQ("Save result", result, LLE_HISTORY_OK);
        
        lle_history_cleanup(&history);
    }
    
    // Load history and verify
    {
        lle_history_t history;
        lle_history_init(&history, 100, temp_file);
        
        int result = lle_history_load(&history);
        ASSERT_EQ("Load result", result, LLE_HISTORY_OK);
        ASSERT_EQ("Loaded count", lle_history_count(&history), 3);
        
        const lle_history_entry_t *entry = lle_history_get(&history, 0);
        ASSERT_STR_EQ("Loaded entry 1", entry->line, "persistent3");
        
        entry = lle_history_get(&history, 2);
        ASSERT_STR_EQ("Loaded entry 3", entry->line, "persistent1");
        
        lle_history_cleanup(&history);
    }
    
    // Cleanup temp file
    unlink(temp_file);
    
    return TEST_PASS;
}

// Test search functionality
static int test_history_search(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    lle_history_add(&history, "git status");
    lle_history_add(&history, "ls -la");
    lle_history_add(&history, "git commit");
    lle_history_add(&history, "git push");
    lle_history_add(&history, "cd /tmp");
    
    // Start search for "git"
    int result = lle_history_search_start(&history, "git");
    ASSERT_EQ("Search start", result, LLE_HISTORY_OK);
    
    // Find first match (most recent with "git")
    const lle_history_entry_t *entry = lle_history_search_next(&history);
    ASSERT_TRUE("First match found", entry != NULL);
    ASSERT_STR_EQ("First match", entry->line, "git push");
    
    // Find second match
    entry = lle_history_search_next(&history);
    ASSERT_TRUE("Second match found", entry != NULL);
    ASSERT_STR_EQ("Second match", entry->line, "git commit");
    
    // Find third match
    entry = lle_history_search_next(&history);
    ASSERT_TRUE("Third match found", entry != NULL);
    ASSERT_STR_EQ("Third match", entry->line, "git status");
    
    // No more matches
    entry = lle_history_search_next(&history);
    ASSERT_TRUE("No more matches", entry == NULL);
    
    lle_history_search_end(&history);
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test get recent
static int test_history_get_recent(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    // Empty history
    const lle_history_entry_t *entry = lle_history_get_recent(&history);
    ASSERT_TRUE("Empty history returns NULL", entry == NULL);
    
    // Add entries
    lle_history_add(&history, "first");
    lle_history_add(&history, "second");
    lle_history_add(&history, "third");
    
    // Get recent should return most recent
    entry = lle_history_get_recent(&history);
    ASSERT_TRUE("Recent exists", entry != NULL);
    ASSERT_STR_EQ("Recent is most recent", entry->line, "third");
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test global index tracking
static int test_history_global_index(void) {
    lle_history_t history;
    lle_history_init(&history, 3, NULL);  // Small capacity
    
    lle_history_add(&history, "cmd1");  // index 0
    lle_history_add(&history, "cmd2");  // index 1
    lle_history_add(&history, "cmd3");  // index 2
    lle_history_add(&history, "cmd4");  // index 3 (overwrites cmd1)
    
    // Get by global index
    const lle_history_entry_t *entry = lle_history_get_by_index(&history, 3);
    ASSERT_TRUE("Entry with index 3 found", entry != NULL);
    ASSERT_STR_EQ("Entry with index 3", entry->line, "cmd4");
    
    // Index 0 should be gone (overwritten)
    entry = lle_history_get_by_index(&history, 0);
    ASSERT_TRUE("Entry with index 0 gone", entry == NULL);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test empty line handling
static int test_history_empty_lines(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    // Empty lines should be ignored
    lle_history_add(&history, "");
    ASSERT_EQ("Empty line ignored", lle_history_count(&history), 0);
    
    lle_history_add(&history, "   ");  // Starts with space, also ignored
    ASSERT_EQ("Space-only line ignored", lle_history_count(&history), 0);
    
    lle_history_add(&history, "valid");
    ASSERT_EQ("Valid line added", lle_history_count(&history), 1);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test timestamp tracking
static int test_history_timestamps(void) {
    lle_history_t history;
    lle_history_init(&history, 100, NULL);
    
    time_t before = time(NULL);
    lle_history_add(&history, "command");
    time_t after = time(NULL);
    
    const lle_history_entry_t *entry = lle_history_get(&history, 0);
    ASSERT_TRUE("Entry has timestamp", entry->timestamp >= before && entry->timestamp <= after);
    
    lle_history_cleanup(&history);
    return TEST_PASS;
}

// Test structure
typedef struct {
    const char *name;
    int (*func)(void);
} test_case_t;

// Test suite
static test_case_t tests[] = {
    {"History initialization", test_history_init},
    {"Add entries", test_history_add},
    {"Get entries", test_history_get},
    {"Circular buffer wraparound", test_history_wraparound},
    {"Duplicate detection", test_history_duplicates},
    {"Ignore space-prefixed commands", test_history_ignore_space},
    {"Clear operation", test_history_clear},
    {"File persistence", test_history_persistence},
    {"Search functionality", test_history_search},
    {"Get recent entry", test_history_get_recent},
    {"Global index tracking", test_history_global_index},
    {"Empty line handling", test_history_empty_lines},
    {"Timestamp tracking", test_history_timestamps},
};

int main(void) {
    int total = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    
    printf("Running History System Test Suite (%d tests)\n", total);
    printf("================================================\n\n");
    
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
    
    printf("\n================================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    
    return (passed == total) ? 0 : 1;
}
