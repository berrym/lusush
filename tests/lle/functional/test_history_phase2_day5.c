/**
 * @file test_history_phase2_day5.c
 * @brief Functional tests for LLE History Phase 2 Day 5 - Lusush Integration Bridge
 * 
 * Tests the bidirectional synchronization between:
 * - LLE history core
 * - GNU Readline history API
 * - POSIX history manager
 * - History builtin compatibility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "lle/history.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "posix_history.h"

/* Test harness */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void name(void); \
    static void run_##name(void) { \
        tests_run++; \
        printf("Running: %s\n", #name); \
        name(); \
        tests_passed++; \
        printf("  PASSED\n"); \
    } \
    static void name(void)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("  FAILED: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_SUCCESS(result) \
    ASSERT((result) == LLE_SUCCESS)

#define ASSERT_EQ(actual, expected) \
    ASSERT((actual) == (expected))

#define ASSERT_STR_EQ(actual, expected) \
    ASSERT(strcmp((actual), (expected)) == 0)

/* ============================================================================
 * TEST SETUP AND TEARDOWN
 * ============================================================================ */

static lle_memory_pool_t *g_pool = NULL;
static lle_history_core_t *g_core = NULL;
static posix_history_manager_t *g_posix = NULL;

static void setup(void) {
    /* Initialize memory pool */
    lle_result_t result = lusush_pool_create(&g_pool, "test_pool", 1024 * 1024);
    ASSERT_SUCCESS(result);
    
    /* Create history core */
    result = lle_history_core_create(&g_core, g_pool, NULL);
    ASSERT_SUCCESS(result);
    
    /* Create POSIX history manager */
    g_posix = posix_history_create(1000);
    ASSERT(g_posix != NULL);
    
    /* Initialize GNU Readline history */
    using_history();
    clear_history();
}

static void teardown(void) {
    /* Shutdown bridge */
    if (lle_history_bridge_is_initialized()) {
        lle_history_bridge_shutdown();
    }
    
    /* Clean up POSIX manager */
    if (g_posix) {
        posix_history_destroy(g_posix);
        g_posix = NULL;
    }
    
    /* Clean up history core */
    if (g_core) {
        lle_history_core_destroy(g_core);
        g_core = NULL;
    }
    
    /* Clean up memory pool */
    if (g_pool) {
        lusush_pool_destroy(g_pool);
        g_pool = NULL;
    }
    
    /* Clear GNU Readline history */
    clear_history();
}

/* ============================================================================
 * BRIDGE INITIALIZATION TESTS
 * ============================================================================ */

TEST(test_bridge_init_basic) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Check that bridge is initialized */
    ASSERT(lle_history_bridge_is_initialized());
    
    teardown();
}

TEST(test_bridge_init_without_posix) {
    setup();
    
    /* Initialize bridge without POSIX manager */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    ASSERT(lle_history_bridge_is_initialized());
    
    teardown();
}

TEST(test_bridge_double_init) {
    setup();
    
    /* First init should succeed */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Second init should fail with ALREADY_INITIALIZED */
    result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_EQ(result, LLE_ERROR_ALREADY_INITIALIZED);
    
    teardown();
}

/* ============================================================================
 * GNU READLINE SYNCHRONIZATION TESTS
 * ============================================================================ */

TEST(test_import_from_readline) {
    setup();
    
    /* Add entries to GNU Readline */
    add_history("echo hello");
    add_history("ls -la");
    add_history("cd /tmp");
    
    /* Initialize bridge (should import on init) */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Check that entries were imported to LLE */
    size_t count = 0;
    result = lle_history_get_entry_count(g_core, &count);
    ASSERT_SUCCESS(result);
    ASSERT_EQ(count, 3);
    
    /* Verify entry contents */
    lle_history_entry_t *entry = NULL;
    result = lle_history_get_entry_by_index(g_core, 0, &entry);
    ASSERT_SUCCESS(result);
    ASSERT_STR_EQ(entry->command, "echo hello");
    
    result = lle_history_get_entry_by_index(g_core, 1, &entry);
    ASSERT_SUCCESS(result);
    ASSERT_STR_EQ(entry->command, "ls -la");
    
    result = lle_history_get_entry_by_index(g_core, 2, &entry);
    ASSERT_SUCCESS(result);
    ASSERT_STR_EQ(entry->command, "cd /tmp");
    
    teardown();
}

TEST(test_export_to_readline) {
    setup();
    
    /* Initialize bridge with import disabled */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Disable auto sync to test manual export */
    result = lle_history_bridge_set_auto_sync(false);
    ASSERT_SUCCESS(result);
    
    /* Add entries to LLE core directly */
    result = lle_history_add_entry(g_core, "echo test1", 0, NULL);
    ASSERT_SUCCESS(result);
    result = lle_history_add_entry(g_core, "echo test2", 0, NULL);
    ASSERT_SUCCESS(result);
    result = lle_history_add_entry(g_core, "echo test3", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Clear readline before export */
    clear_history();
    
    /* Export to readline */
    result = lle_history_bridge_export_to_readline();
    ASSERT_SUCCESS(result);
    
    /* Verify entries in readline */
    ASSERT_EQ(history_length, 3);
    
    HIST_ENTRY *entry = history_get(1);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->line, "echo test1");
    
    entry = history_get(2);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->line, "echo test2");
    
    entry = history_get(3);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->line, "echo test3");
    
    teardown();
}

TEST(test_bidirectional_sync_readline) {
    setup();
    
    /* Add to readline first */
    add_history("command1");
    add_history("command2");
    
    /* Initialize bridge (imports from readline) */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Add new entry via bridge */
    result = lle_history_bridge_add_entry("command3", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Verify in LLE */
    size_t count = 0;
    result = lle_history_get_entry_count(g_core, &count);
    ASSERT_SUCCESS(result);
    ASSERT_EQ(count, 3);
    
    /* Verify in readline */
    ASSERT_EQ(history_length, 3);
    HIST_ENTRY *entry = history_get(3);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->line, "command3");
    
    teardown();
}

/* ============================================================================
 * POSIX HISTORY SYNCHRONIZATION TESTS
 * ============================================================================ */

TEST(test_import_from_posix) {
    setup();
    
    /* Add entries to POSIX manager */
    posix_history_add(g_posix, "pwd");
    posix_history_add(g_posix, "whoami");
    posix_history_add(g_posix, "date");
    
    /* Initialize bridge (should import from POSIX) */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Check that entries were imported to LLE */
    size_t count = 0;
    result = lle_history_get_entry_count(g_core, &count);
    ASSERT_SUCCESS(result);
    ASSERT_EQ(count, 3);
    
    /* Verify entry contents */
    lle_history_entry_t *entry = NULL;
    result = lle_history_get_entry_by_index(g_core, 0, &entry);
    ASSERT_SUCCESS(result);
    ASSERT_STR_EQ(entry->command, "pwd");
    
    result = lle_history_get_entry_by_index(g_core, 1, &entry);
    ASSERT_SUCCESS(result);
    ASSERT_STR_EQ(entry->command, "whoami");
    
    result = lle_history_get_entry_by_index(g_core, 2, &entry);
    ASSERT_SUCCESS(result);
    ASSERT_STR_EQ(entry->command, "date");
    
    teardown();
}

TEST(test_export_to_posix) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Disable auto sync */
    result = lle_history_bridge_set_auto_sync(false);
    ASSERT_SUCCESS(result);
    
    /* Add entries to LLE core */
    result = lle_history_add_entry(g_core, "ls /etc", 0, NULL);
    ASSERT_SUCCESS(result);
    result = lle_history_add_entry(g_core, "cat /etc/hosts", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Clear POSIX before export */
    posix_history_clear(g_posix);
    
    /* Export to POSIX */
    result = lle_history_bridge_export_to_posix();
    ASSERT_SUCCESS(result);
    
    /* Verify entries in POSIX */
    ASSERT_EQ(g_posix->count, 2);
    ASSERT_STR_EQ(g_posix->entries[0].command, "ls /etc");
    ASSERT_STR_EQ(g_posix->entries[1].command, "cat /etc/hosts");
    
    teardown();
}

TEST(test_bidirectional_sync_posix) {
    setup();
    
    /* Add to POSIX first */
    posix_history_add(g_posix, "grep test file.txt");
    
    /* Initialize bridge (imports from POSIX) */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Add new entry via bridge */
    result = lle_history_bridge_add_entry("awk '{print $1}' data.txt", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Verify in LLE */
    size_t count = 0;
    result = lle_history_get_entry_count(g_core, &count);
    ASSERT_SUCCESS(result);
    ASSERT_EQ(count, 2);
    
    /* Verify in POSIX */
    ASSERT_EQ(g_posix->count, 2);
    ASSERT_STR_EQ(g_posix->entries[1].command, "awk '{print $1}' data.txt");
    
    teardown();
}

/* ============================================================================
 * AUTO-SYNC TESTS
 * ============================================================================ */

TEST(test_auto_sync_enabled) {
    setup();
    
    /* Initialize bridge with auto-sync enabled (default) */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Clear readline and POSIX */
    clear_history();
    posix_history_clear(g_posix);
    
    /* Add entry via bridge (should auto-sync) */
    result = lle_history_bridge_add_entry("auto-sync test", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Verify auto-synced to readline */
    ASSERT_EQ(history_length, 1);
    HIST_ENTRY *rl_entry = history_get(1);
    ASSERT(rl_entry != NULL);
    ASSERT_STR_EQ(rl_entry->line, "auto-sync test");
    
    /* Verify auto-synced to POSIX */
    ASSERT_EQ(g_posix->count, 1);
    ASSERT_STR_EQ(g_posix->entries[0].command, "auto-sync test");
    
    teardown();
}

TEST(test_auto_sync_disabled) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Disable auto-sync */
    result = lle_history_bridge_set_auto_sync(false);
    ASSERT_SUCCESS(result);
    
    /* Clear readline and POSIX */
    clear_history();
    posix_history_clear(g_posix);
    
    /* Add entry via bridge (should NOT auto-sync) */
    result = lle_history_bridge_add_entry("no auto-sync", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Verify NOT in readline */
    ASSERT_EQ(history_length, 0);
    
    /* Verify NOT in POSIX */
    ASSERT_EQ(g_posix->count, 0);
    
    /* Manual sync */
    result = lle_history_bridge_sync_all();
    ASSERT_SUCCESS(result);
    
    /* Now should be synced */
    ASSERT_EQ(history_length, 1);
    ASSERT_EQ(g_posix->count, 1);
    
    teardown();
}

/* ============================================================================
 * HISTORY BUILTIN COMPATIBILITY TESTS
 * ============================================================================ */

TEST(test_history_builtin_output) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Add entries */
    result = lle_history_bridge_add_entry("echo line1", 0, NULL);
    ASSERT_SUCCESS(result);
    result = lle_history_bridge_add_entry("echo line2", 0, NULL);
    ASSERT_SUCCESS(result);
    result = lle_history_bridge_add_entry("echo line3", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Get builtin output */
    char *output = NULL;
    result = lle_history_bridge_handle_builtin(0, NULL, &output);
    ASSERT_SUCCESS(result);
    ASSERT(output != NULL);
    
    /* Verify output contains entries */
    ASSERT(strstr(output, "echo line1") != NULL);
    ASSERT(strstr(output, "echo line2") != NULL);
    ASSERT(strstr(output, "echo line3") != NULL);
    
    lle_pool_free(output);
    teardown();
}

TEST(test_get_by_number) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Add entries and track IDs */
    uint64_t id1 = 0, id2 = 0, id3 = 0;
    result = lle_history_bridge_add_entry("first", 0, &id1);
    ASSERT_SUCCESS(result);
    result = lle_history_bridge_add_entry("second", 0, &id2);
    ASSERT_SUCCESS(result);
    result = lle_history_bridge_add_entry("third", 0, &id3);
    ASSERT_SUCCESS(result);
    
    /* Get by number (ID) */
    lle_history_entry_t *entry = NULL;
    result = lle_history_bridge_get_by_number(id2, &entry);
    ASSERT_SUCCESS(result);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->command, "second");
    
    teardown();
}

TEST(test_get_by_reverse_index) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Add entries */
    result = lle_history_bridge_add_entry("oldest", 0, NULL);
    ASSERT_SUCCESS(result);
    result = lle_history_bridge_add_entry("middle", 0, NULL);
    ASSERT_SUCCESS(result);
    result = lle_history_bridge_add_entry("newest", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Get by reverse index */
    lle_history_entry_t *entry = NULL;
    
    /* 0 = newest */
    result = lle_history_bridge_get_by_reverse_index(0, &entry);
    ASSERT_SUCCESS(result);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->command, "newest");
    
    /* 1 = second newest */
    result = lle_history_bridge_get_by_reverse_index(1, &entry);
    ASSERT_SUCCESS(result);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->command, "middle");
    
    /* 2 = oldest */
    result = lle_history_bridge_get_by_reverse_index(2, &entry);
    ASSERT_SUCCESS(result);
    ASSERT(entry != NULL);
    ASSERT_STR_EQ(entry->command, "oldest");
    
    teardown();
}

/* ============================================================================
 * STATISTICS TESTS
 * ============================================================================ */

TEST(test_bridge_statistics) {
    setup();
    
    /* Add to readline before init */
    add_history("pre-import1");
    add_history("pre-import2");
    
    /* Add to POSIX before init */
    posix_history_add(g_posix, "posix-pre1");
    posix_history_add(g_posix, "posix-pre2");
    posix_history_add(g_posix, "posix-pre3");
    
    /* Initialize bridge (imports from both) */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Get statistics */
    size_t rl_imports = 0, rl_exports = 0;
    size_t posix_imports = 0, posix_exports = 0;
    size_t errors = 0;
    
    result = lle_history_bridge_get_stats(
        &rl_imports, &rl_exports, 
        &posix_imports, &posix_exports, 
        &errors
    );
    ASSERT_SUCCESS(result);
    
    /* Verify import counts */
    ASSERT_EQ(rl_imports, 2);
    ASSERT_EQ(posix_imports, 3);
    ASSERT_EQ(errors, 0);
    
    teardown();
}

TEST(test_diagnostics_output) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, g_posix, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Print diagnostics (just verify it doesn't crash) */
    result = lle_history_bridge_print_diagnostics();
    ASSERT_SUCCESS(result);
    
    teardown();
}

/* ============================================================================
 * EDGE CASE TESTS
 * ============================================================================ */

TEST(test_ignore_empty_commands) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Add empty/whitespace commands */
    result = lle_history_bridge_add_entry("", 0, NULL);
    ASSERT_SUCCESS(result);  /* Should succeed but not add */
    
    result = lle_history_bridge_add_entry("   ", 0, NULL);
    ASSERT_SUCCESS(result);  /* Should succeed but not add */
    
    result = lle_history_bridge_add_entry("\t\n", 0, NULL);
    ASSERT_SUCCESS(result);  /* Should succeed but not add */
    
    /* Add valid command */
    result = lle_history_bridge_add_entry("valid command", 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Should only have 1 entry */
    size_t count = 0;
    result = lle_history_get_entry_count(g_core, &count);
    ASSERT_SUCCESS(result);
    ASSERT_EQ(count, 1);
    
    teardown();
}

TEST(test_large_command_sync) {
    setup();
    
    /* Initialize bridge */
    lle_result_t result = lle_history_bridge_init(g_core, NULL, g_pool);
    ASSERT_SUCCESS(result);
    
    /* Create large command (8KB) */
    char large_cmd[8192];
    memset(large_cmd, 'x', sizeof(large_cmd) - 1);
    large_cmd[sizeof(large_cmd) - 1] = '\0';
    
    /* Add via bridge */
    result = lle_history_bridge_add_entry(large_cmd, 0, NULL);
    ASSERT_SUCCESS(result);
    
    /* Verify in LLE */
    lle_history_entry_t *entry = NULL;
    result = lle_history_get_entry_by_index(g_core, 0, &entry);
    ASSERT_SUCCESS(result);
    ASSERT(entry != NULL);
    ASSERT_EQ(strlen(entry->command), strlen(large_cmd));
    
    /* Verify in readline */
    ASSERT_EQ(history_length, 1);
    HIST_ENTRY *rl_entry = history_get(1);
    ASSERT(rl_entry != NULL);
    ASSERT_EQ(strlen(rl_entry->line), strlen(large_cmd));
    
    teardown();
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("=== LLE History Phase 2 Day 5: Lusush Integration Bridge Tests ===\n\n");
    
    /* Bridge initialization */
    run_test_bridge_init_basic();
    run_test_bridge_init_without_posix();
    run_test_bridge_double_init();
    
    /* GNU Readline sync */
    run_test_import_from_readline();
    run_test_export_to_readline();
    run_test_bidirectional_sync_readline();
    
    /* POSIX history sync */
    run_test_import_from_posix();
    run_test_export_to_posix();
    run_test_bidirectional_sync_posix();
    
    /* Auto-sync */
    run_test_auto_sync_enabled();
    run_test_auto_sync_disabled();
    
    /* History builtin */
    run_test_history_builtin_output();
    run_test_get_by_number();
    run_test_get_by_reverse_index();
    
    /* Statistics */
    run_test_bridge_statistics();
    run_test_diagnostics_output();
    
    /* Edge cases */
    run_test_ignore_empty_commands();
    run_test_large_command_sync();
    
    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\nAll tests passed!\n");
        return 0;
    } else {
        printf("\nSome tests failed!\n");
        return 1;
    }
}
