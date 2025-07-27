/*
 * Lusush Line Editor - Syntax Highlighting Framework
 * 
 * This file defines the syntax highlighting system for the Lusush Line Editor.
 * It provides a flexible framework for real-time syntax highlighting with support
 * for shell command syntax, extensible syntax types, and efficient region-based
 * highlighting storage.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#ifndef LLE_SYNTAX_H
#define LLE_SYNTAX_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_syntax_region lle_syntax_region_t;
typedef struct lle_syntax_highlight lle_syntax_highlight_t;
typedef struct lle_syntax_highlighter lle_syntax_highlighter_t;

/**
 * @brief Default initial capacity for syntax regions
 */
#define LLE_SYNTAX_DEFAULT_CAPACITY 16

/**
 * @brief Maximum number of syntax regions (memory limit)
 */
#define LLE_SYNTAX_MAX_REGIONS 1000

/**
 * @brief Types of syntax elements that can be highlighted
 * 
 * These types correspond to different visual styles that can be applied
 * to text regions. Each type should map to appropriate theme colors.
 */
typedef enum {
    LLE_SYNTAX_NORMAL = 0,      // Default text (no highlighting)
    LLE_SYNTAX_KEYWORD,         // Shell keywords (if, then, else, etc.)
    LLE_SYNTAX_COMMAND,         // Command names and executables
    LLE_SYNTAX_STRING,          // Quoted strings
    LLE_SYNTAX_COMMENT,         // Comments (# in shell)
    LLE_SYNTAX_NUMBER,          // Numeric literals
    LLE_SYNTAX_OPERATOR,        // Operators (|, &, >, <, etc.)
    LLE_SYNTAX_VARIABLE,        // Variables ($VAR, ${VAR})
    LLE_SYNTAX_PATH,            // File paths and directories
    LLE_SYNTAX_ERROR           // Syntax errors or invalid constructs
} lle_syntax_type_t;

/**
 * @brief Individual syntax highlighting region
 * 
 * Represents a contiguous region of text with a specific syntax type.
 * Regions are stored in order and should not overlap.
 */
struct lle_syntax_region {
    size_t start;               // Start position in text (byte offset)
    size_t length;              // Length of highlighted region (bytes)
    lle_syntax_type_t type;     // Type of syntax element
    
    // Internal fields for optimization
    size_t char_start;          // Start position in characters (for Unicode)
    size_t char_length;         // Length in characters (for Unicode)
};

/**
 * @brief Collection of syntax highlighting regions
 * 
 * Contains all highlighting information for a piece of text. Regions are
 * stored in order by start position and should not overlap.
 */
struct lle_syntax_highlight {
    lle_syntax_region_t *regions;   // Array of highlighting regions
    size_t count;                   // Number of active regions
    size_t capacity;                // Allocated capacity
    
    // Metadata
    size_t text_length;             // Length of associated text
    bool is_dirty;                  // Whether highlighting needs refresh
};

/**
 * @brief Syntax highlighter state and configuration
 * 
 * Maintains state for syntax highlighting operations and provides
 * configuration for different highlighting modes.
 */
struct lle_syntax_highlighter {
    lle_syntax_highlight_t *current;   // Current highlighting state
    
    // Configuration
    bool enable_shell_syntax;          // Enable shell command highlighting
    bool enable_string_highlighting;   // Enable string highlighting
    bool enable_comment_highlighting;  // Enable comment highlighting
    bool enable_variable_highlighting; // Enable variable highlighting
    
    // Performance settings
    size_t max_highlight_length;       // Maximum text length to highlight
    bool incremental_update;           // Use incremental highlighting updates
    
    // Internal state
    bool initialized;                  // Whether highlighter is initialized
    size_t last_cursor_pos;           // Last cursor position for optimization
};

// ============================================================================
// Core API Functions
// ============================================================================

/**
 * @brief Create a new syntax highlighter
 * 
 * Creates and initializes a new syntax highlighter with default configuration.
 * The highlighter starts with shell syntax highlighting enabled.
 * 
 * @return Pointer to new syntax highlighter, or NULL on error
 * 
 * @note Caller is responsible for calling lle_syntax_destroy() to free memory
 */
lle_syntax_highlighter_t *lle_syntax_create(void);

/**
 * @brief Initialize a syntax highlighter with custom configuration
 * 
 * Initializes an existing syntax highlighter structure with the specified
 * configuration. This is useful for stack-allocated highlighters.
 * 
 * @param highlighter Pointer to highlighter to initialize (must not be NULL)
 * @param enable_shell Whether to enable shell syntax highlighting
 * @param max_length Maximum text length to highlight (0 for unlimited)
 * @return true on success, false on error
 * 
 * @note Use lle_syntax_cleanup() to free internal resources
 */
bool lle_syntax_init(lle_syntax_highlighter_t *highlighter, 
                     bool enable_shell, 
                     size_t max_length);

/**
 * @brief Destroy a syntax highlighter and free all memory
 * 
 * Frees all memory associated with the syntax highlighter including
 * highlighting regions and internal state.
 * 
 * @param highlighter Pointer to highlighter to destroy (can be NULL)
 */
void lle_syntax_destroy(lle_syntax_highlighter_t *highlighter);

/**
 * @brief Clean up internal resources of a syntax highlighter
 * 
 * Frees internal resources but does not free the highlighter structure
 * itself. Use this for stack-allocated highlighters.
 * 
 * @param highlighter Pointer to highlighter to clean up (can be NULL)
 */
void lle_syntax_cleanup(lle_syntax_highlighter_t *highlighter);

// ============================================================================
// Highlighting Functions
// ============================================================================

/**
 * @brief Highlight text and generate syntax regions
 * 
 * Analyzes the provided text and generates syntax highlighting regions
 * based on the highlighter's configuration. This replaces any existing
 * highlighting information.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param text Text to highlight (must not be NULL)
 * @param length Length of text in bytes
 * @return true on success, false on error
 * 
 * @note This function performs complete re-highlighting of the text
 * @note For incremental updates, use lle_syntax_update_region()
 */
bool lle_syntax_highlight_text(lle_syntax_highlighter_t *highlighter,
                               const char *text,
                               size_t length);

/**
 * @brief Update highlighting for a specific text region
 * 
 * Performs incremental highlighting update for a changed region of text.
 * This is more efficient than re-highlighting the entire text.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param text Full text content (must not be NULL)
 * @param text_length Total length of text
 * @param change_start Start position of changed region
 * @param change_length Length of changed region
 * @return true on success, false on error
 * 
 * @note This function optimizes highlighting by only updating affected regions
 */
bool lle_syntax_update_region(lle_syntax_highlighter_t *highlighter,
                              const char *text,
                              size_t text_length,
                              size_t change_start,
                              size_t change_length);

/**
 * @brief Get syntax type at a specific position
 * 
 * Returns the syntax type for the character at the specified position.
 * This is useful for cursor-based operations and display rendering.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param position Position in text (byte offset)
 * @return Syntax type at position, or LLE_SYNTAX_NORMAL if no highlighting
 * 
 * @note Position should be within the bounds of the highlighted text
 */
lle_syntax_type_t lle_syntax_get_type_at_position(const lle_syntax_highlighter_t *highlighter,
                                                  size_t position);

/**
 * @brief Get all syntax regions for display rendering
 * 
 * Returns the current highlighting regions for use by display systems.
 * The returned array remains valid until the next highlighting operation.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param region_count Pointer to store number of regions (can be NULL)
 * @return Pointer to regions array, or NULL if no highlighting data
 * 
 * @note The returned pointer is valid until the next highlighting operation
 * @note Do not modify the returned regions array
 */
const lle_syntax_region_t *lle_syntax_get_regions(const lle_syntax_highlighter_t *highlighter,
                                                  size_t *region_count);

// ============================================================================
// Configuration Functions
// ============================================================================

/**
 * @brief Configure shell syntax highlighting
 * 
 * Enables or disables shell-specific syntax highlighting including
 * keywords, operators, and shell constructs.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param enable Whether to enable shell syntax highlighting
 * @return true on success, false on error
 */
bool lle_syntax_configure_shell(lle_syntax_highlighter_t *highlighter, bool enable);

/**
 * @brief Configure string highlighting
 * 
 * Enables or disables highlighting of quoted strings including
 * single quotes, double quotes, and escape sequences.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param enable Whether to enable string highlighting
 * @return true on success, false on error
 */
bool lle_syntax_configure_strings(lle_syntax_highlighter_t *highlighter, bool enable);

/**
 * @brief Configure variable highlighting
 * 
 * Enables or disables highlighting of shell variables including
 * $VAR, ${VAR}, and special variables.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param enable Whether to enable variable highlighting
 * @return true on success, false on error
 */
bool lle_syntax_configure_variables(lle_syntax_highlighter_t *highlighter, bool enable);

/**
 * @brief Configure comment highlighting
 * 
 * Enables or disables highlighting of comments including
 * shell comments starting with #.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param enable Whether to enable comment highlighting
 * @return true on success, false on error
 */
bool lle_syntax_configure_comments(lle_syntax_highlighter_t *highlighter, bool enable);

/**
 * @brief Set maximum highlighting length
 * 
 * Sets the maximum text length that will be highlighted for performance
 * reasons. Text longer than this limit will not be highlighted.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @param max_length Maximum length to highlight (0 for unlimited)
 * @return true on success, false on error
 */
bool lle_syntax_set_max_length(lle_syntax_highlighter_t *highlighter, size_t max_length);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Check if syntax highlighting is enabled
 * 
 * Returns whether any syntax highlighting is currently enabled.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @return true if highlighting is enabled, false otherwise
 */
bool lle_syntax_is_enabled(const lle_syntax_highlighter_t *highlighter);

/**
 * @brief Check if highlighting data is dirty and needs refresh
 * 
 * Returns whether the current highlighting data is outdated and needs
 * to be refreshed with a new highlighting operation.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @return true if highlighting needs refresh, false otherwise
 */
bool lle_syntax_is_dirty(const lle_syntax_highlighter_t *highlighter);

/**
 * @brief Clear all highlighting regions
 * 
 * Removes all syntax highlighting regions and resets the highlighter
 * to an empty state. This does not change configuration settings.
 * 
 * @param highlighter Pointer to syntax highlighter (must not be NULL)
 * @return true on success, false on error
 */
bool lle_syntax_clear_regions(lle_syntax_highlighter_t *highlighter);

/**
 * @brief Get syntax type name for debugging
 * 
 * Returns a human-readable string name for the given syntax type.
 * Useful for debugging and logging.
 * 
 * @param type Syntax type to get name for
 * @return String name of syntax type (never NULL)
 */
const char *lle_syntax_type_name(lle_syntax_type_t type);

#ifdef __cplusplus
}
#endif

#endif // LLE_SYNTAX_H