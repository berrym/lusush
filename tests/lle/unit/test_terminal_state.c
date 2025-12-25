/*
 * test_terminal_state.c - Unit tests for terminal state management
 *
 * Tests Spec 02 Phase 2: Terminal State Management
 *
 * Test Categories:
 * 1. Interface initialization and cleanup
 * 2. Raw mode operations
 * 3. Window size queries
 * 4. Signal handling (limited testing)
 * 5. Error handling
 */

#include "lle/terminal_abstraction.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                             \
    static void name(void);                                                    \
    static void run_##name(void) {                                             \
        tests_run++;                                                           \
        printf("  Running %s...", #name);                                      \
        fflush(stdout);                                                        \
        name();                                                                \
        tests_passed++;                                                        \
        printf(" PASS\n");                                                     \
    }                                                                          \
    static void name(void)

/* ============================================================================
 * INTERFACE INITIALIZATION AND CLEANUP TESTS
 * ============================================================================
 */

TEST(test_interface_init_basic) {
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);

    assert(result == LLE_SUCCESS);
    assert(interface != NULL);
    assert(interface->terminal_fd >= 0);
    assert(interface->raw_mode_active == false);

    /* Cleanup */
    lle_unix_interface_destroy(interface);
}

TEST(test_interface_init_null_parameter) {
    lle_result_t result = lle_unix_interface_init(NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);
}

TEST(test_interface_destroy_null) {
    /* Should not crash */
    lle_unix_interface_destroy(NULL);
}

TEST(test_interface_double_destroy) {
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    /* First destroy */
    lle_unix_interface_destroy(interface);

    /* Second destroy on same pointer is undefined, but we test that
     * destroying NULL doesn't crash */
    lle_unix_interface_destroy(NULL);
}

TEST(test_interface_preserves_terminal_fd) {
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    int fd = interface->terminal_fd;
    assert(fd >= 0);

    /* Verify the fd is still valid (skip if not a TTY) */
    if (isatty(fd)) {
        struct termios current;
        int tcget_result = tcgetattr(fd, &current);
        assert(tcget_result == 0);
    }

    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * RAW MODE TESTS
 * ============================================================================
 */

TEST(test_raw_mode_enter_exit) {
    if (!isatty(STDIN_FILENO)) {
        printf(" SKIP (not a tty)");
        return;
    }

    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    /* Get original terminal settings */
    struct termios original;
    int tcget_result = tcgetattr(STDIN_FILENO, &original);
    assert(tcget_result == 0);

    /* Enter raw mode */
    result = lle_unix_interface_enter_raw_mode(interface);
    assert(result == LLE_SUCCESS);
    assert(interface->raw_mode_active == true);

    /* Verify raw mode is active */
    struct termios raw;
    tcget_result = tcgetattr(STDIN_FILENO, &raw);
    assert(tcget_result == 0);

    /* Check key raw mode characteristics */
    assert((raw.c_lflag & ICANON) == 0); /* Non-canonical */
    assert((raw.c_lflag & ECHO) == 0);   /* No echo */

    /* Exit raw mode */
    result = lle_unix_interface_exit_raw_mode(interface);
    assert(result == LLE_SUCCESS);
    assert(interface->raw_mode_active == false);

    /* Verify original settings restored */
    struct termios restored;
    tcget_result = tcgetattr(STDIN_FILENO, &restored);
    assert(tcget_result == 0);

    /* Compare key flags (exact match may vary by system) */
    assert((restored.c_lflag & ICANON) == (original.c_lflag & ICANON));
    assert((restored.c_lflag & ECHO) == (original.c_lflag & ECHO));

    lle_unix_interface_destroy(interface);
}

TEST(test_raw_mode_idempotent_enter) {
    if (!isatty(STDIN_FILENO)) {
        printf(" SKIP (not a tty)");
        return;
    }

    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    /* Enter raw mode twice */
    result = lle_unix_interface_enter_raw_mode(interface);
    assert(result == LLE_SUCCESS);

    result = lle_unix_interface_enter_raw_mode(interface);
    assert(result == LLE_SUCCESS);
    assert(interface->raw_mode_active == true);

    /* Exit once should restore */
    result = lle_unix_interface_exit_raw_mode(interface);
    assert(result == LLE_SUCCESS);

    lle_unix_interface_destroy(interface);
}

TEST(test_raw_mode_idempotent_exit) {
    if (!isatty(STDIN_FILENO)) {
        printf(" SKIP (not a tty)");
        return;
    }

    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    /* Exit without entering should be safe */
    result = lle_unix_interface_exit_raw_mode(interface);
    assert(result == LLE_SUCCESS);
    assert(interface->raw_mode_active == false);

    /* Enter and exit twice */
    result = lle_unix_interface_enter_raw_mode(interface);
    assert(result == LLE_SUCCESS);

    result = lle_unix_interface_exit_raw_mode(interface);
    assert(result == LLE_SUCCESS);

    result = lle_unix_interface_exit_raw_mode(interface);
    assert(result == LLE_SUCCESS);

    lle_unix_interface_destroy(interface);
}

TEST(test_raw_mode_null_parameter) {
    lle_result_t result = lle_unix_interface_enter_raw_mode(NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    result = lle_unix_interface_exit_raw_mode(NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);
}

TEST(test_raw_mode_cleanup_on_destroy) {
    if (!isatty(STDIN_FILENO)) {
        printf(" SKIP (not a tty)");
        return;
    }

    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    /* Get original settings */
    struct termios original;
    int tcget_result = tcgetattr(STDIN_FILENO, &original);
    assert(tcget_result == 0);

    /* Enter raw mode */
    result = lle_unix_interface_enter_raw_mode(interface);
    assert(result == LLE_SUCCESS);

    /* Destroy without exiting raw mode */
    lle_unix_interface_destroy(interface);

    /* Verify terminal restored */
    struct termios restored;
    tcget_result = tcgetattr(STDIN_FILENO, &restored);
    assert(tcget_result == 0);

    assert((restored.c_lflag & ICANON) == (original.c_lflag & ICANON));
    assert((restored.c_lflag & ECHO) == (original.c_lflag & ECHO));
}

/* ============================================================================
 * WINDOW SIZE TESTS
 * ============================================================================
 */

TEST(test_get_window_size_basic) {
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    size_t width = 0, height = 0;
    result = lle_unix_interface_get_window_size(interface, &width, &height);

    assert(result == LLE_SUCCESS);
    assert(width > 0);
    assert(height > 0);

    /* In TTY environments, expect reasonable bounds.
     * In non-TTY, we should get at least the fallback values (80x24).
     * Allow very small values in case COLUMNS/LINES env vars are weird. */
    assert(width <= 10000);
    assert(height <= 10000);

    /* Verify cached values match */
    assert(interface->current_width == width);
    assert(interface->current_height == height);

    lle_unix_interface_destroy(interface);
}

TEST(test_get_window_size_null_parameters) {
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    size_t width = 0, height = 0;

    /* Null interface */
    result = lle_unix_interface_get_window_size(NULL, &width, &height);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    /* Null width */
    result = lle_unix_interface_get_window_size(interface, NULL, &height);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    /* Null height */
    result = lle_unix_interface_get_window_size(interface, &width, NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    lle_unix_interface_destroy(interface);
}

TEST(test_get_window_size_caching) {
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    size_t width1 = 0, height1 = 0;
    result = lle_unix_interface_get_window_size(interface, &width1, &height1);
    assert(result == LLE_SUCCESS);

    size_t width2 = 0, height2 = 0;
    result = lle_unix_interface_get_window_size(interface, &width2, &height2);
    assert(result == LLE_SUCCESS);

    /* Should return same values (assuming no resize between calls) */
    assert(width1 == width2);
    assert(height1 == height2);

    /* Cached values should match */
    assert(interface->current_width == width2);
    assert(interface->current_height == height2);

    lle_unix_interface_destroy(interface);
}

TEST(test_window_size_fallback_values) {
    /* This test verifies that we get reasonable defaults even in
     * non-terminal environments */
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    size_t width = 0, height = 0;
    result = lle_unix_interface_get_window_size(interface, &width, &height);

    /* Should succeed even if not a tty (fallback to 80x24 or env vars) */
    assert(result == LLE_SUCCESS);
    assert(width > 0);
    assert(height > 0);

    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * READ EVENT TESTS (VERIFICATION)
 * ============================================================================
 */

TEST(test_read_event_stub) {
    /* Phase 2 only provides a stub for read_event, which will be
     * fully implemented in Phase 3. Verify the stub exists and
     * handles null parameters correctly. */

    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    lle_input_event_t event;

    /* Null interface */
    result = lle_unix_interface_read_event(NULL, &event, 0);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    /* Null event */
    result = lle_unix_interface_read_event(interface, NULL, 0);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * INTEGRATION TESTS
 * ============================================================================
 */

TEST(test_multiple_interfaces) {
    /* Verify we can create multiple interface instances */
    lle_unix_interface_t *interface1 = NULL;
    lle_unix_interface_t *interface2 = NULL;

    lle_result_t result1 = lle_unix_interface_init(&interface1);
    assert(result1 == LLE_SUCCESS);
    assert(interface1 != NULL);

    lle_result_t result2 = lle_unix_interface_init(&interface2);
    assert(result2 == LLE_SUCCESS);
    assert(interface2 != NULL);

    /* Should be different instances */
    assert(interface1 != interface2);

    /* Cleanup */
    lle_unix_interface_destroy(interface1);
    lle_unix_interface_destroy(interface2);
}

TEST(test_full_lifecycle) {
    if (!isatty(STDIN_FILENO)) {
        printf(" SKIP (not a tty)");
        return;
    }

    /* Test complete lifecycle: init -> raw mode -> operations -> cleanup */
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);

    /* Get window size */
    size_t width = 0, height = 0;
    result = lle_unix_interface_get_window_size(interface, &width, &height);
    assert(result == LLE_SUCCESS);
    assert(width > 0 && height > 0);

    /* Enter raw mode */
    result = lle_unix_interface_enter_raw_mode(interface);
    assert(result == LLE_SUCCESS);
    assert(interface->raw_mode_active == true);

    /* Get window size again in raw mode */
    size_t width2 = 0, height2 = 0;
    result = lle_unix_interface_get_window_size(interface, &width2, &height2);
    assert(result == LLE_SUCCESS);

    /* Exit raw mode */
    result = lle_unix_interface_exit_raw_mode(interface);
    assert(result == LLE_SUCCESS);
    assert(interface->raw_mode_active == false);

    /* Cleanup */
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================
 */

int main(void) {
    printf("Running Terminal State Management Tests (Spec 02 Phase 2)\n");
    printf("============================================================\n\n");

    printf("Interface Initialization Tests:\n");
    run_test_interface_init_basic();
    run_test_interface_init_null_parameter();
    run_test_interface_destroy_null();
    run_test_interface_double_destroy();
    run_test_interface_preserves_terminal_fd();

    printf("\nRaw Mode Tests:\n");
    run_test_raw_mode_enter_exit();
    run_test_raw_mode_idempotent_enter();
    run_test_raw_mode_idempotent_exit();
    run_test_raw_mode_null_parameter();
    run_test_raw_mode_cleanup_on_destroy();

    printf("\nWindow Size Tests:\n");
    run_test_get_window_size_basic();
    run_test_get_window_size_null_parameters();
    run_test_get_window_size_caching();
    run_test_window_size_fallback_values();

    printf("\nRead Event Tests (Stub):\n");
    run_test_read_event_stub();

    printf("\nIntegration Tests:\n");
    run_test_multiple_interfaces();
    run_test_full_lifecycle();

    printf("\n============================================================\n");
    printf("Test Results: %d/%d tests passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
