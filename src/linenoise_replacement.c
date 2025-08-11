/*
 * Lusush Shell - Linenoise Replacement Implementation
 * 
 * This file provides a minimal compatibility layer that redirects all
 * linenoise function calls to the new GNU Readline integration system.
 * This allows existing code to continue working without modification.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../include/linenoise_replacement.h"
#include "../include/readline_integration.h"

// ============================================================================
// GLOBAL STATE
// ============================================================================

static bool replacement_initialized = false;

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

/**
 * Initialize the linenoise replacement system
 * This ensures readline is properly set up
 */
bool linenoise_replacement_init(void) {
    if (replacement_initialized) {
        return true;
    }
    
    if (!lusush_readline_init()) {
        fprintf(stderr, "Failed to initialize readline integration\n");
        return false;
    }
    
    replacement_initialized = true;
    return true;
}

/**
 * Cleanup the linenoise replacement system
 */
void linenoise_replacement_cleanup(void) {
    if (!replacement_initialized) {
        return;
    }
    
    lusush_readline_cleanup();
    replacement_initialized = false;
}

// ============================================================================
// COMPATIBILITY FUNCTIONS (if needed for non-inline implementations)
// ============================================================================

/**
 * Ensure the system is initialized before any operation
 */
static inline void ensure_initialized(void) {
    if (!replacement_initialized) {
        linenoise_replacement_init();
    }
}

/**
 * Main readline function with auto-initialization
 */
char *linenoise_impl(const char *prompt) {
    ensure_initialized();
    return lusush_readline();
}

/**
 * Add to history with auto-initialization
 */
int linenoiseHistoryAdd_impl(const char *line) {
    ensure_initialized();
    lusush_history_add(line);
    return 0;  // Success
}

/**
 * Set history max length with auto-initialization
 */
int linenoiseHistorySetMaxLen_impl(int len) {
    ensure_initialized();
    lusush_history_set_max_length(len);
    return 0;  // Success
}

/**
 * Save history with auto-initialization
 */
int linenoiseHistorySave_impl(const char *filename) {
    ensure_initialized();
    if (filename) {
        // For custom filename, would need to temporarily change path
        // For now, just use the default
        lusush_history_save();
    } else {
        lusush_history_save();
    }
    return 0;  // Success
}

/**
 * Load history with auto-initialization
 */
int linenoiseHistoryLoad_impl(const char *filename) {
    ensure_initialized();
    if (filename) {
        // For custom filename, would need to temporarily change path
        // For now, just use the default
        return lusush_history_load() ? 0 : -1;
    } else {
        return lusush_history_load() ? 0 : -1;
    }
}

/**
 * Print history with auto-initialization
 */
void linenoiseHistoryPrint_impl(void) {
    ensure_initialized();
    int len = lusush_history_length();
    for (int i = 0; i < len; i++) {
        const char *entry = lusush_history_get(i);
        if (entry) {
            printf("%4d  %s\n", i + 1, entry);
        }
    }
}

/**
 * Get history entry with auto-initialization
 */
char *linenoiseHistoryGet_impl(int index) {
    ensure_initialized();
    const char *entry = lusush_history_get(index - 1);  // Convert to 0-based
    return entry ? strdup(entry) : NULL;
}

/**
 * Remove duplicate history entries
 */
int linenoiseHistoryRemoveDups_impl(void) {
    ensure_initialized();
    return lusush_history_remove_duplicates();
}

/**
 * Clear screen with auto-initialization
 */
void linenoiseClearScreen_impl(void) {
    ensure_initialized();
    lusush_clear_screen();
}

/**
 * Set multiline mode
 */
void linenoiseSetMultiLine_impl(int ml) {
    ensure_initialized();
    lusush_multiline_set_enabled(ml != 0);
}

// ============================================================================
// DEBUGGING AND UTILITIES
// ============================================================================

/**
 * Check if replacement system is available
 */
bool linenoise_replacement_available(void) {
    return lusush_readline_available();
}

/**
 * Get version information
 */
const char *linenoise_replacement_version(void) {
    return lusush_readline_version();
}

/**
 * Enable debug mode
 */
void linenoise_replacement_set_debug(bool enabled) {
    lusush_readline_set_debug(enabled);
}