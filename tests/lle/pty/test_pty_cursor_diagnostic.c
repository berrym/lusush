/*
 * LLE PTY Cursor Movement Diagnostic Tests
 * 
 * These tests are designed to isolate and diagnose the cursor movement bug
 * identified in basic tests. Each test is minimal and focused on specific
 * cursor operations to pinpoint the exact failure mode.
 * 
 * Bug Symptom: When left arrow keys are pressed, display shows corruption
 * with prompt fragments appearing in command text (e.g., "hello" → "helloberry@")
 */

#include "pty_test_harness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* ========================================================================== */
/*                         HELPER FUNCTIONS                                   */
/* ========================================================================== */

static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static void test_passed(const char *name, double elapsed_ms) {
    printf("✓ PASS: %s (%.2fms)\n", name, elapsed_ms);
}

static void test_failed(const char *name, const char *reason) {
    printf("✗ FAIL: %s\n", name);
    printf("  Reason: %s\n", reason);
}

/* Helper to dump raw output with visible escape sequences */
static void dump_raw_output(const char *output, size_t length) {
    printf("  Raw output (%zu bytes):\n  ", length);
    for (size_t i = 0; i < length && i < 500; i++) {
        unsigned char c = output[i];
        if (c == '\033') {
            printf("\\e");
        } else if (c == '\r') {
            printf("\\r");
        } else if (c == '\n') {
            printf("\\n\n  ");
        } else if (c == '\t') {
            printf("\\t");
        } else if (c >= 32 && c < 127) {
            printf("%c", c);
        } else {
            printf("\\x%02x", c);
        }
    }
    printf("\n");
}

/* Helper to extract the last command line from output */
static bool extract_last_command_line(const char *output, char *command, size_t command_size) {
    /* Strategy: Find the last occurrence of " $ " (prompt end), 
       then extract everything after it until newline or end */
    
    const char *last_prompt = NULL;
    const char *ptr = output;
    
    /* Find last occurrence of " $ " */
    while ((ptr = strstr(ptr, " $ ")) != NULL) {
        last_prompt = ptr + 3;  /* Point after " $ " */
        ptr += 3;
    }
    
    if (!last_prompt) {
        return false;
    }
    
    /* Extract command text after last prompt */
    size_t i = 0;
    while (*last_prompt && *last_prompt != '\r' && *last_prompt != '\n' && i < command_size - 1) {
        command[i++] = *last_prompt++;
    }
    command[i] = '\0';
    
    return i > 0;
}

/* ========================================================================== */
/*                     TEST 1: ABSOLUTE MINIMAL - 2 CHARS                     */
/* ========================================================================== */

static void test_minimal_two_chars(void) {
    const char *TEST_NAME = "Minimal: Type 'ab', left arrow, type 'X' → 'aXb'";
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

    if (!pty_wait_for_prompt(session, 2000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    /* Clear buffer after prompt */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    /* Type "ab" */
    pty_session_send_input(session, "ab", 0);
    pty_session_read_output(session, 100);

    /* Move left once */
    pty_session_send_key(session, "left");
    pty_session_read_output(session, 100);

    /* Type "X" */
    pty_session_send_input(session, "X", 0);
    pty_session_read_output(session, 100);

    /* Dump raw output for analysis */
    printf("\n  After typing 'ab', left arrow, 'X':\n");
    dump_raw_output(session->output_buffer, session->output_length);

    /* Extract last command line */
    char command[256];
    if (extract_last_command_line(session->output_buffer, command, sizeof(command))) {
        printf("  Extracted command: '%s'\n", command);
    }

    /* Strip ANSI codes and check */
    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));
    
    if (strstr(plain, "aXb") != NULL) {
        pty_session_send_input(session, "\x03", 0);  /* Ctrl-C to cancel */
        pty_session_close(session);
        test_passed(TEST_NAME, get_time_ms() - start_time);
        return;
    }

    /* Test failed - show what we got */
    test_failed(TEST_NAME, "Expected 'aXb' not found in output");
    printf("  Plain output: '%s'\n", plain);
    
    pty_session_send_input(session, "\x03", 0);
    pty_session_close(session);
}

/* ========================================================================== */
/*                     TEST 2: SINGLE CHARACTER + LEFT                        */
/* ========================================================================== */

static void test_single_char_left(void) {
    const char *TEST_NAME = "Single char: Type 'a', left arrow, type 'X' → 'Xa'";
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

    if (!pty_wait_for_prompt(session, 2000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    /* Clear buffer */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    /* Type "a" */
    pty_session_send_input(session, "a", 0);
    pty_session_read_output(session, 100);

    /* Move left */
    pty_session_send_key(session, "left");
    pty_session_read_output(session, 100);

    /* Type "X" */
    pty_session_send_input(session, "X", 0);
    pty_session_read_output(session, 100);

    printf("\n  After typing 'a', left arrow, 'X':\n");
    dump_raw_output(session->output_buffer, session->output_length);

    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));
    
    if (strstr(plain, "Xa") != NULL) {
        pty_session_send_input(session, "\x03", 0);
        pty_session_close(session);
        test_passed(TEST_NAME, get_time_ms() - start_time);
        return;
    }

    test_failed(TEST_NAME, "Expected 'Xa' not found");
    printf("  Plain output: '%s'\n", plain);
    
    pty_session_send_input(session, "\x03", 0);
    pty_session_close(session);
}

/* ========================================================================== */
/*                     TEST 3: THREE CHARS - LEFT ONCE                        */
/* ========================================================================== */

static void test_three_chars_left_once(void) {
    const char *TEST_NAME = "Three chars: Type 'abc', left once, type 'X' → 'abXc'";
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

    if (!pty_wait_for_prompt(session, 2000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    pty_session_send_input(session, "abc", 0);
    pty_session_read_output(session, 100);

    pty_session_send_key(session, "left");
    pty_session_read_output(session, 100);

    pty_session_send_input(session, "X", 0);
    pty_session_read_output(session, 100);

    printf("\n  After typing 'abc', left arrow, 'X':\n");
    dump_raw_output(session->output_buffer, session->output_length);

    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));
    
    if (strstr(plain, "abXc") != NULL) {
        pty_session_send_input(session, "\x03", 0);
        pty_session_close(session);
        test_passed(TEST_NAME, get_time_ms() - start_time);
        return;
    }

    test_failed(TEST_NAME, "Expected 'abXc' not found");
    printf("  Plain output: '%s'\n", plain);
    
    pty_session_send_input(session, "\x03", 0);
    pty_session_close(session);
}

/* ========================================================================== */
/*                     TEST 4: JUST LEFT ARROW - NO INSERT                    */
/* ========================================================================== */

static void test_just_left_arrow(void) {
    const char *TEST_NAME = "Just arrow: Type 'test', left arrow (no insert)";
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

    if (!pty_wait_for_prompt(session, 2000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    pty_session_send_input(session, "test", 0);
    pty_session_read_output(session, 100);

    printf("\n  After typing 'test':\n");
    dump_raw_output(session->output_buffer, session->output_length);

    /* Now press left arrow */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    pty_session_send_key(session, "left");
    pty_session_read_output(session, 100);

    printf("\n  After pressing left arrow:\n");
    dump_raw_output(session->output_buffer, session->output_length);

    /* Check if output still contains "test" without corruption */
    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));
    
    /* The display should be redrawn with cursor moved left, but text should be "test" */
    if (strstr(plain, "test") != NULL && strstr(plain, "berry") == NULL) {
        pty_session_send_input(session, "\x03", 0);
        pty_session_close(session);
        test_passed(TEST_NAME, get_time_ms() - start_time);
        return;
    }

    test_failed(TEST_NAME, "Display corrupted after left arrow");
    printf("  Plain output: '%s'\n", plain);
    
    pty_session_send_input(session, "\x03", 0);
    pty_session_close(session);
}

/* ========================================================================== */
/*                     TEST 5: ANSI SEQUENCE ANALYSIS                         */
/* ========================================================================== */

static void test_ansi_sequence_analysis(void) {
    const char *TEST_NAME = "ANSI analysis: Type 'ab', left, show exact sequences";
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

    if (!pty_wait_for_prompt(session, 2000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    /* Capture sequence for typing 'a' */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);
    
    pty_session_send_input(session, "a", 0);
    pty_session_read_output(session, 100);
    
    printf("\n  Sequence after typing 'a':\n");
    dump_raw_output(session->output_buffer, session->output_length);

    /* Analyze ANSI codes */
    ansi_analysis_t analysis;
    pty_analyze_ansi_output(session->output_buffer, &analysis);
    printf("  Has color codes: %s\n", analysis.has_color_codes ? "yes" : "no");
    printf("  Has cursor positioning: %s\n", analysis.has_cursor_positioning ? "yes" : "no");
    printf("  Cursor moves: %d\n", analysis.cursor_moves);
    printf("  Color changes: %d\n", analysis.color_count);

    /* Capture sequence for typing 'b' */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);
    
    pty_session_send_input(session, "b", 0);
    pty_session_read_output(session, 100);
    
    printf("\n  Sequence after typing 'b':\n");
    dump_raw_output(session->output_buffer, session->output_length);

    /* Capture sequence for left arrow */
    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);
    
    pty_session_send_key(session, "left");
    pty_session_read_output(session, 100);
    
    printf("\n  Sequence after left arrow:\n");
    dump_raw_output(session->output_buffer, session->output_length);
    
    pty_analyze_ansi_output(session->output_buffer, &analysis);
    printf("  Has cursor positioning: %s\n", analysis.has_cursor_positioning ? "yes" : "no");
    printf("  Cursor moves: %d\n", analysis.cursor_moves);

    /* This is a diagnostic test - always "passes" but shows output */
    pty_session_send_input(session, "\x03", 0);
    pty_session_close(session);
    test_passed(TEST_NAME, get_time_ms() - start_time);
}

/* ========================================================================== */
/*                     TEST 6: RIGHT ARROW (CONTROL TEST)                     */
/* ========================================================================== */

static void test_right_arrow_control(void) {
    const char *TEST_NAME = "Control: Type 'ab', left, left, right → cursor moves";
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

    if (!pty_wait_for_prompt(session, 2000)) {
        test_failed(TEST_NAME, "Timeout waiting for prompt");
        pty_session_close(session);
        return;
    }

    session->output_length = 0;
    memset(session->output_buffer, 0, PTY_OUTPUT_BUFFER_SIZE);

    pty_session_send_input(session, "ab", 0);
    pty_session_read_output(session, 100);

    pty_session_send_key(session, "left");
    pty_session_read_output(session, 100);
    
    pty_session_send_key(session, "left");
    pty_session_read_output(session, 100);

    pty_session_send_key(session, "right");
    pty_session_read_output(session, 100);

    printf("\n  After 'ab', left, left, right:\n");
    dump_raw_output(session->output_buffer, session->output_length);

    char plain[1024];
    pty_strip_ansi_codes(session->output_buffer, plain, sizeof(plain));
    
    /* Should still show "ab" without corruption */
    if (strstr(plain, "ab") != NULL && strstr(plain, "berry") == NULL) {
        pty_session_send_input(session, "\x03", 0);
        pty_session_close(session);
        test_passed(TEST_NAME, get_time_ms() - start_time);
        return;
    }

    test_failed(TEST_NAME, "Display corrupted with arrow keys");
    printf("  Plain output: '%s'\n", plain);
    
    pty_session_send_input(session, "\x03", 0);
    pty_session_close(session);
}

/* ========================================================================== */
/*                         MAIN TEST RUNNER                                   */
/* ========================================================================== */

int main(void) {
    printf("=========================================================\n");
    printf("LLE PTY Cursor Movement Diagnostic Tests\n");
    printf("=========================================================\n");
    printf("\n");
    printf("These tests isolate the cursor movement bug by testing\n");
    printf("minimal operations and capturing detailed output.\n");
    printf("\n\n");

    /* Run tests in order of increasing complexity */
    test_ansi_sequence_analysis();      /* Diagnostic - shows sequences */
    printf("\n");
    
    test_just_left_arrow();             /* Does left arrow alone cause corruption? */
    printf("\n");
    
    test_single_char_left();            /* Minimal: 1 char + left */
    printf("\n");
    
    test_minimal_two_chars();           /* Simple: 2 chars + left + insert */
    printf("\n");
    
    test_three_chars_left_once();       /* Slightly more complex */
    printf("\n");
    
    test_right_arrow_control();         /* Control: does right arrow work? */
    printf("\n");

    printf("=========================================================\n");
    printf("Diagnostic Tests Complete\n");
    printf("=========================================================\n");
    printf("\n");
    printf("Review the raw ANSI sequences above to identify:\n");
    printf("  1. When corruption first appears\n");
    printf("  2. What ANSI codes are sent during cursor movement\n");
    printf("  3. Whether display refresh is correct\n");
    printf("\n");

    return 0;
}
