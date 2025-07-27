#include "../include/linenoise_compat.h"
#include "line_editor/lle_integration.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file linenoise_compat.c
 * @brief Linenoise Compatibility Layer Implementation
 *
 * This module implements the compatibility layer for existing linenoise
 * completion and callback code. It provides seamless integration between
 * the old linenoise API and the new LLE system.
 *
 * The implementation maintains full API compatibility while internally
 * routing calls through LLE's advanced completion and suggestion systems.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

// ============================================================================
// Global State Management
// ============================================================================

/**
 * @brief Compatibility layer state
 */
static struct {
    bool initialized;
    bool debug_mode;
    size_t completion_calls;
    size_t hint_calls;
    size_t memory_used;
    linenoiseCompletionCallback *completion_callback;
    linenoiseHintsCallback *hints_callback;
    linenoiseFreeHintsCallback *free_hints_callback;
} compat_state = {0};

/**
 * @brief Debug logging macro
 */
#define COMPAT_DEBUG(fmt, ...) do { \
    if (compat_state.debug_mode) { \
        fprintf(stderr, "[COMPAT] " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

// ============================================================================
// Memory Management
// ============================================================================

/**
 * @brief Track allocated memory for statistics
 *
 * @param size Number of bytes allocated
 */
static void compat_track_allocation(size_t size) {
    compat_state.memory_used += size;
}

/**
 * @brief Track freed memory for statistics
 *
 * @param size Number of bytes freed
 */
static void compat_track_deallocation(size_t size) {
    if (compat_state.memory_used >= size) {
        compat_state.memory_used -= size;
    } else {
        compat_state.memory_used = 0;
    }
}

// ============================================================================
// Completion List Management
// ============================================================================

linenoiseCompletions *linenoise_compat_create_completions(void) {
    linenoiseCompletions *lc = malloc(sizeof(linenoiseCompletions));
    if (!lc) {
        return NULL;
    }

    lc->len = 0;
    lc->cvec = NULL;

    compat_track_allocation(sizeof(linenoiseCompletions));
    COMPAT_DEBUG("Created completion list at %p", (void*)lc);

    return lc;
}

void linenoise_compat_free_completions(linenoiseCompletions *lc) {
    if (!lc) {
        return;
    }

    COMPAT_DEBUG("Freeing completion list at %p with %zu entries", (void*)lc, lc->len);

    // Free all completion strings
    for (size_t i = 0; i < lc->len; i++) {
        if (lc->cvec[i]) {
            size_t str_len = strlen(lc->cvec[i]) + 1;
            free(lc->cvec[i]);
            compat_track_deallocation(str_len);
        }
    }

    // Free the array
    if (lc->cvec) {
        compat_track_deallocation(lc->len * sizeof(char*));
        free(lc->cvec);
    }

    // Free the structure
    compat_track_deallocation(sizeof(linenoiseCompletions));
    free(lc);
}

// ============================================================================
// Linenoise API Compatibility Functions
// ============================================================================

void linenoiseAddCompletion(linenoiseCompletions *lc, const char *str) {
    if (!lc || !str) {
        return;
    }

    COMPAT_DEBUG("Adding completion: '%s' (current count: %zu)", str, lc->len);

    // Resize the array if needed
    char **new_cvec = realloc(lc->cvec, (lc->len + 1) * sizeof(char*));
    if (!new_cvec) {
        return;
    }

    lc->cvec = new_cvec;
    compat_track_allocation(sizeof(char*));

    // Copy the string
    lc->cvec[lc->len] = strdup(str);
    if (!lc->cvec[lc->len]) {
        return;
    }

    compat_track_allocation(strlen(str) + 1);
    lc->len++;

    COMPAT_DEBUG("Completion added, new count: %zu", lc->len);
}

void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn) {
    COMPAT_DEBUG("Setting completion callback: %p", (void*)fn);
    compat_state.completion_callback = fn;
}

void linenoiseSetHintsCallback(linenoiseHintsCallback *fn) {
    COMPAT_DEBUG("Setting hints callback: %p", (void*)fn);
    compat_state.hints_callback = fn;
}

void linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *fn) {
    COMPAT_DEBUG("Setting free hints callback: %p", (void*)fn);
    compat_state.free_hints_callback = fn;
}

// ============================================================================
// LLE Integration Callbacks
// ============================================================================

/**
 * @brief Wrapper callback for LLE completion integration
 *
 * This function is called by LLE when completion is requested. It creates
 * a linenoise-compatible completion list, calls the registered linenoise
 * completion callback, and then converts the results to LLE format.
 *
 * @param text Current input text being completed
 * @param cursor_pos Current cursor position
 * @param completions LLE completion list to populate
 * @return true on success, false on failure
 */
static bool lle_completion_wrapper(const char *text, size_t cursor_pos, void *completions) {
    if (!compat_state.completion_callback || !text) {
        return false;
    }

    COMPAT_DEBUG("LLE completion wrapper called for: '%s' at pos %zu", text, cursor_pos);
    compat_state.completion_calls++;

    // Create linenoise-compatible completion list
    linenoiseCompletions *lc = linenoise_compat_create_completions();
    if (!lc) {
        return false;
    }

    // Call the linenoise completion callback
    compat_state.completion_callback(text, lc);

    COMPAT_DEBUG("Linenoise callback returned %zu completions", lc->len);

    // Convert completions to LLE format
    // Note: This would need to integrate with LLE's actual completion system
    // For now, this is a placeholder that demonstrates the pattern
    bool success = linenoise_compat_convert_completions(lc);

    // Clean up
    linenoise_compat_free_completions(lc);

    return success;
}

/**
 * @brief Wrapper callback for LLE hints integration
 *
 * This function is called by LLE when hints are requested. It calls the
 * registered linenoise hints callback and returns the result in LLE format.
 *
 * @param text Current input text
 * @param hint_text Pointer to store hint text
 * @param color Pointer to store hint color
 * @param bold Pointer to store hint bold flag
 * @return true on success, false on failure
 */
static bool lle_hints_wrapper(const char *text, char **hint_text, int *color, int *bold) {
    if (!compat_state.hints_callback || !text) {
        return false;
    }

    COMPAT_DEBUG("LLE hints wrapper called for: '%s'", text);
    compat_state.hint_calls++;

    // Call the linenoise hints callback
    char *hint = compat_state.hints_callback(text, color, bold);
    
    if (hint) {
        *hint_text = strdup(hint);
        compat_track_allocation(strlen(hint) + 1);
        
        // Free the original hint if a free callback is registered
        if (compat_state.free_hints_callback) {
            compat_state.free_hints_callback(hint);
        } else {
            free(hint);
        }
        
        COMPAT_DEBUG("Hint generated: '%s' (color: %d, bold: %d)", *hint_text, *color, *bold);
        return true;
    }

    return false;
}

// ============================================================================
// Compatibility Support Functions
// ============================================================================

bool linenoise_compat_init(void) {
    if (compat_state.initialized) {
        return true;
    }

    COMPAT_DEBUG("Initializing linenoise compatibility layer");

    // Initialize LLE integration if not already done
    if (!lle_integration_init()) {
        COMPAT_DEBUG("Failed to initialize LLE integration");
        return false;
    }

    // Register our wrapper callbacks with LLE
    // Note: This would need actual LLE API functions to register callbacks
    // For now, this demonstrates the intended integration pattern

    // Clear statistics
    compat_state.completion_calls = 0;
    compat_state.hint_calls = 0;
    compat_state.memory_used = 0;

    compat_state.initialized = true;
    COMPAT_DEBUG("Linenoise compatibility layer initialized successfully");

    return true;
}

void linenoise_compat_shutdown(void) {
    if (!compat_state.initialized) {
        return;
    }

    COMPAT_DEBUG("Shutting down linenoise compatibility layer");

    // Clear callbacks
    compat_state.completion_callback = NULL;
    compat_state.hints_callback = NULL;
    compat_state.free_hints_callback = NULL;

    // Note: Any memory leaks at this point should be logged
    if (compat_state.memory_used > 0) {
        COMPAT_DEBUG("Warning: %zu bytes still allocated at shutdown", compat_state.memory_used);
    }

    compat_state.initialized = false;
    COMPAT_DEBUG("Linenoise compatibility layer shutdown complete");
}

bool linenoise_compat_convert_completions(const linenoiseCompletions *lc) {
    if (!lc) {
        return false;
    }

    COMPAT_DEBUG("Converting %zu linenoise completions to LLE format", lc->len);

    // This function would integrate with LLE's completion system
    // to convert linenoise completion entries to LLE's internal format
    // 
    // For now, this is a placeholder that demonstrates the conversion pattern
    
    for (size_t i = 0; i < lc->len; i++) {
        if (lc->cvec[i]) {
            COMPAT_DEBUG("  [%zu]: '%s'", i, lc->cvec[i]);
            // Here we would call LLE's completion addition function
            // something like: lle_add_completion_item(lc->cvec[i], NULL, 0);
        }
    }

    return true;
}

// ============================================================================
// Debugging and Diagnostics
// ============================================================================

bool linenoise_compat_get_stats(size_t *completion_calls, size_t *hint_calls, 
                                size_t *memory_used) {
    if (!completion_calls || !hint_calls || !memory_used) {
        return false;
    }

    *completion_calls = compat_state.completion_calls;
    *hint_calls = compat_state.hint_calls;
    *memory_used = compat_state.memory_used;

    return true;
}

void linenoise_compat_set_debug(bool enable) {
    compat_state.debug_mode = enable;
    COMPAT_DEBUG("Debug mode %s", enable ? "enabled" : "disabled");
}