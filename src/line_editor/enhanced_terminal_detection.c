/*
 * Enhanced Terminal Detection for Lusush Line Editor (LLE)
 * 
 * This module provides comprehensive terminal capability detection that goes
 * beyond simple isatty() checks to identify modern terminals like Zed, iTerm2,
 * VS Code, and other editor-embedded terminals that should be treated as
 * interactive environments despite having non-TTY stdin.
 *
 * Key improvements:
 * - Detects editor-embedded terminals (Zed, VS Code, etc.)
 * - Probes terminal capabilities through escape sequences
 * - Uses environment variables for enhanced detection
 * - Provides fallback interactive mode for capable terminals
 * - Fixes cross-platform color and completion issues
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "enhanced_terminal_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>

// ============================================================================
// Internal State and Configuration
// ============================================================================

static lle_enhanced_terminal_info_t g_enhanced_term_info = {0};
static bool g_enhanced_detection_performed = false;

/**
 * @brief Known terminal signatures for detection
 */

static const lle_terminal_signature_t known_terminals[] = {
    // Modern editor terminals
    {
        .name = "zed",
        .term_program_pattern = "zed",
        .term_pattern = "xterm-256color",
        .env_var_check = "COLORTERM",
        .capability_level = LLE_TERMINAL_CAPABILITY_FULL,
        .interactive_mode = LLE_INTERACTIVE_ENHANCED,
        .force_interactive = true
    },
    {
        .name = "vscode",
        .term_program_pattern = "vscode",
        .term_pattern = "xterm-256color",
        .env_var_check = "COLORTERM",
        .capability_level = LLE_TERMINAL_CAPABILITY_FULL,
        .interactive_mode = LLE_INTERACTIVE_ENHANCED,
        .force_interactive = true
    },
    {
        .name = "cursor",
        .term_program_pattern = "cursor",
        .term_pattern = "xterm-256color",
        .env_var_check = "COLORTERM",
        .capability_level = LLE_TERMINAL_CAPABILITY_FULL,
        .interactive_mode = LLE_INTERACTIVE_ENHANCED,
        .force_interactive = true
    },
    
    // Native terminals
    {
        .name = "iterm2",
        .term_program_pattern = "iTerm",
        .term_pattern = "*",
        .env_var_check = "ITERM_SESSION_ID",
        .capability_level = LLE_TERMINAL_CAPABILITY_PREMIUM,
        .interactive_mode = LLE_INTERACTIVE_NATIVE,
        .force_interactive = false
    },
    {
        .name = "konsole",
        .term_program_pattern = "konsole",
        .term_pattern = "konsole*",
        .env_var_check = NULL,
        .capability_level = LLE_TERMINAL_CAPABILITY_FULL,
        .interactive_mode = LLE_INTERACTIVE_NATIVE,
        .force_interactive = false
    },
    {
        .name = "gnome-terminal",
        .term_program_pattern = "gnome-terminal",
        .term_pattern = "gnome*",
        .env_var_check = NULL,
        .capability_level = LLE_TERMINAL_CAPABILITY_FULL,
        .interactive_mode = LLE_INTERACTIVE_NATIVE,
        .force_interactive = false
    },
    {
        .name = "terminal.app",
        .term_program_pattern = "Apple_Terminal",
        .term_pattern = "xterm*",
        .env_var_check = NULL,
        .capability_level = LLE_TERMINAL_CAPABILITY_STANDARD,
        .interactive_mode = LLE_INTERACTIVE_NATIVE,
        .force_interactive = false
    },
    
    // Terminal multiplexers
    {
        .name = "tmux",
        .term_program_pattern = NULL,
        .term_pattern = "tmux*",
        .env_var_check = "TMUX",
        .capability_level = LLE_TERMINAL_CAPABILITY_FULL,
        .interactive_mode = LLE_INTERACTIVE_MULTIPLEXED,
        .force_interactive = false
    },
    {
        .name = "screen",
        .term_program_pattern = NULL,
        .term_pattern = "screen*",
        .env_var_check = "STY",
        .capability_level = LLE_TERMINAL_CAPABILITY_STANDARD,
        .interactive_mode = LLE_INTERACTIVE_MULTIPLEXED,
        .force_interactive = false
    }
};

#define NUM_KNOWN_TERMINALS (sizeof(known_terminals) / sizeof(known_terminals[0]))

// ============================================================================
// Terminal Capability Probing
// ============================================================================

/**
 * @brief Probe terminal for specific capability using escape sequences
 * 
 * @param probe_sequence Escape sequence to send
 * @param expected_response_prefix Expected response prefix
 * @param timeout_ms Timeout in milliseconds
 * @return true if capability is supported
 */
static bool lle_probe_terminal_capability(const char *probe_sequence,
                                         const char *expected_response_prefix,
                                         int timeout_ms) {
    if (!probe_sequence || !isatty(STDOUT_FILENO)) {
        return false;
    }
    
    // Save current terminal settings
    struct termios orig_termios;
    if (tcgetattr(STDIN_FILENO, &orig_termios) != 0) {
        return false;
    }
    
    // Set raw mode for response reading
    struct termios raw_termios = orig_termios;
    raw_termios.c_lflag &= ~(ICANON | ECHO);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) != 0) {
        return false;
    }
    
    // Send probe sequence
    if (write(STDOUT_FILENO, probe_sequence, strlen(probe_sequence)) <= 0) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        return false;
    }
    
    // Wait for response with timeout
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    bool capability_detected = false;
    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0) {
        char response[64];
        ssize_t bytes_read = read(STDIN_FILENO, response, sizeof(response) - 1);
        if (bytes_read > 0) {
            response[bytes_read] = '\0';
            if (expected_response_prefix &&
                strncmp(response, expected_response_prefix, strlen(expected_response_prefix)) == 0) {
                capability_detected = true;
            } else if (!expected_response_prefix && bytes_read > 0) {
                // Any response indicates capability
                capability_detected = true;
            }
        }
    }
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    
    return capability_detected;
}

/**
 * @brief Test if terminal supports true color
 * 
 * @return true if truecolor is supported
 */
static bool lle_test_truecolor_support(void) {
    // Check COLORTERM environment variable first
    const char *colorterm = getenv("COLORTERM");
    if (colorterm && (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit"))) {
        return true;
    }
    
    // Test with escape sequence probe
    return lle_probe_terminal_capability(
        "\x1b[38;2;1;2;3m\x1b[48;2;4;5;6m",  // Set truecolor fg/bg
        NULL,  // Any response indicates support
        100    // 100ms timeout
    );
}

/**
 * @brief Test if terminal supports 256 colors
 * 
 * @return true if 256-color is supported
 */
static bool lle_test_256color_support(void) {
    const char *term = getenv("TERM");
    if (term && strstr(term, "256")) {
        return true;
    }
    
    return lle_probe_terminal_capability(
        "\x1b[38;5;196m",  // Set 256-color red
        NULL,
        100
    );
}

/**
 * @brief Test if terminal supports cursor position queries
 * 
 * @return true if cursor queries are supported
 */
static bool lle_test_cursor_query_support(void) {
    return lle_probe_terminal_capability(
        "\x1b[6n",     // Device Status Report - cursor position
        "\x1b[",       // Expected response starts with ESC[
        200            // 200ms timeout
    );
}

// ============================================================================
// Enhanced Terminal Detection Logic
// ============================================================================

/**
 * @brief Check if terminal matches a known signature
 * 
 * @param signature Terminal signature to check
 * @return true if terminal matches the signature
 */
static bool lle_matches_terminal_signature(const lle_terminal_signature_t *signature) {
    if (!signature) return false;
    
    // Check TERM_PROGRAM environment variable
    if (signature->term_program_pattern) {
        const char *term_program = getenv("TERM_PROGRAM");
        if (term_program && strstr(term_program, signature->term_program_pattern)) {
            return true;
        }
    }
    
    // Check TERM environment variable
    if (signature->term_pattern) {
        const char *term = getenv("TERM");
        if (term) {
            if (strcmp(signature->term_pattern, "*") == 0) {
                // Wildcard pattern matches any TERM
            } else if (strstr(term, signature->term_pattern)) {
                return true;
            }
        }
    }
    
    // Check specific environment variable
    if (signature->env_var_check) {
        const char *env_value = getenv(signature->env_var_check);
        if (env_value && *env_value) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Detect terminal capabilities through probing
 * 
 * @param info Terminal info structure to populate
 */
static void lle_detect_terminal_capabilities(lle_enhanced_terminal_info_t *info) {
    if (!info) return;
    
    // Basic TTY detection
    info->stdin_is_tty = isatty(STDIN_FILENO);
    info->stdout_is_tty = isatty(STDOUT_FILENO);
    info->stderr_is_tty = isatty(STDERR_FILENO);
    
    // Color support detection
    info->supports_colors = true;  // Assume basic color support
    info->supports_256_colors = lle_test_256color_support();
    info->supports_truecolor = lle_test_truecolor_support();
    
    // Cursor and positioning support
    info->supports_cursor_queries = lle_test_cursor_query_support();
    info->supports_cursor_positioning = info->stdout_is_tty;
    
    // Terminal size detection
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        info->terminal_width = ws.ws_col;
        info->terminal_height = ws.ws_row;
        info->supports_terminal_resize = true;
    } else {
        info->terminal_width = 80;   // Default fallback
        info->terminal_height = 24;  // Default fallback
        info->supports_terminal_resize = false;
    }
    
    // Advanced feature detection
    info->supports_mouse = info->stdout_is_tty;
    info->supports_bracketed_paste = info->stdout_is_tty;
    info->supports_focus_events = info->stdout_is_tty;
    
    // Unicode support (assume modern terminals support it)
    info->supports_unicode = true;
}

/**
 * @brief Determine if terminal should be treated as interactive
 * 
 * @param info Terminal info structure
 * @return Interactive mode recommendation
 */
static lle_interactive_mode_t lle_determine_interactive_mode(const lle_enhanced_terminal_info_t *info) {
    if (!info) return LLE_INTERACTIVE_NONE;
    
    // Check if any known terminal signature forces interactive mode
    for (size_t i = 0; i < NUM_KNOWN_TERMINALS; i++) {
        if (lle_matches_terminal_signature(&known_terminals[i])) {
            if (known_terminals[i].force_interactive) {
                return known_terminals[i].interactive_mode;
            }
        }
    }
    
    // Traditional TTY-based detection
    if (info->stdin_is_tty && info->stdout_is_tty) {
        return LLE_INTERACTIVE_NATIVE;
    }
    
    // Enhanced detection: capable terminal even if stdin is not TTY
    if (info->stdout_is_tty && info->supports_colors && info->supports_cursor_positioning) {
        // Terminal can display colors and position cursor - treat as interactive
        return LLE_INTERACTIVE_ENHANCED;
    }
    
    return LLE_INTERACTIVE_NONE;
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool lle_enhanced_terminal_detection_init(void) {
    if (g_enhanced_detection_performed) {
        return true;
    }
    
    // Initialize info structure
    memset(&g_enhanced_term_info, 0, sizeof(g_enhanced_term_info));
    
    // Get basic environment information
    const char *term = getenv("TERM");
    const char *term_program = getenv("TERM_PROGRAM");
    const char *colorterm = getenv("COLORTERM");
    
    strncpy(g_enhanced_term_info.term_name, term ? term : "unknown", 
            sizeof(g_enhanced_term_info.term_name) - 1);
    strncpy(g_enhanced_term_info.term_program, term_program ? term_program : "unknown",
            sizeof(g_enhanced_term_info.term_program) - 1);
    strncpy(g_enhanced_term_info.colorterm, colorterm ? colorterm : "",
            sizeof(g_enhanced_term_info.colorterm) - 1);
    
    // Detect terminal capabilities
    lle_detect_terminal_capabilities(&g_enhanced_term_info);
    
    // Find matching terminal signature
    g_enhanced_term_info.terminal_signature = NULL;
    for (size_t i = 0; i < NUM_KNOWN_TERMINALS; i++) {
        if (lle_matches_terminal_signature(&known_terminals[i])) {
            g_enhanced_term_info.terminal_signature = &known_terminals[i];
            g_enhanced_term_info.capability_level = known_terminals[i].capability_level;
            break;
        }
    }
    
    // Default capability level if no signature matched
    if (!g_enhanced_term_info.terminal_signature) {
        if (g_enhanced_term_info.supports_truecolor) {
            g_enhanced_term_info.capability_level = LLE_TERMINAL_CAPABILITY_FULL;
        } else if (g_enhanced_term_info.supports_256_colors) {
            g_enhanced_term_info.capability_level = LLE_TERMINAL_CAPABILITY_STANDARD;
        } else {
            g_enhanced_term_info.capability_level = LLE_TERMINAL_CAPABILITY_BASIC;
        }
    }
    
    // Determine interactive mode
    g_enhanced_term_info.interactive_mode = lle_determine_interactive_mode(&g_enhanced_term_info);
    
    g_enhanced_detection_performed = true;
    return true;
}

const lle_enhanced_terminal_info_t *lle_enhanced_terminal_get_info(void) {
    if (!g_enhanced_detection_performed) {
        lle_enhanced_terminal_detection_init();
    }
    return &g_enhanced_term_info;
}

bool lle_enhanced_terminal_should_be_interactive(void) {
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    return info->interactive_mode != LLE_INTERACTIVE_NONE;
}

bool lle_enhanced_terminal_supports_colors(void) {
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    return info->supports_colors;
}

bool lle_enhanced_terminal_supports_advanced_features(void) {
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    return info->capability_level >= LLE_TERMINAL_CAPABILITY_FULL;
}

const char *lle_enhanced_terminal_get_debug_info(void) {
    static char debug_buffer[1024];
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    
    snprintf(debug_buffer, sizeof(debug_buffer),
        "Enhanced Terminal Detection Report:\n"
        "  TERM: %s\n"
        "  TERM_PROGRAM: %s\n"
        "  COLORTERM: %s\n"
        "  Terminal: %s\n"
        "  TTY Status: stdin=%s, stdout=%s, stderr=%s\n"
        "  Color Support: basic=%s, 256=%s, truecolor=%s\n"
        "  Capabilities: cursor_queries=%s, positioning=%s, resize=%s\n"
        "  Size: %dx%d\n"
        "  Interactive Mode: %d\n"
        "  Capability Level: %d\n"
        "  Should Be Interactive: %s\n",
        info->term_name,
        info->term_program,
        info->colorterm,
        info->terminal_signature ? info->terminal_signature->name : "unknown",
        info->stdin_is_tty ? "yes" : "no",
        info->stdout_is_tty ? "yes" : "no", 
        info->stderr_is_tty ? "yes" : "no",
        info->supports_colors ? "yes" : "no",
        info->supports_256_colors ? "yes" : "no",
        info->supports_truecolor ? "yes" : "no",
        info->supports_cursor_queries ? "yes" : "no",
        info->supports_cursor_positioning ? "yes" : "no",
        info->supports_terminal_resize ? "yes" : "no",
        info->terminal_width,
        info->terminal_height,
        info->interactive_mode,
        info->capability_level,
        lle_enhanced_terminal_should_be_interactive() ? "YES" : "NO"
    );
    
    return debug_buffer;
}

void lle_enhanced_terminal_detection_cleanup(void) {
    memset(&g_enhanced_term_info, 0, sizeof(g_enhanced_term_info));
    g_enhanced_detection_performed = false;
}