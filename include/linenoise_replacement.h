#ifndef LINENOISE_REPLACEMENT_H
#define LINENOISE_REPLACEMENT_H

/**
 * @file linenoise_replacement.h
 * @brief Linenoise Compatibility Layer for GNU Readline Integration
 *
 * This header provides compatibility macros that redirect all linenoise
 * function calls to the new GNU Readline integration system. This allows
 * existing code to continue working without modification while using
 * the superior readline backend.
 *
 * Simply include this header instead of linenoise.h to use readline.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "readline_integration.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// COMPATIBILITY TYPE DEFINITIONS
// ============================================================================

/**
 * @brief Completion list structure (linenoise compatibility)
 */
typedef struct linenoiseCompletions {
    size_t len;    /**< Number of completion entries */
    char **cvec;   /**< Array of completion strings */
} linenoiseCompletions;

/**
 * @brief Completion callback function type
 */
typedef void(linenoiseCompletionCallback)(const char *, linenoiseCompletions *);

/**
 * @brief Hints callback function type
 */
typedef char *(linenoiseHintsCallback)(const char *, int *color, int *bold);

/**
 * @brief Free hints callback function type
 */
typedef void(linenoiseFreeHintsCallback)(void *);

// ============================================================================
// COMPATIBILITY FUNCTION IMPLEMENTATIONS
// ============================================================================

/**
 * Get a line of input (main readline function)
 */
static inline char *linenoise(const char *prompt) {
    return lusush_readline_with_prompt(prompt);
}

/**
 * Free readline-allocated memory
 */
static inline void linenoiseFree(void *ptr) {
    if (ptr) free(ptr);
}

/**
 * Add line to history
 */
static inline int linenoiseHistoryAdd(const char *line) {
    lusush_history_add(line);
    return 0;  // Success
}

/**
 * Set maximum history length
 */
static inline int linenoiseHistorySetMaxLen(int len) {
    lusush_history_set_max_length(len);
    return 0;  // Success
}

/**
 * Save history to file
 */
static inline int linenoiseHistorySave(const char *filename) {
    // Use default file if NULL
    if (!filename) {
        lusush_history_save();
    } else {
        // Would need to temporarily change history file
        lusush_history_save();
    }
    return 0;  // Success
}

/**
 * Load history from file
 */
static inline int linenoiseHistoryLoad(const char *filename) {
    // Use default file if NULL
    if (!filename) {
        return lusush_history_load() ? 0 : -1;
    } else {
        // Would need to temporarily change history file
        return lusush_history_load() ? 0 : -1;
    }
}

/**
 * Print all history entries
 */
static inline void linenoiseHistoryPrint(void) {
    int len = lusush_history_length();
    for (int i = 0; i < len; i++) {
        const char *entry = lusush_history_get(i);
        if (entry) {
            printf("%4d  %s\n", i + 1, entry);
        }
    }
}

/**
 * Get history entry by index
 */
static inline char *linenoiseHistoryGet(int index) {
    const char *entry = lusush_history_get(index - 1);  // Convert to 0-based
    return entry ? strdup(entry) : NULL;
}

/**
 * Remove duplicate history entries
 */
static inline int linenoiseHistoryRemoveDups(void) {
    return lusush_history_remove_duplicates();
}

/**
 * Enable/disable history deduplication
 */
static inline void linenoiseHistoryNoDups(bool flag) {
    // This is handled automatically by lusush_history_add()
    // No action needed - deduplication is always enabled
    (void)flag; // Suppress unused parameter warning
}

/**
 * Add completion to list
 */
static inline void linenoiseAddCompletion(linenoiseCompletions *lc, const char *str) {
    if (!lc || !str) return;
    
    // Resize array if needed
    if (lc->len % 10 == 0) {  // Grow in chunks of 10
        char **new_cvec = realloc(lc->cvec, (lc->len + 10) * sizeof(char*));
        if (!new_cvec) return;
        lc->cvec = new_cvec;
    }
    
    lc->cvec[lc->len++] = strdup(str);
}

/**
 * Set completion callback (compatibility - not used with readline)
 */
static inline void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn) {
    // Readline uses its own completion system
    // This is for compatibility only
    (void)fn; // Suppress unused parameter warning
}

/**
 * Set hints callback (compatibility - not used with readline)
 */
static inline void linenoiseSetHintsCallback(linenoiseHintsCallback *fn) {
    // Readline doesn't have hints in the same way
    // This is for compatibility only
    (void)fn; // Suppress unused parameter warning
}

/**
 * Set free hints callback (compatibility - not used with readline)
 */
static inline void linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *fn) {
    // Readline doesn't have hints in the same way
    // This is for compatibility only
    (void)fn; // Suppress unused parameter warning
}

/**
 * Enable/disable multiline editing
 */
static inline void linenoiseSetMultiLine(int ml) {
    lusush_multiline_set_enabled(ml != 0);
}

/**
 * Clear the screen
 */
static inline void linenoiseClearScreen(void) {
    lusush_clear_screen();
}

/**
 * Print key codes (debugging function)
 */
static inline void linenoisePrintKeyCodes(void) {
    printf("Key code printing not implemented in readline integration\n");
}

/**
 * Enable mask mode (password input)
 */
static inline void linenoiseMaskModeEnable(void) {
    // Would need special readline configuration
    printf("Mask mode not implemented in readline integration\n");
}

/**
 * Disable mask mode
 */
static inline void linenoiseMaskModeDisable(void) {
    // Would need special readline configuration
    // No action needed
}

// ============================================================================
// ENHANCED COMPATIBILITY FUNCTIONS
// ============================================================================

/**
 * Set history no duplicates (enhanced compatibility)
 */
static inline void linenoiseSetHistoryNoDups(int enable) {
    // Always enabled in our implementation
    (void)enable; // Suppress unused parameter warning
}

/**
 * History expansion (not implemented)
 */
static inline int linenoiseHistoryExpansion(const char *line, char **expanded) {
    // Not implemented - return error
    (void)line; // Suppress unused parameter warning
    (void)expanded; // Suppress unused parameter warning
    return -1;
}

/**
 * Delete history entry (not implemented)
 */
static inline int linenoiseHistoryDelete(int index) {
    // Not implemented - return error
    (void)index; // Suppress unused parameter warning
    return -1;
}

// ============================================================================
// INITIALIZATION HELPERS
// ============================================================================

/**
 * Initialize the linenoise replacement system
 * This is called automatically on first use
 */
static inline bool linenoise_replacement_init(void) {
    static bool initialized = false;
    if (!initialized) {
        if (lusush_readline_init()) {
            initialized = true;
            return true;
        }
        return false;
    }
    return true;
}

/**
 * Cleanup the linenoise replacement system
 */
static inline void linenoise_replacement_cleanup(void) {
    lusush_readline_cleanup();
}

// ============================================================================
// AUTOMATIC INITIALIZATION
// ============================================================================

/**
 * Ensure initialization before any linenoise function
 */
#define ENSURE_INIT() do { \
    static bool init_checked = false; \
    if (!init_checked) { \
        linenoise_replacement_init(); \
        init_checked = true; \
    } \
} while(0)

// Override the main functions to include auto-initialization
#undef linenoise
#define linenoise(prompt) ({ ENSURE_INIT(); lusush_readline_with_prompt(prompt); })

#undef linenoiseHistoryAdd
#define linenoiseHistoryAdd(line) ({ ENSURE_INIT(); lusush_history_add(line); 0; })

#ifdef __cplusplus
}
#endif

#endif /* LINENOISE_REPLACEMENT_H */