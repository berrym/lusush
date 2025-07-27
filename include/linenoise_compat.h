#ifndef LINENOISE_COMPAT_H
#define LINENOISE_COMPAT_H

/**
 * @file linenoise_compat.h
 * @brief Linenoise Compatibility Layer for LLE Integration
 *
 * This header provides compatibility definitions for existing code that uses
 * linenoise types and structures. It maps linenoise types to LLE equivalents
 * while preserving existing API compatibility.
 *
 * This allows existing completion and callback code to continue working
 * without modification during the transition to LLE.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Linenoise Type Compatibility Definitions
// ============================================================================

/**
 * @brief Completion list structure (linenoise compatibility)
 *
 * This structure provides compatibility with existing linenoise completion
 * code. It maintains the same memory layout and access patterns.
 */
typedef struct linenoiseCompletions {
    size_t len;    /**< Number of completion entries */
    char **cvec;   /**< Array of completion strings */
} linenoiseCompletions;

/**
 * @brief Completion callback function type (linenoise compatibility)
 *
 * This function type provides compatibility with existing linenoise
 * completion callback code.
 */
typedef void(linenoiseCompletionCallback)(const char *, linenoiseCompletions *);

/**
 * @brief Hints callback function type (linenoise compatibility)
 *
 * This function type provides compatibility with existing linenoise
 * hints callback code.
 */
typedef char *(linenoiseHintsCallback)(const char *, int *color, int *bold);

/**
 * @brief Free hints callback function type (linenoise compatibility)
 *
 * This function type provides compatibility with existing linenoise
 * free hints callback code.
 */
typedef void(linenoiseFreeHintsCallback)(void *);

// ============================================================================
// Linenoise API Compatibility Functions
// ============================================================================

/**
 * @brief Add completion entry (linenoise compatibility)
 *
 * Adds a completion string to the completion list. This function provides
 * compatibility with existing linenoise completion code.
 *
 * @param lc Completion list to add to
 * @param str Completion string to add
 *
 * @note The string is copied, so the original can be freed
 * @note Memory management is handled automatically
 */
void linenoiseAddCompletion(linenoiseCompletions *lc, const char *str);

/**
 * @brief Set completion callback (linenoise compatibility)
 *
 * Sets the completion callback function for tab completion. This function
 * provides compatibility with existing linenoise completion code.
 *
 * @param fn Completion callback function
 *
 * @note The callback will be integrated with LLE's completion system
 * @note Multiple callbacks can be registered and will be called in order
 */
void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn);

/**
 * @brief Set hints callback (linenoise compatibility)
 *
 * Sets the hints callback function for inline hints. This function provides
 * compatibility with existing linenoise hints code.
 *
 * @param fn Hints callback function
 *
 * @note The callback will be integrated with LLE's completion system
 * @note Hints are displayed as inline suggestions while typing
 */
void linenoiseSetHintsCallback(linenoiseHintsCallback *fn);

/**
 * @brief Set free hints callback (linenoise compatibility)
 *
 * Sets the callback function for freeing hint memory. This function provides
 * compatibility with existing linenoise hints code.
 *
 * @param fn Free hints callback function
 *
 * @note This callback is called to free memory allocated by the hints callback
 * @note Proper memory management is essential to prevent leaks
 */
void linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *fn);

// ============================================================================
// Internal Compatibility Support
// ============================================================================

/**
 * @brief Initialize linenoise compatibility layer
 *
 * Initializes the compatibility layer for linenoise functions. This should
 * be called during shell initialization to set up the compatibility mappings.
 *
 * @return true on success, false on failure
 *
 * @note This function is called automatically by LLE integration
 * @note Safe to call multiple times
 */
bool linenoise_compat_init(void);

/**
 * @brief Shutdown linenoise compatibility layer
 *
 * Cleans up the compatibility layer and frees any allocated resources.
 * This should be called during shell shutdown.
 *
 * @note This function is called automatically by LLE integration
 * @note Safe to call multiple times
 */
void linenoise_compat_shutdown(void);

/**
 * @brief Create compatibility completion list
 *
 * Creates a new completion list structure for compatibility with existing
 * linenoise completion code.
 *
 * @return Pointer to new completion list, or NULL on failure
 *
 * @note The returned list must be freed with linenoise_compat_free_completions()
 * @note The list is initially empty and entries should be added with linenoiseAddCompletion()
 */
linenoiseCompletions *linenoise_compat_create_completions(void);

/**
 * @brief Free compatibility completion list
 *
 * Frees a completion list structure and all associated memory.
 *
 * @param lc Completion list to free (can be NULL)
 *
 * @note Safe to call with NULL pointer
 * @note Frees all completion strings and the list structure itself
 */
void linenoise_compat_free_completions(linenoiseCompletions *lc);

/**
 * @brief Convert completion list to LLE format
 *
 * Converts a linenoise completion list to LLE's internal completion format.
 * This is used internally by the compatibility layer.
 *
 * @param lc Linenoise completion list
 * @return true on success, false on failure
 *
 * @note This function is used internally and should not be called directly
 * @note The conversion integrates with LLE's completion system
 */
bool linenoise_compat_convert_completions(const linenoiseCompletions *lc);

// ============================================================================
// Compatibility Macros and Constants
// ============================================================================

/**
 * @brief Maximum completion entries (linenoise compatibility)
 */
#define LINENOISE_MAX_COMPLETIONS 1024

/**
 * @brief Default completion color (linenoise compatibility)
 */
#define LINENOISE_DEFAULT_HINT_COLOR 35

/**
 * @brief Default completion bold flag (linenoise compatibility)
 */
#define LINENOISE_DEFAULT_HINT_BOLD 0

// ============================================================================
// Debugging and Diagnostics
// ============================================================================

/**
 * @brief Get compatibility layer statistics
 *
 * Provides statistics about the compatibility layer usage for debugging
 * and performance monitoring.
 *
 * @param completion_calls Pointer to receive number of completion calls
 * @param hint_calls Pointer to receive number of hint calls
 * @param memory_used Pointer to receive bytes currently allocated
 * @return true on success, false on failure
 */
bool linenoise_compat_get_stats(size_t *completion_calls, size_t *hint_calls, 
                                size_t *memory_used);

/**
 * @brief Enable or disable compatibility debug mode
 *
 * Controls debug output from the compatibility layer. When enabled,
 * the layer will log callback invocations and memory operations.
 *
 * @param enable true to enable debug mode, false to disable
 */
void linenoise_compat_set_debug(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* LINENOISE_COMPAT_H */