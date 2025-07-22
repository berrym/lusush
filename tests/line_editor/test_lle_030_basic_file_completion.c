/*
 * Tests for LLE-030: Basic File Completion
 * 
 * This file contains comprehensive tests for the basic file completion system
 * including word extraction, filesystem interaction, and completion generation.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "test_framework.h"
#include "../src/line_editor/completion.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* Test filesystem setup */
static const char *test_dir = "/tmp/lle_test_completion";
static const char *test_files[] = {
    "test_file1.txt",
    "test_file2.c",
    "test_file3.h",
    "hidden_file.txt",
    ".hidden_dotfile",
    "another_test.py"
};

static const char *test_dirs[] = {
    "test_directory",
    "another_dir",
    ".hidden_dir"
};

/* Setup test filesystem for completion testing */
static bool setup_test_filesystem(void) {
    char path[512];
    
    // Create test directory
    if (mkdir(test_dir, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    
    // Create test files
    for (size_t i = 0; i < sizeof(test_files) / sizeof(test_files[0]); i++) {
        snprintf(path, sizeof(path), "%s/%s", test_dir, test_files[i]);
        FILE *f = fopen(path, "w");
        if (!f) return false;
        fprintf(f, "test content\n");
        fclose(f);
    }
    
    // Create test directories
    for (size_t i = 0; i < sizeof(test_dirs) / sizeof(test_dirs[0]); i++) {
        snprintf(path, sizeof(path), "%s/%s", test_dir, test_dirs[i]);
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            return false;
        }
    }
    
    return true;
}

/* Cleanup test filesystem */
static void cleanup_test_filesystem(void) {
    char command[1024];
    
    // Remove test directory and all contents
    snprintf(command, sizeof(command), "rm -rf %s", test_dir);
    system(command);
}

/* Test word extraction functionality */
LLE_TEST(word_extraction_basic) {
    printf("Testing basic word extraction... ");
    
    char word[256];
    size_t word_start;
    
    // Test simple filename at end
    LLE_ASSERT(lle_completion_extract_word("ls file", 7, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "file") == 0);
    LLE_ASSERT(word_start == 3);
    
    // Test filename with extension
    LLE_ASSERT(lle_completion_extract_word("cat test.txt", 12, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "test.txt") == 0);
    LLE_ASSERT(word_start == 4);
    
    // Test path with directory
    LLE_ASSERT(lle_completion_extract_word("cd dir/file", 11, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "dir/file") == 0);
    LLE_ASSERT(word_start == 3);
    
    printf("PASSED\n");
}

LLE_TEST(word_extraction_edge_cases) {
    printf("Testing word extraction edge cases... ");
    
    char word[256];
    size_t word_start;
    
    // Test cursor in middle of word
    LLE_ASSERT(lle_completion_extract_word("ls filename", 5, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "filename") == 0);
    LLE_ASSERT(word_start == 3);
    
    // Test empty input
    LLE_ASSERT(lle_completion_extract_word("", 0, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "") == 0);
    LLE_ASSERT(word_start == 0);
    
    // Test cursor at beginning
    LLE_ASSERT(lle_completion_extract_word("filename", 0, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "filename") == 0);
    LLE_ASSERT(word_start == 0);
    
    // Test with quotes (cursor at position 5 = 'f' in 'file')
    LLE_ASSERT(lle_completion_extract_word("cat \"file name\"", 5, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "\"file") == 0);
    LLE_ASSERT(word_start == 4);
    
    printf("PASSED\n");
}

LLE_TEST(word_extraction_special_characters) {
    printf("Testing word extraction with special characters... ");
    
    char word[256];
    size_t word_start;
    
    // Test pipe separator
    LLE_ASSERT(lle_completion_extract_word("cat file1.txt | grep test", 13, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "file1.txt") == 0);
    LLE_ASSERT(word_start == 4);
    
    // Test redirect
    LLE_ASSERT(lle_completion_extract_word("echo test > output.txt", 19, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "output.txt") == 0);
    LLE_ASSERT(word_start == 12);
    
    // Test simple path completion (cursor at end of 'file')
    LLE_ASSERT(lle_completion_extract_word("ls /tmp/file", 12, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "/tmp/file") == 0);
    LLE_ASSERT(word_start == 3);
    
    printf("PASSED\n");
}

LLE_TEST(word_extraction_parameter_validation) {
    printf("Testing word extraction parameter validation... ");
    
    char word[256];
    size_t word_start;
    
    // Test NULL input
    LLE_ASSERT(!lle_completion_extract_word(NULL, 0, word, sizeof(word), &word_start));
    
    // Test NULL word buffer
    LLE_ASSERT(!lle_completion_extract_word("test", 0, NULL, sizeof(word), &word_start));
    
    // Test zero word size
    LLE_ASSERT(!lle_completion_extract_word("test", 0, word, 0, &word_start));
    
    // Test NULL word_start
    LLE_ASSERT(!lle_completion_extract_word("test", 0, word, sizeof(word), NULL));
    
    // Test cursor past end (should clamp)
    LLE_ASSERT(lle_completion_extract_word("test", 100, word, sizeof(word), &word_start));
    LLE_ASSERT(strcmp(word, "test") == 0);
    
    printf("PASSED\n");
}

/* Test file completion functionality */
LLE_TEST(file_completion_current_directory) {
    printf("Testing file completion in current directory... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    LLE_ASSERT(chdir(test_dir) == 0);
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Test completion of files starting with "test"
    LLE_ASSERT(lle_complete_files("ls test", 7, completions));
    LLE_ASSERT(completions->count > 0);
    
    // Verify we got expected files
    bool found_dir = false;
    for (size_t i = 0; i < completions->count; i++) {
        if (strstr(completions->items[i].text, "test_directory/")) {
            found_dir = true;
            break;
        }
    }
    LLE_ASSERT(found_dir);
    
    lle_completion_list_destroy(completions);
    chdir(old_cwd);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_with_path) {
    printf("Testing file completion with path prefix... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Test completion with absolute path
    char input[512];
    snprintf(input, sizeof(input), "cat %s/test", test_dir);
    size_t cursor_pos = strlen(input);
    
    LLE_ASSERT(lle_complete_files(input, cursor_pos, completions));
    LLE_ASSERT(completions->count > 0);
    
    // Verify all results start with the path prefix
    for (size_t i = 0; i < completions->count; i++) {
        LLE_ASSERT(strstr(completions->items[i].text, test_dir) != NULL);
    }
    
    lle_completion_list_destroy(completions);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_prefix_matching) {
    printf("Testing file completion prefix matching... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    LLE_ASSERT(chdir(test_dir) == 0);
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Test completion with specific prefix
    LLE_ASSERT(lle_complete_files("vim test", 8, completions));
    
    // Verify all results start with "test"
    for (size_t i = 0; i < completions->count; i++) {
        LLE_ASSERT(strncmp(completions->items[i].text, "test", 4) == 0);
    }
    
    lle_completion_list_destroy(completions);
    chdir(old_cwd);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_hidden_files) {
    printf("Testing file completion with hidden files... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    LLE_ASSERT(chdir(test_dir) == 0);
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Test completion without dot prefix (should not show hidden files)
    LLE_ASSERT(lle_complete_files("ls h", 4, completions));
    
    // Should only find "hidden_file.txt", not ".hidden_dotfile"
    bool found_normal_hidden = false;
    bool found_dotfile = false;
    
    for (size_t i = 0; i < completions->count; i++) {
        if (strcmp(completions->items[i].text, "hidden_file.txt") == 0) {
            found_normal_hidden = true;
        }
        if (strcmp(completions->items[i].text, ".hidden_dotfile") == 0) {
            found_dotfile = true;
        }
    }
    
    LLE_ASSERT(found_normal_hidden);
    LLE_ASSERT(!found_dotfile);
    
    // Clear and test with dot prefix
    lle_completion_list_clear(completions);
    LLE_ASSERT(lle_complete_files("ls .", 4, completions));
    
    // Should now find dotfiles
    found_dotfile = false;
    for (size_t i = 0; i < completions->count; i++) {
        if (strcmp(completions->items[i].text, ".hidden_dotfile") == 0) {
            found_dotfile = true;
            break;
        }
    }
    LLE_ASSERT(found_dotfile);
    
    lle_completion_list_destroy(completions);
    chdir(old_cwd);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_directory_priority) {
    printf("Testing file completion directory priority... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    LLE_ASSERT(chdir(test_dir) == 0);
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Complete files starting with "test"
    LLE_ASSERT(lle_complete_files("cd test", 7, completions));
    LLE_ASSERT(completions->count > 0);
    
    // Find directory and file items
    int dir_priority = -1;
    int file_priority = -1;
    
    for (size_t i = 0; i < completions->count; i++) {
        if (strstr(completions->items[i].text, "test_directory/")) {
            dir_priority = completions->items[i].priority;
        } else if (strstr(completions->items[i].text, "test_file")) {
            file_priority = completions->items[i].priority;
        }
    }
    
    // Directories should have higher priority than files
    LLE_ASSERT(dir_priority > file_priority);
    
    lle_completion_list_destroy(completions);
    chdir(old_cwd);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_nonexistent_directory) {
    printf("Testing file completion in nonexistent directory... ");
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Try to complete in a directory that doesn't exist
    LLE_ASSERT(lle_complete_files("ls /nonexistent/path/file", 25, completions));
    
    // Should succeed but return no completions
    LLE_ASSERT(completions->count == 0);
    
    lle_completion_list_destroy(completions);
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_parameter_validation) {
    printf("Testing file completion parameter validation... ");
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Test NULL input
    LLE_ASSERT(!lle_complete_files(NULL, 0, completions));
    
    // Test NULL completions
    LLE_ASSERT(!lle_complete_files("ls test", 7, NULL));
    
    lle_completion_list_destroy(completions);
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_empty_input) {
    printf("Testing file completion with empty input... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    LLE_ASSERT(chdir(test_dir) == 0);
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Test completion with empty word
    LLE_ASSERT(lle_complete_files("ls ", 3, completions));
    
    // Should find all non-hidden files and directories
    LLE_ASSERT(completions->count > 0);
    
    // Should not contain hidden files starting with '.'
    for (size_t i = 0; i < completions->count; i++) {
        LLE_ASSERT(completions->items[i].text[0] != '.');
    }
    
    lle_completion_list_destroy(completions);
    chdir(old_cwd);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_exact_match_priority) {
    printf("Testing file completion exact match priority... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    LLE_ASSERT(chdir(test_dir) == 0);
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Complete with prefix that exactly matches one file
    LLE_ASSERT(lle_complete_files("cat test_file1.txt", 18, completions));
    LLE_ASSERT(completions->count > 0);
    
    // Find the exact match
    bool found_exact = false;
    int exact_priority = -1;
    
    for (size_t i = 0; i < completions->count; i++) {
        if (strcmp(completions->items[i].text, "test_file1.txt") == 0) {
            found_exact = true;
            exact_priority = completions->items[i].priority;
            break;
        }
    }
    
    LLE_ASSERT(found_exact);
    LLE_ASSERT(exact_priority == 200); // LLE_COMPLETION_PRIORITY_EXACT
    
    lle_completion_list_destroy(completions);
    chdir(old_cwd);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

LLE_TEST(file_completion_sorting) {
    printf("Testing file completion result sorting... ");
    
    char old_cwd[512];
    getcwd(old_cwd, sizeof(old_cwd));
    
    LLE_ASSERT(setup_test_filesystem());
    LLE_ASSERT(chdir(test_dir) == 0);
    
    lle_completion_list_t *completions = lle_completion_list_create(16);
    LLE_ASSERT(completions != NULL);
    
    // Complete files to get a mixed list
    LLE_ASSERT(lle_complete_files("ls ", 3, completions));
    LLE_ASSERT(completions->count > 0);
    
    // Verify results are sorted by priority (highest first)
    for (size_t i = 1; i < completions->count; i++) {
        LLE_ASSERT(completions->items[i-1].priority >= completions->items[i].priority);
    }
    
    lle_completion_list_destroy(completions);
    chdir(old_cwd);
    cleanup_test_filesystem();
    
    printf("PASSED\n");
}

/* Main test function */
int main(void) {
    printf("Running LLE-030 Basic File Completion Tests...\n");
    printf("==============================================\n");
    
    // Word extraction tests
    test_word_extraction_basic();
    test_word_extraction_edge_cases();
    test_word_extraction_special_characters();
    test_word_extraction_parameter_validation();
    
    // File completion tests
    test_file_completion_current_directory();
    test_file_completion_with_path();
    test_file_completion_prefix_matching();
    test_file_completion_hidden_files();
    test_file_completion_directory_priority();
    test_file_completion_nonexistent_directory();
    test_file_completion_parameter_validation();
    test_file_completion_empty_input();
    test_file_completion_exact_match_priority();
    test_file_completion_sorting();
    
    printf("\n==============================================\n");
    printf("All LLE-030 tests completed successfully!\n");
    printf("Word extraction: 4 tests PASSED\n");
    printf("File completion: 10 tests PASSED\n");
    printf("Total: 14 tests PASSED\n");
    
    return 0;
}