/**
 * @file test_pty_basic_display.c
 * @brief Basic PTY Display Tests for LLE
 * 
 * These tests validate that LLE actually displays output correctly
 * in a pseudo-terminal environment, addressing the critical compliance
 * violation of having zero automated terminal output tests.
 * 
 * Test Coverage:
 * 1. Prompt displays
 * 2. Command text echoes
 * 3. Backspace deletes visually
 * 4. Cursor movement works
 * 5. Multiline commands display
 */

#include "pty_test_harness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ========================================================================== */
/*                         TEST INFRASTRUCTURE                                */
/* ========================================================================== */

typedef struct test_suite_t {
    int total_tests;
    int passed_tests;
    int failed_tests;
    double total_duration_ms;
} test_suite_t;

static test_suite_t suite = {0, 0, 0, 0.0};

#define RUN_TEST(test_func) \
    do { \
        suite.total_tests++; \
        printf("\n"); \
        test_func(); \
    } while (0)

static void test_passed(const char *test_name, double duration_ms) {
    suite.passed_tests++;
    suite.total_duration_ms += duration_ms;
    printf("✓ PASS: %s (%.2fms)\n", test_name, duration_ms);
}

static void test_failed(const char *test_name, const char *reason) {
    suite.failed_tests++;
    printf("✗ FAIL: %s\n", test_name);
    printf("  Reason: %s\n", reason);
}

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/* ========================================================================== */
/*                         TEST 1: PROMPT DISPLAYS                            */
/* ========================================================================== */

static void test_pty_simple_prompt(void) {
    const char *TEST_NAME = "Simple Prompt Display";
    double start_time = get_time_ms();
    
    pty_session_t *session = NULL;
    char error[256] = {0};

    /* Create PTY with standard dimensions */
    if (pty_session_create(&session, 80, 24) != 0) {
        test_failed(TEST_NAME, "Failed to create PTY");
        return;
    }

    /* Spawn lusush with simple command that exits */
    if (pty_session_spawn_lusush(session, "exit", "xterm-256color") != 0) {
        test_failed(TEST_NAME, "Failed to spawn lusush");
        pty_session_close(session);
        return;
    }

    /* Wait for output (should see prompt before "exit" executes) */
    pty_session_read_output(session, 500);

    /* Validate output */
    output_expectation_t expect = {
        .contains = NULL,  /* We'll check for any output */
        .not_contains = NULL,
        .requires_color = false,  /* Don't require color for basic test */
        .requires_cursor_move = false,
        .min_length = 1,  /* At least some output */
        .max_length = 0
    };

    if (!pty_validate_output(session, &expect, error, sizeof(error))) {
        test_failed(TEST_NAME, error);
        pty_session_close(session);
        return;
    }

    /* Check that SOMETHING was output (prompt or command result) */
    if (session->output_length == 0) {
        test_failed(TEST_NAME, "No output captured from PTY");
        pty_session_close(session);
        return;
    }

    pty_session_close(session);
    test_passed(TEST_NAME, get_time_ms() - start_time);
}

/* ========================================================================== */
/*                     TEST 2: COMMAND TEXT DISPLAYS                          */
/* ========================================================================== */

static void test_pty_command_echo(void) {
    const char *TEST_NAME = "Command Text Echo";
    double start_time = get_time_ms();
    
    pty_session_t *session = NULL;
    char error[256] = {0};

    if (pty_session_create(&session, 80, 24) != 0) {
        test_failed(TEST_NAME, "Failed to create PTY");
        return;
    }

    /* Spawn interactive lusush */
    if (pty_session_spawn_lusush(session, NULL, "xterm-256color") != 0) {
        test_failed(TEST_NAME, "Failed to spawn lusush");
        pty_session_close(session);
        return;
    }

    /* Wait for initial prompt */
    if (!pty_wait_for_prompt(session, 2000)) {
        test_failed(TEST_NAME, "Timeout waiting for initial prompt");
        pty_session_close(session);
        return;
    }

    /* Clear output buffer to start fresh */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    /* Send simple command */
    const char *test_command = "echo test";
    pty_session_send_input(session, test_command, 0);
    
    /* Wait for echo - give more time */
    pty_session_read_output(session, 1000);

    /* Validate that the command appears in output */
    output_expectation_t expect = {
        .contains = "echo",  /* Should see the command */
        .not_contains = NULL,
        .requires_color = false,
        .requires_cursor_move = false,
        .min_length = 4,  /* At least "echo" */
        .max_length = 0
    };

    if (!pty_validate_output(session, &expect, error, sizeof(error))) {
        test_failed(TEST_NAME, error);
        printf("  Output captured: '%s'\n", session->output_buffer);
        pty_session_close(session);
        return;
    }

    /* Send Enter to execute */
    pty_session_send_key(session, "enter");
    pty_session_read_output(session, 500);

    /* Should see "test" in output */
    if (strstr(session->output_buffer, "test") == NULL) {
        test_failed(TEST_NAME, "Command did not execute (no 'test' in output)");
        printf("  Output captured: '%s'\n", session->output_buffer);
        pty_session_close(session);
        return;
    }

    /* Cleanup */
    pty_session_send_input(session, "exit\n", 0);
    pty_session_read_output(session, 200);
    
    pty_session_close(session);
    test_passed(TEST_NAME, get_time_ms() - start_time);
}

/* ========================================================================== */
/*                     TEST 3: BACKSPACE DELETES                              */
/* ========================================================================== */

static void test_pty_backspace(void) {
    const char *TEST_NAME = "Backspace Deletion";
    double start_time = get_time_ms();
    
    pty_session_t *session = NULL;

    if (pty_session_create(&session, 80, 24) != 0) {
        test_failed(TEST_NAME, "Failed to create PTY");
        return;
    }

    if (pty_session_spawn_lusush(session, NULL, "xterm-256color") != 0) {
        test_failed(TEST_NAME, "Failed to spawn lusush");
        pty_session_close(session);
        return;
    }

    /* Wait for prompt */
    if (!pty_wait_for_prompt(session, 1000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    /* Type "testXX" */
    pty_session_send_input(session, "testXX", 0);
    pty_session_read_output(session, 200);

    /* Send two backspaces to delete "XX" */
    pty_session_send_key(session, "backspace");
    pty_session_send_key(session, "backspace");
    pty_session_read_output(session, 200);

    /* Type final text */
    pty_session_send_input(session, "123", 0);
    pty_session_read_output(session, 200);

    /* Clear buffer to check only the final command execution */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    /* Execute command */
    pty_session_send_input(session, "\n", 0);
    pty_session_read_output(session, 500);

    /* Verify output contains "test123" (not "testXX123") */
    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));

    if (strstr(plain, "testXX") != NULL) {
        test_failed(TEST_NAME, "Backspace did not delete characters (testXX still present)");
        printf("  Plain output: '%s'\n", plain);
        pty_session_close(session);
        return;
    }

    if (strstr(plain, "test123") == NULL) {
        test_failed(TEST_NAME, "Expected 'test123' not found in output");
        printf("  Plain output: '%s'\n", plain);
        pty_session_close(session);
        return;
    }

    pty_session_send_input(session, "exit\n", 0);
    pty_session_close(session);
    test_passed(TEST_NAME, get_time_ms() - start_time);
}

/* ========================================================================== */
/*                     TEST 4: CURSOR MOVEMENT                                */
/* ========================================================================== */

static void test_pty_cursor_movement(void) {
    const char *TEST_NAME = "Cursor Movement (Arrow Keys)";
    double start_time = get_time_ms();
    
    pty_session_t *session = NULL;

    if (pty_session_create(&session, 80, 24) != 0) {
        test_failed(TEST_NAME, "Failed to create PTY");
        return;
    }

    if (pty_session_spawn_lusush(session, NULL, "xterm-256color") != 0) {
        test_failed(TEST_NAME, "Failed to spawn lusush");
        pty_session_close(session);
        return;
    }

    if (!pty_wait_for_prompt(session, 1000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    /* Type "hello" */
    pty_session_send_input(session, "hello", 0);
    pty_session_read_output(session, 200);

    /* Move left twice */
    pty_session_send_key(session, "left");
    pty_session_send_key(session, "left");
    pty_session_read_output(session, 200);

    /* Insert "XX" in the middle */
    pty_session_send_input(session, "XX", 0);
    pty_session_read_output(session, 200);

    /* Result should be "helXXlo" */
    pty_session_send_input(session, "\n", 0);
    pty_session_read_output(session, 500);

    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));

    if (strstr(plain, "helXXlo") == NULL) {
        test_failed(TEST_NAME, "Cursor movement did not work (expected 'helXXlo')");
        printf("  Plain output: '%s'\n", plain);
        pty_session_close(session);
        return;
    }

    pty_session_send_input(session, "exit\n", 0);
    pty_session_close(session);
    test_passed(TEST_NAME, get_time_ms() - start_time);
}

/* ========================================================================== */
/*                     TEST 5: MULTILINE COMMANDS                             */
/* ========================================================================== */

static void test_pty_multiline(void) {
    const char *TEST_NAME = "Multiline Command Display";
    double start_time = get_time_ms();
    
    pty_session_t *session = NULL;

    if (pty_session_create(&session, 80, 24) != 0) {
        test_failed(TEST_NAME, "Failed to create PTY");
        return;
    }

    if (pty_session_spawn_lusush(session, NULL, "xterm-256color") != 0) {
        test_failed(TEST_NAME, "Failed to spawn lusush");
        pty_session_close(session);
        return;
    }

    if (!pty_wait_for_prompt(session, 1000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    /* Start a multiline command with unclosed quote */
    pty_session_send_input(session, "echo \"line1", 0);
    pty_session_send_input(session, "\n", 0);
    pty_session_read_output(session, 500);

    /* Should see continuation prompt (PS2) */
    /* For now, just verify we got SOME output */
    if (session->output_length == 0) {
        test_failed(TEST_NAME, "No output after multiline start");
        pty_session_close(session);
        return;
    }

    /* Clear buffer before completing command to check final output */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    /* Complete the command */
    pty_session_send_input(session, "line2\"", 0);
    pty_session_send_input(session, "\n", 0);
    pty_session_read_output(session, 500);

    /* Should see both lines in output */
    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));

    if (strstr(plain, "line1") == NULL || strstr(plain, "line2") == NULL) {
        test_failed(TEST_NAME, "Multiline command did not execute correctly");
        printf("  Plain output: '%s'\n", plain);
        pty_session_close(session);
        return;
    }

    pty_session_send_input(session, "exit\n", 0);
    pty_session_close(session);
    test_passed(TEST_NAME, get_time_ms() - start_time);
}

/* ========================================================================== */
/*                         MAIN TEST RUNNER                                   */
/* ========================================================================== */

int main(void) {
    printf("=========================================================\n");
    printf("LLE PTY Basic Display Tests\n");
    printf("=========================================================\n");
    printf("\n");
    printf("These tests validate actual terminal output using PTY.\n");
    printf("They address the critical compliance violation of having\n");
    printf("zero automated tests for terminal rendering.\n");
    printf("\n");

    /* Run all tests */
    RUN_TEST(test_pty_simple_prompt);
    RUN_TEST(test_pty_command_echo);
    RUN_TEST(test_pty_backspace);
    RUN_TEST(test_pty_cursor_movement);
    RUN_TEST(test_pty_multiline);

    /* Print summary */
    printf("\n");
    printf("=========================================================\n");
    printf("Test Summary\n");
    printf("=========================================================\n");
    printf("Total:  %d tests\n", suite.total_tests);
    printf("Passed: %d tests\n", suite.passed_tests);
    printf("Failed: %d tests\n", suite.failed_tests);
    printf("Time:   %.2f ms\n", suite.total_duration_ms);
    printf("\n");

    if (suite.failed_tests > 0) {
        printf("❌ SOME TESTS FAILED\n");
        return 1;
    } else {
        printf("✅ ALL TESTS PASSED\n");
        return 0;
    }
}
