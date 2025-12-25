/**
 * adaptive_terminal_detection.c - Adaptive Terminal Detection Implementation
 *
 * Comprehensive terminal capability detection with multi-tier approach:
 * 1. Environment variable analysis
 * 2. Terminal signature matching
 * 3. Safe runtime capability probing
 * 4. Fallback mode determination
 *
 * Specification:
 * docs/lle_specification/critical_gaps/26_adaptive_terminal_integration_complete.md
 * Date: 2025-11-02
 */

#include "lle/adaptive_terminal_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* Performance monitoring */
static lle_detection_performance_stats_t detection_stats = {0};

/* Detection cache */
static lle_terminal_detection_result_t *cached_result = NULL;
static uint64_t cache_timestamp_us = 0;
#define CACHE_TTL_US 30000000 /* 30 seconds */

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

/**
 * Get current time in microseconds.
 */
static uint64_t get_current_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

/**
 * Simple wildcard pattern matching.
 * Supports * wildcard only (matches any sequence of characters).
 */
static bool pattern_match(const char *pattern, const char *string) {
    if (!pattern || !string) {
        return false;
    }

    /* Simple wildcard matching implementation */
    const char *p = pattern;
    const char *s = string;
    const char *star = NULL;
    const char *ss = NULL;

    while (*s) {
        if (*p == '*') {
            /* Remember position of * for backtracking */
            star = p++;
            ss = s;
        } else if (*p == *s || (*p >= 'A' && *p <= 'Z' && *p + 32 == *s) ||
                   (*p >= 'a' && *p <= 'z' && *p - 32 == *s)) {
            /* Match (case-insensitive) */
            p++;
            s++;
        } else if (star) {
            /* Backtrack to * */
            p = star + 1;
            s = ++ss;
        } else {
            return false;
        }
    }

    /* Skip trailing * in pattern */
    while (*p == '*') {
        p++;
    }

    return *p == '\0';
}

/**
 * Safe string copy with null termination.
 */
static void safe_strncpy(char *dest, const char *src, size_t size) {
    if (!dest || size == 0) {
        return;
    }

    if (!src) {
        dest[0] = '\0';
        return;
    }

    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

/* ============================================================================
 * ENVIRONMENT ANALYSIS
 * ============================================================================
 */

/**
 * Analyze environment variables for terminal information.
 */
static lle_result_t
analyze_environment_variables(lle_terminal_detection_result_t *detection) {
    const char *term = getenv("TERM");
    const char *term_program = getenv("TERM_PROGRAM");
    const char *colorterm = getenv("COLORTERM");

    /* Copy environment values */
    safe_strncpy(detection->term_name, term ? term : "",
                 sizeof(detection->term_name));
    safe_strncpy(detection->term_program, term_program ? term_program : "",
                 sizeof(detection->term_program));
    safe_strncpy(detection->colorterm, colorterm ? colorterm : "",
                 sizeof(detection->colorterm));

    /* Basic capability inference from environment */
    detection->supports_colors =
        (term && (strstr(term, "color") || strstr(term, "256")));
    detection->supports_256_colors = (term && strstr(term, "256"));
    detection->supports_truecolor =
        (colorterm && (strcmp(colorterm, "truecolor") == 0 ||
                       strcmp(colorterm, "24bit") == 0));
    detection->supports_unicode =
        (colorterm != NULL); /* COLORTERM usually implies UTF-8 */

    /* Detect terminal multiplexer */
    const char *tmux = getenv("TMUX");
    const char *sty = getenv("STY");

    if (tmux && tmux[0] != '\0') {
        detection->multiplexer_type = LLE_MUX_TYPE_TMUX;
    } else if ((term && strstr(term, "screen")) || (sty && sty[0] != '\0')) {
        detection->multiplexer_type = LLE_MUX_TYPE_SCREEN;
    } else {
        detection->multiplexer_type = LLE_MUX_TYPE_NONE;
    }

    return LLE_SUCCESS;
}

/* ============================================================================
 * TERMINAL SIGNATURE MATCHING
 * ============================================================================
 */

/**
 * Match terminal signature from database.
 */
const lle_terminal_signature_t *
lle_match_terminal_signature(const lle_terminal_detection_result_t *detection) {

    if (!detection) {
        return NULL;
    }

    size_t count = 0;
    const lle_terminal_signature_t *signatures =
        lle_get_terminal_signature_database(&count);

    /* Iterate through signatures in priority order */
    for (size_t i = 0; i < count; i++) {
        const lle_terminal_signature_t *sig = &signatures[i];
        bool match = true;

        /* Check TERM_PROGRAM pattern */
        if (sig->term_program_pattern) {
            if (detection->term_program[0] == '\0' ||
                !pattern_match(sig->term_program_pattern,
                               detection->term_program)) {
                match = false;
            }
        }

        /* Check TERM pattern */
        if (match && sig->term_pattern) {
            if (detection->term_name[0] == '\0' ||
                !pattern_match(sig->term_pattern, detection->term_name)) {
                match = false;
            }
        }

        /* Check additional environment variable */
        if (match && sig->env_var_check) {
            const char *env_val = getenv(sig->env_var_check);
            if (!env_val || env_val[0] == '\0') {
                match = false;
            }
        }

        if (match) {
            return sig;
        }
    }

    return NULL; /* No match found */
}

/* ============================================================================
 * SAFE CAPABILITY PROBING
 * ============================================================================
 */

/**
 * Probe single capability with timeout.
 */
static bool probe_capability_with_timeout(const char *query, int timeout_ms) {
    /* Only probe if stdout is a TTY */
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }

    /* Write query sequence */
    if (write(STDOUT_FILENO, query, strlen(query)) < 0) {
        return false;
    }

    /* Wait for response with timeout */
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

    if (result > 0) {
        /* Data available - read and discard response */
        char buffer[256];
        ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        return bytes_read > 0;
    }

    return false; /* Timeout or error */
}

/**
 * Safe terminal capability probing with timeout protection.
 */
lle_result_t lle_probe_terminal_capabilities_safe(
    lle_terminal_detection_result_t *detection) {

    /* Cannot probe without stdout TTY - mark as unsuccessful but not an error
     */
    if (!detection->stdout_is_tty) {
        detection->probing_successful = false;
        /* Set all probe flags to false - this is complete behavior for non-TTY
         */
        detection->supports_cursor_positioning = false;
        detection->supports_cursor_queries = false;
        detection->supports_bracketed_paste = false;
        detection->supports_mouse = false;
        return LLE_SUCCESS; /* Successfully determined we cannot probe */
    }

    /* Save terminal state */
    struct termios saved_termios;
    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        detection->probing_successful = false;
        return LLE_ERROR_TERMINAL_ABSTRACTION;
    }

    /* Set raw mode for accurate probing */
    struct termios raw_termios = saved_termios;
    raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw_termios.c_oflag &= ~(OPOST);
    raw_termios.c_cflag |= (CS8);
    raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios) != 0) {
        detection->probing_successful = false;
        return LLE_ERROR_TERMINAL_ABSTRACTION;
    }

    /* Progressive capability probing with timeout protection */

    /* Test cursor positioning (DSR - Device Status Report) */
    detection->supports_cursor_positioning =
        probe_capability_with_timeout("\x1b[6n", 100);

    /* If basic cursor support works, we likely have ANSI capability */
    if (detection->supports_cursor_positioning) {
        detection->supports_cursor_queries = true;
    }

    /* Test bracketed paste mode */
    detection->supports_bracketed_paste =
        probe_capability_with_timeout("\x1b[?2004h", 25);

    /* Test mouse support */
    detection->supports_mouse =
        probe_capability_with_timeout("\x1b[?1000h", 50);

    /* Restore terminal state */
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);

    detection->probing_successful = true;
    return LLE_SUCCESS;
}

/* ============================================================================
 * MODE DETERMINATION
 * ============================================================================
 */

/**
 * Determine fallback mode based on detection results.
 */
static lle_adaptive_mode_t
determine_fallback_mode(const lle_terminal_detection_result_t *detection) {

    /* No TTY at all - non-interactive */
    if (!detection->stdin_is_tty && !detection->stdout_is_tty) {
        return LLE_ADAPTIVE_MODE_NONE;
    }

    /* Both stdin and stdout are TTY - native mode */
    if (detection->stdin_is_tty && detection->stdout_is_tty) {
        return LLE_ADAPTIVE_MODE_NATIVE;
    }

    /* Only stdout is TTY - enhanced mode (editor terminal pattern) */
    if (!detection->stdin_is_tty && detection->stdout_is_tty) {
        return LLE_ADAPTIVE_MODE_ENHANCED;
    }

    /* Capable output but no TTY - minimal mode */
    return LLE_ADAPTIVE_MODE_MINIMAL;
}

/**
 * Validate and adjust recommended mode.
 */
static lle_adaptive_mode_t
validate_and_adjust_mode(const lle_terminal_detection_result_t *detection) {

    lle_adaptive_mode_t mode = detection->recommended_mode;

    /* Validate mode is compatible with TTY status */
    switch (mode) {
    case LLE_ADAPTIVE_MODE_NATIVE:
        /* Native mode requires stdin TTY */
        if (!detection->stdin_is_tty) {
            mode = LLE_ADAPTIVE_MODE_ENHANCED;
        }
        break;

    case LLE_ADAPTIVE_MODE_ENHANCED:
        /* Enhanced mode requires stdout TTY */
        if (!detection->stdout_is_tty) {
            mode = LLE_ADAPTIVE_MODE_MINIMAL;
        }
        break;

    case LLE_ADAPTIVE_MODE_MULTIPLEXED:
        /* Multiplexed mode requires both TTYs */
        if (!detection->stdin_is_tty || !detection->stdout_is_tty) {
            mode = LLE_ADAPTIVE_MODE_ENHANCED;
        }
        break;

    case LLE_ADAPTIVE_MODE_MINIMAL:
    case LLE_ADAPTIVE_MODE_NONE:
        /* No validation needed */
        break;
    }

    return mode;
}

/* ============================================================================
 * MAIN DETECTION API
 * ============================================================================
 */

/**
 * Perform comprehensive terminal capability detection.
 */
lle_result_t lle_detect_terminal_capabilities_comprehensive(
    lle_terminal_detection_result_t **result) {

    if (!result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Allocate detection result */
    lle_terminal_detection_result_t *detection =
        lle_pool_alloc(sizeof(lle_terminal_detection_result_t));
    if (!detection) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    memset(detection, 0, sizeof(lle_terminal_detection_result_t));

    uint64_t start_time = get_current_time_us();

    /* Step 1: Basic TTY status detection */
    detection->stdin_is_tty = isatty(STDIN_FILENO);
    detection->stdout_is_tty = isatty(STDOUT_FILENO);
    detection->stderr_is_tty = isatty(STDERR_FILENO);

    /* Get terminal dimensions */
    lle_get_terminal_size(&detection->terminal_cols, &detection->terminal_rows);

    /* Step 2: Environment variable analysis */
    lle_result_t env_result = analyze_environment_variables(detection);
    if (env_result != LLE_SUCCESS) {
        /* Continue with conservative defaults */
        detection->capability_level = LLE_CAPABILITY_BASIC;
    }

    /* Step 3: Terminal signature matching */
    detection->matched_signature = lle_match_terminal_signature(detection);
    if (detection->matched_signature) {
        detection->capability_level =
            detection->matched_signature->capability_level;
        detection->recommended_mode =
            detection->matched_signature->preferred_mode;
        detection->detection_confidence_high = true;
    } else {
        /* Step 4: Runtime capability probing (for unknown terminals) */
        lle_result_t probe_result =
            lle_probe_terminal_capabilities_safe(detection);
        detection->probing_successful = (probe_result == LLE_SUCCESS);
        detection->detection_confidence_high = detection->probing_successful;

        /* Step 5: Fallback mode determination */
        detection->recommended_mode = determine_fallback_mode(detection);

        /* Infer capability level from probing results */
        if (detection->supports_truecolor) {
            detection->capability_level = LLE_CAPABILITY_PREMIUM;
        } else if (detection->supports_256_colors) {
            detection->capability_level = LLE_CAPABILITY_FULL;
        } else if (detection->supports_colors) {
            detection->capability_level = LLE_CAPABILITY_STANDARD;
        } else {
            detection->capability_level = LLE_CAPABILITY_BASIC;
        }
    }

    /* Step 6: Final mode validation and adjustment */
    detection->recommended_mode = validate_and_adjust_mode(detection);

    detection->detection_time_us = get_current_time_us() - start_time;

    /* Update statistics */
    detection_stats.total_detections++;
    detection_stats.avg_detection_time_us =
        ((detection_stats.avg_detection_time_us *
          (detection_stats.total_detections - 1)) +
         detection->detection_time_us) /
        detection_stats.total_detections;
    if (detection->detection_time_us > detection_stats.max_detection_time_us) {
        detection_stats.max_detection_time_us = detection->detection_time_us;
    }

    *result = detection;
    return LLE_SUCCESS;
}

/**
 * Optimized detection with caching.
 */
lle_result_t lle_detect_terminal_capabilities_optimized(
    lle_terminal_detection_result_t **result) {

    if (!result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    uint64_t current_time = get_current_time_us();

    /* Check cache validity */
    if (cached_result && (current_time - cache_timestamp_us) < CACHE_TTL_US) {
        /* Return cached result */
        *result = cached_result;
        detection_stats.cache_hits++;
        return LLE_SUCCESS;
    }

    /* Cache miss - perform full detection */
    detection_stats.cache_misses++;

    lle_result_t res = lle_detect_terminal_capabilities_comprehensive(result);
    if (res == LLE_SUCCESS) {
        /* Update cache */
        if (cached_result) {
            lle_pool_free(cached_result);
        }
        cached_result = *result;
        cache_timestamp_us = current_time;
    }

    return res;
}

/**
 * Free detection result.
 */
void lle_terminal_detection_result_destroy(
    lle_terminal_detection_result_t *result) {
    if (result && result != cached_result) {
        lle_pool_free(result);
    }
}

/* ============================================================================
 * PERFORMANCE MONITORING
 * ============================================================================
 */

/**
 * Get detection performance statistics.
 */
lle_result_t
lle_adaptive_get_detection_stats(lle_detection_performance_stats_t *stats) {

    if (!stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    *stats = detection_stats;
    return LLE_SUCCESS;
}

/**
 * Reset detection performance statistics.
 */
void lle_adaptive_reset_detection_stats(void) {
    memset(&detection_stats, 0, sizeof(detection_stats));
}

/* ============================================================================
 * UTILITY IMPLEMENTATIONS
 * ============================================================================
 */

/**
 * Get human-readable mode name.
 */
const char *lle_adaptive_mode_to_string(lle_adaptive_mode_t mode) {
    switch (mode) {
    case LLE_ADAPTIVE_MODE_NONE:
        return "none";
    case LLE_ADAPTIVE_MODE_MINIMAL:
        return "minimal";
    case LLE_ADAPTIVE_MODE_ENHANCED:
        return "enhanced";
    case LLE_ADAPTIVE_MODE_NATIVE:
        return "native";
    case LLE_ADAPTIVE_MODE_MULTIPLEXED:
        return "multiplexed";
    default:
        return "unknown";
    }
}

/**
 * Get human-readable capability level name.
 */
const char *lle_capability_level_to_string(lle_capability_level_t level) {
    switch (level) {
    case LLE_CAPABILITY_NONE:
        return "none";
    case LLE_CAPABILITY_BASIC:
        return "basic";
    case LLE_CAPABILITY_STANDARD:
        return "standard";
    case LLE_CAPABILITY_FULL:
        return "full";
    case LLE_CAPABILITY_PREMIUM:
        return "premium";
    default:
        return "unknown";
    }
}

/* ============================================================================
 * TERMINAL TYPE DETECTION HELPERS
 * ============================================================================
 */

/**
 * Check if running in iTerm2.
 */
bool lle_is_iterm2(const lle_terminal_detection_result_t *detection) {
    if (detection) {
        return (detection->term_program[0] != '\0' &&
                strstr(detection->term_program, "iTerm") != NULL);
    }

    /* Check environment directly */
    const char *term_program = getenv("TERM_PROGRAM");
    return (term_program && strstr(term_program, "iTerm") != NULL);
}

/**
 * Check if running inside tmux.
 */
bool lle_is_tmux(const lle_terminal_detection_result_t *detection) {
    if (detection) {
        return (detection->multiplexer_type == LLE_MUX_TYPE_TMUX);
    }

    /* Check environment directly */
    const char *tmux = getenv("TMUX");
    return (tmux != NULL && tmux[0] != '\0');
}

/**
 * Check if running inside GNU screen.
 */
bool lle_is_screen(const lle_terminal_detection_result_t *detection) {
    if (detection) {
        return (detection->multiplexer_type == LLE_MUX_TYPE_SCREEN);
    }

    /* Check environment directly */
    const char *term = getenv("TERM");
    const char *sty = getenv("STY");

    return ((term && strstr(term, "screen") != NULL) ||
            (sty != NULL && sty[0] != '\0'));
}

/**
 * Check if running inside any terminal multiplexer.
 */
bool lle_is_multiplexed(const lle_terminal_detection_result_t *detection) {
    if (detection) {
        return (detection->multiplexer_type != LLE_MUX_TYPE_NONE);
    }

    /* Check for common multiplexers */
    return lle_is_tmux(NULL) || lle_is_screen(NULL);
}

/**
 * Get terminal type string (e.g., "xterm-256color").
 */
const char *
lle_get_terminal_type(const lle_terminal_detection_result_t *detection) {
    if (detection && detection->term_name[0] != '\0') {
        return detection->term_name;
    }

    return getenv("TERM");
}

/**
 * Get current terminal dimensions.
 */
lle_result_t lle_get_terminal_size(int *cols, int *rows) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (cols) {
            *cols = ws.ws_col > 0 ? ws.ws_col : 80;
        }
        if (rows) {
            *rows = ws.ws_row > 0 ? ws.ws_row : 24;
        }
        return LLE_SUCCESS;
    }

    /* Fallback to environment variables */
    const char *columns = getenv("COLUMNS");
    const char *lines = getenv("LINES");

    if (cols) {
        *cols = columns ? atoi(columns) : 80;
        if (*cols <= 0) {
            *cols = 80;
        }
    }
    if (rows) {
        *rows = lines ? atoi(lines) : 24;
        if (*rows <= 0) {
            *rows = 24;
        }
    }

    return LLE_SUCCESS;
}

/**
 * Check if stdout is a TTY.
 */
bool lle_is_tty(void) {
    return isatty(STDOUT_FILENO) != 0;
}

/**
 * Reset terminal to clean state.
 */
void lle_terminal_reset(void) {
    if (!isatty(STDOUT_FILENO)) {
        return;
    }

    /* Reset all attributes, show cursor, move to new line */
    static const char reset_seq[] = "\x1b[0m\x1b[?25h\n";
    (void)write(STDOUT_FILENO, reset_seq, sizeof(reset_seq) - 1);
}
