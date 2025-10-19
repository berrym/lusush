/**
 * LLE Terminal Capability Detection - Public API
 * 
 * Week 1 Implementation: Fast, environment-based terminal capability detection
 * Spec Reference: 02_terminal_abstraction_complete.md (Spec 02)
 *                 audit_26_into_02.md (Spec 26 adaptive detection)
 * 
 * Design Principles (MANDATORY):
 * - NO terminal queries during operation (detect once at initialization)
 * - Environment variables and terminfo ONLY
 * - Conservative fallback for unknown terminals
 * - Must complete in <50ms
 * - Stored in internal state, never re-queried
 */

#ifndef LLE_TERMINAL_H
#define LLE_TERMINAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/**
 * Terminal type classification
 */
typedef enum {
    LLE_TERMINAL_UNKNOWN = 0,
    LLE_TERMINAL_XTERM,
    LLE_TERMINAL_SCREEN,
    LLE_TERMINAL_TMUX,
    LLE_TERMINAL_KONSOLE,
    LLE_TERMINAL_GNOME_TERMINAL,
    LLE_TERMINAL_ALACRITTY,
    LLE_TERMINAL_KITTY,
    LLE_TERMINAL_ITERM2,
    LLE_TERMINAL_VSCODE,
    LLE_TERMINAL_ENHANCED,      // Enhanced editor terminal (Spec 26)
    LLE_TERMINAL_AI_ENVIRONMENT, // AI assistant environment (Spec 26)
} lle_terminal_type_t;

/**
 * Color depth detection
 */
typedef enum {
    LLE_COLOR_DEPTH_NONE = 0,
    LLE_COLOR_DEPTH_16 = 4,     // 4-bit color (16 colors)
    LLE_COLOR_DEPTH_256 = 8,    // 8-bit color (256 colors)
    LLE_COLOR_DEPTH_TRUECOLOR = 24, // 24-bit color (16M colors)
} lle_color_depth_t;

/**
 * Terminal capabilities structure
 * 
 * Detected once at initialization, stored in internal state.
 * NEVER re-queried during operation.
 */
typedef struct {
    // Terminal identification
    bool is_tty;                     // Running in TTY
    char *term_type;                 // TERM environment variable
    char *term_program;              // TERM_PROGRAM environment variable
    lle_terminal_type_t terminal_type; // Classified terminal type
    
    // Color capabilities (from environment/terminfo)
    bool supports_ansi_colors;       // Basic 16-color support
    bool supports_256_colors;        // 256-color support
    bool supports_truecolor;         // 24-bit color support
    lle_color_depth_t color_depth;   // Detected color depth
    
    // Text attributes (from terminfo)
    bool supports_bold;
    bool supports_italic;
    bool supports_underline;
    bool supports_strikethrough;
    bool supports_reverse;
    bool supports_dim;
    
    // Advanced features (from environment)
    bool supports_mouse_reporting;
    bool supports_bracketed_paste;
    bool supports_focus_events;
    bool supports_synchronized_output;
    bool supports_unicode;
    
    // Terminal geometry (from ioctl TIOCGWINSZ)
    uint16_t terminal_width;         // Columns
    uint16_t terminal_height;        // Rows
    
    // Adaptive terminal detection (Spec 26)
    bool is_enhanced_terminal;       // Modern editor terminal
    bool is_ai_environment;          // AI assistant environment
    
    // Performance characteristics
    uint32_t estimated_latency_ms;   // Estimated terminal latency
    bool supports_fast_updates;      // Can handle rapid updates
    
    // Detection metadata
    uint64_t detection_time_us;      // Time taken to detect (<50ms required)
    bool detection_complete;         // Detection successful
    
} lle_terminal_capabilities_t;

/**
 * Result codes for terminal operations
 */
typedef enum {
    LLE_TERMINAL_SUCCESS = 0,
    LLE_TERMINAL_ERROR_MEMORY,
    LLE_TERMINAL_ERROR_NOT_TTY,
    LLE_TERMINAL_ERROR_INVALID_PARAMETER,
    LLE_TERMINAL_ERROR_DETECTION_TIMEOUT,
    LLE_TERMINAL_ERROR_DETECTION_FAILED,
} lle_terminal_result_t;

/**
 * Detect terminal capabilities (ONCE at initialization)
 * 
 * This function MUST complete in <50ms. It:
 * 1. Checks TTY status (isatty)
 * 2. Reads environment variables ($TERM, $COLORTERM, $TERM_PROGRAM)
 * 3. Queries terminfo database (NO terminal interaction)
 * 4. Gets terminal size via ioctl (TIOCGWINSZ)
 * 5. Applies adaptive detection (Spec 26)
 * 6. Sets conservative fallbacks for unknowns
 * 
 * @param capabilities Output: Detected capabilities (allocated by function)
 * @return LLE_TERMINAL_SUCCESS or error code
 * 
 * CRITICAL: This function does NOT query the terminal state.
 *           All detection is environment/system-based.
 */
lle_terminal_result_t lle_terminal_detect_capabilities(
    lle_terminal_capabilities_t **capabilities
);

/**
 * Destroy capabilities structure
 * 
 * @param capabilities Capabilities to destroy
 */
void lle_terminal_capabilities_destroy(
    lle_terminal_capabilities_t *capabilities
);

/**
 * Get human-readable terminal type name
 * 
 * @param type Terminal type enum
 * @return Terminal type name string (static, do not free)
 */
const char *lle_terminal_type_name(
    lle_terminal_type_t type
);

/**
 * Get human-readable color depth description
 * 
 * @param depth Color depth enum
 * @return Color depth description (static, do not free)
 */
const char *lle_color_depth_name(
    lle_color_depth_t depth
);

/**
 * Print capabilities to file (for debugging/testing)
 * 
 * @param capabilities Capabilities to print
 * @param output Output file (e.g., stdout, stderr)
 */
void lle_terminal_capabilities_print(
    const lle_terminal_capabilities_t *capabilities,
    FILE *output
);

#endif /* LLE_TERMINAL_H */
