/**
 * platform_detection.c - Cross-platform detection for perfect history navigation
 *
 * This module implements platform detection to provide Linux users with the same
 * perfect history navigation experience as macOS users. Uses platform-specific
 * backspace sequences and terminal handling for flawless cross-platform operation.
 *
 * SUCCESS FOUNDATION: Based on 100% perfect macOS implementation using exact
 * backspace replication that achieved "happiest user has been with history recall"
 *
 * DESIGN PRINCIPLE: Preserve perfect macOS behavior while adding Linux compatibility
 */

#include "platform_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

/* ======================= Global State ======================= */

static lle_platform_info_t g_platform_info = {0};
static bool g_platform_initialized = false;

/* ======================= Platform-Specific Backspace Sequences ======================= */

/* Perfect macOS sequence (proven 100% successful) */
static const char *MACOS_ITERM2_BACKSPACE = LLE_PLATFORM_BACKSPACE_SEQ_MACOS_ITERM2;

/* Linux-specific sequences for different terminals */
static const char *LINUX_GNOME_BACKSPACE = LLE_PLATFORM_BACKSPACE_SEQ_LINUX_GNOME;
static const char *LINUX_KONSOLE_BACKSPACE = LLE_PLATFORM_BACKSPACE_SEQ_LINUX_KONSOLE;
static const char *LINUX_XTERM_BACKSPACE = LLE_PLATFORM_BACKSPACE_SEQ_LINUX_XTERM;
static const char *LINUX_ALACRITTY_BACKSPACE = LLE_PLATFORM_BACKSPACE_SEQ_LINUX_XTERM;
static const char *LINUX_KITTY_BACKSPACE = LLE_PLATFORM_BACKSPACE_SEQ_LINUX_XTERM;

/* Fallback sequence (universal compatibility) */
static const char *FALLBACK_BACKSPACE = LLE_PLATFORM_BACKSPACE_SEQ_FALLBACK;

/* ======================= Internal Detection Functions ======================= */

/**
 * @brief Detect operating system using compile-time and runtime detection
 */
static lle_platform_os_t lle_platform_detect_os(void) {
#ifdef __APPLE__
    return LLE_PLATFORM_OS_MACOS;
#elif defined(__linux__)
    return LLE_PLATFORM_OS_LINUX;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    return LLE_PLATFORM_OS_BSD;
#elif defined(__CYGWIN__)
    return LLE_PLATFORM_OS_CYGWIN;
#elif defined(__MSYS__)
    return LLE_PLATFORM_OS_MSYS;
#else
    return LLE_PLATFORM_OS_UNKNOWN;
#endif
}

/**
 * @brief Detect terminal type using environment variables and terminal queries
 */
static lle_platform_terminal_t lle_platform_detect_terminal(void) {
    const char *term = getenv("TERM");
    const char *term_program = getenv("TERM_PROGRAM");
    const char *colorterm = getenv("COLORTERM");
    const char *konsole_version = getenv("KONSOLE_VERSION");
    const char *wezterm_exe = getenv("WEZTERM_EXE");
    
    /* iTerm2 detection (macOS) */
    if (term_program && strstr(term_program, "iTerm")) {
        return LLE_PLATFORM_TERM_ITERM2;
    }
    
    /* GNOME Terminal detection */
    if (term_program && strstr(term_program, "gnome-terminal")) {
        return LLE_PLATFORM_TERM_GNOME;
    }
    if (colorterm && strstr(colorterm, "gnome-terminal")) {
        return LLE_PLATFORM_TERM_GNOME;
    }
    
    /* Konsole detection (KDE) */
    if (konsole_version || (term && strstr(term, "konsole"))) {
        return LLE_PLATFORM_TERM_KONSOLE;
    }
    
    /* Alacritty detection */
    if (term && strstr(term, "alacritty")) {
        return LLE_PLATFORM_TERM_ALACRITTY;
    }
    
    /* Kitty terminal detection */
    if (term && strstr(term, "xterm-kitty")) {
        return LLE_PLATFORM_TERM_KITTY;
    }
    
    /* WezTerm detection */
    if (wezterm_exe || (term && strstr(term, "wezterm"))) {
        return LLE_PLATFORM_TERM_WEZTERM;
    }
    
    /* tmux/screen detection */
    if (term && strstr(term, "tmux")) {
        return LLE_PLATFORM_TERM_TMUX;
    }
    if (term && strstr(term, "screen")) {
        return LLE_PLATFORM_TERM_SCREEN;
    }
    
    /* xterm variants */
    if (term && (strstr(term, "xterm") || strstr(term, "rxvt"))) {
        return LLE_PLATFORM_TERM_XTERM;
    }
    
    /* Fallback to VT100 for basic compatibility */
    return LLE_PLATFORM_TERM_VT100;
}

/**
 * @brief Determine optimal backspace type based on platform and terminal
 */
static lle_platform_backspace_type_t lle_platform_determine_backspace_type(
    lle_platform_os_t os, lle_platform_terminal_t terminal) {
    
    switch (os) {
        case LLE_PLATFORM_OS_MACOS:
            if (terminal == LLE_PLATFORM_TERM_ITERM2) {
                return LLE_PLATFORM_BACKSPACE_MACOS_ITERM;
            }
            return LLE_PLATFORM_BACKSPACE_STANDARD;
            
        case LLE_PLATFORM_OS_LINUX:
            switch (terminal) {
                case LLE_PLATFORM_TERM_GNOME:
                    return LLE_PLATFORM_BACKSPACE_LINUX_GNOME;
                case LLE_PLATFORM_TERM_KONSOLE:
                    return LLE_PLATFORM_BACKSPACE_LINUX_KDE;
                case LLE_PLATFORM_TERM_XTERM:
                case LLE_PLATFORM_TERM_ALACRITTY:
                case LLE_PLATFORM_TERM_KITTY:
                    return LLE_PLATFORM_BACKSPACE_LINUX_XTERM;
                default:
                    return LLE_PLATFORM_BACKSPACE_LINUX_GNOME;  /* Safe Linux default */
            }
            
        case LLE_PLATFORM_OS_BSD:
            return LLE_PLATFORM_BACKSPACE_LINUX_XTERM;  /* BSD uses xterm-like behavior */
            
        default:
            return LLE_PLATFORM_BACKSPACE_FALLBACK;
    }
}

/**
 * @brief Set platform-specific performance characteristics
 */
static void lle_platform_set_performance_characteristics(lle_platform_info_t *info) {
    switch (info->os) {
        case LLE_PLATFORM_OS_MACOS:
            info->fast_clearing = true;
            info->efficient_cursor_queries = true;
            info->batch_output_preferred = false;
            break;
            
        case LLE_PLATFORM_OS_LINUX:
            /* Linux terminals vary more, use conservative defaults */
            info->fast_clearing = true;
            info->efficient_cursor_queries = (info->terminal != LLE_PLATFORM_TERM_TMUX);
            info->batch_output_preferred = (info->terminal == LLE_PLATFORM_TERM_TMUX);
            break;
            
        default:
            /* Conservative defaults for unknown platforms */
            info->fast_clearing = false;
            info->efficient_cursor_queries = false;
            info->batch_output_preferred = true;
            break;
    }
}

/* ======================= Public API Implementation ======================= */

bool lle_platform_init(void) {
    if (g_platform_initialized) {
        return true;  /* Already initialized */
    }
    
    /* Clear platform info */
    memset(&g_platform_info, 0, sizeof(g_platform_info));
    
    /* Detect operating system */
    g_platform_info.os = lle_platform_detect_os();
    
    /* Detect terminal type */
    g_platform_info.terminal = lle_platform_detect_terminal();
    
    /* Determine optimal backspace type */
    g_platform_info.backspace = lle_platform_determine_backspace_type(
        g_platform_info.os, g_platform_info.terminal);
    
    /* Set basic capabilities */
    g_platform_info.supports_unicode = true;  /* Assume modern terminal */
    g_platform_info.supports_color = true;
    g_platform_info.supports_mouse = (g_platform_info.terminal != LLE_PLATFORM_TERM_VT100);
    
    /* Platform-specific settings */
    g_platform_info.requires_special_handling = 
        (g_platform_info.os == LLE_PLATFORM_OS_LINUX) ||
        (g_platform_info.terminal == LLE_PLATFORM_TERM_TMUX);
    
    /* Set performance characteristics */
    lle_platform_set_performance_characteristics(&g_platform_info);
    
    /* Set detection confidence based on available information */
    if (g_platform_info.os != LLE_PLATFORM_OS_UNKNOWN && 
        g_platform_info.terminal != LLE_PLATFORM_TERM_UNKNOWN) {
        g_platform_info.detection_confidence = LLE_PLATFORM_CONFIDENCE_HIGH;
    } else if (g_platform_info.os != LLE_PLATFORM_OS_UNKNOWN) {
        g_platform_info.detection_confidence = LLE_PLATFORM_CONFIDENCE_MEDIUM;
    } else {
        g_platform_info.detection_confidence = LLE_PLATFORM_CONFIDENCE_LOW;
    }
    
    g_platform_info.detection_complete = true;
    g_platform_initialized = true;
    
    return true;
}

void lle_platform_cleanup(void) {
    g_platform_initialized = false;
    memset(&g_platform_info, 0, sizeof(g_platform_info));
}

const lle_platform_info_t *lle_platform_get_info(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    return &g_platform_info;
}

/* ======================= Operating System Detection ======================= */

lle_platform_os_t lle_platform_get_os(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    return g_platform_info.os;
}

bool lle_platform_is_macos(void) {
    return lle_platform_get_os() == LLE_PLATFORM_OS_MACOS;
}

bool lle_platform_is_linux(void) {
    return lle_platform_get_os() == LLE_PLATFORM_OS_LINUX;
}

bool lle_platform_is_bsd(void) {
    return lle_platform_get_os() == LLE_PLATFORM_OS_BSD;
}

/* ======================= Terminal Detection ======================= */

lle_platform_terminal_t lle_platform_get_terminal(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    return g_platform_info.terminal;
}

bool lle_platform_is_iterm2(void) {
    return lle_platform_get_terminal() == LLE_PLATFORM_TERM_ITERM2;
}

bool lle_platform_is_gnome_terminal(void) {
    return lle_platform_get_terminal() == LLE_PLATFORM_TERM_GNOME;
}

bool lle_platform_is_konsole(void) {
    return lle_platform_get_terminal() == LLE_PLATFORM_TERM_KONSOLE;
}

bool lle_platform_is_xterm(void) {
    return lle_platform_get_terminal() == LLE_PLATFORM_TERM_XTERM;
}

bool lle_platform_is_multiplexer(void) {
    lle_platform_terminal_t term = lle_platform_get_terminal();
    return (term == LLE_PLATFORM_TERM_TMUX || term == LLE_PLATFORM_TERM_SCREEN);
}

/* ======================= Backspace Sequence Management ======================= */

const char *lle_platform_get_backspace_sequence(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    
    switch (g_platform_info.backspace) {
        case LLE_PLATFORM_BACKSPACE_MACOS_ITERM:
            return MACOS_ITERM2_BACKSPACE;
            
        case LLE_PLATFORM_BACKSPACE_LINUX_GNOME:
            return LINUX_GNOME_BACKSPACE;
            
        case LLE_PLATFORM_BACKSPACE_LINUX_KDE:
            return LINUX_KONSOLE_BACKSPACE;
            
        case LLE_PLATFORM_BACKSPACE_LINUX_XTERM:
            return LINUX_XTERM_BACKSPACE;
            
        case LLE_PLATFORM_BACKSPACE_TMUX:
            return FALLBACK_BACKSPACE;  /* tmux uses standard sequence */
            
        case LLE_PLATFORM_BACKSPACE_STANDARD:
        case LLE_PLATFORM_BACKSPACE_FALLBACK:
        default:
            return FALLBACK_BACKSPACE;
    }
}

size_t lle_platform_get_backspace_length(void) {
    /* All current sequences are 3 bytes ("\b \b") */
    return LLE_PLATFORM_BACKSPACE_LENGTH_STANDARD;
}

lle_platform_backspace_type_t lle_platform_get_backspace_type(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    return g_platform_info.backspace;
}

bool lle_platform_requires_special_backspace(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    
    /* Linux terminals may require special handling */
    return (g_platform_info.os == LLE_PLATFORM_OS_LINUX) ||
           (g_platform_info.terminal == LLE_PLATFORM_TERM_TMUX) ||
           (g_platform_info.terminal == LLE_PLATFORM_TERM_SCREEN);
}

/* ======================= Performance Characteristics ======================= */

bool lle_platform_supports_efficient_clearing(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    return g_platform_info.fast_clearing;
}

bool lle_platform_needs_clearing_verification(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    
    /* Linux terminals sometimes need verification */
    return (g_platform_info.os == LLE_PLATFORM_OS_LINUX) &&
           (g_platform_info.terminal == LLE_PLATFORM_TERM_KONSOLE ||
            g_platform_info.terminal == LLE_PLATFORM_TERM_GNOME);
}

bool lle_platform_has_reliable_clear_eol(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    
    /* Most modern terminals have reliable clear-to-EOL */
    return (g_platform_info.terminal != LLE_PLATFORM_TERM_VT100) &&
           (g_platform_info.terminal != LLE_PLATFORM_TERM_UNKNOWN);
}

size_t lle_platform_get_optimal_batch_size(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    
    switch (g_platform_info.os) {
        case LLE_PLATFORM_OS_MACOS:
            return LLE_PLATFORM_BATCH_SIZE_MACOS;
        case LLE_PLATFORM_OS_LINUX:
            return LLE_PLATFORM_BATCH_SIZE_LINUX;
        default:
            return LLE_PLATFORM_BATCH_SIZE_DEFAULT;
    }
}

bool lle_platform_prefers_buffered_output(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    return g_platform_info.batch_output_preferred;
}

/* ======================= Advanced Detection ======================= */

int lle_platform_get_description(char *buffer, size_t size) {
    if (!buffer || size == 0) {
        return LLE_PLATFORM_ERROR_INVALID_PARAM;
    }
    
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    
    const char *os_name = "Unknown";
    const char *term_name = "Unknown";
    
    /* OS name mapping */
    switch (g_platform_info.os) {
        case LLE_PLATFORM_OS_MACOS: os_name = "macOS"; break;
        case LLE_PLATFORM_OS_LINUX: os_name = "Linux"; break;
        case LLE_PLATFORM_OS_BSD: os_name = "BSD"; break;
        case LLE_PLATFORM_OS_CYGWIN: os_name = "Cygwin"; break;
        case LLE_PLATFORM_OS_MSYS: os_name = "MSYS"; break;
        default: os_name = "Unknown"; break;
    }
    
    /* Terminal name mapping */
    switch (g_platform_info.terminal) {
        case LLE_PLATFORM_TERM_ITERM2: term_name = "iTerm2"; break;
        case LLE_PLATFORM_TERM_GNOME: term_name = "GNOME Terminal"; break;
        case LLE_PLATFORM_TERM_KONSOLE: term_name = "Konsole"; break;
        case LLE_PLATFORM_TERM_XTERM: term_name = "xterm"; break;
        case LLE_PLATFORM_TERM_ALACRITTY: term_name = "Alacritty"; break;
        case LLE_PLATFORM_TERM_KITTY: term_name = "Kitty"; break;
        case LLE_PLATFORM_TERM_WEZTERM: term_name = "WezTerm"; break;
        case LLE_PLATFORM_TERM_TMUX: term_name = "tmux"; break;
        case LLE_PLATFORM_TERM_SCREEN: term_name = "GNU screen"; break;
        case LLE_PLATFORM_TERM_VT100: term_name = "VT100"; break;
        default: term_name = "Unknown"; break;
    }
    
    return snprintf(buffer, size, "%s on %s (confidence: %d%%)", 
                   term_name, os_name, g_platform_info.detection_confidence);
}



bool lle_platform_redetect(void) {
    g_platform_initialized = false;
    return lle_platform_init();
}

int lle_platform_validate_detection(void) {
    if (!g_platform_initialized) {
        lle_platform_init();
    }
    
    /* Validate that we have reasonable detection results */
    if (g_platform_info.os == LLE_PLATFORM_OS_UNKNOWN) {
        return LLE_PLATFORM_ERROR_DETECTION;
    }
    
    if (g_platform_info.detection_confidence < LLE_PLATFORM_CONFIDENCE_LOW) {
        return LLE_PLATFORM_ERROR_DETECTION;
    }
    
    return LLE_PLATFORM_OK;
}

/* ======================= Linux-Specific Detection ======================= */

bool lle_platform_detect_linux_distribution(char *buffer, size_t size) {
    if (!lle_platform_is_linux() || !buffer || size == 0) {
        return false;
    }
    
    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp) {
        /* Fallback to generic Linux */
        snprintf(buffer, size, "Linux");
        return true;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "NAME=", 5) == 0) {
            /* Extract distribution name */
            char *start = strchr(line, '"');
            if (start) {
                start++;
                char *end = strchr(start, '"');
                if (end) {
                    *end = '\0';
                    snprintf(buffer, size, "%s", start);
                    fclose(fp);
                    return true;
                }
            }
        }
    }
    
    fclose(fp);
    snprintf(buffer, size, "Linux");
    return true;
}

bool lle_platform_detect_desktop_environment(char *buffer, size_t size) {
    if (!lle_platform_is_linux() || !buffer || size == 0) {
        return false;
    }
    
    const char *desktop = getenv("XDG_CURRENT_DESKTOP");
    const char *session = getenv("DESKTOP_SESSION");
    const char *kde_session = getenv("KDE_FULL_SESSION");
    
    if (desktop) {
        snprintf(buffer, size, "%s", desktop);
        return true;
    } else if (session) {
        snprintf(buffer, size, "%s", session);
        return true;
    } else if (kde_session && strcmp(kde_session, "true") == 0) {
        snprintf(buffer, size, "KDE");
        return true;
    }
    
    /* Check for common desktop environments */
    if (getenv("GNOME_DESKTOP_SESSION_ID")) {
        snprintf(buffer, size, "GNOME");
        return true;
    }
    
    snprintf(buffer, size, "Unknown");
    return false;
}

/* ======================= Convenience Functions ======================= */

/* Note: lle_platform_is_unix and lle_platform_is_posix are defined as inline functions in header */