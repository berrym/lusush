/*
 * Enhanced Terminal Detection Header for Lusush Line Editor (LLE)
 * 
 * This header defines comprehensive terminal capability detection that goes
 * beyond simple isatty() checks to identify modern terminals and their
 * specific capabilities for optimal LLE functionality.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#ifndef LLE_ENHANCED_TERMINAL_DETECTION_H
#define LLE_ENHANCED_TERMINAL_DETECTION_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Terminal Capability and Mode Enumerations
// ============================================================================

/**
 * @brief Terminal capability levels
 */
typedef enum {
    LLE_TERMINAL_CAPABILITY_NONE = 0,      /**< No terminal capabilities */
    LLE_TERMINAL_CAPABILITY_BASIC = 1,     /**< Basic ANSI colors only */
    LLE_TERMINAL_CAPABILITY_STANDARD = 2,  /**< 256 colors, cursor positioning */
    LLE_TERMINAL_CAPABILITY_FULL = 3,      /**< Truecolor, advanced features */
    LLE_TERMINAL_CAPABILITY_PREMIUM = 4    /**< All features + proprietary extensions */
} lle_terminal_capability_level_t;

/**
 * @brief Interactive mode recommendations
 */
typedef enum {
    LLE_INTERACTIVE_NONE = 0,         /**< Non-interactive mode only */
    LLE_INTERACTIVE_ENHANCED = 1,     /**< Interactive despite non-TTY stdin */
    LLE_INTERACTIVE_NATIVE = 2,       /**< Traditional TTY-based interactive */
    LLE_INTERACTIVE_MULTIPLEXED = 3   /**< Interactive through multiplexer */
} lle_interactive_mode_t;

// ============================================================================
// Terminal Signature Structure
// ============================================================================

/**
 * @brief Terminal signature for identification and capability mapping
 */
typedef struct {
    const char *name;
    const char *term_program_pattern;
    const char *term_pattern;
    const char *env_var_check;
    lle_terminal_capability_level_t capability_level;
    lle_interactive_mode_t interactive_mode;
    bool force_interactive;
} lle_terminal_signature_t;

// ============================================================================
// Enhanced Terminal Information Structure
// ============================================================================

/**
 * @brief Comprehensive terminal information and capabilities
 */
typedef struct {
    // Basic terminal identification
    char term_name[64];           /**< TERM environment variable */
    char term_program[64];        /**< TERM_PROGRAM environment variable */
    char colorterm[32];           /**< COLORTERM environment variable */
    
    // TTY status for each file descriptor
    bool stdin_is_tty;            /**< stdin is a TTY */
    bool stdout_is_tty;           /**< stdout is a TTY */
    bool stderr_is_tty;           /**< stderr is a TTY */
    
    // Color support capabilities
    bool supports_colors;         /**< Basic ANSI color support */
    bool supports_256_colors;     /**< 256-color support */
    bool supports_truecolor;      /**< 24-bit truecolor support */
    
    // Cursor and positioning capabilities
    bool supports_cursor_queries; /**< Cursor position queries */
    bool supports_cursor_positioning; /**< Absolute cursor positioning */
    bool supports_terminal_resize; /**< Terminal resize detection */
    
    // Terminal dimensions
    int terminal_width;           /**< Current terminal width */
    int terminal_height;          /**< Current terminal height */
    
    // Advanced feature support
    bool supports_mouse;          /**< Mouse input support */
    bool supports_bracketed_paste; /**< Bracketed paste mode */
    bool supports_focus_events;   /**< Focus in/out events */
    bool supports_unicode;        /**< Unicode/UTF-8 support */
    
    // Terminal classification
    const lle_terminal_signature_t *terminal_signature; /**< Matched terminal signature */
    lle_terminal_capability_level_t capability_level;   /**< Overall capability level */
    lle_interactive_mode_t interactive_mode;            /**< Recommended interactive mode */
    
} lle_enhanced_terminal_info_t;

// ============================================================================
// Public API Functions
// ============================================================================

/**
 * @brief Initialize enhanced terminal detection system
 * 
 * Performs comprehensive terminal capability detection including environment
 * variable analysis, escape sequence probing, and capability classification.
 * 
 * @return true on successful initialization
 * 
 * @note This function may send escape sequences to the terminal for capability
 *       probing. It safely handles timeouts and restores terminal state.
 */
bool lle_enhanced_terminal_detection_init(void);

/**
 * @brief Get comprehensive terminal information
 * 
 * @return Pointer to terminal information structure
 * 
 * @note Automatically initializes detection if not already done.
 *       The returned pointer remains valid until cleanup is called.
 */
const lle_enhanced_terminal_info_t *lle_enhanced_terminal_get_info(void);

/**
 * @brief Check if terminal should be treated as interactive
 * 
 * This function considers multiple factors beyond isatty() including:
 * - Known terminal signatures (Zed, VS Code, etc.)
 * - Color and cursor positioning capabilities
 * - Environment variable indicators
 * 
 * @return true if interactive mode is recommended
 * 
 * @note This may return true even if stdin is not a TTY for capable
 *       editor-embedded terminals.
 */
bool lle_enhanced_terminal_should_be_interactive(void);

/**
 * @brief Check if terminal supports color output
 * 
 * @return true if any level of color support is available
 */
bool lle_enhanced_terminal_supports_colors(void);

/**
 * @brief Check if terminal supports advanced features
 * 
 * Advanced features include truecolor, cursor queries, mouse support,
 * and other modern terminal capabilities.
 * 
 * @return true if terminal has full or premium capability level
 */
bool lle_enhanced_terminal_supports_advanced_features(void);

/**
 * @brief Get detailed debug information about terminal detection
 * 
 * Returns a formatted string containing comprehensive information about
 * the detected terminal capabilities and classification results.
 * 
 * @return Pointer to static debug information string
 * 
 * @note The returned string is updated on each call and should be
 *       copied if persistence is needed.
 */
const char *lle_enhanced_terminal_get_debug_info(void);

/**
 * @brief Clean up enhanced terminal detection resources
 * 
 * Resets all detection state and clears cached information.
 * The system can be re-initialized after cleanup.
 */
void lle_enhanced_terminal_detection_cleanup(void);

// ============================================================================
// Convenience Macros
// ============================================================================

/**
 * @brief Check if current terminal is a known editor terminal
 */
#define LLE_IS_EDITOR_TERMINAL() \
    (lle_enhanced_terminal_get_info()->interactive_mode == LLE_INTERACTIVE_ENHANCED)

/**
 * @brief Check if current terminal supports truecolor
 */
#define LLE_SUPPORTS_TRUECOLOR() \
    (lle_enhanced_terminal_get_info()->supports_truecolor)

/**
 * @brief Check if current terminal supports 256 colors
 */
#define LLE_SUPPORTS_256_COLORS() \
    (lle_enhanced_terminal_get_info()->supports_256_colors)

/**
 * @brief Get current terminal width
 */
#define LLE_TERMINAL_WIDTH() \
    (lle_enhanced_terminal_get_info()->terminal_width)

/**
 * @brief Get current terminal height
 */
#define LLE_TERMINAL_HEIGHT() \
    (lle_enhanced_terminal_get_info()->terminal_height)

// ============================================================================
// Debug and Testing Support
// ============================================================================

#ifdef LLE_DEBUG_TERMINAL_DETECTION

/**
 * @brief Print comprehensive terminal detection debug information
 * 
 * Only available when LLE_DEBUG_TERMINAL_DETECTION is defined.
 * Outputs detailed information to stderr for debugging purposes.
 */
void lle_enhanced_terminal_debug_print(void);

/**
 * @brief Force specific terminal type for testing
 * 
 * Only available when LLE_DEBUG_TERMINAL_DETECTION is defined.
 * Allows overriding detected terminal type for testing purposes.
 * 
 * @param term_name Terminal name to simulate
 * @param term_program TERM_PROGRAM value to simulate
 * @param force_interactive Whether to force interactive mode
 */
void lle_enhanced_terminal_force_type(const char *term_name,
                                     const char *term_program,
                                     bool force_interactive);

#endif /* LLE_DEBUG_TERMINAL_DETECTION */

#ifdef __cplusplus
}
#endif

#endif /* LLE_ENHANCED_TERMINAL_DETECTION_H */