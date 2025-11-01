/**
 * @file pty_test_harness.c
 * @brief PTY Test Harness Implementation
 * 
 * Core infrastructure for PTY-based terminal testing.
 */

#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include "pty_test_harness.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>

#if defined(__linux__)
#include <pty.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <util.h>
#endif

/* ========================================================================== */
/*                         PTY SESSION MANAGEMENT                             */
/* ========================================================================== */

int pty_session_create(pty_session_t **session, int cols, int rows) {
    if (!session) {
        return -1;
    }

    pty_session_t *s = calloc(1, sizeof(pty_session_t));
    if (!s) {
        return -1;
    }

    /* Set up terminal window size */
    struct winsize ws;
    memset(&ws, 0, sizeof(ws));
    ws.ws_col = cols;
    ws.ws_row = rows;

    /* Create PTY pair */
    if (openpty(&s->master_fd, &s->slave_fd, s->slave_name, NULL, &ws) != 0) {
        free(s);
        return -1;
    }

    /* Set master to non-blocking */
    int flags = fcntl(s->master_fd, F_GETFL, 0);
    fcntl(s->master_fd, F_SETFL, flags | O_NONBLOCK);

    s->output_length = 0;
    s->child_exited = false;
    s->child_exit_code = 0;
    s->child_pid = -1;

    *session = s;
    return 0;
}

int pty_session_spawn_lusush(pty_session_t *session, 
                               const char *command,
                               const char *term_type) {
    if (!session) {
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        /* Child process */
        
        /* Close master side */
        close(session->master_fd);

        /* Create new session */
        setsid();

        /* Set controlling terminal */
        if (ioctl(session->slave_fd, TIOCSCTTY, 0) < 0) {
            _exit(1);
        }

        /* Redirect stdin/stdout/stderr to slave */
        dup2(session->slave_fd, STDIN_FILENO);
        dup2(session->slave_fd, STDOUT_FILENO);
        dup2(session->slave_fd, STDERR_FILENO);

        if (session->slave_fd > STDERR_FILENO) {
            close(session->slave_fd);
        }

        /* Set environment */
        setenv("LLE_ENABLED", "1", 1);
        setenv("TERM", term_type ? term_type : "xterm-256color", 1);
        setenv("LUSUSH_TEST_MODE", "1", 1);

        /* Change to project root directory - lusush needs to run from there */
        /* Try to detect project root by finding builddir */
        if (access("../../../builddir", F_OK) == 0) {
            /* Running from tests/lle/pty */
            if (chdir("../../..") != 0) {
                _exit(1);
            }
        }
        /* If we're already at project root, chdir will have no effect */

        /* Redirect stderr to /dev/null to avoid noise in test output */
        int null_fd = open("/dev/null", O_WRONLY);
        if (null_fd >= 0) {
            dup2(null_fd, STDERR_FILENO);
            close(null_fd);
        }
        
        /* Execute lusush - path relative to project root */
        const char *lusush_paths[] = {
            "./builddir/lusush",        /* If run from project root (meson build) */
            "../../../builddir/lusush", /* If run from tests/lle/pty (meson build) */
            "./build/lusush",           /* If run from project root (old path) */
            "../../../build/lusush",    /* If run from tests/lle/pty (old path) */
            NULL
        };
        
        for (int i = 0; lusush_paths[i]; i++) {
            if (command) {
                /* Use -i for interactive mode even with -c command */
                execl(lusush_paths[i], "lusush", "-i", "-c", command, NULL);
            } else {
                /* Force interactive mode for PTY testing */
                execl(lusush_paths[i], "lusush", "-i", NULL);
            }
            /* Continue to next path if exec failed */
        }

        /* If all execs fail */
        _exit(127);
    }

    /* Parent process */
    session->child_pid = pid;
    close(session->slave_fd);  /* Parent doesn't need slave */
    session->slave_fd = -1;

    return 0;
}

int pty_session_send_input(pty_session_t *session, 
                             const char *input, 
                             size_t length) {
    if (!session || !input) {
        return -1;
    }

    if (length == 0) {
        length = strlen(input);
    }

    ssize_t written = write(session->master_fd, input, length);
    return (int)written;
}

int pty_session_send_key(pty_session_t *session, const char *key) {
    if (!session || !key) {
        return -1;
    }

    const char *sequence = NULL;

    /* Map key names to escape sequences */
    if (strcmp(key, "up") == 0) {
        sequence = "\033[A";
    } else if (strcmp(key, "down") == 0) {
        sequence = "\033[B";
    } else if (strcmp(key, "right") == 0) {
        sequence = "\033[C";
    } else if (strcmp(key, "left") == 0) {
        sequence = "\033[D";
    } else if (strcmp(key, "home") == 0) {
        sequence = "\033[H";
    } else if (strcmp(key, "end") == 0) {
        sequence = "\033[F";
    } else if (strcmp(key, "delete") == 0) {
        sequence = "\033[3~";
    } else if (strcmp(key, "backspace") == 0) {
        sequence = "\177";  /* DEL character */
    } else if (strcmp(key, "enter") == 0) {
        sequence = "\n";
    } else if (strcmp(key, "tab") == 0) {
        sequence = "\t";
    } else if (strcmp(key, "escape") == 0) {
        sequence = "\033";
    } else if (strcmp(key, "ctrl-c") == 0) {
        sequence = "\003";  /* ETX */
    } else if (strcmp(key, "ctrl-d") == 0) {
        sequence = "\004";  /* EOT */
    } else {
        return -1;  /* Unknown key */
    }

    return pty_session_send_input(session, sequence, 0);
}

int pty_session_read_output(pty_session_t *session, int timeout_ms) {
    if (!session) {
        return -1;
    }

    int total_read = 0;
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Keep reading until timeout or no more data */
    while (1) {
        /* Check timeout */
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                          (now.tv_nsec - start.tv_nsec) / 1000000;
        long remaining_ms = timeout_ms - elapsed_ms;
        
        if (remaining_ms <= 0) {
            break;  /* Timeout reached */
        }

        fd_set read_fds;
        struct timeval timeout;
        timeout.tv_sec = remaining_ms / 1000;
        timeout.tv_usec = (remaining_ms % 1000) * 1000;

        FD_ZERO(&read_fds);
        FD_SET(session->master_fd, &read_fds);

        int ready = select(session->master_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready <= 0) {
            break;  /* Timeout or error - no more data available */
        }

        /* Read available data */
        size_t space_left = PTY_OUTPUT_BUFFER_SIZE - session->output_length - 1;
        if (space_left == 0) {
            return -1;  /* Buffer full */
        }

        ssize_t n = read(session->master_fd, 
                         session->output_buffer + session->output_length,
                         space_left);

        if (n > 0) {
            session->output_length += n;
            session->output_buffer[session->output_length] = '\0';
            total_read += n;
            
            /* If we got data, use a shorter timeout for next read to drain quickly */
            timeout_ms = elapsed_ms + 50;  /* Give 50ms more after each successful read */
        } else if (n == 0) {
            break;  /* EOF */
        } else {
            break;  /* Error */
        }
    }

    return total_read;
}

bool pty_session_wait_for_output(pty_session_t *session,
                                   const char *expected,
                                   int timeout_ms) {
    if (!session || !expected) {
        return false;
    }

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        /* Check if expected string is in output */
        if (strstr(session->output_buffer, expected) != NULL) {
            return true;
        }

        /* Check timeout */
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                          (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= timeout_ms) {
            return false;
        }

        /* Read more output */
        int remaining_ms = timeout_ms - elapsed_ms;
        if (remaining_ms > 100) remaining_ms = 100;  /* Poll every 100ms */
        
        pty_session_read_output(session, remaining_ms);
    }
}

void pty_session_close(pty_session_t *session) {
    if (!session) {
        return;
    }

    /* Terminate child if still running */
    if (session->child_pid > 0 && !session->child_exited) {
        kill(session->child_pid, SIGTERM);
        
        /* Wait briefly for clean exit */
        struct timespec wait_time = {0, 100000000};  /* 100ms */
        nanosleep(&wait_time, NULL);
        
        /* Force kill if still running */
        if (waitpid(session->child_pid, &session->child_exit_code, WNOHANG) == 0) {
            kill(session->child_pid, SIGKILL);
            waitpid(session->child_pid, &session->child_exit_code, 0);
        }
    }

    /* Close file descriptors */
    if (session->master_fd >= 0) {
        close(session->master_fd);
    }
    if (session->slave_fd >= 0) {
        close(session->slave_fd);
    }

    free(session);
}

/* ========================================================================== */
/*                         ANSI OUTPUT ANALYSIS                               */
/* ========================================================================== */

void pty_analyze_ansi_output(const char *output, ansi_analysis_t *analysis) {
    if (!output || !analysis) {
        return;
    }

    memset(analysis, 0, sizeof(*analysis));

    const char *p = output;
    while (*p) {
        if (*p == '\033' && *(p + 1) == '[') {
            /* Found CSI sequence */
            p += 2;
            
            /* Parse the sequence */
            char code[32];
            size_t i = 0;
            while (*p && *p != 'm' && *p != 'H' && *p != 'A' && 
                   *p != 'B' && *p != 'C' && *p != 'D' && *p != 'J' && *p != 'K' &&
                   i < sizeof(code) - 1) {
                code[i++] = *p++;
            }
            code[i] = '\0';

            if (*p == 'm') {
                /* SGR (color/attribute) sequence */
                analysis->has_color_codes = true;
                analysis->color_count++;
                
                if (strstr(code, "1") || strstr(code, "01")) {
                    analysis->bold_count++;
                }
                if (strstr(code, "0") && strlen(code) == 1) {
                    analysis->reset_count++;
                }
            } else if (*p == 'H' || *p == 'A' || *p == 'B' || 
                       *p == 'C' || *p == 'D') {
                /* Cursor positioning */
                analysis->has_cursor_positioning = true;
                analysis->cursor_moves++;
            } else if (*p == 'J' || *p == 'K') {
                /* Clear/erase sequences */
                analysis->has_clear_sequences = true;
            }

            if (*p) p++;
        } else {
            p++;
        }
    }
}

bool pty_output_has_color(const char *output, int color_code) {
    if (!output) {
        return false;
    }

    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\033[%dm", color_code);
    
    return strstr(output, pattern) != NULL;
}

bool pty_output_has_cursor_move(const char *output, int row, int col) {
    if (!output) {
        return false;
    }

    const char *p = output;
    while ((p = strstr(p, "\033[")) != NULL) {
        p += 2;
        
        /* Check for cursor positioning codes */
        if (strchr(p, 'H') || strchr(p, 'A') || strchr(p, 'B') || 
            strchr(p, 'C') || strchr(p, 'D')) {
            
            if (row == 0 && col == 0) {
                return true;  /* Any cursor move */
            }
            
            /* Parse row and column from sequence */
            int r = 0, c = 0;
            if (sscanf(p, "%d;%d", &r, &c) == 2) {
                if ((row == 0 || row == r) && (col == 0 || col == c)) {
                    return true;
                }
            }
        }
    }

    return false;
}

size_t pty_strip_ansi_codes(const char *output, char *plain, size_t plain_size) {
    if (!output || !plain || plain_size == 0) {
        return 0;
    }

    const char *p = output;
    size_t j = 0;

    while (*p && j < plain_size - 1) {
        if (*p == '\033' && *(p + 1) == '[') {
            /* Skip CSI sequence */
            p += 2;
            while (*p && !(*p >= 'A' && *p <= 'Z') && 
                   !(*p >= 'a' && *p <= 'z')) {
                p++;
            }
            if (*p) p++;  /* Skip final character */
        } else {
            plain[j++] = *p++;
        }
    }

    plain[j] = '\0';
    return j;
}

/* ========================================================================== */
/*                         OUTPUT VALIDATION                                  */
/* ========================================================================== */

bool pty_validate_output(const pty_session_t *session,
                          const output_expectation_t *expect,
                          char *error_msg,
                          size_t error_size) {
    if (!session || !expect) {
        return false;
    }

    const char *output = session->output_buffer;
    size_t output_len = session->output_length;

    /* Check minimum length */
    if (expect->min_length > 0 && output_len < (size_t)expect->min_length) {
        if (error_msg) {
            snprintf(error_msg, error_size, 
                    "Output too short: %zu < %d", output_len, expect->min_length);
        }
        return false;
    }

    /* Check maximum length */
    if (expect->max_length > 0 && output_len > (size_t)expect->max_length) {
        if (error_msg) {
            snprintf(error_msg, error_size,
                    "Output too long: %zu > %d", output_len, expect->max_length);
        }
        return false;
    }

    /* Check required string */
    if (expect->contains && strstr(output, expect->contains) == NULL) {
        if (error_msg) {
            snprintf(error_msg, error_size,
                    "Output missing required string: '%s'", expect->contains);
        }
        return false;
    }

    /* Check forbidden string */
    if (expect->not_contains && strstr(output, expect->not_contains) != NULL) {
        if (error_msg) {
            snprintf(error_msg, error_size,
                    "Output contains forbidden string: '%s'", expect->not_contains);
        }
        return false;
    }

    /* Check color requirement */
    if (expect->requires_color) {
        ansi_analysis_t analysis;
        pty_analyze_ansi_output(output, &analysis);
        if (!analysis.has_color_codes) {
            if (error_msg) {
                snprintf(error_msg, error_size,
                        "Output missing required color codes");
            }
            return false;
        }
    }

    /* Check cursor movement requirement */
    if (expect->requires_cursor_move) {
        if (!pty_output_has_cursor_move(output, 0, 0)) {
            if (error_msg) {
                snprintf(error_msg, error_size,
                        "Output missing required cursor positioning");
            }
            return false;
        }
    }

    return true;
}

bool pty_compare_golden(const pty_session_t *session,
                         const char *golden_file_path,
                         bool strip_ansi) {
    if (!session || !golden_file_path) {
        return false;
    }

    /* Read golden file */
    FILE *f = fopen(golden_file_path, "r");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *golden = malloc(fsize + 1);
    if (!golden) {
        fclose(f);
        return false;
    }

    fread(golden, 1, fsize, f);
    golden[fsize] = '\0';
    fclose(f);

    bool match = false;

    if (strip_ansi) {
        /* Compare plain text only */
        char plain_output[PTY_OUTPUT_BUFFER_SIZE];
        char plain_golden[PTY_OUTPUT_BUFFER_SIZE];
        
        pty_strip_ansi_codes(session->output_buffer, plain_output, sizeof(plain_output));
        pty_strip_ansi_codes(golden, plain_golden, sizeof(plain_golden));
        
        match = (strcmp(plain_output, plain_golden) == 0);
    } else {
        /* Compare exact output */
        match = (strcmp(session->output_buffer, golden) == 0);
    }

    free(golden);
    return match;
}

int pty_save_golden(const pty_session_t *session, const char *golden_file_path) {
    if (!session || !golden_file_path) {
        return -1;
    }

    FILE *f = fopen(golden_file_path, "w");
    if (!f) {
        return -1;
    }

    fwrite(session->output_buffer, 1, session->output_length, f);
    fclose(f);

    return 0;
}

/* ========================================================================== */
/*                         TEST HELPERS                                       */
/* ========================================================================== */

void pty_print_output_debug(const char *output, size_t max_length) {
    if (!output) {
        return;
    }

    printf("=== OUTPUT (showing ANSI codes) ===\n");
    
    size_t len = strlen(output);
    if (max_length > 0 && len > max_length) {
        len = max_length;
    }

    for (size_t i = 0; i < len; i++) {
        if (output[i] == '\033') {
            printf("\\033");
        } else if (output[i] == '\n') {
            printf("\\n\n");
        } else if (output[i] == '\r') {
            printf("\\r");
        } else if (output[i] == '\t') {
            printf("\\t");
        } else if (output[i] < 32 || output[i] > 126) {
            printf("\\x%02x", (unsigned char)output[i]);
        } else {
            putchar(output[i]);
        }
    }
    printf("\n=== END OUTPUT ===\n");
}

int pty_simulate_typing(pty_session_t *session, 
                         const char *text, 
                         int delay_ms) {
    if (!session || !text) {
        return -1;
    }

    struct timespec delay;
    delay.tv_sec = delay_ms / 1000;
    delay.tv_nsec = (delay_ms % 1000) * 1000000;

    while (*text) {
        if (pty_session_send_input(session, text, 1) < 0) {
            return -1;
        }
        text++;
        
        if (delay_ms > 0) {
            nanosleep(&delay, NULL);
        }
    }

    return 0;
}

bool pty_wait_for_prompt(pty_session_t *session, int timeout_ms) {
    if (!session) {
        return false;
    }

    /* Common prompt patterns */
    const char *patterns[] = {
        "$ ",
        "> ",
        "# ",
        "lusush> ",
        NULL
    };

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        /* Read output first */
        int remaining_ms = timeout_ms;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                          (now.tv_nsec - start.tv_nsec) / 1000000;
        remaining_ms = timeout_ms - elapsed_ms;
        if (remaining_ms <= 0) {
            return false;
        }
        if (remaining_ms > 50) remaining_ms = 50;  /* Poll every 50ms */
        
        pty_session_read_output(session, remaining_ms);
        
        /* Check for any prompt pattern */
        for (int i = 0; patterns[i]; i++) {
            if (strstr(session->output_buffer, patterns[i]) != NULL) {
                return true;
            }
        }

        /* Check timeout */
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                     (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= timeout_ms) {
            return false;
        }
    }
}

/* ========================================================================== */
/*                         TEST RESULT REPORTING                              */
/* ========================================================================== */

void pty_test_result_init(pty_test_result_t *result, const char *test_name) {
    if (!result) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->test_name = test_name;
    result->passed = false;
    result->duration_ms = 0.0;
}

void pty_test_result_pass(pty_test_result_t *result) {
    if (!result) {
        return;
    }

    result->passed = true;
}

void pty_test_result_fail(pty_test_result_t *result, const char *reason) {
    if (!result) {
        return;
    }

    result->passed = false;
    if (reason) {
        strncpy(result->failure_reason, reason, sizeof(result->failure_reason) - 1);
    }
}

void pty_test_result_print(const pty_test_result_t *result) {
    if (!result) {
        return;
    }

    if (result->passed) {
        printf("✓ PASS: %s (%.2fms)\n", result->test_name, result->duration_ms);
    } else {
        printf("✗ FAIL: %s\n", result->test_name);
        if (result->failure_reason[0]) {
            printf("  Reason: %s\n", result->failure_reason);
        }
    }
}
