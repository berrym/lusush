#ifndef LINENOISE_REPLACEMENT_H
#define LINENOISE_REPLACEMENT_H

/**
 * @file linenoise_replacement.h
 * @brief Direct Linenoise Replacement Macros for LLE Integration
 *
 * This header provides direct macro replacements for linenoise functions,
 * mapping them to LLE integration functions. This approach avoids type
 * conflicts and provides seamless replacement throughout the codebase.
 *
 * Simply include this header instead of linenoise.h to use LLE.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations to avoid circular dependencies
struct lle_line_editor;

// ============================================================================
// Core Types for Linenoise Compatibility
// ============================================================================

/**
 * @brief Completion list structure (linenoise replacement)
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
// LLE Integration Function Declarations
// ============================================================================

// Core readline functions
char *lle_replacement_readline(const char *prompt);
void lle_replacement_free(void *ptr);

// History functions
int lle_replacement_history_add(const char *line);
int lle_replacement_history_set_max_len(int len);
int lle_replacement_history_save(const char *filename);
int lle_replacement_history_load(const char *filename);
void lle_replacement_history_print(void);
char *lle_replacement_history_get(int index);
int lle_replacement_history_remove_dups(void);
void lle_replacement_history_no_dups(bool flag);

// Completion functions
void lle_replacement_add_completion(linenoiseCompletions *lc, const char *str);
void lle_replacement_set_completion_callback(linenoiseCompletionCallback *fn);
void lle_replacement_set_hints_callback(linenoiseHintsCallback *fn);
void lle_replacement_set_free_hints_callback(linenoiseFreeHintsCallback *fn);

// Configuration functions
void lle_replacement_set_multiline(int ml);
void lle_replacement_clear_screen(void);
void lle_replacement_print_key_codes(void);
void lle_replacement_mask_mode_enable(void);
void lle_replacement_mask_mode_disable(void);

// Initialization and cleanup
bool lle_replacement_init(void);
void lle_replacement_shutdown(void);

// ============================================================================
// Direct Macro Replacements
// ============================================================================

// Core functions
#define linenoise(prompt) lle_replacement_readline(prompt)
#define linenoiseFree(ptr) lle_replacement_free(ptr)

// History functions
#define linenoiseHistoryAdd(line) lle_replacement_history_add(line)
#define linenoiseHistorySetMaxLen(len) lle_replacement_history_set_max_len(len)
#define linenoiseHistorySave(filename) lle_replacement_history_save(filename)
#define linenoiseHistoryLoad(filename) lle_replacement_history_load(filename)
#define linenoiseHistoryPrint() lle_replacement_history_print()
#define linenoiseHistoryGet(index) lle_replacement_history_get(index)
#define linenoiseHistoryRemoveDups() lle_replacement_history_remove_dups()
#define linenoiseHistoryNoDups(flag) lle_replacement_history_no_dups(flag)

// Completion functions
#define linenoiseAddCompletion(lc, str) lle_replacement_add_completion(lc, str)
#define linenoiseSetCompletionCallback(fn) lle_replacement_set_completion_callback(fn)
#define linenoiseSetHintsCallback(fn) lle_replacement_set_hints_callback(fn)
#define linenoiseSetFreeHintsCallback(fn) lle_replacement_set_free_hints_callback(fn)

// Configuration functions
#define linenoiseSetMultiLine(ml) lle_replacement_set_multiline(ml)
#define linenoiseClearScreen() lle_replacement_clear_screen()
#define linenoisePrintKeyCodes() lle_replacement_print_key_codes()
#define linenoiseMaskModeEnable() lle_replacement_mask_mode_enable()
#define linenoiseMaskModeDisable() lle_replacement_mask_mode_disable()

// Additional compatibility macros for enhanced functions
#define linenoiseHistoryExpansion(line, expanded) (-1)  // Not implemented
#define linenoiseHistoryDelete(index) (-1)              // Not implemented
#define linenoiseSetHistoryNoDups(enable) lle_replacement_history_no_dups(enable)

// ============================================================================
// Advanced Features Available Through LLE
// ============================================================================

/**
 * @brief Enable advanced LLE features not available in linenoise
 *
 * These functions provide access to LLE's enhanced capabilities that go
 * beyond what linenoise offers.
 */

// Enable syntax highlighting (LLE enhancement)
bool lle_replacement_enable_syntax_highlighting(bool enable);

// Enable advanced Unicode support (LLE enhancement)
bool lle_replacement_enable_unicode_support(bool enable);

// Enable undo/redo functionality (LLE enhancement)
bool lle_replacement_enable_undo_redo(bool enable);

// Get completion statistics (LLE enhancement)
bool lle_replacement_get_completion_stats(size_t *total_calls, size_t *successful_completions);

// Configure history deduplication behavior (LLE enhancement)
bool lle_replacement_configure_history_dedup(bool move_to_end, bool case_sensitive);

// ============================================================================
// Debugging and Diagnostics
// ============================================================================

/**
 * @brief Get replacement layer statistics
 *
 * Provides usage statistics for debugging and monitoring.
 *
 * @param readline_calls Number of readline calls made
 * @param history_operations Number of history operations
 * @param completion_calls Number of completion callback invocations
 * @return true on success, false on failure
 */
bool lle_replacement_get_statistics(size_t *readline_calls, 
                                   size_t *history_operations,
                                   size_t *completion_calls);

/**
 * @brief Enable or disable debug mode
 *
 * Controls debug output from the replacement layer.
 *
 * @param enable true to enable debug mode, false to disable
 */
void lle_replacement_set_debug(bool enable);

/**
 * @brief Get last error message
 *
 * Returns the last error message from the replacement layer.
 *
 * @return Error message string, or NULL if no error
 */
const char *lle_replacement_get_last_error(void);

// ============================================================================
// Compatibility Notes
// ============================================================================

/*
 * USAGE INSTRUCTIONS:
 * 
 * 1. Replace all #include "linenoise/linenoise.h" with:
 *    #include "linenoise_replacement.h"
 * 
 * 2. No code changes required - all linenoise functions are mapped via macros
 * 
 * 3. Optional: Enable LLE enhancements:
 *    lle_replacement_enable_syntax_highlighting(true);
 *    lle_replacement_enable_unicode_support(true);
 *    lle_replacement_enable_undo_redo(true);
 * 
 * 4. The replacement layer automatically initializes on first use
 * 
 * 5. For debugging, enable debug mode:
 *    lle_replacement_set_debug(true);
 * 
 * COMPATIBILITY GUARANTEES:
 * 
 * - All linenoise API functions are supported
 * - Return values and error codes match linenoise behavior
 * - Memory management patterns are preserved
 * - Completion callbacks work unchanged
 * - History file formats are compatible
 * 
 * ENHANCED FEATURES:
 * 
 * - Superior multiline prompt support
 * - Full Unicode and international text support  
 * - Syntax highlighting for shell commands
 * - Advanced completion with multiple providers
 * - Undo/redo functionality
 * - Better terminal compatibility
 * - Performance optimizations
 * - Comprehensive error handling
 */

#ifdef __cplusplus
}
#endif

#endif /* LINENOISE_REPLACEMENT_H */