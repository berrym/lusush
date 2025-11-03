/*
 * Test Suite: Completion Type Classification System
 * Tests Phase 1 of pragmatic completion system
 */

#include "../../include/completion_types.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// MOCK IMPLEMENTATIONS FOR TESTING
// ============================================================================

// Mock builtin structure (minimal)
typedef struct builtin_s {
    const char *name;
    const char *doc;
    void *func; // We don't need the actual function
} builtin;

// Mock builtin list for testing
static builtin test_builtins[] = {
    {"cd", "Change directory", NULL},
    {"echo", "Echo arguments", NULL},
    {"pwd", "Print working directory", NULL},
    {"exit", "Exit shell", NULL},
    {"help", "Show help", NULL},
};

builtin builtins[] = {
    {"cd", "Change directory", NULL},
    {"echo", "Echo arguments", NULL},
    {"pwd", "Print working directory", NULL},
    {"exit", "Exit shell", NULL},
    {"help", "Show help", NULL},
};

const size_t builtins_count = sizeof(builtins) / sizeof(builtins[0]);

// Mock alias lookup (always returns NULL - no aliases in test)
char *lookup_alias(const char *name) {
    (void)name;
    return NULL;
}

// Mock lusush_add_completion (we won't test conversion functions)
// lusush_completions_t is defined in readline_integration.h (included by completion_types.h)
void lusush_add_completion(lusush_completions_t *lc, const char *completion) {
    (void)lc;
    (void)completion;
    // Not used in our tests - just a stub for linking
}

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  ✓ %s\n", message); \
    } else { \
        printf("  ✗ %s\n", message); \
    } \
} while(0)

// ============================================================================
// TEST: Type Information Database
// ============================================================================

static void test_type_info_database(void) {
    printf("\n=== Type Information Database Tests ===\n");
    
    // Test each completion type has valid info
    const completion_type_info_t *info;
    
    info = completion_type_get_info(COMPLETION_TYPE_BUILTIN);
    TEST_ASSERT(info != NULL, "Get builtin type info");
    TEST_ASSERT(strcmp(info->type_name, "Built-in") == 0, "Builtin type name");
    TEST_ASSERT(strcmp(info->category_name, "BUILT-INS") == 0, "Builtin category");
    TEST_ASSERT(info->default_priority == 900, "Builtin priority");
    
    info = completion_type_get_info(COMPLETION_TYPE_COMMAND);
    TEST_ASSERT(info != NULL, "Get command type info");
    TEST_ASSERT(strcmp(info->type_name, "Command") == 0, "Command type name");
    
    info = completion_type_get_info(COMPLETION_TYPE_FILE);
    TEST_ASSERT(info != NULL, "Get file type info");
    
    info = completion_type_get_info(COMPLETION_TYPE_DIRECTORY);
    TEST_ASSERT(info != NULL, "Get directory type info");
    
    info = completion_type_get_info(COMPLETION_TYPE_VARIABLE);
    TEST_ASSERT(info != NULL, "Get variable type info");
    
    info = completion_type_get_info(COMPLETION_TYPE_ALIAS);
    TEST_ASSERT(info != NULL, "Get alias type info");
    TEST_ASSERT(info->default_priority == 950, "Alias has highest priority");
    
    info = completion_type_get_info(COMPLETION_TYPE_HISTORY);
    TEST_ASSERT(info != NULL, "Get history type info");
    TEST_ASSERT(info->default_priority == 400, "History has lower priority");
    
    // Test category name retrieval
    const char *category = completion_type_get_category(COMPLETION_TYPE_COMMAND);
    TEST_ASSERT(strcmp(category, "COMMANDS") == 0, "Get command category");
    
    // Test indicator retrieval
    const char *indicator = completion_type_get_indicator(COMPLETION_TYPE_BUILTIN);
    TEST_ASSERT(indicator != NULL && strlen(indicator) > 0, "Get builtin indicator");
}

// ============================================================================
// TEST: Completion Item Creation and Management
// ============================================================================

static void test_completion_item_management(void) {
    printf("\n=== Completion Item Management Tests ===\n");
    
    // Test basic item creation
    completion_item_t *item = completion_item_create("echo", " ", 
                                                     COMPLETION_TYPE_BUILTIN, 900);
    TEST_ASSERT(item != NULL, "Create completion item");
    TEST_ASSERT(strcmp(item->text, "echo") == 0, "Item text is correct");
    TEST_ASSERT(strcmp(item->suffix, " ") == 0, "Item suffix is correct");
    TEST_ASSERT(item->type == COMPLETION_TYPE_BUILTIN, "Item type is correct");
    TEST_ASSERT(item->relevance_score == 900, "Item score is correct");
    TEST_ASSERT(item->owns_text == true, "Item owns its text");
    TEST_ASSERT(item->description == NULL, "Item has no description");
    
    completion_item_free(item);
    printf("  ✓ Free completion item\n");
    tests_run++; tests_passed++;
    
    // Test item creation with description
    item = completion_item_create_with_description("cd", " ", COMPLETION_TYPE_BUILTIN,
                                                   950, "Change directory");
    TEST_ASSERT(item != NULL, "Create item with description");
    TEST_ASSERT(item->description != NULL, "Item has description");
    TEST_ASSERT(strcmp(item->description, "Change directory") == 0, 
               "Description is correct");
    TEST_ASSERT(item->owns_description == true, "Item owns description");
    
    completion_item_free(item);
    printf("  ✓ Free item with description\n");
    tests_run++; tests_passed++;
    
    // Test score clamping
    item = completion_item_create("test", " ", COMPLETION_TYPE_COMMAND, 2000);
    TEST_ASSERT(item->relevance_score == 1000, "Score clamped to max (1000)");
    completion_item_free(item);
    
    item = completion_item_create("test", " ", COMPLETION_TYPE_COMMAND, -100);
    TEST_ASSERT(item->relevance_score == 0, "Score clamped to min (0)");
    completion_item_free(item);
}

// ============================================================================
// TEST: Completion Result Management
// ============================================================================

static void test_completion_result_management(void) {
    printf("\n=== Completion Result Management Tests ===\n");
    
    // Test result creation
    completion_result_t *result = completion_result_create(16);
    TEST_ASSERT(result != NULL, "Create completion result");
    TEST_ASSERT(result->count == 0, "Result starts empty");
    TEST_ASSERT(result->capacity >= 16, "Result has initial capacity");
    
    // Test adding items
    bool success = completion_result_add(result, "echo", " ", 
                                        COMPLETION_TYPE_BUILTIN, 900);
    TEST_ASSERT(success, "Add builtin item");
    TEST_ASSERT(result->count == 1, "Result count increased");
    TEST_ASSERT(result->builtin_count == 1, "Builtin count updated");
    
    success = completion_result_add(result, "ls", " ", 
                                    COMPLETION_TYPE_COMMAND, 800);
    TEST_ASSERT(success, "Add command item");
    TEST_ASSERT(result->count == 2, "Result count is 2");
    TEST_ASSERT(result->command_count == 1, "Command count updated");
    
    success = completion_result_add(result, "file.txt", " ", 
                                    COMPLETION_TYPE_FILE, 600);
    TEST_ASSERT(success, "Add file item");
    TEST_ASSERT(result->file_count == 1, "File count updated");
    
    success = completion_result_add(result, "/tmp/", "/", 
                                    COMPLETION_TYPE_DIRECTORY, 700);
    TEST_ASSERT(success, "Add directory item");
    TEST_ASSERT(result->directory_count == 1, "Directory count updated");
    
    success = completion_result_add(result, "$HOME", "", 
                                    COMPLETION_TYPE_VARIABLE, 500);
    TEST_ASSERT(success, "Add variable item");
    TEST_ASSERT(result->variable_count == 1, "Variable count updated");
    
    // Test category counting
    size_t builtin_count = completion_result_count_by_type(result, 
                                                           COMPLETION_TYPE_BUILTIN);
    TEST_ASSERT(builtin_count == 1, "Count builtins correctly");
    
    size_t command_count = completion_result_count_by_type(result, 
                                                           COMPLETION_TYPE_COMMAND);
    TEST_ASSERT(command_count == 1, "Count commands correctly");
    
    // Test array growth
    for (int i = 0; i < 20; i++) {
        char name[32];
        snprintf(name, sizeof(name), "cmd%d", i);
        success = completion_result_add(result, name, " ", 
                                       COMPLETION_TYPE_COMMAND, 500 + i);
        TEST_ASSERT(success, "Add item during growth");
    }
    TEST_ASSERT(result->count > 16, "Result grew beyond initial capacity");
    
    completion_result_free(result);
    printf("  ✓ Free completion result\n");
    tests_run++; tests_passed++;
}

// ============================================================================
// TEST: Completion Sorting
// ============================================================================

static void test_completion_sorting(void) {
    printf("\n=== Completion Sorting Tests ===\n");
    
    completion_result_t *result = completion_result_create(16);
    
    // Add items in random order
    completion_result_add(result, "file.txt", " ", COMPLETION_TYPE_FILE, 600);
    completion_result_add(result, "ll", " ", COMPLETION_TYPE_ALIAS, 950);
    completion_result_add(result, "echo", " ", COMPLETION_TYPE_BUILTIN, 900);
    completion_result_add(result, "ls", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_add(result, "$PATH", "", COMPLETION_TYPE_VARIABLE, 500);
    
    // Sort the result
    completion_result_sort(result);
    printf("  ✓ Sort completion result\n");
    tests_run++; tests_passed++;
    
    // Verify sorting: items are sorted by type (enum order), then by relevance
    // Type order: BUILTIN(0), COMMAND(1), FILE(2), DIRECTORY(3), VARIABLE(4), ALIAS(5), HISTORY(6)
    // So builtins come first, then commands, etc.
    TEST_ASSERT(result->items[0].type == COMPLETION_TYPE_BUILTIN, 
               "Builtins sorted first by enum order");
    TEST_ASSERT(strcmp(result->items[0].text, "echo") == 0, 
               "First builtin is 'echo'");
    
    // Verify builtins come before commands (by type enum value)
    bool found_builtin = false;
    bool found_command = false;
    bool found_alias = false;
    size_t builtin_idx = 0, command_idx = 0, alias_idx = 0;
    
    for (size_t i = 0; i < result->count; i++) {
        if (result->items[i].type == COMPLETION_TYPE_BUILTIN) {
            if (!found_builtin) {
                found_builtin = true;
                builtin_idx = i;
            }
        }
        if (result->items[i].type == COMPLETION_TYPE_COMMAND) {
            if (!found_command) {
                found_command = true;
                command_idx = i;
            }
        }
        if (result->items[i].type == COMPLETION_TYPE_ALIAS) {
            if (!found_alias) {
                found_alias = true;
                alias_idx = i;
            }
        }
    }
    
    TEST_ASSERT(found_builtin && found_command && found_alias, "Found all types");
    TEST_ASSERT(builtin_idx < command_idx, "Builtins before commands");
    TEST_ASSERT(command_idx < alias_idx, "Commands before aliases");
    
    completion_result_free(result);
}

// ============================================================================
// TEST: Classification Helpers
// ============================================================================

static void test_classification_helpers(void) {
    printf("\n=== Classification Helper Tests ===\n");
    
    // Test builtin detection
    bool is_builtin = completion_is_builtin("echo");
    TEST_ASSERT(is_builtin, "Detect 'echo' as builtin");
    
    is_builtin = completion_is_builtin("cd");
    TEST_ASSERT(is_builtin, "Detect 'cd' as builtin");
    
    is_builtin = completion_is_builtin("not_a_builtin");
    TEST_ASSERT(!is_builtin, "Reject non-builtin");
    
    // Test directory detection
    bool is_dir = completion_is_directory("/tmp");
    TEST_ASSERT(is_dir, "Detect /tmp as directory");
    
    is_dir = completion_is_directory("/etc");
    TEST_ASSERT(is_dir, "Detect /etc as directory");
    
    is_dir = completion_is_directory("/nonexistent");
    TEST_ASSERT(!is_dir, "Reject non-existent path");
    
    // Test text classification
    completion_type_t type;
    
    type = completion_classify_text("$HOME", false);
    TEST_ASSERT(type == COMPLETION_TYPE_VARIABLE, "Classify $HOME as variable");
    
    type = completion_classify_text("$PATH", false);
    TEST_ASSERT(type == COMPLETION_TYPE_VARIABLE, "Classify $PATH as variable");
    
    type = completion_classify_text("echo", true);
    TEST_ASSERT(type == COMPLETION_TYPE_BUILTIN, "Classify echo as builtin");
    
    type = completion_classify_text("/tmp", false);
    TEST_ASSERT(type == COMPLETION_TYPE_DIRECTORY, "Classify /tmp as directory");
}

// ============================================================================
// TEST: Adding Items via Item Structure
// ============================================================================

static void test_add_item_via_struct(void) {
    printf("\n=== Add Item Via Struct Tests ===\n");
    
    completion_result_t *result = completion_result_create(8);
    
    // Create item and add via structure
    completion_item_t *item = completion_item_create("test", " ", 
                                                     COMPLETION_TYPE_COMMAND, 750);
    TEST_ASSERT(item != NULL, "Create item for adding");
    
    bool success = completion_result_add_item(result, item);
    TEST_ASSERT(success, "Add item via structure");
    TEST_ASSERT(result->count == 1, "Result has one item");
    TEST_ASSERT(result->command_count == 1, "Command count updated");
    
    // Verify the item is in the result
    TEST_ASSERT(strcmp(result->items[0].text, "test") == 0, 
               "Item text correct");
    TEST_ASSERT(result->items[0].relevance_score == 750, 
               "Item score correct");
    
    completion_result_free(result);
}

// ============================================================================
// TEST: Statistics and Debugging
// ============================================================================

static void test_statistics(void) {
    printf("\n=== Statistics Tests ===\n");
    
    completion_result_t *result = completion_result_create(16);
    
    // Add various items
    completion_result_add(result, "echo", " ", COMPLETION_TYPE_BUILTIN, 900);
    completion_result_add(result, "cd", " ", COMPLETION_TYPE_BUILTIN, 900);
    completion_result_add(result, "ls", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_add(result, "grep", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_add(result, "file.txt", " ", COMPLETION_TYPE_FILE, 600);
    
    // Print stats (visual verification)
    printf("\n");
    completion_result_print_stats(result);
    printf("\n");
    tests_run++; tests_passed++;
    printf("  ✓ Print statistics\n");
    
    // Verify counts
    TEST_ASSERT(result->builtin_count == 2, "Two builtins");
    TEST_ASSERT(result->command_count == 2, "Two commands");
    TEST_ASSERT(result->file_count == 1, "One file");
    TEST_ASSERT(result->count == 5, "Total of 5 items");
    
    completion_result_free(result);
}

// ============================================================================
// TEST: Edge Cases and Error Handling
// ============================================================================

static void test_edge_cases(void) {
    printf("\n=== Edge Cases and Error Handling Tests ===\n");
    
    // Test NULL handling
    completion_item_t *null_item = completion_item_create(NULL, " ", 
                                                          COMPLETION_TYPE_COMMAND, 500);
    TEST_ASSERT(null_item == NULL, "Reject NULL text in item creation");
    
    // Test empty string handling
    completion_item_t *empty_item = completion_item_create("", " ", 
                                                           COMPLETION_TYPE_COMMAND, 500);
    TEST_ASSERT(empty_item != NULL, "Accept empty string");
    TEST_ASSERT(strcmp(empty_item->text, "") == 0, "Empty text preserved");
    completion_item_free(empty_item);
    
    // Test NULL suffix handling
    completion_item_t *no_suffix = completion_item_create("test", NULL, 
                                                          COMPLETION_TYPE_COMMAND, 500);
    TEST_ASSERT(no_suffix != NULL, "Accept NULL suffix");
    TEST_ASSERT(no_suffix->suffix == NULL, "NULL suffix preserved");
    completion_item_free(no_suffix);
    
    // Test zero capacity result
    completion_result_t *zero_cap = completion_result_create(0);
    TEST_ASSERT(zero_cap != NULL, "Create result with zero capacity");
    TEST_ASSERT(zero_cap->capacity > 0, "Zero capacity uses default");
    completion_result_free(zero_cap);
    
    // Test double free protection (free NULL)
    completion_item_free(NULL);
    completion_result_free(NULL);
    tests_run++; tests_passed++;
    printf("  ✓ NULL free handling\n");
    
    // Test invalid type info
    const completion_type_info_t *invalid = completion_type_get_info(999);
    TEST_ASSERT(invalid != NULL, "Invalid type returns fallback");
    TEST_ASSERT(invalid->type == COMPLETION_TYPE_UNKNOWN, 
               "Fallback is UNKNOWN type");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  Completion Type Classification System - Test Suite           ║\n");
    printf("║  Testing Phase 1 of Pragmatic Completion System               ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    // Run all test suites
    test_type_info_database();
    test_completion_item_management();
    test_completion_result_management();
    test_completion_sorting();
    test_classification_helpers();
    test_add_item_via_struct();
    test_statistics();
    test_edge_cases();
    
    // Print summary
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  Test Results                                                  ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║  Tests Run:    %3d                                            ║\n", tests_run);
    printf("║  Tests Passed: %3d                                            ║\n", tests_passed);
    printf("║  Tests Failed: %3d                                            ║\n", tests_run - tests_passed);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (tests_passed == tests_run) {
        printf("✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("✗ Some tests failed!\n\n");
        return 1;
    }
}
