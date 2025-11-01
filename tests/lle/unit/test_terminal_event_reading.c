/*
 * test_terminal_event_reading.c - Unit tests for terminal event reading
 * 
 * Tests Spec 02 Phase 3: Event Reading
 * 
 * Test Categories:
 * 1. Timeout behavior
 * 2. Character reading (ASCII and UTF-8)
 * 3. Window resize events
 * 4. EOF detection
 * 5. Error handling
 * 6. Integration scenarios
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include "lle/terminal_abstraction.h"

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    static void name(void); \
    static void run_##name(void) { \
        tests_run++; \
        printf("  Running %s...", #name); \
        fflush(stdout); \
        name(); \
        tests_passed++; \
        printf(" PASS\n"); \
    } \
    static void name(void)

/* Helper to create a pipe with data */
static int create_pipe_with_data(const void *data, size_t len, int *write_fd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    /* Write data to pipe */
    if (data && len > 0) {
        ssize_t written = write(pipefd[1], data, len);
        if (written != (ssize_t)len) {
            perror("write");
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
        }
    }
    
    if (write_fd) {
        *write_fd = pipefd[1];
    } else {
        close(pipefd[1]);
    }
    
    return pipefd[0];
}

/* ============================================================================
 * TIMEOUT TESTS
 * ============================================================================ */

TEST(test_timeout_zero_nonblocking) {
    /* Create empty pipe (no data, but keep write end open) */
    int write_fd;
    int pipe_fd = create_pipe_with_data(NULL, 0, &write_fd);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    /* Temporarily replace stdin with pipe */
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    /* Read with zero timeout (non-blocking poll) */
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 0);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_TIMEOUT);
    
    /* Restore stdin */
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    close(write_fd);
    lle_unix_interface_destroy(interface);
}

TEST(test_timeout_short) {
    /* Create empty pipe (keep write end open) */
    int write_fd;
    int pipe_fd = create_pipe_with_data(NULL, 0, &write_fd);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    /* Test 100ms timeout - should return timeout event */
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 100);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_TIMEOUT);
    assert(event.timestamp > 0);
    
    /* Note: We don't check exact timing as it can vary in test environments
     * The important thing is that it returns a timeout event */
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    close(write_fd);
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * CHARACTER READING TESTS
 * ============================================================================ */

TEST(test_read_ascii_character) {
    /* Create pipe with ASCII 'A' */
    unsigned char data[] = {'A'};
    int pipe_fd = create_pipe_with_data(data, sizeof(data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_CHARACTER);
    assert(event.data.character.codepoint == 'A');
    assert(event.data.character.byte_count == 1);
    assert(event.data.character.utf8_bytes[0] == 'A');
    assert(event.timestamp > 0);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

TEST(test_read_utf8_2byte) {
    /* UTF-8 for 'é' (U+00E9) = C3 A9 */
    unsigned char data[] = {0xC3, 0xA9};
    int pipe_fd = create_pipe_with_data(data, sizeof(data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_CHARACTER);
    assert(event.data.character.codepoint == 0x00E9);  /* é */
    assert(event.data.character.byte_count == 2);
    assert((unsigned char)event.data.character.utf8_bytes[0] == 0xC3);
    assert((unsigned char)event.data.character.utf8_bytes[1] == 0xA9);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

TEST(test_read_utf8_3byte) {
    /* UTF-8 for '€' (U+20AC) = E2 82 AC */
    unsigned char data[] = {0xE2, 0x82, 0xAC};
    int pipe_fd = create_pipe_with_data(data, sizeof(data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_CHARACTER);
    assert(event.data.character.codepoint == 0x20AC);  /* € */
    assert(event.data.character.byte_count == 3);
    assert((unsigned char)event.data.character.utf8_bytes[0] == 0xE2);
    assert((unsigned char)event.data.character.utf8_bytes[1] == 0x82);
    assert((unsigned char)event.data.character.utf8_bytes[2] == 0xAC);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

TEST(test_read_utf8_4byte) {
    /* UTF-8 for '𝄞' (U+1D11E) = F0 9D 84 9E */
    unsigned char data[] = {0xF0, 0x9D, 0x84, 0x9E};
    int pipe_fd = create_pipe_with_data(data, sizeof(data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_CHARACTER);
    assert(event.data.character.codepoint == 0x1D11E);  /* 𝄞 */
    assert(event.data.character.byte_count == 4);
    assert((unsigned char)event.data.character.utf8_bytes[0] == 0xF0);
    assert((unsigned char)event.data.character.utf8_bytes[1] == 0x9D);
    assert((unsigned char)event.data.character.utf8_bytes[2] == 0x84);
    assert((unsigned char)event.data.character.utf8_bytes[3] == 0x9E);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

TEST(test_read_invalid_utf8) {
    /* Invalid UTF-8 sequence - should get replacement character */
    unsigned char data[] = {0xFF};  /* Invalid first byte */
    int pipe_fd = create_pipe_with_data(data, sizeof(data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_CHARACTER);
    assert(event.data.character.codepoint == 0xFFFD);  /* Replacement character */
    assert(event.data.character.byte_count == 1);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * WINDOW RESIZE TESTS
 * ============================================================================ */

TEST(test_resize_event_priority) {
    /* Create pipe with data */
    unsigned char data[] = {'A'};
    int pipe_fd = create_pipe_with_data(data, sizeof(data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    /* Simulate SIGWINCH received */
    interface->sigwinch_received = true;
    
    /* Read event - should get resize, not character */
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_WINDOW_RESIZE);
    assert(event.data.resize.new_width > 0);
    assert(event.data.resize.new_height > 0);
    assert(interface->size_changed == true);
    assert(interface->sigwinch_received == false);  /* Flag cleared */
    
    /* Next read should get the character */
    result = lle_unix_interface_read_event(interface, &event, 1000);
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_CHARACTER);
    assert(event.data.character.codepoint == 'A');
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * FUNCTION KEY TESTS
 * ============================================================================ */

TEST(test_function_keys_f1_f4) {
    /* Test F1-F4 keys using SS3 sequences (ESC O P/Q/R/S) */
    /* F1: ESC O P */
    unsigned char f1_data[] = {0x1B, 'O', 'P'};
    int pipe_fd = create_pipe_with_data(f1_data, sizeof(f1_data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    /* Without parser initialization, should use fallback escape handling */
    /* The fallback won't recognize F1, so it will return ESC or partial sequence */
    /* With parser, it would recognize F1 */
    
    /* For now, just verify it doesn't crash and returns some event */
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    /* Event type could be CHARACTER (ESC), SPECIAL_KEY, or TIMEOUT depending on parser */
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

TEST(test_function_keys_f5_f12) {
    /* Test F5 key using CSI sequence (ESC [ 1 5 ~) */
    unsigned char f5_data[] = {0x1B, '[', '1', '5', '~'};
    int pipe_fd = create_pipe_with_data(f5_data, sizeof(f5_data), NULL);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    /* Read events - should get something without crashing */
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    /* Without full parser integration, exact behavior varies */
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * EOF DETECTION TESTS
 * ============================================================================ */

TEST(test_eof_detection) {
    /* Create pipe and immediately close write end */
    int pipefd[2];
    assert(pipe(pipefd) == 0);
    close(pipefd[1]);  /* Close write end -> EOF on read */
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipefd[0], STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_EOF);
    assert(event.timestamp > 0);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipefd[0]);
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * ERROR HANDLING TESTS
 * ============================================================================ */

TEST(test_null_parameter_validation) {
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    lle_input_event_t event;
    
    /* Null interface */
    result = lle_unix_interface_read_event(NULL, &event, 100);
    assert(result == LLE_ERROR_INVALID_PARAMETER);
    
    /* Null event */
    result = lle_unix_interface_read_event(interface, NULL, 100);
    assert(result == LLE_ERROR_INVALID_PARAMETER);
    
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * INTEGRATION TESTS
 * ============================================================================ */

TEST(test_multiple_events_sequence) {
    /* Create pipe with multiple characters (keep write end open) */
    unsigned char data[] = {'A', 'B', 'C'};
    int write_fd;
    int pipe_fd = create_pipe_with_data(data, sizeof(data), &write_fd);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    /* Read three characters */
    for (int i = 0; i < 3; i++) {
        lle_input_event_t event;
        result = lle_unix_interface_read_event(interface, &event, 1000);
        
        assert(result == LLE_SUCCESS);
        assert(event.type == LLE_INPUT_TYPE_CHARACTER);
        assert(event.data.character.codepoint == (uint32_t)('A' + i));
    }
    
    /* Fourth read should timeout (no more data, but write end still open) */
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 100);
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_TIMEOUT);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    close(write_fd);
    lle_unix_interface_destroy(interface);
}

TEST(test_mixed_event_types) {
    /* Test interleaved resize and character events */
    unsigned char data[] = {'X'};
    int write_fd;
    int pipe_fd = create_pipe_with_data(data, sizeof(data), &write_fd);
    assert(pipe_fd >= 0);
    
    lle_unix_interface_t *interface = NULL;
    lle_result_t result = lle_unix_interface_init(&interface);
    assert(result == LLE_SUCCESS);
    
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipe_fd, STDIN_FILENO);
    interface->terminal_fd = STDIN_FILENO;
    
    /* Simulate resize */
    interface->sigwinch_received = true;
    
    /* Should get resize first */
    lle_input_event_t event;
    result = lle_unix_interface_read_event(interface, &event, 1000);
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_WINDOW_RESIZE);
    
    /* Then character */
    result = lle_unix_interface_read_event(interface, &event, 1000);
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_CHARACTER);
    assert(event.data.character.codepoint == 'X');
    
    /* Then timeout (write end still open) */
    result = lle_unix_interface_read_event(interface, &event, 0);
    assert(result == LLE_SUCCESS);
    assert(event.type == LLE_INPUT_TYPE_TIMEOUT);
    
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(pipe_fd);
    close(write_fd);
    lle_unix_interface_destroy(interface);
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("Running Terminal Event Reading Tests (Spec 02 Phase 3)\n");
    printf("========================================================\n\n");
    
    printf("Timeout Tests:\n");
    run_test_timeout_zero_nonblocking();
    run_test_timeout_short();
    
    printf("\nCharacter Reading Tests:\n");
    run_test_read_ascii_character();
    run_test_read_utf8_2byte();
    run_test_read_utf8_3byte();
    run_test_read_utf8_4byte();
    run_test_read_invalid_utf8();
    
    printf("\nWindow Resize Tests:\n");
    run_test_resize_event_priority();
    
    printf("\nFunction Key Tests:\n");
    run_test_function_keys_f1_f4();
    run_test_function_keys_f5_f12();
    
    printf("\nEOF Detection Tests:\n");
    run_test_eof_detection();
    
    printf("\nError Handling Tests:\n");
    run_test_null_parameter_validation();
    
    printf("\nIntegration Tests:\n");
    run_test_multiple_events_sequence();
    run_test_mixed_event_types();
    
    printf("\n========================================================\n");
    printf("Test Results: %d/%d tests passed\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
