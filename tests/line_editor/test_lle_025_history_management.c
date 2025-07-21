/**
 * @file test_lle_025_history_management.c
 * @brief Tests for LLE-025 History Management (save/load functionality)
 * 
 * This file contains comprehensive tests for the history management functions
 * including file save/load operations, error handling, and edge cases.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "../src/line_editor/lle_history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Test file paths
#define TEST_HISTORY_FILE "test_history.txt"
#define TEST_HISTORY_FILE_2 "test_history_2.txt"
#define TEST_INVALID_FILE "/invalid/path/history.txt"
#define TEST_EMPTY_FILE "test_empty_history.txt"

// Helper function to create test file with content
static bool create_test_file(const char *filename, const char *content) {
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    if (content && fprintf(file, "%s", content) < 0) {
        fclose(file);
        return false;
    }
    
    return fclose(file) == 0;
}

// Helper function to read file content
static char *read_file_content(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    
    fclose(file);
    return content;
}

// Helper function to clean up test files
static void cleanup_test_files(void) {
    unlink(TEST_HISTORY_FILE);
    unlink(TEST_HISTORY_FILE_2);
    unlink(TEST_EMPTY_FILE);
}

// Test basic save functionality
LLE_TEST(history_save_basic) {
    printf("Testing basic history save functionality... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Add some test entries
    LLE_ASSERT(lle_history_add(history, "echo hello", false));
    LLE_ASSERT(lle_history_add(history, "ls -la", false));
    LLE_ASSERT(lle_history_add(history, "cd /tmp", false));
    
    // Save to file
    LLE_ASSERT(lle_history_save(history, TEST_HISTORY_FILE));
    
    // Verify file exists
    struct stat st;
    LLE_ASSERT(stat(TEST_HISTORY_FILE, &st) == 0);
    
    // Read and verify content
    char *content = read_file_content(TEST_HISTORY_FILE);
    LLE_ASSERT(content != NULL);
    LLE_ASSERT(strstr(content, "echo hello") != NULL);
    LLE_ASSERT(strstr(content, "ls -la") != NULL);
    LLE_ASSERT(strstr(content, "cd /tmp") != NULL);
    
    free(content);
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test save with timestamps
LLE_TEST(history_save_with_timestamps) {
    printf("Testing history save with timestamp metadata... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Add entries
    LLE_ASSERT(lle_history_add(history, "command1", false));
    LLE_ASSERT(lle_history_add(history, "command2", false));
    
    // Save to file
    LLE_ASSERT(lle_history_save(history, TEST_HISTORY_FILE));
    
    // Read content and verify timestamp format
    char *content = read_file_content(TEST_HISTORY_FILE);
    LLE_ASSERT(content != NULL);
    
    // Check for timestamp format (#timestamp:command)
    LLE_ASSERT(content[0] == '#');
    LLE_ASSERT(strstr(content, ":command1\n") != NULL);
    LLE_ASSERT(strstr(content, ":command2\n") != NULL);
    
    free(content);
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test save empty history
LLE_TEST(history_save_empty) {
    printf("Testing save of empty history... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Save empty history
    LLE_ASSERT(lle_history_save(history, TEST_HISTORY_FILE));
    
    // Verify file exists but is empty
    struct stat st;
    LLE_ASSERT(stat(TEST_HISTORY_FILE, &st) == 0);
    LLE_ASSERT(st.st_size == 0);
    
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test save error conditions
LLE_TEST(history_save_error_conditions) {
    printf("Testing history save error conditions... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Test NULL parameters
    LLE_ASSERT(!lle_history_save(NULL, TEST_HISTORY_FILE));
    LLE_ASSERT(!lle_history_save(history, NULL));
    
    // Test invalid file path
    LLE_ASSERT(!lle_history_save(history, TEST_INVALID_FILE));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test basic load functionality
LLE_TEST(history_load_basic) {
    printf("Testing basic history load functionality... ");
    
    // Create test file with content
    const char *test_content = "#1234567890:echo hello\n#1234567891:ls -la\n#1234567892:cd /tmp\n";
    LLE_ASSERT(create_test_file(TEST_HISTORY_FILE, test_content));
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Load from file
    LLE_ASSERT(lle_history_load(history, TEST_HISTORY_FILE, false));
    
    // Verify entries were loaded
    LLE_ASSERT(lle_history_size(history) == 3);
    
    const lle_history_entry_t *entry0 = lle_history_get(history, 0);
    const lle_history_entry_t *entry1 = lle_history_get(history, 1);
    const lle_history_entry_t *entry2 = lle_history_get(history, 2);
    
    LLE_ASSERT(entry0 != NULL && strcmp(entry0->command, "echo hello") == 0);
    LLE_ASSERT(entry1 != NULL && strcmp(entry1->command, "ls -la") == 0);
    LLE_ASSERT(entry2 != NULL && strcmp(entry2->command, "cd /tmp") == 0);
    
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test load without clearing existing
LLE_TEST(history_load_preserve_existing) {
    printf("Testing history load preserving existing entries... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Add existing entries
    LLE_ASSERT(lle_history_add(history, "existing1", false));
    LLE_ASSERT(lle_history_add(history, "existing2", false));
    
    // Create test file
    const char *test_content = "#1234567890:loaded1\n#1234567891:loaded2\n";
    LLE_ASSERT(create_test_file(TEST_HISTORY_FILE, test_content));
    
    // Load without clearing
    LLE_ASSERT(lle_history_load(history, TEST_HISTORY_FILE, false));
    
    // Verify we have both existing and loaded entries
    LLE_ASSERT(lle_history_size(history) == 4);
    
    const lle_history_entry_t *entry0 = lle_history_get(history, 0);
    const lle_history_entry_t *entry1 = lle_history_get(history, 1);
    const lle_history_entry_t *entry2 = lle_history_get(history, 2);
    const lle_history_entry_t *entry3 = lle_history_get(history, 3);
    
    LLE_ASSERT(entry0 != NULL && strcmp(entry0->command, "existing1") == 0);
    LLE_ASSERT(entry1 != NULL && strcmp(entry1->command, "existing2") == 0);
    LLE_ASSERT(entry2 != NULL && strcmp(entry2->command, "loaded1") == 0);
    LLE_ASSERT(entry3 != NULL && strcmp(entry3->command, "loaded2") == 0);
    
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test load with clearing existing
LLE_TEST(history_load_clear_existing) {
    printf("Testing history load with clearing existing entries... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Add existing entries
    LLE_ASSERT(lle_history_add(history, "existing1", false));
    LLE_ASSERT(lle_history_add(history, "existing2", false));
    LLE_ASSERT(lle_history_size(history) == 2);
    
    // Create test file
    const char *test_content = "#1234567890:loaded1\n#1234567891:loaded2\n";
    LLE_ASSERT(create_test_file(TEST_HISTORY_FILE, test_content));
    
    // Load with clearing
    LLE_ASSERT(lle_history_load(history, TEST_HISTORY_FILE, true));
    
    // Verify only loaded entries remain
    LLE_ASSERT(lle_history_size(history) == 2);
    
    const lle_history_entry_t *entry0 = lle_history_get(history, 0);
    const lle_history_entry_t *entry1 = lle_history_get(history, 1);
    
    LLE_ASSERT(entry0 != NULL && strcmp(entry0->command, "loaded1") == 0);
    LLE_ASSERT(entry1 != NULL && strcmp(entry1->command, "loaded2") == 0);
    
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test load from empty file
LLE_TEST(history_load_empty_file) {
    printf("Testing load from empty file... ");
    
    // Create empty file
    LLE_ASSERT(create_test_file(TEST_EMPTY_FILE, ""));
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Add existing entry
    LLE_ASSERT(lle_history_add(history, "existing", false));
    
    // Load from empty file without clearing
    LLE_ASSERT(lle_history_load(history, TEST_EMPTY_FILE, false));
    
    // Verify existing entry is preserved
    LLE_ASSERT(lle_history_size(history) == 1);
    
    const lle_history_entry_t *entry = lle_history_get(history, 0);
    LLE_ASSERT(entry != NULL && strcmp(entry->command, "existing") == 0);
    
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test load with various line formats
LLE_TEST(history_load_line_formats) {
    printf("Testing load with various line formats... ");
    
    // Create file with mixed formats
    const char *test_content = 
        "#1234567890:timestamped command\n"
        "plain command\n"
        "\n"  // empty line
        "#invalid:format:with:colons\n"
        "#:empty timestamp\n"
        "another plain command\n";
    
    LLE_ASSERT(create_test_file(TEST_HISTORY_FILE, test_content));
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Load file
    LLE_ASSERT(lle_history_load(history, TEST_HISTORY_FILE, false));
    
    // Verify correct entries were loaded (empty lines skipped)
    LLE_ASSERT(lle_history_size(history) == 5);
    
    const lle_history_entry_t *entry0 = lle_history_get(history, 0);
    const lle_history_entry_t *entry1 = lle_history_get(history, 1);
    const lle_history_entry_t *entry2 = lle_history_get(history, 2);
    const lle_history_entry_t *entry3 = lle_history_get(history, 3);
    const lle_history_entry_t *entry4 = lle_history_get(history, 4);
    
    LLE_ASSERT(entry0 != NULL && strcmp(entry0->command, "timestamped command") == 0);
    LLE_ASSERT(entry1 != NULL && strcmp(entry1->command, "plain command") == 0);
    LLE_ASSERT(entry2 != NULL && strcmp(entry2->command, "format:with:colons") == 0);
    LLE_ASSERT(entry3 != NULL && strcmp(entry3->command, "empty timestamp") == 0);
    LLE_ASSERT(entry4 != NULL && strcmp(entry4->command, "another plain command") == 0);
    
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test load error conditions
LLE_TEST(history_load_error_conditions) {
    printf("Testing history load error conditions... ");
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Test NULL parameters
    LLE_ASSERT(!lle_history_load(NULL, TEST_HISTORY_FILE, false));
    LLE_ASSERT(!lle_history_load(history, NULL, false));
    
    // Test non-existent file
    LLE_ASSERT(!lle_history_load(history, "non_existent_file.txt", false));
    
    lle_history_destroy(history);
    printf("PASSED\n");
}

// Test save and load round trip
LLE_TEST(history_save_load_roundtrip) {
    printf("Testing save/load round trip... ");
    
    lle_history_t *history1 = lle_history_create(20);
    LLE_ASSERT(history1 != NULL);
    
    // Add test data
    const char *commands[] = {
        "echo 'hello world'",
        "ls -la /home",
        "grep -r pattern .",
        "make clean && make",
        "git status"
    };
    
    for (int i = 0; i < 5; i++) {
        LLE_ASSERT(lle_history_add(history1, commands[i], false));
    }
    
    // Save to file
    LLE_ASSERT(lle_history_save(history1, TEST_HISTORY_FILE));
    
    // Create new history and load
    lle_history_t *history2 = lle_history_create(20);
    LLE_ASSERT(history2 != NULL);
    LLE_ASSERT(lle_history_load(history2, TEST_HISTORY_FILE, false));
    
    // Verify they match
    LLE_ASSERT(lle_history_size(history1) == lle_history_size(history2));
    
    for (size_t i = 0; i < lle_history_size(history1); i++) {
        const lle_history_entry_t *entry1 = lle_history_get(history1, i);
        const lle_history_entry_t *entry2 = lle_history_get(history2, i);
        
        LLE_ASSERT(entry1 != NULL && entry2 != NULL);
        LLE_ASSERT(strcmp(entry1->command, entry2->command) == 0);
    }
    
    lle_history_destroy(history1);
    lle_history_destroy(history2);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test large history save/load
LLE_TEST(history_save_load_large) {
    printf("Testing save/load with large history... ");
    
    lle_history_t *history = lle_history_create(1000);
    LLE_ASSERT(history != NULL);
    
    // Add many entries
    char command[64];
    for (int i = 0; i < 500; i++) {
        snprintf(command, sizeof(command), "command_%d", i);
        LLE_ASSERT(lle_history_add(history, command, false));
    }
    
    // Save and reload
    LLE_ASSERT(lle_history_save(history, TEST_HISTORY_FILE));
    
    lle_history_t *history2 = lle_history_create(1000);
    LLE_ASSERT(history2 != NULL);
    LLE_ASSERT(lle_history_load(history2, TEST_HISTORY_FILE, false));
    
    // Verify size
    LLE_ASSERT(lle_history_size(history2) == 500);
    
    // Spot check some entries
    const lle_history_entry_t *entry_first = lle_history_get(history2, 0);
    const lle_history_entry_t *entry_last = lle_history_get(history2, 499);
    
    LLE_ASSERT(entry_first != NULL && strcmp(entry_first->command, "command_0") == 0);
    LLE_ASSERT(entry_last != NULL && strcmp(entry_last->command, "command_499") == 0);
    
    lle_history_destroy(history);
    lle_history_destroy(history2);
    cleanup_test_files();
    printf("PASSED\n");
}

// Test memory management during load
LLE_TEST(history_load_memory_management) {
    printf("Testing memory management during load operations... ");
    
    // Create file with commands
    const char *test_content = 
        "#1234567890:command1\n"
        "#1234567891:command2\n"
        "#1234567892:command3\n";
    
    LLE_ASSERT(create_test_file(TEST_HISTORY_FILE, test_content));
    
    lle_history_t *history = lle_history_create(20);
    LLE_ASSERT(history != NULL);
    
    // Load multiple times to test memory cleanup
    for (int i = 0; i < 3; i++) {
        LLE_ASSERT(lle_history_load(history, TEST_HISTORY_FILE, true));
        LLE_ASSERT(lle_history_size(history) == 3);
    }
    
    // Verify final state
    const lle_history_entry_t *entry = lle_history_get(history, 0);
    LLE_ASSERT(entry != NULL && strcmp(entry->command, "command1") == 0);
    
    lle_history_destroy(history);
    cleanup_test_files();
    printf("PASSED\n");
}

// Main test function
int main(void) {
    printf("=== LLE-025 History Management Tests ===\n\n");
    
    // Ensure clean start
    cleanup_test_files();
    
    // Run all tests
    test_history_save_basic();
    test_history_save_with_timestamps();
    test_history_save_empty();
    test_history_save_error_conditions();
    test_history_load_basic();
    test_history_load_preserve_existing();
    test_history_load_clear_existing();
    test_history_load_empty_file();
    test_history_load_line_formats();
    test_history_load_error_conditions();
    test_history_save_load_roundtrip();
    test_history_save_load_large();
    test_history_load_memory_management();
    
    // Final cleanup
    cleanup_test_files();
    
    printf("\n=== All LLE-025 History Management Tests Passed! ===\n");
    return 0;
}