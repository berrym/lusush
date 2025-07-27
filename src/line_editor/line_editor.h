#ifndef LLE_LINE_EDITOR_H
#define LLE_LINE_EDITOR_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @file line_editor.h
 * @brief Lusush Line Editor - Main Public API
 *
 * This module provides the main public API for the Lusush Line Editor (LLE).
 * It encapsulates all LLE components into a single, easy-to-use interface
 * that serves as a drop-in replacement for linenoise and other line editors.
 *
 * The API is designed to be simple, memory-safe, and compatible with existing
 * shell implementations while providing advanced features like multiline
 * prompts, syntax highlighting, Unicode support, and comprehensive history.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

// Include component headers for complete type definitions
#include "text_buffer.h"
#include "terminal_manager.h"
#include "display.h"
#include "command_history.h"
#include "theme_integration.h"
#include "completion.h"
#include "undo.h"

/**
 * @brief Main line editor structure
 *
 * This structure encapsulates all components of the Lusush Line Editor,
 * providing a complete line editing environment with advanced features.
 * The structure is opaque to users and should only be accessed through
 * the provided API functions.
 *
 * Components included:
 * - Text buffer: UTF-8 text storage and manipulation
 * - Terminal manager: Terminal control and capability detection
 * - Display system: Multiline prompt and input rendering
 * - History system: Command history with file persistence
 * - Theme integration: Visual styling and color support
 * - Completion system: Tab completion with multiple providers
 * - Undo/redo system: Complete operation reversal
 */
typedef struct {
    lle_text_buffer_t *buffer;          /**< Text buffer for input storage */
    lle_terminal_manager_t *terminal;   /**< Terminal interface manager */
    lle_display_state_t *display;       /**< Display state and rendering */
    lle_history_t *history;             /**< Command history management */
    lle_theme_integration_t *theme;     /**< Theme integration interface */
    lle_completion_list_t *completions; /**< Completion system */
    lle_undo_stack_t *undo_stack;       /**< Undo/redo operation stack */
    
    /* Configuration flags */
    bool multiline_mode;                /**< Enable multiline editing */
    bool syntax_highlighting;          /**< Enable syntax highlighting */
    bool auto_completion;               /**< Enable auto-completion */
    bool history_enabled;               /**< Enable command history */
    bool undo_enabled;                  /**< Enable undo/redo operations */
    
    /* Internal state */
    bool initialized;                   /**< Whether editor is initialized */
    char *current_prompt;               /**< Current prompt string (cached) */
    size_t max_history_size;            /**< Maximum history entries */
    size_t max_undo_actions;            /**< Maximum undo actions */
} lle_line_editor_t;

/**
 * @brief Configuration options for line editor initialization
 *
 * This structure provides configuration options that can be set during
 * line editor creation to customize behavior and resource limits.
 */
typedef struct {
    size_t max_history_size;            /**< Maximum history entries (0 = default) */
    size_t max_undo_actions;            /**< Maximum undo actions (0 = default) */
    bool enable_multiline;              /**< Enable multiline editing */
    bool enable_syntax_highlighting;    /**< Enable syntax highlighting */
    bool enable_auto_completion;        /**< Enable auto-completion */
    bool enable_history;                /**< Enable command history */
    bool enable_undo;                   /**< Enable undo/redo */
} lle_config_t;

/**
 * @brief Error codes returned by line editor functions
 */
typedef enum {
    LLE_SUCCESS = 0,                    /**< Operation successful */
    LLE_ERROR_INVALID_PARAMETER,        /**< Invalid parameter provided */
    LLE_ERROR_MEMORY_ALLOCATION,        /**< Memory allocation failed */
    LLE_ERROR_TERMINAL_INIT,            /**< Terminal initialization failed */
    LLE_ERROR_NOT_INITIALIZED,          /**< Editor not properly initialized */
    LLE_ERROR_IO_ERROR,                 /**< Input/output error */
    LLE_ERROR_INTERRUPTED               /**< Operation interrupted (Ctrl+C) */
} lle_error_t;

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Core API Functions
// ============================================================================

/**
 * @brief Create a new line editor instance with default configuration
 *
 * Creates and initializes a new line editor with default settings. The editor
 * will have all features enabled and use standard resource limits. For custom
 * configuration, use lle_create_with_config() instead.
 *
 * @return Pointer to new line editor instance, or NULL on failure
 *
 * @note The returned pointer must be freed with lle_destroy()
 * @note This function may fail if terminal initialization fails
 *
 * @see lle_create_with_config()
 * @see lle_destroy()
 */
lle_line_editor_t *lle_create(void);

/**
 * @brief Create a new line editor instance with custom configuration
 *
 * Creates and initializes a new line editor with the specified configuration.
 * This allows fine-grained control over features and resource limits.
 *
 * @param config Configuration options for the editor (NULL for defaults)
 * @return Pointer to new line editor instance, or NULL on failure
 *
 * @note The returned pointer must be freed with lle_destroy()
 * @note Configuration is copied, so the config parameter can be freed after this call
 *
 * @see lle_create()
 * @see lle_destroy()
 */
lle_line_editor_t *lle_create_with_config(const lle_config_t *config);

/**
 * @brief Destroy a line editor instance and free all resources
 *
 * Properly destroys a line editor instance, freeing all allocated memory
 * and restoring terminal state. This function is safe to call with NULL.
 *
 * @param editor Line editor instance to destroy (can be NULL)
 *
 * @note After calling this function, the editor pointer is invalid
 * @note Terminal state is automatically restored
 * @note All pending history is saved before destruction
 */
void lle_destroy(lle_line_editor_t *editor);

/**
 * @brief Read a line of input from the user with the specified prompt
 *
 * This is the main function for line editing. It displays the prompt,
 * handles user input with full editing capabilities, and returns the
 * final line when the user presses Enter.
 *
 * Features available during editing:
 * - Full Unicode support with proper cursor movement
 * - Multiline editing with automatic line wrapping
 * - Command history navigation (Up/Down arrows)
 * - Tab completion with multiple providers
 * - Undo/redo operations (Ctrl+Z/Ctrl+Y)
 * - Syntax highlighting (if enabled)
 * - All standard editing keys (Home, End, Delete, etc.)
 *
 * @param editor Line editor instance
 * @param prompt Prompt string to display (supports ANSI escape codes)
 * @return Dynamically allocated string containing user input, or NULL on error/EOF
 *
 * @note The returned string must be freed by the caller
 * @note Returns NULL on EOF (Ctrl+D) or error
 * @note The prompt can contain ANSI escape codes for styling
 * @note Multiline prompts are automatically handled
 *
 * @see lle_add_history()
 */
char *lle_readline(lle_line_editor_t *editor, const char *prompt);

/**
 * @brief Add a line to the command history
 *
 * Adds the specified line to the command history for later retrieval.
 * The line will be available via Up/Down arrow navigation in subsequent
 * lle_readline() calls.
 *
 * @param editor Line editor instance
 * @param line Line to add to history (must not be NULL or empty)
 * @return true on success, false on failure
 *
 * @note Empty lines and lines consisting only of whitespace are ignored
 * @note Duplicate consecutive entries may be filtered based on configuration
 * @note History is automatically saved to disk periodically
 * @note Very long lines may be truncated based on limits
 */
bool lle_add_history(lle_line_editor_t *editor, const char *line);

// ============================================================================
// Configuration and Status Functions
// ============================================================================

/**
 * @brief Get the last error code from the line editor
 *
 * Returns the error code from the most recent operation. This can be used
 * to determine the specific cause of failure when a function returns an
 * error indicator.
 *
 * @param editor Line editor instance
 * @return Last error code, or LLE_SUCCESS if no error
 */
lle_error_t lle_get_last_error(lle_line_editor_t *editor);

/**
 * @brief Check if the line editor is properly initialized
 *
 * Verifies that the line editor instance is valid and properly initialized.
 * This can be used to check the state before performing operations.
 *
 * @param editor Line editor instance to check
 * @return true if editor is valid and initialized, false otherwise
 */
bool lle_is_initialized(lle_line_editor_t *editor);

/**
 * @brief Get the current configuration of the line editor
 *
 * Retrieves the current configuration settings of the line editor.
 * The returned configuration reflects the current state, which may
 * differ from the initial configuration if settings were changed.
 *
 * @param editor Line editor instance
 * @param config Pointer to configuration structure to fill
 * @return true on success, false on failure
 */
bool lle_get_config(lle_line_editor_t *editor, lle_config_t *config);

// ============================================================================
// Feature Control Functions
// ============================================================================

/**
 * @brief Enable or disable multiline editing mode
 *
 * Controls whether the line editor allows multiline input. When enabled,
 * users can create multiline input using appropriate key combinations.
 *
 * @param editor Line editor instance
 * @param enable true to enable multiline mode, false to disable
 * @return true on success, false on failure
 */
bool lle_set_multiline_mode(lle_line_editor_t *editor, bool enable);

/**
 * @brief Enable or disable syntax highlighting
 *
 * Controls whether the line editor applies syntax highlighting to user input.
 * When enabled, different types of text (keywords, strings, etc.) are
 * displayed in different colors.
 *
 * @param editor Line editor instance
 * @param enable true to enable syntax highlighting, false to disable
 * @return true on success, false on failure
 */
bool lle_set_syntax_highlighting(lle_line_editor_t *editor, bool enable);

/**
 * @brief Enable or disable auto-completion
 *
 * Controls whether the line editor provides tab completion functionality.
 * When enabled, pressing Tab will attempt to complete the current word.
 *
 * @param editor Line editor instance
 * @param enable true to enable auto-completion, false to disable
 * @return true on success, false on failure
 */
bool lle_set_auto_completion(lle_line_editor_t *editor, bool enable);

/**
 * @brief Enable or disable command history
 *
 * Controls whether the line editor maintains and provides access to command
 * history. When enabled, previous commands can be accessed via Up/Down arrows.
 *
 * @param editor Line editor instance
 * @param enable true to enable history, false to disable
 * @return true on success, false on failure
 */
bool lle_set_history_enabled(lle_line_editor_t *editor, bool enable);

/**
 * @brief Enable or disable undo/redo functionality
 *
 * Controls whether the line editor provides undo/redo operations.
 * When enabled, users can undo and redo editing operations.
 *
 * @param editor Line editor instance
 * @param enable true to enable undo/redo, false to disable
 * @return true on success, false on failure
 */
bool lle_set_undo_enabled(lle_line_editor_t *editor, bool enable);

// ============================================================================
// History Management Functions
// ============================================================================

/**
 * @brief Clear all command history
 *
 * Removes all entries from the command history. This operation cannot be undone.
 *
 * @param editor Line editor instance
 * @return true on success, false on failure
 */
bool lle_clear_history(lle_line_editor_t *editor);

/**
 * @brief Get the number of entries in command history
 *
 * Returns the current number of entries stored in the command history.
 *
 * @param editor Line editor instance
 * @return Number of history entries, or 0 on failure
 */
size_t lle_get_history_count(lle_line_editor_t *editor);

/**
 * @brief Load command history from file
 *
 * Loads command history from the specified file. The file should contain
 * one command per line. Existing history is preserved and new entries are added.
 *
 * @param editor Line editor instance
 * @param filename Path to history file to load
 * @return true on success, false on failure
 */
bool lle_load_history(lle_line_editor_t *editor, const char *filename);

/**
 * @brief Save command history to file
 *
 * Saves the current command history to the specified file. Each command
 * is written as a separate line.
 *
 * @param editor Line editor instance
 * @param filename Path to history file to save
 * @return true on success, false on failure
 */
bool lle_save_history(lle_line_editor_t *editor, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* LLE_LINE_EDITOR_H */