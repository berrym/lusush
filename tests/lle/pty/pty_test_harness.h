/**
 * @file pty_test_harness.h
 * @brief PTY Test Harness for LLE Display Testing
 * 
 * Provides infrastructure for testing actual terminal output using pseudo-TTYs.
 * This allows automated validation of display rendering, ANSI escape sequences,
 * cursor positioning, and visual appearance.
 * 
 * CRITICAL: This infrastructure addresses the compliance violation of having
 * zero automated tests for actual terminal output.
 */

#ifndef PTY_TEST_HARNESS_H
#define PTY_TEST_HARNESS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                              CONSTANTS                                     */
/* ========================================================================== */

#define PTY_OUTPUT_BUFFER_SIZE 65536
#define PTY_INPUT_BUFFER_SIZE 4096
#define PTY_DEFAULT_TIMEOUT_MS 2000
#define PTY_MAX_TERMINAL_COLS 256
#define PTY_MAX_TERMINAL_ROWS 100

/* ========================================================================== */
/*                              TYPE DEFINITIONS                              */
/* ========================================================================== */

/**
 * @brief PTY session handle
 */
typedef struct pty_session_t {
    int master_fd;                  /* Master PTY file descriptor */
    int slave_fd;                   /* Slave PTY file descriptor */
    pid_t child_pid;                /* Child process PID */
    char slave_name[256];           /* PTY slave device name */
    char output_buffer[PTY_OUTPUT_BUFFER_SIZE];  /* Captured output */
    size_t output_length;           /* Length of captured output */
    bool child_exited;              /* Child process exit status */
    int child_exit_code;            /* Child exit code */
} pty_session_t;

/**
 * @brief ANSI escape sequence analysis
 */
typedef struct ansi_analysis_t {
    bool has_color_codes;           /* Contains color escape sequences */
    bool has_cursor_positioning;    /* Contains cursor movement codes */
    bool has_clear_sequences;       /* Contains clear/erase codes */
    int color_count;                /* Number of color changes */
    int cursor_moves;               /* Number of cursor movements */
    int bold_count;                 /* Number of bold attributes */
    int reset_count;                /* Number of reset codes */
} ansi_analysis_t;

/**
 * @brief Terminal output expectations
 */
typedef struct output_expectation_t {
    const char *contains;           /* String that must be present */
    const char *not_contains;       /* String that must NOT be present */
    bool requires_color;            /* Must contain color codes */
    bool requires_cursor_move;      /* Must contain cursor positioning */
    int min_length;                 /* Minimum output length */
    int max_length;                 /* Maximum output length */
} output_expectation_t;

/* ========================================================================== */
/*                         PTY SESSION MANAGEMENT                             */
/* ========================================================================== */

/**
 * @brief Create a new PTY session
 * 
 * Creates a pseudo-terminal pair and prepares it for testing.
 * Sets up terminal dimensions and characteristics.
 * 
 * @param session Output pointer for PTY session
 * @param cols Terminal width in columns
 * @param rows Terminal height in rows
 * @return 0 on success, -1 on failure
 */
int pty_session_create(pty_session_t **session, int cols, int rows);

/**
 * @brief Spawn lusush in PTY with LLE enabled
 * 
 * Forks and executes lusush in the slave PTY with LLE_ENABLED=1.
 * The child process will run in a real terminal environment.
 * 
 * @param session PTY session
 * @param command Optional command to execute (NULL for interactive)
 * @param term_type Terminal type (e.g., "xterm-256color")
 * @return 0 on success, -1 on failure
 */
int pty_session_spawn_lusush(pty_session_t *session, 
                               const char *command,
                               const char *term_type);

/**
 * @brief Send input to PTY
 * 
 * Writes input string to the master PTY, simulating user typing.
 * 
 * @param session PTY session
 * @param input Input string to send
 * @param length Length of input (0 = strlen)
 * @return Number of bytes written, -1 on failure
 */
int pty_session_send_input(pty_session_t *session, 
                             const char *input, 
                             size_t length);

/**
 * @brief Send special key sequence to PTY
 * 
 * Sends terminal escape sequences for special keys (arrows, Home, End, etc.)
 * 
 * @param session PTY session
 * @param key Key name ("up", "down", "left", "right", "home", "end", etc.)
 * @return 0 on success, -1 on failure
 */
int pty_session_send_key(pty_session_t *session, const char *key);

/**
 * @brief Read output from PTY
 * 
 * Reads available output from the master PTY with timeout.
 * Accumulates output in session->output_buffer.
 * 
 * @param session PTY session
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes read, -1 on failure, 0 on timeout
 */
int pty_session_read_output(pty_session_t *session, int timeout_ms);

/**
 * @brief Wait for specific output pattern
 * 
 * Reads from PTY until the expected string appears or timeout occurs.
 * 
 * @param session PTY session
 * @param expected Expected string to wait for
 * @param timeout_ms Timeout in milliseconds
 * @return true if pattern found, false on timeout
 */
bool pty_session_wait_for_output(pty_session_t *session,
                                   const char *expected,
                                   int timeout_ms);

/**
 * @brief Close PTY session
 * 
 * Terminates child process, closes file descriptors, and frees resources.
 * 
 * @param session PTY session to close
 */
void pty_session_close(pty_session_t *session);

/* ========================================================================== */
/*                         ANSI OUTPUT ANALYSIS                               */
/* ========================================================================== */

/**
 * @brief Analyze ANSI escape sequences in output
 * 
 * Parses the output buffer and identifies ANSI codes for colors,
 * cursor movement, and other terminal control sequences.
 * 
 * @param output Output string to analyze
 * @param analysis Output analysis structure
 */
void pty_analyze_ansi_output(const char *output, ansi_analysis_t *analysis);

/**
 * @brief Check if output contains specific ANSI color
 * 
 * Searches for ANSI color code in the output.
 * 
 * @param output Output string
 * @param color_code ANSI color code (e.g., 31 for red, 32 for green)
 * @return true if color found, false otherwise
 */
bool pty_output_has_color(const char *output, int color_code);

/**
 * @brief Check if output contains cursor positioning
 * 
 * Looks for ANSI cursor positioning codes (CSI codes).
 * 
 * @param output Output string
 * @param row Expected row (0 = any)
 * @param col Expected column (0 = any)
 * @return true if cursor positioning found, false otherwise
 */
bool pty_output_has_cursor_move(const char *output, int row, int col);

/**
 * @brief Strip ANSI escape sequences from output
 * 
 * Removes all ANSI codes to get plain text content.
 * 
 * @param output Input string with ANSI codes
 * @param plain Output buffer for plain text
 * @param plain_size Size of output buffer
 * @return Length of plain text
 */
size_t pty_strip_ansi_codes(const char *output, char *plain, size_t plain_size);

/* ========================================================================== */
/*                         OUTPUT VALIDATION                                  */
/* ========================================================================== */

/**
 * @brief Validate output against expectations
 * 
 * Checks if the output meets all specified expectations.
 * 
 * @param session PTY session (uses output_buffer)
 * @param expect Expectations structure
 * @param error_msg Buffer for error message (can be NULL)
 * @param error_size Size of error message buffer
 * @return true if all expectations met, false otherwise
 */
bool pty_validate_output(const pty_session_t *session,
                          const output_expectation_t *expect,
                          char *error_msg,
                          size_t error_size);

/**
 * @brief Compare output against golden file
 * 
 * Compares the captured output against a known-good reference file.
 * 
 * @param session PTY session (uses output_buffer)
 * @param golden_file_path Path to golden file
 * @param strip_ansi If true, compare plain text only (ignore ANSI codes)
 * @return true if output matches golden file, false otherwise
 */
bool pty_compare_golden(const pty_session_t *session,
                         const char *golden_file_path,
                         bool strip_ansi);

/**
 * @brief Save output to golden file
 * 
 * Saves the current output as a golden reference file.
 * 
 * @param session PTY session (uses output_buffer)
 * @param golden_file_path Path where to save golden file
 * @return 0 on success, -1 on failure
 */
int pty_save_golden(const pty_session_t *session, const char *golden_file_path);

/* ========================================================================== */
/*                         TEST HELPERS                                       */
/* ========================================================================== */

/**
 * @brief Print output with visible ANSI codes
 * 
 * Utility for debugging - prints output with escape sequences visible.
 * 
 * @param output Output string
 * @param max_length Maximum length to print (0 = all)
 */
void pty_print_output_debug(const char *output, size_t max_length);

/**
 * @brief Simulate typing with delays
 * 
 * Sends input character-by-character with realistic typing delays.
 * 
 * @param session PTY session
 * @param text Text to type
 * @param delay_ms Delay between characters in milliseconds
 * @return 0 on success, -1 on failure
 */
int pty_simulate_typing(pty_session_t *session, 
                         const char *text, 
                         int delay_ms);

/**
 * @brief Wait for prompt to appear
 * 
 * Reads output until a shell prompt pattern is detected.
 * Recognizes common prompt patterns ($, >, #, etc.)
 * 
 * @param session PTY session
 * @param timeout_ms Timeout in milliseconds
 * @return true if prompt detected, false on timeout
 */
bool pty_wait_for_prompt(pty_session_t *session, int timeout_ms);

/* ========================================================================== */
/*                         TEST RESULT REPORTING                              */
/* ========================================================================== */

/**
 * @brief Test result structure
 */
typedef struct pty_test_result_t {
    const char *test_name;
    bool passed;
    char failure_reason[256];
    double duration_ms;
} pty_test_result_t;

/**
 * @brief Initialize test result
 */
void pty_test_result_init(pty_test_result_t *result, const char *test_name);

/**
 * @brief Mark test as passed
 */
void pty_test_result_pass(pty_test_result_t *result);

/**
 * @brief Mark test as failed with reason
 */
void pty_test_result_fail(pty_test_result_t *result, const char *reason);

/**
 * @brief Print test result
 */
void pty_test_result_print(const pty_test_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* PTY_TEST_HARNESS_H */
