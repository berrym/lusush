// src/lle/foundation/terminal/capability.c
//
// Terminal capability detection
// This runs ONCE at initialization with a timeout to detect terminal features

#include "terminal.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

// Query terminal with timeout (returns bytes read, -1 on error/timeout)
static int query_terminal_with_timeout(int fd, const char *query,
                                        char *response, size_t max_len,
                                        uint32_t timeout_ms) {
    // Write query sequence
    ssize_t written = write(fd, query, strlen(query));
    if (written != (ssize_t)strlen(query)) {
        return -1;
    }
    
    // Setup select for timeout
    fd_set readfds;
    struct timeval timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000
    };
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    // Wait for response
    int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (ret <= 0) {
        return -1;  // Timeout or error
    }
    
    // Read response
    ssize_t n = read(fd, response, max_len - 1);
    if (n > 0) {
        response[n] = '\0';
        return (int)n;
    }
    
    return -1;
}

// Detect terminal type from TERM environment variable
static lle_term_type_t detect_term_type(void) {
    const char *term_env = getenv("TERM");
    if (!term_env) {
        return LLE_TERM_TYPE_UNKNOWN;
    }
    
    // Match terminal types
    if (strstr(term_env, "alacritty")) {
        return LLE_TERM_TYPE_ALACRITTY;
    } else if (strstr(term_env, "kitty")) {
        return LLE_TERM_TYPE_KITTY;
    } else if (strstr(term_env, "konsole")) {
        return LLE_TERM_TYPE_KONSOLE;
    } else if (strstr(term_env, "gnome")) {
        return LLE_TERM_TYPE_GNOME_TERMINAL;
    } else if (strstr(term_env, "xterm-256")) {
        return LLE_TERM_TYPE_XTERM_256COLOR;
    } else if (strstr(term_env, "xterm")) {
        return LLE_TERM_TYPE_XTERM;
    } else if (strstr(term_env, "rxvt")) {
        return LLE_TERM_TYPE_RXVT;
    } else if (strstr(term_env, "screen")) {
        return LLE_TERM_TYPE_SCREEN;
    } else if (strstr(term_env, "tmux")) {
        return LLE_TERM_TYPE_TMUX;
    } else if (strstr(term_env, "vt100")) {
        return LLE_TERM_TYPE_VT100;
    }
    
    return LLE_TERM_TYPE_UNKNOWN;
}

// Detect color support from TERM and COLORTERM
static void detect_color_support(lle_terminal_capabilities_t *caps) {
    const char *term_env = getenv("TERM");
    const char *colorterm_env = getenv("COLORTERM");
    
    // Check for truecolor support
    if (colorterm_env) {
        if (strcmp(colorterm_env, "truecolor") == 0 ||
            strcmp(colorterm_env, "24bit") == 0) {
            caps->has_true_color = true;
            caps->has_256_color = true;
            caps->has_color = true;
            return;
        }
    }
    
    // Check TERM for color support
    if (term_env) {
        if (strstr(term_env, "256color")) {
            caps->has_256_color = true;
            caps->has_color = true;
        } else if (strstr(term_env, "color")) {
            caps->has_color = true;
        }
        
        // Modern terminals typically support truecolor
        if (strstr(term_env, "alacritty") ||
            strstr(term_env, "kitty") ||
            strstr(term_env, "konsole")) {
            caps->has_true_color = true;
            caps->has_256_color = true;
            caps->has_color = true;
        }
    }
}

// Detect Unicode support from locale
static bool detect_unicode_support(void) {
    const char *lang = getenv("LANG");
    const char *lc_all = getenv("LC_ALL");
    const char *lc_ctype = getenv("LC_CTYPE");
    
    // Check in order of precedence
    const char *locale = lc_all ? lc_all : (lc_ctype ? lc_ctype : lang);
    
    if (locale && strstr(locale, "UTF-8")) {
        return true;
    }
    
    return false;
}

// Detect text attributes support
static void detect_text_attributes(lle_terminal_capabilities_t *caps) {
    // Most modern terminals support these
    // We use conservative defaults based on terminal type
    
    switch (caps->terminal_type) {
        case LLE_TERM_TYPE_VT100:
            caps->has_bold = true;
            caps->has_underline = true;
            caps->has_italic = false;  // VT100 doesn't support italic
            break;
            
        case LLE_TERM_TYPE_XTERM:
        case LLE_TERM_TYPE_XTERM_256COLOR:
        case LLE_TERM_TYPE_KONSOLE:
        case LLE_TERM_TYPE_GNOME_TERMINAL:
        case LLE_TERM_TYPE_ALACRITTY:
        case LLE_TERM_TYPE_KITTY:
            caps->has_bold = true;
            caps->has_underline = true;
            caps->has_italic = true;
            break;
            
        case LLE_TERM_TYPE_RXVT:
        case LLE_TERM_TYPE_SCREEN:
        case LLE_TERM_TYPE_TMUX:
            caps->has_bold = true;
            caps->has_underline = true;
            caps->has_italic = true;  // Most support italic now
            break;
            
        default:
            // Conservative defaults for unknown terminals
            caps->has_bold = true;
            caps->has_underline = true;
            caps->has_italic = false;
            break;
    }
}

// Detect interactive features (mouse, bracketed paste, focus events)
static void detect_interactive_features(lle_terminal_capabilities_t *caps) {
    // Modern terminals typically support these features
    
    switch (caps->terminal_type) {
        case LLE_TERM_TYPE_XTERM:
        case LLE_TERM_TYPE_XTERM_256COLOR:
        case LLE_TERM_TYPE_KONSOLE:
        case LLE_TERM_TYPE_GNOME_TERMINAL:
        case LLE_TERM_TYPE_ALACRITTY:
        case LLE_TERM_TYPE_KITTY:
        case LLE_TERM_TYPE_RXVT:
            caps->has_mouse = true;
            caps->has_bracketed_paste = true;
            caps->has_focus_events = true;
            break;
            
        case LLE_TERM_TYPE_SCREEN:
        case LLE_TERM_TYPE_TMUX:
            // Terminal multiplexers support these if underlying terminal does
            caps->has_mouse = true;
            caps->has_bracketed_paste = true;
            caps->has_focus_events = true;
            break;
            
        case LLE_TERM_TYPE_VT100:
            // VT100 doesn't support modern features
            caps->has_mouse = false;
            caps->has_bracketed_paste = false;
            caps->has_focus_events = false;
            break;
            
        default:
            // Conservative defaults for unknown terminals
            caps->has_mouse = false;
            caps->has_bracketed_paste = false;
            caps->has_focus_events = false;
            break;
    }
}

// Main capability detection function
lle_result_t lle_capabilities_detect_environment(lle_terminal_capabilities_t **caps_out,
                                                 const lle_unix_interface_t *unix_iface) {
    if (!caps_out || !unix_iface) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Allocate capabilities structure
    lle_terminal_capabilities_t *caps = malloc(sizeof(lle_terminal_capabilities_t));
    if (!caps) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize capabilities structure
    memset(caps, 0, sizeof(*caps));
    
    // Detect terminal type from environment
    caps->terminal_type = detect_term_type();
    
    // Store environment variables
    const char *term_env = getenv("TERM");
    const char *colorterm_env = getenv("COLORTERM");
    if (term_env) {
        strncpy(caps->term_env, term_env, sizeof(caps->term_env) - 1);
    }
    if (colorterm_env) {
        strncpy(caps->colorterm_env, colorterm_env, sizeof(caps->colorterm_env) - 1);
    }
    
    // Detect color support
    detect_color_support(caps);
    
    // Detect Unicode support
    caps->has_unicode = detect_unicode_support();
    
    // Detect text attributes
    detect_text_attributes(caps);
    
    // Detect interactive features
    detect_interactive_features(caps);
    
    // Optionally query terminal for device attributes (DA1)
    // This is the ONLY time we query terminal, with strict timeout
    // Note: unix_iface provides the file descriptor
    char response[256];
    int query_result = query_terminal_with_timeout(
        unix_iface->output_fd,
        "\x1b[c",  // Device Attributes query
        response,
        sizeof(response),
        100  // 100ms timeout
    );
    
    if (query_result > 0) {
        // Successfully got response - terminal is responsive
        caps->detection_successful = true;
        
        // Parse response for additional capabilities if needed
        // For now, we rely on environment-based detection
    } else {
        // Query timed out or failed - not critical, we have environment data
        caps->detection_successful = false;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    caps->detection_time_ms = 
        (uint32_t)((end.tv_sec - start.tv_sec) * 1000 +
                   (end.tv_nsec - start.tv_nsec) / 1000000);
    
    caps->detection_complete = true;
    
    // Set alternate screen support based on terminal type
    caps->has_alternate_screen = (caps->terminal_type != LLE_TERM_TYPE_VT100 &&
                                   caps->terminal_type != LLE_TERM_TYPE_UNKNOWN);
    
    *caps_out = caps;
    return LLE_SUCCESS;
}

// Cleanup capabilities structure
void lle_capabilities_destroy(lle_terminal_capabilities_t *caps) {
    if (caps) {
        free(caps);
    }
}
