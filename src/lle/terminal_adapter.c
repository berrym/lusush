/**
 * @file terminal_adapter.c
 * @brief LLE Terminal Adapter Implementation
 * 
 * Provides terminal capability detection and adaptation layer for multi-terminal
 * compatibility. Detects terminal type from environment and provides capability
 * queries for features like colors, UTF-8, cursor movement, etc.
 * 
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION PLAN: docs/lle_implementation/SPEC_08_IMPLEMENTATION_PLAN.md
 * 
 * ZERO-TOLERANCE COMPLIANCE:
 * - Complete implementations (no stubs)
 * - Full error handling
 * - 100% spec-compliant terminal detection
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* ========================================================================== */
/*                       TERMINAL DETECTION HELPERS                           */
/* ========================================================================== */

/**
 * @brief Detect terminal type from environment variables
 * 
 * Checks $TERM, $TERM_PROGRAM, and other environment variables to identify
 * the terminal emulator type.
 * 
 * @return Detected terminal type
 */
static lle_terminal_type_t detect_terminal_type(void) {
    const char *term = getenv("TERM");
    const char *term_program = getenv("TERM_PROGRAM");
    const char *tmux = getenv("TMUX");
    const char *kitty_window_id = getenv("KITTY_WINDOW_ID");
    
    /* Check for tmux first (multiplexer) */
    if (tmux && strlen(tmux) > 0) {
        return LLE_TERMINAL_TMUX;
    }
    
    /* Check for GNU Screen */
    if (term && strstr(term, "screen")) {
        return LLE_TERMINAL_SCREEN;
    }
    
    /* Check for Kitty */
    if (kitty_window_id || (term && strstr(term, "kitty"))) {
        return LLE_TERMINAL_KITTY;
    }
    
    /* Check for iTerm2 (macOS) */
    if (term_program && strstr(term_program, "iTerm")) {
        return LLE_TERMINAL_ITERM2;
    }
    
    /* Check for GNOME Terminal */
    if (term_program && strstr(term_program, "gnome-terminal")) {
        return LLE_TERMINAL_GNOME;
    }
    
    /* Check for Alacritty */
    if (term && strstr(term, "alacritty")) {
        return LLE_TERMINAL_ALACRITTY;
    }
    
    /* Check for xterm or xterm-compatible */
    if (term && (strstr(term, "xterm") || strstr(term, "256color"))) {
        return LLE_TERMINAL_XTERM;
    }
    
    /* Check for Linux console */
    if (term && strcmp(term, "linux") == 0) {
        return LLE_TERMINAL_CONSOLE;
    }
    
    /* Unknown or undetected terminal */
    return LLE_TERMINAL_UNKNOWN;
}

/**
 * @brief Detect terminal size from ioctl
 * 
 * Queries the terminal size using TIOCGWINSZ ioctl.
 * 
 * @param width Output for terminal width
 * @param height Output for terminal height
 * @return LLE_SUCCESS on success, error code on failure
 */
static lle_result_t detect_terminal_size(size_t *width, size_t *height) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *width = ws.ws_col;
        *height = ws.ws_row;
        return LLE_SUCCESS;
    }
    
    /* Fallback to default size if ioctl fails */
    *width = 80;
    *height = 24;
    return LLE_SUCCESS; /* Not an error, just use defaults */
}

/**
 * @brief Detect terminal capabilities based on terminal type
 * 
 * Determines what features are supported by the detected terminal type.
 * 
 * @param type Terminal type
 * @param caps Output capabilities structure
 * @return LLE_SUCCESS on success, error code on failure
 */
static lle_result_t detect_capabilities_for_type(lle_terminal_type_t type,
                                                 lle_terminal_capabilities_t *caps) {
    if (!caps) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Set terminal type */
    caps->terminal_type = type;
    
    /* Detect terminal size */
    detect_terminal_size(&caps->terminal_width, &caps->terminal_height);
    
    /* Set capabilities based on terminal type */
    switch (type) {
        case LLE_TERMINAL_KITTY:
            caps->supports_colors = true;
            caps->supports_256_colors = true;
            caps->supports_truecolor = true;
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_ALACRITTY:
            caps->supports_colors = true;
            caps->supports_256_colors = true;
            caps->supports_truecolor = true;
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_ITERM2:
            caps->supports_colors = true;
            caps->supports_256_colors = true;
            caps->supports_truecolor = true;
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_GNOME:
            caps->supports_colors = true;
            caps->supports_256_colors = true;
            caps->supports_truecolor = true;
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_XTERM:
            caps->supports_colors = true;
            caps->supports_256_colors = true;
            caps->supports_truecolor = false; /* Most xterms don't support truecolor */
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_TMUX:
            /* tmux capabilities depend on underlying terminal */
            caps->supports_colors = true;
            caps->supports_256_colors = true;
            caps->supports_truecolor = false; /* Conservative default */
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_SCREEN:
            /* GNU Screen capabilities */
            caps->supports_colors = true;
            caps->supports_256_colors = true;
            caps->supports_truecolor = false;
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_CONSOLE:
            /* Linux console - basic capabilities */
            caps->supports_colors = true;
            caps->supports_256_colors = false;
            caps->supports_truecolor = false;
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
            
        case LLE_TERMINAL_UNKNOWN:
        default:
            /* Conservative defaults for unknown terminals */
            caps->supports_colors = true;
            caps->supports_256_colors = false;
            caps->supports_truecolor = false;
            caps->supports_utf8 = true;
            caps->supports_cursor_movement = true;
            break;
    }
    
    return LLE_SUCCESS;
}

/* ========================================================================== */
/*                    COMPATIBILITY MATRIX INITIALIZATION                     */
/* ========================================================================== */

/**
 * @brief Initialize compatibility matrix with feature support data
 * 
 * Creates a matrix mapping terminal types to supported features.
 * 
 * @param matrix Compatibility matrix to initialize
 * @return LLE_SUCCESS on success, error code on failure
 */
static lle_result_t init_compatibility_matrix(lle_compatibility_matrix_t *matrix) {
    if (!matrix) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Zero out the matrix */
    memset(matrix, 0, sizeof(lle_compatibility_matrix_t));
    
    /* Define feature names */
    matrix->feature_names[0] = "colors";
    matrix->feature_names[1] = "256colors";
    matrix->feature_names[2] = "truecolor";
    matrix->feature_names[3] = "utf8";
    matrix->feature_names[4] = "cursor_movement";
    matrix->feature_names[5] = "mouse_support";
    matrix->feature_names[6] = "focus_events";
    matrix->feature_names[7] = "bracketed_paste";
    matrix->feature_names[8] = "alternate_screen";
    matrix->feature_names[9] = "title_setting";
    
    /* Feature indices */
    enum {
        FEAT_COLORS = 0,
        FEAT_256COLORS = 1,
        FEAT_TRUECOLOR = 2,
        FEAT_UTF8 = 3,
        FEAT_CURSOR = 4,
        FEAT_MOUSE = 5,
        FEAT_FOCUS = 6,
        FEAT_PASTE = 7,
        FEAT_ALTSCREEN = 8,
        FEAT_TITLE = 9
    };
    
    /* Kitty - modern terminal with all features */
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_256COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_TRUECOLOR] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_UTF8] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_CURSOR] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_MOUSE] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_FOCUS] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_PASTE] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_ALTSCREEN] = true;
    matrix->feature_support[LLE_TERMINAL_KITTY][FEAT_TITLE] = true;
    
    /* Alacritty - modern terminal */
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_256COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_TRUECOLOR] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_UTF8] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_CURSOR] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_MOUSE] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_FOCUS] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_PASTE] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_ALTSCREEN] = true;
    matrix->feature_support[LLE_TERMINAL_ALACRITTY][FEAT_TITLE] = true;
    
    /* GNOME Terminal */
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_256COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_TRUECOLOR] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_UTF8] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_CURSOR] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_MOUSE] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_FOCUS] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_PASTE] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_ALTSCREEN] = true;
    matrix->feature_support[LLE_TERMINAL_GNOME][FEAT_TITLE] = true;
    
    /* xterm - standard features */
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_256COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_TRUECOLOR] = false;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_UTF8] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_CURSOR] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_MOUSE] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_FOCUS] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_PASTE] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_ALTSCREEN] = true;
    matrix->feature_support[LLE_TERMINAL_XTERM][FEAT_TITLE] = true;
    
    /* Linux console - limited features */
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_COLORS] = true;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_256COLORS] = false;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_TRUECOLOR] = false;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_UTF8] = true;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_CURSOR] = true;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_MOUSE] = false;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_FOCUS] = false;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_PASTE] = false;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_ALTSCREEN] = false;
    matrix->feature_support[LLE_TERMINAL_CONSOLE][FEAT_TITLE] = false;
    
    return LLE_SUCCESS;
}

/* ========================================================================== */
/*                    TERMINAL ADAPTER PUBLIC API                             */
/* ========================================================================== */

/**
 * @brief Initialize terminal adapter
 * 
 * Creates and initializes a terminal adapter with capability detection.
 * Detects terminal type from environment and populates capability information.
 * 
 * @param adapter Output pointer to receive initialized adapter
 * @param display_controller Display controller reference
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * SPEC COMPLIANCE: Section 3.8 "Terminal Compatibility"
 * ERROR HANDLING: All allocation failures handled with proper cleanup
 */
lle_result_t lle_terminal_adapter_init(lle_terminal_adapter_t **adapter,
                                       display_controller_t *display_controller,
                                       lle_memory_pool_t *memory_pool) {
    lle_result_t result = LLE_SUCCESS;
    lle_terminal_adapter_t *ta = NULL;
    
    /* Step 1: Validate parameters */
    if (!adapter) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    if (!display_controller || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Step 2: Allocate terminal adapter */
    ta = lle_pool_alloc(sizeof(lle_terminal_adapter_t));
    if (!ta) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(ta, 0, sizeof(lle_terminal_adapter_t));
    
    /* Step 3: Store references */
    ta->display_controller = display_controller;
    ta->memory_pool = memory_pool;
    
    /* Step 4: Allocate and initialize capabilities */
    ta->capabilities = lle_pool_alloc(sizeof(lle_terminal_capabilities_t));
    if (!ta->capabilities) {
        lle_pool_free(ta);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(ta->capabilities, 0, sizeof(lle_terminal_capabilities_t));
    
    /* Step 5: Detect terminal type and capabilities */
    lle_terminal_type_t type = detect_terminal_type();
    result = detect_capabilities_for_type(type, ta->capabilities);
    if (result != LLE_SUCCESS) {
        lle_pool_free(ta->capabilities);
        lle_pool_free(ta);
        return result;
    }
    
    /* Step 6: Allocate and initialize compatibility matrix */
    ta->compat_matrix = lle_pool_alloc(sizeof(lle_compatibility_matrix_t));
    if (!ta->compat_matrix) {
        lle_pool_free(ta->capabilities);
        lle_pool_free(ta);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    result = init_compatibility_matrix(ta->compat_matrix);
    if (result != LLE_SUCCESS) {
        lle_pool_free(ta->compat_matrix);
        lle_pool_free(ta->capabilities);
        lle_pool_free(ta);
        return result;
    }
    
    /* Step 7: Return initialized adapter */
    *adapter = ta;
    return LLE_SUCCESS;
}

/**
 * @brief Clean up terminal adapter
 * 
 * Frees all resources associated with the terminal adapter.
 * 
 * @param adapter Terminal adapter to clean up
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * SPEC COMPLIANCE: Section 3.8 "Terminal Compatibility"
 */
lle_result_t lle_terminal_adapter_cleanup(lle_terminal_adapter_t *adapter) {
    if (!adapter) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free sub-structures */
    if (adapter->compat_matrix) {
        lle_pool_free(adapter->compat_matrix);
    }
    if (adapter->capabilities) {
        lle_pool_free(adapter->capabilities);
    }
    
    /* Free adapter itself */
    lle_pool_free(adapter);
    
    return LLE_SUCCESS;
}

/**
 * @brief Get terminal capabilities
 * 
 * Returns the detected capabilities for the current terminal.
 * 
 * @param adapter Terminal adapter
 * @return Terminal capabilities, or NULL if adapter is NULL
 */
const lle_terminal_capabilities_t* lle_terminal_adapter_get_capabilities(
    const lle_terminal_adapter_t *adapter) {
    if (!adapter) {
        return NULL;
    }
    return adapter->capabilities;
}

/**
 * @brief Check if terminal supports a specific feature
 * 
 * Queries the compatibility matrix to determine if the current terminal
 * supports a specific feature.
 * 
 * @param adapter Terminal adapter
 * @param feature_name Name of feature to check (e.g., "truecolor")
 * @return true if supported, false otherwise
 */
bool lle_terminal_adapter_supports_feature(const lle_terminal_adapter_t *adapter,
                                           const char *feature_name) {
    if (!adapter || !adapter->capabilities || !adapter->compat_matrix || !feature_name) {
        return false;
    }
    
    lle_terminal_type_t type = adapter->capabilities->terminal_type;
    
    /* Search for feature in matrix */
    for (int i = 0; i < 16; i++) {
        if (adapter->compat_matrix->feature_names[i] == NULL) {
            break;
        }
        if (strcmp(adapter->compat_matrix->feature_names[i], feature_name) == 0) {
            return adapter->compat_matrix->feature_support[type][i];
        }
    }
    
    return false;
}
