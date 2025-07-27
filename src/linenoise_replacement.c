#include "../include/linenoise_replacement.h"
#include "line_editor/lle_integration.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file linenoise_replacement.c
 * @brief Linenoise Replacement Implementation Using LLE Integration
 *
 * This module implements the linenoise replacement functions that provide
 * a seamless transition from linenoise to LLE. All functions maintain
 * linenoise API compatibility while internally using LLE's advanced features.
 *
 * The implementation uses macro-based function replacement to avoid type
 * conflicts and provides enhanced functionality through LLE.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

// ============================================================================
// Global State and Statistics
// ============================================================================

/**
 * @brief Replacement layer statistics and configuration
 */
static struct {
    bool initialized;
    bool debug_mode;
    size_t readline_calls;
    size_t history_operations;
    size_t completion_calls;
    char last_error[256];
    
    // Callback storage for compatibility
    linenoiseCompletionCallback *completion_callback;
    linenoiseHintsCallback *hints_callback;
    linenoiseFreeHintsCallback *free_hints_callback;
} replacement_state = {0};

/**
 * @brief Debug logging macro
 */
#define REPLACEMENT_DEBUG(fmt, ...) do { \
    if (replacement_state.debug_mode) { \
        fprintf(stderr, "[LLE_REPLACEMENT] " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

/**
 * @brief Set error message for debugging
 *
 * @param error Error message to store
 */
static void lle_replacement_set_error(const char *error) {
    if (error && strlen(error) < sizeof(replacement_state.last_error)) {
        strncpy(replacement_state.last_error, error, 
                sizeof(replacement_state.last_error) - 1);
        replacement_state.last_error[sizeof(replacement_state.last_error) - 1] = '\0';
    }
}

/**
 * @brief Clear error message
 */
static void lle_replacement_clear_error(void) {
    replacement_state.last_error[0] = '\0';
}

// ============================================================================
// Initialization and Cleanup
// ============================================================================

bool lle_replacement_init(void) {
    if (replacement_state.initialized) {
        return true;
    }
    
    // Check for debug mode from environment variable
    const char *debug_env = getenv("LLE_REPLACEMENT_DEBUG");
    if (debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0)) {
        replacement_state.debug_mode = true;
    }
    
    REPLACEMENT_DEBUG("Initializing linenoise replacement layer");
    
    // Initialize LLE integration
    if (!lle_integration_init()) {
        lle_replacement_set_error("Failed to initialize LLE integration");
        return false;
    }
    
    // Clear statistics
    replacement_state.readline_calls = 0;
    replacement_state.history_operations = 0;
    replacement_state.completion_calls = 0;
    
    // Clear callbacks
    replacement_state.completion_callback = NULL;
    replacement_state.hints_callback = NULL;
    replacement_state.free_hints_callback = NULL;
    
    replacement_state.initialized = true;
    REPLACEMENT_DEBUG("Linenoise replacement layer initialized successfully");
    
    return true;
}

void lle_replacement_shutdown(void) {
    if (!replacement_state.initialized) {
        return;
    }
    
    REPLACEMENT_DEBUG("Shutting down linenoise replacement layer");
    
    // Shutdown LLE integration
    lle_integration_shutdown();
    
    // Clear state
    memset(&replacement_state, 0, sizeof(replacement_state));
    
    REPLACEMENT_DEBUG("Linenoise replacement layer shutdown complete");
}

// ============================================================================
// Core Readline Functions
// ============================================================================

char *lle_replacement_readline(const char *prompt) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return NULL;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.readline_calls++;
    
    REPLACEMENT_DEBUG("readline called with prompt: '%s'", prompt ? prompt : "(null)");
    
    char *result = lle_integration_readline(prompt);
    
    if (result) {
        REPLACEMENT_DEBUG("readline returned %zu characters", strlen(result));
    } else {
        REPLACEMENT_DEBUG("readline returned NULL (EOF or error)");
    }
    
    return result;
}

void lle_replacement_free(void *ptr) {
    if (ptr) {
        REPLACEMENT_DEBUG("freeing pointer: %p", ptr);
        lle_integration_free(ptr);
    }
}

// ============================================================================
// History Functions
// ============================================================================

int lle_replacement_history_add(const char *line) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return -1;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_add called with line: '%s'", line ? line : "(null)");
    
    return lle_integration_history_add(line);
}

int lle_replacement_history_set_max_len(int len) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return 0;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_set_max_len called with len: %d", len);
    
    return lle_integration_history_set_max_len(len);
}

int lle_replacement_history_save(const char *filename) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return -1;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_save called with filename: '%s'", filename ? filename : "(null)");
    
    return lle_integration_history_save(filename);
}

int lle_replacement_history_load(const char *filename) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return -1;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_load called with filename: '%s'", filename ? filename : "(null)");
    
    return lle_integration_history_load(filename);
}

void lle_replacement_history_print(void) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_print called");
    
    lle_integration_history_print();
}

char *lle_replacement_history_get(int index) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return NULL;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_get called with index: %d", index);
    
    return lle_integration_history_get(index);
}

int lle_replacement_history_remove_dups(void) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return 0;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_remove_dups called");
    
    return lle_integration_history_remove_dups();
}

void lle_replacement_history_no_dups(bool flag) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return;
        }
    }
    
    lle_replacement_clear_error();
    replacement_state.history_operations++;
    
    REPLACEMENT_DEBUG("history_no_dups called with flag: %s", flag ? "true" : "false");
    
    lle_integration_history_no_dups(flag);
}

// ============================================================================
// Completion Functions
// ============================================================================

void lle_replacement_add_completion(linenoiseCompletions *lc, const char *str) {
    if (!lc || !str) {
        return;
    }
    
    replacement_state.completion_calls++;
    
    REPLACEMENT_DEBUG("add_completion called: '%s' (current count: %zu)", str, lc->len);
    
    // Resize the array if needed
    char **new_cvec = realloc(lc->cvec, (lc->len + 1) * sizeof(char*));
    if (!new_cvec) {
        lle_replacement_set_error("Failed to allocate memory for completion");
        return;
    }
    
    lc->cvec = new_cvec;
    
    // Copy the string
    lc->cvec[lc->len] = strdup(str);
    if (!lc->cvec[lc->len]) {
        lle_replacement_set_error("Failed to duplicate completion string");
        return;
    }
    
    lc->len++;
    
    REPLACEMENT_DEBUG("completion added, new count: %zu", lc->len);
}

void lle_replacement_set_completion_callback(linenoiseCompletionCallback *fn) {
    REPLACEMENT_DEBUG("set_completion_callback called: %p", (void*)fn);
    replacement_state.completion_callback = fn;
    
    // TODO: Register with LLE's completion system
    // This would require LLE to support external completion callbacks
}

void lle_replacement_set_hints_callback(linenoiseHintsCallback *fn) {
    REPLACEMENT_DEBUG("set_hints_callback called: %p", (void*)fn);
    replacement_state.hints_callback = fn;
    
    // TODO: Register with LLE's hints system
    // This would require LLE to support external hints callbacks
}

void lle_replacement_set_free_hints_callback(linenoiseFreeHintsCallback *fn) {
    REPLACEMENT_DEBUG("set_free_hints_callback called: %p", (void*)fn);
    replacement_state.free_hints_callback = fn;
}

// ============================================================================
// Configuration Functions
// ============================================================================

void lle_replacement_set_multiline(int ml) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return;
        }
    }
    
    REPLACEMENT_DEBUG("set_multiline called with ml: %d", ml);
    
    lle_integration_set_multiline_mode(ml != 0);
}

void lle_replacement_clear_screen(void) {
    REPLACEMENT_DEBUG("clear_screen called");
    
    // Clear screen using standard escape sequence
    printf("\033[H\033[2J");
    fflush(stdout);
}

void lle_replacement_print_key_codes(void) {
    REPLACEMENT_DEBUG("print_key_codes called");
    
    printf("Linenoise key codes mode not implemented in LLE replacement\n");
    printf("Use LLE's native debugging features instead\n");
}

void lle_replacement_mask_mode_enable(void) {
    REPLACEMENT_DEBUG("mask_mode_enable called");
    
    // LLE doesn't have a direct mask mode equivalent
    // This could be implemented as a feature request
    printf("Mask mode not implemented in LLE replacement\n");
}

void lle_replacement_mask_mode_disable(void) {
    REPLACEMENT_DEBUG("mask_mode_disable called");
    
    // LLE doesn't have a direct mask mode equivalent
    printf("Mask mode not implemented in LLE replacement\n");
}

// ============================================================================
// Enhanced LLE Features
// ============================================================================

bool lle_replacement_enable_syntax_highlighting(bool enable) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return false;
        }
    }
    
    REPLACEMENT_DEBUG("enable_syntax_highlighting called: %s", enable ? "true" : "false");
    
    return lle_integration_set_syntax_highlighting(enable);
}

bool lle_replacement_enable_unicode_support(bool enable) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return false;
        }
    }
    
    REPLACEMENT_DEBUG("enable_unicode_support called: %s", enable ? "true" : "false");
    
    // LLE has Unicode support enabled by default
    // This function is provided for API completeness
    return true;
}

bool lle_replacement_enable_undo_redo(bool enable) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return false;
        }
    }
    
    REPLACEMENT_DEBUG("enable_undo_redo called: %s", enable ? "true" : "false");
    
    return lle_integration_set_undo_enabled(enable);
}

bool lle_replacement_get_completion_stats(size_t *total_calls, size_t *successful_completions) {
    if (!total_calls || !successful_completions) {
        return false;
    }
    
    *total_calls = replacement_state.completion_calls;
    *successful_completions = replacement_state.completion_calls; // Simplified
    
    return true;
}

bool lle_replacement_configure_history_dedup(bool move_to_end, bool case_sensitive) {
    // Auto-initialize if needed
    if (!replacement_state.initialized) {
        if (!lle_replacement_init()) {
            return false;
        }
    }
    
    REPLACEMENT_DEBUG("configure_history_dedup called: move_to_end=%s, case_sensitive=%s",
                     move_to_end ? "true" : "false", case_sensitive ? "true" : "false");
    
    // Enable LLE's hist_no_dups feature with move-to-end behavior
    lle_integration_history_no_dups(move_to_end);
    
    // Case sensitivity would need to be a separate LLE feature
    return true;
}

// ============================================================================
// Debugging and Diagnostics
// ============================================================================

bool lle_replacement_get_statistics(size_t *readline_calls, 
                                   size_t *history_operations,
                                   size_t *completion_calls) {
    if (!readline_calls || !history_operations || !completion_calls) {
        return false;
    }
    
    *readline_calls = replacement_state.readline_calls;
    *history_operations = replacement_state.history_operations;
    *completion_calls = replacement_state.completion_calls;
    
    return true;
}

void lle_replacement_set_debug(bool enable) {
    replacement_state.debug_mode = enable;
    REPLACEMENT_DEBUG("Debug mode %s", enable ? "enabled" : "disabled");
}

const char *lle_replacement_get_last_error(void) {
    if (replacement_state.last_error[0] != '\0') {
        return replacement_state.last_error;
    }
    
    // Also check LLE integration for errors
    const char *lle_error = lle_integration_get_last_error();
    if (lle_error) {
        return lle_error;
    }
    
    return NULL;
}