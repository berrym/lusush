/*
 * test_key_detector.c - Unit Tests for Key Sequence Detector
 *
 * Tests key sequence detection and mapping including:
 * - Function keys (F1-F12)
 * - Cursor keys (arrows, Home, End, PgUp, PgDn)
 * - Editing keys (Insert, Delete, Backspace)
 * - Control character detection
 * - Modified keys (Ctrl, Alt, Shift combinations)
 * - Ambiguous sequence timeout handling
 * - Sequence matching algorithms
 *
 * Spec 06: Input Parsing - Phase 4 Tests
 */

#include "../../../include/lle/error_handling.h"
#include "../../../include/lle/input_parsing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Test framework macros */
#define TEST(name)                                                             \
    printf("  Testing: %s...", name);                                          \
    fflush(stdout)
#define TEST_END printf(" PASS\n")
#define ASSERT(condition, message)                                             \
    do {                                                                       \
        if (!(condition)) {                                                    \
            printf("\n    FAILED: %s\n", message);                             \
            printf("    at %s:%d\n", __FILE__, __LINE__);                      \
            return;                                                            \
        }                                                                      \
    } while (0)

/* Mock terminal capabilities and memory pool */
static int mock_terminal_dummy = 42;
static int mock_pool_dummy = 43;
static lle_terminal_capabilities_t *mock_terminal =
    (lle_terminal_capabilities_t *)&mock_terminal_dummy;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t *)&mock_pool_dummy;

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/*
 * Test: Initialize and destroy key detector
 */
void test_init_destroy(void) {
    TEST("init and destroy");

    lle_key_detector_t *detector = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");
    ASSERT(detector != NULL, "Detector should not be NULL");

    result = lle_key_detector_destroy(detector);
    ASSERT(result == LLE_SUCCESS, "Destroy should succeed");

    TEST_END;
    tests_passed++;
}

/*
 * Test: Initialize with invalid parameters
 */
void test_init_invalid_params(void) {
    TEST("init with invalid parameters");

    lle_key_detector_t *detector = NULL;
    lle_result_t result;

    result = lle_key_detector_init(NULL, mock_terminal, mock_pool);
    ASSERT(result == LLE_ERROR_INVALID_PARAMETER,
           "Init with NULL detector should fail");

    result = lle_key_detector_init(&detector, NULL, mock_pool);
    ASSERT(result == LLE_ERROR_INVALID_PARAMETER,
           "Init with NULL terminal should fail");

    result = lle_key_detector_init(&detector, mock_terminal, NULL);
    ASSERT(result == LLE_ERROR_INVALID_PARAMETER,
           "Init with NULL pool should fail");

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect function key F1
 */
void test_detect_f1_key(void) {
    TEST("detect F1 key");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // F1 = ESC O P
    const char f1_sequence[] = "\x1BOP";
    result =
        lle_key_detector_process_sequence(detector, f1_sequence, 3, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_FUNCTION, "Should be function key");
    ASSERT(key_info->keycode == 1, "Should be F1");
    ASSERT(strcmp(key_info->key_name, "F1") == 0, "Key name should be F1");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect cursor key (Up arrow)
 */
void test_detect_cursor_up(void) {
    TEST("detect cursor up key");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Up = ESC [ A
    const char up_sequence[] = "\x1B[A";
    result =
        lle_key_detector_process_sequence(detector, up_sequence, 3, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_CURSOR, "Should be cursor key");
    ASSERT(strcmp(key_info->key_name, "Up") == 0, "Key name should be Up");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect control character (Ctrl+C)
 */
void test_detect_ctrl_c(void) {
    TEST("detect Ctrl+C");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Ctrl+C = 0x03
    const char ctrl_c[] = "\x03";
    result = lle_key_detector_process_sequence(detector, ctrl_c, 1, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_CONTROL, "Should be control key");
    ASSERT(key_info->keycode == 'C', "Should be C");
    ASSERT(key_info->modifiers == LLE_KEY_MOD_CTRL,
           "Should have Ctrl modifier");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect modified cursor key (Shift+Up)
 */
void test_detect_shift_up(void) {
    TEST("detect Shift+Up");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Shift+Up = ESC[1;2A
    const char shift_up[] = "\x1B[1;2A";
    result =
        lle_key_detector_process_sequence(detector, shift_up, 6, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_CURSOR, "Should be cursor key");
    ASSERT(key_info->modifiers == LLE_KEY_MOD_SHIFT,
           "Should have Shift modifier");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect modified cursor key (Ctrl+Right)
 */
void test_detect_ctrl_right(void) {
    TEST("detect Ctrl+Right");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Ctrl+Right = ESC[1;5C
    const char ctrl_right[] = "\x1B[1;5C";
    result =
        lle_key_detector_process_sequence(detector, ctrl_right, 6, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_CURSOR, "Should be cursor key");
    ASSERT(key_info->modifiers == LLE_KEY_MOD_CTRL,
           "Should have Ctrl modifier");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect Home key
 */
void test_detect_home_key(void) {
    TEST("detect Home key");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Home = ESC[H
    const char home[] = "\x1B[H";
    result = lle_key_detector_process_sequence(detector, home, 3, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_CURSOR, "Should be cursor key");
    ASSERT(strcmp(key_info->key_name, "Home") == 0, "Key name should be Home");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect Delete key
 */
void test_detect_delete_key(void) {
    TEST("detect Delete key");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Delete = ESC[3~
    const char delete_key[] = "\x1B[3~";
    result =
        lle_key_detector_process_sequence(detector, delete_key, 4, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_EDITING, "Should be editing key");
    ASSERT(strcmp(key_info->key_name, "Delete") == 0,
           "Key name should be Delete");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Detect Tab key
 */
void test_detect_tab_key(void) {
    TEST("detect Tab key");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Tab = 0x09
    const char tab[] = "\x09";
    result = lle_key_detector_process_sequence(detector, tab, 1, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info != NULL, "Should detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_SPECIAL, "Should be special key");
    ASSERT(strcmp(key_info->key_name, "Tab") == 0, "Key name should be Tab");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Partial sequence detection
 */
void test_partial_sequence(void) {
    TEST("partial sequence detection");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Send partial sequence: ESC[
    const char partial[] = "\x1B[";
    result = lle_key_detector_process_sequence(detector, partial, 2, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info == NULL, "Should not detect key yet (prefix match)");

    // Check if waiting for more data
    bool is_waiting = lle_key_detector_is_waiting(detector);
    ASSERT(is_waiting == true, "Should be waiting for more data");

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Complete partial sequence
 */
void test_complete_partial_sequence(void) {
    TEST("complete partial sequence");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Send first part: ESC[
    const char part1[] = "\x1B[";
    result = lle_key_detector_process_sequence(detector, part1, 2, &key_info);
    ASSERT(result == LLE_SUCCESS, "First process should succeed");
    ASSERT(key_info == NULL, "Should not detect key yet");

    // Send second part: A
    const char part2[] = "A";
    result = lle_key_detector_process_sequence(detector, part2, 1, &key_info);
    ASSERT(result == LLE_SUCCESS, "Second process should succeed");
    ASSERT(key_info != NULL, "Should now detect key");
    ASSERT(key_info->type == LLE_KEY_TYPE_CURSOR, "Should be cursor key");

    if (key_info) {
        lle_pool_free(key_info);
    }

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Reset detector
 */
void test_reset_detector(void) {
    TEST("reset detector");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Send partial sequence
    const char partial[] = "\x1B[";
    result = lle_key_detector_process_sequence(detector, partial, 2, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");

    bool is_waiting = lle_key_detector_is_waiting(detector);
    ASSERT(is_waiting == true, "Should be waiting");

    // Reset
    result = lle_key_detector_reset(detector);
    ASSERT(result == LLE_SUCCESS, "Reset should succeed");

    is_waiting = lle_key_detector_is_waiting(detector);
    ASSERT(is_waiting == false, "Should not be waiting after reset");

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: Get statistics
 */
void test_get_statistics(void) {
    TEST("get statistics");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Detect a few keys
    const char f1[] = "\x1BOP";
    result = lle_key_detector_process_sequence(detector, f1, 3, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    if (key_info) {
        lle_pool_free(key_info);
        key_info = NULL;
    }

    const char up[] = "\x1B[A";
    result = lle_key_detector_process_sequence(detector, up, 3, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    if (key_info) {
        lle_pool_free(key_info);
        key_info = NULL;
    }

    // Get stats
    uint64_t detected, resolved, timeouts;
    result =
        lle_key_detector_get_stats(detector, &detected, &resolved, &timeouts);
    ASSERT(result == LLE_SUCCESS, "Get stats should succeed");
    ASSERT(detected >= 2, "Should have detected at least 2 sequences");
    ASSERT(resolved >= 2, "Should have resolved at least 2 sequences");

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Test: No match for unknown sequence
 */
void test_unknown_sequence(void) {
    TEST("unknown sequence");

    lle_key_detector_t *detector = NULL;
    lle_key_info_t *key_info = NULL;
    lle_result_t result;

    result = lle_key_detector_init(&detector, mock_terminal, mock_pool);
    ASSERT(result == LLE_SUCCESS, "Init should succeed");

    // Send unknown sequence
    const char unknown[] = "\x1B[999Z";
    result = lle_key_detector_process_sequence(detector, unknown, 6, &key_info);
    ASSERT(result == LLE_SUCCESS, "Process should succeed");
    ASSERT(key_info == NULL, "Should not detect key for unknown sequence");

    lle_key_detector_destroy(detector);

    TEST_END;
    tests_passed++;
}

/*
 * Main test runner
 */
int main(void) {
    printf("\n=== LLE Key Detector Unit Tests ===\n\n");

    tests_run = 0;
    tests_passed = 0;

#define RUN_TEST(test)                                                         \
    do {                                                                       \
        tests_run++;                                                           \
        test();                                                                \
    } while (0)

    RUN_TEST(test_init_destroy);
    RUN_TEST(test_init_invalid_params);
    RUN_TEST(test_detect_f1_key);
    RUN_TEST(test_detect_cursor_up);
    RUN_TEST(test_detect_ctrl_c);
    RUN_TEST(test_detect_shift_up);
    RUN_TEST(test_detect_ctrl_right);
    RUN_TEST(test_detect_home_key);
    RUN_TEST(test_detect_delete_key);
    RUN_TEST(test_detect_tab_key);
    RUN_TEST(test_partial_sequence);
    RUN_TEST(test_complete_partial_sequence);
    RUN_TEST(test_reset_detector);
    RUN_TEST(test_get_statistics);
    RUN_TEST(test_unknown_sequence);

    printf("\n=== Test Summary ===\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        printf("\n✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n\n");
        return 1;
    }
}
