/**
 * platform_detection.h - Cross-Platform Terminal Behavior Detection for LLE
 *
 * This module provides platform-specific terminal behavior detection to enable
 * optimal backspace sequences and terminal control across different operating
 * systems and terminal emulators. Designed to preserve the perfect macOS
 * implementation while adding Linux-specific optimizations.
 *
 * Features:
 * - Operating system detection (macOS, Linux, BSD)
 * - Terminal emulator identification (iTerm2, GNOME Terminal, Konsole, xterm)
 * - Platform-specific backspace sequence selection
 * - Cross-platform compatibility preservation
 * - Performance-optimized detection with caching
 *
 * Usage:
 *   lle_platform_init();                         // Initialize platform detection
 *   lle_platform_os_t os = lle_platform_get_os(); // Get operating system
 *   const char *seq = lle_platform_get_backspace_sequence(); // Get optimal backspace
 *   bool is_linux = lle_platform_is_linux();     // Quick platform checks
 */

#ifndef LLE_PLATFORM_DETECTION_H
#define LLE_PLATFORM_DETECTION_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================= Platform Types ======================= */

/**
 * Operating system detection enumeration
 */
typedef enum {
    LLE_PLATFORM_OS_UNKNOWN = 0,
    LLE_PLATFORM_OS_MACOS = 1,
    LLE_PLATFORM_OS_LINUX = 2,
    LLE_PLATFORM_OS_BSD = 3,
    LLE_PLATFORM_OS_CYGWIN = 4,
    LLE_PLATFORM_OS_MSYS = 5
} lle_platform_os_t;

/**
 * Terminal emulator detection enumeration
 */
typedef enum {
    LLE_PLATFORM_TERM_UNKNOWN = 0,
    LLE_PLATFORM_TERM_ITERM2 = 1,
    LLE_PLATFORM_TERM_GNOME = 2,
    LLE_PLATFORM_TERM_KONSOLE = 3,
    LLE_PLATFORM_TERM_XTERM = 4,
    LLE_PLATFORM_TERM_ALACRITTY = 5,
    LLE_PLATFORM_TERM_KITTY = 6,
    LLE_PLATFORM_TERM_WEZTERM = 7,
    LLE_PLATFORM_TERM_TMUX = 8,
    LLE_PLATFORM_TERM_SCREEN = 9,
    LLE_PLATFORM_TERM_VT100 = 10
} lle_platform_terminal_t;

/**
 * Backspace behavior characteristics
 */
typedef enum {
    LLE_PLATFORM_BACKSPACE_STANDARD = 0,    /* Standard \b \b sequence */
    LLE_PLATFORM_BACKSPACE_LINUX_GNOME = 1, /* GNOME Terminal optimized */
    LLE_PLATFORM_BACKSPACE_LINUX_KDE = 2,   /* Konsole/KDE optimized */
    LLE_PLATFORM_BACKSPACE_LINUX_XTERM = 3, /* xterm compatibility */
    LLE_PLATFORM_BACKSPACE_MACOS_ITERM = 4, /* iTerm2 optimized (current perfect) */
    LLE_PLATFORM_BACKSPACE_TMUX = 5,        /* tmux/screen compatibility */
    LLE_PLATFORM_BACKSPACE_FALLBACK = 6     /* Conservative fallback */
} lle_platform_backspace_type_t;

/**
 * Platform detection information structure
 */
typedef struct {
    lle_platform_os_t os;                      /* Detected operating system */
    lle_platform_terminal_t terminal;          /* Detected terminal emulator */
    lle_platform_backspace_type_t backspace;   /* Optimal backspace type */
    
    /* Platform characteristics */
    bool supports_unicode;                      /* UTF-8 support */
    bool supports_color;                        /* Color capability */
    bool supports_mouse;                        /* Mouse events */
    bool requires_special_handling;             /* Needs platform-specific code */
    
    /* Performance characteristics */
    bool fast_clearing;                         /* Fast screen clearing */
    bool efficient_cursor_queries;             /* Fast cursor position queries */
    bool batch_output_preferred;               /* Benefits from output batching */
    
    /* Terminal identification strings */
    char term_name[64];                         /* Terminal name from $TERM */
    char term_program[64];                      /* Terminal program name */
    char term_version[32];                      /* Terminal version if available */
    
    /* Detection confidence */
    int detection_confidence;                   /* 0-100 confidence in detection */
    bool detection_complete;                    /* Detection finished successfully */
} lle_platform_info_t;

/* ======================= Core Platform Detection ======================= */

/**
 * @brief Initialize platform detection system
 *
 * Performs comprehensive platform and terminal detection using environment
 * variables, system calls, and terminal capability queries. Caches results
 * for optimal performance on subsequent calls.
 *
 * @return true on successful detection, false on error
 */
bool lle_platform_init(void);

/**
 * @brief Cleanup platform detection resources
 *
 * Releases any resources allocated during platform detection and resets
 * the detection state for clean shutdown.
 */
void lle_platform_cleanup(void);

/**
 * @brief Get complete platform detection information
 *
 * Returns the complete platform detection results including operating system,
 * terminal emulator, and optimal settings for the current environment.
 *
 * @return Pointer to platform information structure, NULL on error
 */
const lle_platform_info_t *lle_platform_get_info(void);

/* ======================= Operating System Detection ======================= */

/**
 * @brief Get detected operating system
 *
 * Returns the detected operating system using compile-time detection
 * combined with runtime environment analysis for maximum accuracy.
 *
 * @return Operating system enumeration value
 */
lle_platform_os_t lle_platform_get_os(void);

/**
 * @brief Check if running on macOS
 *
 * Quick check for macOS platform with cached results for performance.
 * Useful for preserving the perfect macOS implementation behavior.
 *
 * @return true if running on macOS, false otherwise
 */
bool lle_platform_is_macos(void);

/**
 * @brief Check if running on Linux
 *
 * Quick check for Linux platform with cached results for performance.
 * Enables Linux-specific terminal optimizations.
 *
 * @return true if running on Linux, false otherwise
 */
bool lle_platform_is_linux(void);

/**
 * @brief Check if running on BSD
 *
 * Quick check for BSD platform (FreeBSD, OpenBSD, NetBSD) with cached
 * results for performance.
 *
 * @return true if running on BSD, false otherwise
 */
bool lle_platform_is_bsd(void);

/* ======================= Terminal Emulator Detection ======================= */

/**
 * @brief Get detected terminal emulator
 *
 * Returns the detected terminal emulator using environment variable analysis,
 * terminal capability queries, and behavior testing for accurate identification.
 *
 * @return Terminal emulator enumeration value
 */
lle_platform_terminal_t lle_platform_get_terminal(void);

/**
 * @brief Check if running in iTerm2
 *
 * Specialized detection for iTerm2 on macOS, which is the current perfect
 * implementation target. Preserves exact behavior for this terminal.
 *
 * @return true if running in iTerm2, false otherwise
 */
bool lle_platform_is_iterm2(void);

/**
 * @brief Check if running in GNOME Terminal
 *
 * Detection for GNOME Terminal on Linux, which is the most common Linux
 * terminal emulator and requires specific backspace sequence optimization.
 *
 * @return true if running in GNOME Terminal, false otherwise
 */
bool lle_platform_is_gnome_terminal(void);

/**
 * @brief Check if running in Konsole
 *
 * Detection for Konsole (KDE terminal), which may require different
 * backspace handling than GNOME Terminal for optimal Linux support.
 *
 * @return true if running in Konsole, false otherwise
 */
bool lle_platform_is_konsole(void);

/**
 * @brief Check if running in xterm or compatible
 *
 * Detection for xterm and xterm-compatible terminals, which use standard
 * ANSI escape sequences and serve as the baseline for Linux compatibility.
 *
 * @return true if running in xterm-compatible terminal, false otherwise
 */
bool lle_platform_is_xterm(void);

/**
 * @brief Check if running inside tmux or screen
 *
 * Detection for terminal multiplexers which may require different escape
 * sequence handling for optimal backspace behavior.
 *
 * @return true if inside tmux or screen, false otherwise
 */
bool lle_platform_is_multiplexer(void);

/* ======================= Backspace Sequence Optimization ======================= */

/**
 * @brief Get optimal backspace sequence for current platform
 *
 * Returns the most effective backspace sequence for the detected platform
 * and terminal combination. Preserves the perfect macOS behavior while
 * providing Linux-optimized sequences.
 *
 * @return Pointer to optimal backspace sequence string, never NULL
 */
const char *lle_platform_get_backspace_sequence(void);

/**
 * @brief Get backspace sequence length
 *
 * Returns the length in bytes of the optimal backspace sequence for
 * efficient terminal write operations.
 *
 * @return Length of backspace sequence in bytes
 */
size_t lle_platform_get_backspace_length(void);

/**
 * @brief Get backspace type for current platform
 *
 * Returns the detected backspace behavior type for advanced platform-specific
 * handling and optimization strategies.
 *
 * @return Backspace type enumeration value
 */
lle_platform_backspace_type_t lle_platform_get_backspace_type(void);

/**
 * @brief Check if platform requires special backspace handling
 *
 * Determines if the current platform requires special handling beyond
 * standard ANSI backspace sequences for optimal clearing behavior.
 *
 * @return true if special handling required, false for standard sequences
 */
bool lle_platform_requires_special_backspace(void);

/* ======================= Advanced Platform Features ======================= */

/**
 * @brief Check if platform supports efficient clearing
 *
 * Determines if the platform supports efficient multi-character clearing
 * operations for performance optimization in history navigation.
 *
 * @return true if efficient clearing supported, false otherwise
 */
bool lle_platform_supports_efficient_clearing(void);

/**
 * @brief Check if platform needs clearing verification
 *
 * Some platforms may require additional verification or cleanup after
 * backspace operations to ensure perfect visual results.
 *
 * @return true if clearing verification needed, false otherwise
 */
bool lle_platform_needs_clearing_verification(void);

/**
 * @brief Get platform-specific clear-to-EOL behavior
 *
 * Returns whether the platform's clear-to-EOL implementation is reliable
 * for artifact elimination in history navigation.
 *
 * @return true if clear-to-EOL is reliable, false if alternative needed
 */
bool lle_platform_has_reliable_clear_eol(void);

/* ======================= Performance Optimization ======================= */

/**
 * @brief Get recommended batch size for platform
 *
 * Returns the optimal number of backspace operations to batch together
 * for best performance on the detected platform.
 *
 * @return Recommended batch size (1 = no batching, >1 = batch operations)
 */
size_t lle_platform_get_optimal_batch_size(void);

/**
 * @brief Check if platform benefits from output buffering
 *
 * Determines if the platform performs better with buffered output
 * operations rather than individual write calls.
 *
 * @return true if buffering recommended, false for immediate writes
 */
bool lle_platform_prefers_buffered_output(void);

/* ======================= Debugging and Diagnostics ======================= */

/**
 * @brief Get human-readable platform description
 *
 * Generates a comprehensive description of the detected platform for
 * debugging and diagnostic purposes.
 *
 * @param buffer Buffer to store description
 * @param buffer_size Size of buffer in bytes
 * @return Number of characters written, -1 on error
 */
int lle_platform_get_description(char *buffer, size_t buffer_size);

/**
 * @brief Force platform re-detection
 *
 * Forces a complete re-detection of platform characteristics, useful
 * for testing or when terminal environment changes during runtime.
 *
 * @return true on successful re-detection, false on error
 */
bool lle_platform_redetect(void);

/**
 * @brief Validate platform detection accuracy
 *
 * Performs additional validation tests to verify the accuracy of the
 * platform detection results and backspace sequence selection.
 *
 * @return Detection confidence score (0-100)
 */
int lle_platform_validate_detection(void);

/* ======================= Linux-Specific Functions ======================= */

/**
 * @brief Detect specific Linux distribution
 *
 * Attempts to identify the specific Linux distribution for targeted
 * optimizations based on distribution-specific terminal defaults.
 *
 * @param buffer Buffer to store distribution name
 * @param buffer_size Size of buffer in bytes
 * @return true if distribution detected, false if unknown
 */
bool lle_platform_detect_linux_distribution(char *buffer, size_t buffer_size);

/**
 * @brief Get Linux desktop environment
 *
 * Detects the desktop environment (GNOME, KDE, XFCE, etc.) to select
 * the most appropriate terminal handling for the user's environment.
 *
 * @param buffer Buffer to store desktop environment name
 * @param buffer_size Size of buffer in bytes
 * @return true if desktop environment detected, false if unknown
 */
bool lle_platform_detect_desktop_environment(char *buffer, size_t buffer_size);

/* ======================= Constants and Defaults ======================= */

/* Platform-specific backspace sequences */
#define LLE_PLATFORM_BACKSPACE_SEQ_MACOS_ITERM2   "\b \b"    /* Perfect macOS sequence */
#define LLE_PLATFORM_BACKSPACE_SEQ_LINUX_GNOME    "\b \b"    /* GNOME Terminal optimized */
#define LLE_PLATFORM_BACKSPACE_SEQ_LINUX_KONSOLE  "\b \b"    /* Konsole optimized */
#define LLE_PLATFORM_BACKSPACE_SEQ_LINUX_XTERM    "\b \b"    /* xterm standard */
#define LLE_PLATFORM_BACKSPACE_SEQ_FALLBACK       "\b \b"    /* Conservative fallback */

/* Sequence lengths for performance */
#define LLE_PLATFORM_BACKSPACE_LENGTH_STANDARD    3      /* Length of "\b \b" */
#define LLE_PLATFORM_BACKSPACE_LENGTH_EXTENDED    4      /* Extended sequences */

/* Detection confidence thresholds */
#define LLE_PLATFORM_CONFIDENCE_HIGH      90     /* High confidence detection */
#define LLE_PLATFORM_CONFIDENCE_MEDIUM    70     /* Medium confidence detection */
#define LLE_PLATFORM_CONFIDENCE_LOW       50     /* Low confidence detection */

/* Performance tuning constants */
#define LLE_PLATFORM_BATCH_SIZE_DEFAULT    1     /* Default batch size */
#define LLE_PLATFORM_BATCH_SIZE_LINUX      1     /* Linux optimal batch size */
#define LLE_PLATFORM_BATCH_SIZE_MACOS      1     /* macOS optimal batch size */

/* ======================= Inline Convenience Functions ======================= */

/**
 * @brief Quick check for Unix-like systems
 *
 * Fast inline check for Unix-like operating systems (macOS, Linux, BSD)
 * for general compatibility decisions.
 *
 * @return true if running on Unix-like system, false otherwise
 */
static inline bool lle_platform_is_unix(void) {
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    return true;
#else
    return false;
#endif
}

/**
 * @brief Quick check for POSIX compatibility
 *
 * Fast inline check for POSIX-compatible systems for terminal
 * operation compatibility decisions.
 *
 * @return true if POSIX-compatible, false otherwise
 */
static inline bool lle_platform_is_posix(void) {
#ifdef _POSIX_VERSION
    return true;
#else
    return lle_platform_is_unix();
#endif
}

/* ======================= Error Codes ======================= */

#define LLE_PLATFORM_OK                    0     /* Success */
#define LLE_PLATFORM_ERROR_INIT           -1     /* Initialization failed */
#define LLE_PLATFORM_ERROR_DETECTION      -2     /* Detection failed */
#define LLE_PLATFORM_ERROR_UNSUPPORTED    -3     /* Platform not supported */
#define LLE_PLATFORM_ERROR_INVALID_PARAM  -4     /* Invalid parameter */
#define LLE_PLATFORM_ERROR_BUFFER_SIZE    -5     /* Buffer too small */

/* ======================= Integration Macros ======================= */

/**
 * Platform-specific compilation macros for conditional behavior
 */
#ifdef __APPLE__
    #define LLE_PLATFORM_COMPILE_TIME_OS    LLE_PLATFORM_OS_MACOS
    #define LLE_PLATFORM_DEFAULT_BACKSPACE  LLE_PLATFORM_BACKSPACE_MACOS_ITERM
#elif defined(__linux__)
    #define LLE_PLATFORM_COMPILE_TIME_OS    LLE_PLATFORM_OS_LINUX
    #define LLE_PLATFORM_DEFAULT_BACKSPACE  LLE_PLATFORM_BACKSPACE_LINUX_GNOME
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    #define LLE_PLATFORM_COMPILE_TIME_OS    LLE_PLATFORM_OS_BSD
    #define LLE_PLATFORM_DEFAULT_BACKSPACE  LLE_PLATFORM_BACKSPACE_LINUX_XTERM
#else
    #define LLE_PLATFORM_COMPILE_TIME_OS    LLE_PLATFORM_OS_UNKNOWN
    #define LLE_PLATFORM_DEFAULT_BACKSPACE  LLE_PLATFORM_BACKSPACE_FALLBACK
#endif

/**
 * Convenience macro for platform-specific code blocks
 */
#define LLE_PLATFORM_SWITCH(os_var) \
    switch(os_var)

#define LLE_PLATFORM_CASE_MACOS \
    case LLE_PLATFORM_OS_MACOS:

#define LLE_PLATFORM_CASE_LINUX \
    case LLE_PLATFORM_OS_LINUX:

#define LLE_PLATFORM_CASE_BSD \
    case LLE_PLATFORM_OS_BSD:

#define LLE_PLATFORM_CASE_DEFAULT \
    default:

#ifdef __cplusplus
}
#endif

#endif /* LLE_PLATFORM_DETECTION_H */