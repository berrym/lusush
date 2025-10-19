/**
 * LLE Terminal Capability Detection - Implementation
 * 
 * Week 1 Implementation: Fast, environment-based terminal capability detection
 * Spec Reference: 02_terminal_abstraction_complete.md (Spec 02)
 *                 audit_26_into_02.md (Spec 26 adaptive detection)
 * 
 * CRITICAL DESIGN PRINCIPLE:
 * This implementation does NOT query terminal state during operation.
 * All detection is performed ONCE at initialization using:
 * - Environment variables ($TERM, $COLORTERM, $TERM_PROGRAM, etc.)
 * - System calls (isatty, ioctl TIOCGWINSZ)
 * - Terminfo database queries (ncurses)
 * 
 * NO escape sequences are sent to the terminal.
 * NO terminal responses are read.
 * 
 * Performance requirement: Must complete in <50ms
 */

#include <lle/terminal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <time.h>

/* Forward declarations for internal functions */
static void detect_terminal_type(lle_terminal_capabilities_t *caps);
static void detect_color_capabilities(lle_terminal_capabilities_t *caps);
static void detect_text_attributes(lle_terminal_capabilities_t *caps);
static void detect_advanced_features(lle_terminal_capabilities_t *caps);
static void detect_terminal_geometry(lle_terminal_capabilities_t *caps);
static void detect_adaptive_features(lle_terminal_capabilities_t *caps);
static void apply_terminal_specific_optimizations(lle_terminal_capabilities_t *caps);
static void set_conservative_fallbacks(lle_terminal_capabilities_t *caps);
static uint64_t get_time_microseconds(void);

/**
 * Detect terminal capabilities (ONCE at initialization)
 * 
 * This is the CRITICAL FIRST STEP for Week 1.
 * Everything else in LLE depends on knowing terminal capabilities.
 */
lle_terminal_result_t lle_terminal_detect_capabilities(
    lle_terminal_capabilities_t **capabilities)
{
    if (!capabilities) {
        return LLE_TERMINAL_ERROR_INVALID_PARAMETER;
    }
    
    uint64_t start_time = get_time_microseconds();
    
    /* Allocate capabilities structure */
    lle_terminal_capabilities_t *caps = calloc(1, sizeof(lle_terminal_capabilities_t));
    if (!caps) {
        return LLE_TERMINAL_ERROR_MEMORY;
    }
    
    /* Step 1: Check TTY status (CRITICAL - determines if we're in terminal) */
    caps->is_tty = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    
    /* Step 2: Store TERM environment variable (even for non-TTY) */
    const char *term = getenv("TERM");
    if (term) {
        caps->term_type = strdup(term);
    } else {
        caps->term_type = strdup("unknown");
    }
    
    /* Step 3: Store TERM_PROGRAM environment variable */
    const char *term_program = getenv("TERM_PROGRAM");
    if (term_program) {
        caps->term_program = strdup(term_program);
    }
    
    /* Step 4: Detect terminal type from environment */
    detect_terminal_type(caps);
    
    /* Step 5: Handle non-TTY early (after reading environment) */
    if (!caps->is_tty) {
        /* Not a TTY - read environment but use conservative capabilities */
        set_conservative_fallbacks(caps);
        caps->terminal_width = 80;
        caps->terminal_height = 24;
        caps->terminal_type = LLE_TERMINAL_UNKNOWN;  /* Force UNKNOWN for non-TTY */
        
        /* Still detect adaptive features (Spec 26) even for non-TTY */
        detect_adaptive_features(caps);
        
        caps->detection_complete = true;
        
        uint64_t end_time = get_time_microseconds();
        caps->detection_time_us = end_time - start_time;
        
        *capabilities = caps;
        return LLE_TERMINAL_SUCCESS;
    }
    
    /* Step 6: Detect color capabilities from environment (TTY only) */
    detect_color_capabilities(caps);
    
    /* Step 7: Detect text attributes (would use terminfo, conservative for now) */
    detect_text_attributes(caps);
    
    /* Step 8: Detect advanced features from environment */
    detect_advanced_features(caps);
    
    /* Step 9: Detect terminal geometry via ioctl */
    detect_terminal_geometry(caps);
    
    /* Step 10: Adaptive terminal detection (Spec 26) */
    detect_adaptive_features(caps);
    
    /* Step 11: Apply terminal-specific optimizations */
    apply_terminal_specific_optimizations(caps);
    
    /* Step 12: Validate capabilities and set fallbacks if needed */
    if (caps->terminal_width == 0 || caps->terminal_height == 0) {
        caps->terminal_width = 80;
        caps->terminal_height = 24;
    }
    
    /* Mark detection as complete */
    caps->detection_complete = true;
    
    /* Calculate detection time */
    uint64_t end_time = get_time_microseconds();
    caps->detection_time_us = end_time - start_time;
    
    /* Verify <50ms requirement (50,000 microseconds) */
    if (caps->detection_time_us > 50000) {
        fprintf(stderr, "WARNING: Terminal detection took %lu us (>50ms)\n",
                (unsigned long)caps->detection_time_us);
    }
    
    *capabilities = caps;
    return LLE_TERMINAL_SUCCESS;
}

/**
 * Detect terminal type from TERM and TERM_PROGRAM environment variables
 * 
 * NO terminal queries - pure environment variable analysis
 */
static void detect_terminal_type(lle_terminal_capabilities_t *caps)
{
    const char *term = caps->term_type;
    const char *term_program = caps->term_program;
    
    /* Check TERM_PROGRAM first (most specific) */
    if (term_program) {
        if (strstr(term_program, "iTerm")) {
            caps->terminal_type = LLE_TERMINAL_ITERM2;
            return;
        }
        if (strstr(term_program, "vscode") || strstr(term_program, "cursor")) {
            caps->terminal_type = LLE_TERMINAL_VSCODE;
            return;
        }
        if (strstr(term_program, "konsole")) {
            caps->terminal_type = LLE_TERMINAL_KONSOLE;
            return;
        }
        if (strstr(term_program, "gnome-terminal")) {
            caps->terminal_type = LLE_TERMINAL_GNOME_TERMINAL;
            return;
        }
    }
    
    /* Check TERM variable */
    if (term) {
        if (strstr(term, "kitty")) {
            caps->terminal_type = LLE_TERMINAL_KITTY;
            return;
        }
        if (strstr(term, "alacritty")) {
            caps->terminal_type = LLE_TERMINAL_ALACRITTY;
            return;
        }
        if (strstr(term, "tmux")) {
            caps->terminal_type = LLE_TERMINAL_TMUX;
            return;
        }
        if (strstr(term, "screen")) {
            caps->terminal_type = LLE_TERMINAL_SCREEN;
            return;
        }
        if (strstr(term, "xterm")) {
            caps->terminal_type = LLE_TERMINAL_XTERM;
            return;
        }
    }
    
    /* Unknown terminal type */
    caps->terminal_type = LLE_TERMINAL_UNKNOWN;
}

/**
 * Detect color capabilities from environment variables
 * 
 * Priority:
 * 1. COLORTERM=truecolor/24bit → 24-bit color
 * 2. TERM=*-256color → 256 colors
 * 3. TERM=*-color → 16 colors
 * 4. Default → No color (conservative)
 */
static void detect_color_capabilities(lle_terminal_capabilities_t *caps)
{
    /* Check COLORTERM for truecolor support */
    const char *colorterm = getenv("COLORTERM");
    if (colorterm) {
        if (strcmp(colorterm, "truecolor") == 0 || strcmp(colorterm, "24bit") == 0) {
            caps->supports_truecolor = true;
            caps->supports_256_colors = true;
            caps->supports_ansi_colors = true;
            caps->color_depth = LLE_COLOR_DEPTH_TRUECOLOR;
            return;
        }
    }
    
    /* Check TERM for color support */
    const char *term = caps->term_type;
    if (term) {
        if (strstr(term, "256color") || strstr(term, "256")) {
            caps->supports_256_colors = true;
            caps->supports_ansi_colors = true;
            caps->color_depth = LLE_COLOR_DEPTH_256;
            return;
        }
        if (strstr(term, "color")) {
            caps->supports_ansi_colors = true;
            caps->color_depth = LLE_COLOR_DEPTH_16;
            return;
        }
    }
    
    /* Terminal-specific color support */
    switch (caps->terminal_type) {
        case LLE_TERMINAL_KITTY:
        case LLE_TERMINAL_ALACRITTY:
        case LLE_TERMINAL_ITERM2:
        case LLE_TERMINAL_VSCODE:
            /* Modern terminals - assume truecolor */
            caps->supports_truecolor = true;
            caps->supports_256_colors = true;
            caps->supports_ansi_colors = true;
            caps->color_depth = LLE_COLOR_DEPTH_TRUECOLOR;
            break;
            
        case LLE_TERMINAL_TMUX:
        case LLE_TERMINAL_SCREEN:
            /* Multiplexers - usually 256 color */
            caps->supports_256_colors = true;
            caps->supports_ansi_colors = true;
            caps->color_depth = LLE_COLOR_DEPTH_256;
            break;
            
        case LLE_TERMINAL_XTERM:
        case LLE_TERMINAL_GNOME_TERMINAL:
        case LLE_TERMINAL_KONSOLE:
            /* Traditional terminals - conservative 256 color */
            caps->supports_256_colors = true;
            caps->supports_ansi_colors = true;
            caps->color_depth = LLE_COLOR_DEPTH_256;
            break;
            
        default:
            /* Unknown terminal - no color (conservative) */
            caps->supports_ansi_colors = false;
            caps->supports_256_colors = false;
            caps->supports_truecolor = false;
            caps->color_depth = LLE_COLOR_DEPTH_NONE;
            break;
    }
}

/**
 * Detect text attribute capabilities
 * 
 * For Week 1, we use conservative detection.
 * Future: Could query terminfo database for specific capabilities.
 */
static void detect_text_attributes(lle_terminal_capabilities_t *caps)
{
    /* Most modern terminals support basic text attributes */
    if (caps->is_tty && caps->terminal_type != LLE_TERMINAL_UNKNOWN) {
        caps->supports_bold = true;
        caps->supports_underline = true;
        caps->supports_reverse = true;
        caps->supports_dim = true;
        
        /* Italic support is less universal */
        switch (caps->terminal_type) {
            case LLE_TERMINAL_KITTY:
            case LLE_TERMINAL_ALACRITTY:
            case LLE_TERMINAL_ITERM2:
            case LLE_TERMINAL_VSCODE:
                caps->supports_italic = true;
                break;
            default:
                caps->supports_italic = false;
                break;
        }
        
        /* Strikethrough is even less common */
        caps->supports_strikethrough = false;
    }
}

/**
 * Detect advanced features from environment
 * 
 * Features like bracketed paste, mouse reporting, etc.
 */
static void detect_advanced_features(lle_terminal_capabilities_t *caps)
{
    /* Unicode support - check LANG/LC_ALL */
    const char *lang = getenv("LANG");
    const char *lc_all = getenv("LC_ALL");
    
    if ((lang && strstr(lang, "UTF-8")) || (lc_all && strstr(lc_all, "UTF-8"))) {
        caps->supports_unicode = true;
    } else {
        /* Default to UTF-8 for modern terminals */
        caps->supports_unicode = (caps->terminal_type != LLE_TERMINAL_UNKNOWN);
    }
    
    /* Advanced features based on terminal type */
    switch (caps->terminal_type) {
        case LLE_TERMINAL_KITTY:
        case LLE_TERMINAL_ALACRITTY:
        case LLE_TERMINAL_ITERM2:
            caps->supports_mouse_reporting = true;
            caps->supports_bracketed_paste = true;
            caps->supports_focus_events = true;
            caps->supports_synchronized_output = true;
            break;
            
        case LLE_TERMINAL_XTERM:
        case LLE_TERMINAL_VSCODE:
            caps->supports_mouse_reporting = true;
            caps->supports_bracketed_paste = true;
            caps->supports_focus_events = false;
            caps->supports_synchronized_output = false;
            break;
            
        default:
            /* Conservative - assume no advanced features */
            caps->supports_mouse_reporting = false;
            caps->supports_bracketed_paste = false;
            caps->supports_focus_events = false;
            caps->supports_synchronized_output = false;
            break;
    }
}

/**
 * Detect terminal geometry using ioctl TIOCGWINSZ
 * 
 * This is a system call, NOT a terminal query.
 * It reads kernel's knowledge of terminal size.
 */
static void detect_terminal_geometry(lle_terminal_capabilities_t *caps)
{
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        caps->terminal_width = (ws.ws_col > 0) ? ws.ws_col : 80;
        caps->terminal_height = (ws.ws_row > 0) ? ws.ws_row : 24;
    } else {
        /* ioctl failed - use conservative defaults */
        caps->terminal_width = 80;
        caps->terminal_height = 24;
    }
}

/**
 * Adaptive terminal detection (Spec 26)
 * 
 * Detects enhanced editor terminals and AI assistant environments
 */
static void detect_adaptive_features(lle_terminal_capabilities_t *caps)
{
    /* Enhanced terminal detection (Spec 26 Section 2.1) */
    const char *term_program = caps->term_program;
    if (term_program) {
        if (strstr(term_program, "zed") ||
            strstr(term_program, "vscode") ||
            strstr(term_program, "cursor") ||
            strstr(term_program, "iTerm") ||
            strstr(term_program, "Hyper")) {
            caps->is_enhanced_terminal = true;
        }
    }
    
    /* AI environment detection (Spec 26 Section 2.2) */
    if (getenv("AI_ENVIRONMENT") || 
        getenv("ANTHROPIC_API_KEY") ||
        getenv("OPENAI_API_KEY") ||
        getenv("CLAUDE_CODE")) {
        caps->is_ai_environment = true;
    }
    
    /* If enhanced terminal or AI environment, upgrade capabilities */
    if (caps->is_enhanced_terminal || caps->is_ai_environment) {
        /* These environments typically support advanced features */
        if (caps->color_depth < LLE_COLOR_DEPTH_256) {
            caps->supports_256_colors = true;
            caps->supports_ansi_colors = true;
            caps->color_depth = LLE_COLOR_DEPTH_256;
        }
        caps->supports_unicode = true;
        caps->supports_fast_updates = true;
        caps->estimated_latency_ms = 10; /* Low latency */
    }
}

/**
 * Apply terminal-specific optimizations
 * 
 * Set performance characteristics based on known terminal behavior
 */
static void apply_terminal_specific_optimizations(lle_terminal_capabilities_t *caps)
{
    switch (caps->terminal_type) {
        case LLE_TERMINAL_KITTY:
        case LLE_TERMINAL_ALACRITTY:
            /* GPU-accelerated terminals - very fast */
            caps->supports_fast_updates = true;
            caps->estimated_latency_ms = 5;
            break;
            
        case LLE_TERMINAL_ITERM2:
        case LLE_TERMINAL_VSCODE:
            /* Fast terminals */
            caps->supports_fast_updates = true;
            caps->estimated_latency_ms = 10;
            break;
            
        case LLE_TERMINAL_TMUX:
        case LLE_TERMINAL_SCREEN:
            /* Multiplexers add latency */
            caps->supports_fast_updates = false;
            caps->estimated_latency_ms = 30;
            break;
            
        case LLE_TERMINAL_XTERM:
        case LLE_TERMINAL_GNOME_TERMINAL:
        case LLE_TERMINAL_KONSOLE:
            /* Traditional terminals - moderate speed */
            caps->supports_fast_updates = false;
            caps->estimated_latency_ms = 20;
            break;
            
        default:
            /* Unknown - conservative */
            caps->supports_fast_updates = false;
            caps->estimated_latency_ms = 50;
            break;
    }
}

/**
 * Set conservative fallback capabilities
 * 
 * Used when terminal type is unknown or not a TTY
 */
static void set_conservative_fallbacks(lle_terminal_capabilities_t *caps)
{
    caps->supports_ansi_colors = false;
    caps->supports_256_colors = false;
    caps->supports_truecolor = false;
    caps->color_depth = LLE_COLOR_DEPTH_NONE;
    
    caps->supports_bold = false;
    caps->supports_italic = false;
    caps->supports_underline = false;
    caps->supports_strikethrough = false;
    caps->supports_reverse = false;
    caps->supports_dim = false;
    
    caps->supports_mouse_reporting = false;
    caps->supports_bracketed_paste = false;
    caps->supports_focus_events = false;
    caps->supports_synchronized_output = false;
    caps->supports_unicode = false;
    
    caps->supports_fast_updates = false;
    caps->estimated_latency_ms = 100;
}

/**
 * Destroy capabilities structure
 */
void lle_terminal_capabilities_destroy(lle_terminal_capabilities_t *capabilities)
{
    if (!capabilities) {
        return;
    }
    
    free(capabilities->term_type);
    free(capabilities->term_program);
    free(capabilities);
}

/**
 * Get human-readable terminal type name
 */
const char *lle_terminal_type_name(lle_terminal_type_t type)
{
    switch (type) {
        case LLE_TERMINAL_UNKNOWN: return "Unknown";
        case LLE_TERMINAL_XTERM: return "XTerm";
        case LLE_TERMINAL_SCREEN: return "GNU Screen";
        case LLE_TERMINAL_TMUX: return "tmux";
        case LLE_TERMINAL_KONSOLE: return "Konsole";
        case LLE_TERMINAL_GNOME_TERMINAL: return "GNOME Terminal";
        case LLE_TERMINAL_ALACRITTY: return "Alacritty";
        case LLE_TERMINAL_KITTY: return "Kitty";
        case LLE_TERMINAL_ITERM2: return "iTerm2";
        case LLE_TERMINAL_VSCODE: return "VS Code Terminal";
        case LLE_TERMINAL_ENHANCED: return "Enhanced Editor Terminal";
        case LLE_TERMINAL_AI_ENVIRONMENT: return "AI Assistant Environment";
        default: return "Unknown";
    }
}

/**
 * Get human-readable color depth description
 */
const char *lle_color_depth_name(lle_color_depth_t depth)
{
    switch (depth) {
        case LLE_COLOR_DEPTH_NONE: return "No color";
        case LLE_COLOR_DEPTH_16: return "16 colors (4-bit)";
        case LLE_COLOR_DEPTH_256: return "256 colors (8-bit)";
        case LLE_COLOR_DEPTH_TRUECOLOR: return "Truecolor (24-bit)";
        default: return "Unknown";
    }
}

/**
 * Print capabilities to file (for debugging/testing)
 */
void lle_terminal_capabilities_print(
    const lle_terminal_capabilities_t *capabilities,
    FILE *output)
{
    if (!capabilities || !output) {
        return;
    }
    
    fprintf(output, "=== LLE Terminal Capabilities ===\n");
    fprintf(output, "Detection time: %lu microseconds (%s)\n",
            (unsigned long)capabilities->detection_time_us,
            capabilities->detection_time_us < 50000 ? "PASS" : "FAIL - >50ms");
    fprintf(output, "Detection complete: %s\n\n",
            capabilities->detection_complete ? "Yes" : "No");
    
    fprintf(output, "Terminal Identification:\n");
    fprintf(output, "  Is TTY: %s\n", capabilities->is_tty ? "Yes" : "No");
    fprintf(output, "  TERM: %s\n", capabilities->term_type ? capabilities->term_type : "(null)");
    fprintf(output, "  TERM_PROGRAM: %s\n", capabilities->term_program ? capabilities->term_program : "(null)");
    fprintf(output, "  Terminal Type: %s\n", lle_terminal_type_name(capabilities->terminal_type));
    fprintf(output, "  Enhanced Terminal: %s\n", capabilities->is_enhanced_terminal ? "Yes" : "No");
    fprintf(output, "  AI Environment: %s\n\n", capabilities->is_ai_environment ? "Yes" : "No");
    
    fprintf(output, "Color Capabilities:\n");
    fprintf(output, "  Color Depth: %s\n", lle_color_depth_name(capabilities->color_depth));
    fprintf(output, "  ANSI colors: %s\n", capabilities->supports_ansi_colors ? "Yes" : "No");
    fprintf(output, "  256 colors: %s\n", capabilities->supports_256_colors ? "Yes" : "No");
    fprintf(output, "  Truecolor: %s\n\n", capabilities->supports_truecolor ? "Yes" : "No");
    
    fprintf(output, "Text Attributes:\n");
    fprintf(output, "  Bold: %s\n", capabilities->supports_bold ? "Yes" : "No");
    fprintf(output, "  Italic: %s\n", capabilities->supports_italic ? "Yes" : "No");
    fprintf(output, "  Underline: %s\n", capabilities->supports_underline ? "Yes" : "No");
    fprintf(output, "  Strikethrough: %s\n", capabilities->supports_strikethrough ? "Yes" : "No");
    fprintf(output, "  Reverse: %s\n", capabilities->supports_reverse ? "Yes" : "No");
    fprintf(output, "  Dim: %s\n\n", capabilities->supports_dim ? "Yes" : "No");
    
    fprintf(output, "Advanced Features:\n");
    fprintf(output, "  Mouse reporting: %s\n", capabilities->supports_mouse_reporting ? "Yes" : "No");
    fprintf(output, "  Bracketed paste: %s\n", capabilities->supports_bracketed_paste ? "Yes" : "No");
    fprintf(output, "  Focus events: %s\n", capabilities->supports_focus_events ? "Yes" : "No");
    fprintf(output, "  Synchronized output: %s\n", capabilities->supports_synchronized_output ? "Yes" : "No");
    fprintf(output, "  Unicode: %s\n\n", capabilities->supports_unicode ? "Yes" : "No");
    
    fprintf(output, "Terminal Geometry:\n");
    fprintf(output, "  Width: %u columns\n", capabilities->terminal_width);
    fprintf(output, "  Height: %u rows\n\n", capabilities->terminal_height);
    
    fprintf(output, "Performance:\n");
    fprintf(output, "  Fast updates: %s\n", capabilities->supports_fast_updates ? "Yes" : "No");
    fprintf(output, "  Estimated latency: %u ms\n", capabilities->estimated_latency_ms);
    
    fprintf(output, "=================================\n");
}

/**
 * Get current time in microseconds
 */
static uint64_t get_time_microseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}
