/**
 * LLE Display Integration - Public API
 * 
 * Week 2 Implementation: Minimal viable display integration
 * 
 * Goal: Prove LLE can display content through Lusush display system
 *       with ZERO direct terminal writes.
 * 
 * Design Principles (MANDATORY):
 * - NO direct terminal writes (everything through Lusush)
 * - NO escape sequences in LLE code
 * - Uses Week 1 terminal capabilities for adaptive rendering
 * - Simple, incremental implementation
 * 
 * Spec Reference: 08_display_integration_complete.md (simplified for Week 2)
 */

#ifndef LLE_DISPLAY_H
#define LLE_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <lle/terminal.h>

/* Forward declarations for Lusush types */
typedef struct command_layer_s command_layer_t;

/**
 * Display integration handle
 * 
 * Opaque structure - implementation details hidden
 */
typedef struct lle_display_s lle_display_t;

/**
 * Display operation result codes
 */
typedef enum {
    LLE_DISPLAY_SUCCESS = 0,
    LLE_DISPLAY_ERROR_MEMORY,
    LLE_DISPLAY_ERROR_INVALID_PARAMETER,
    LLE_DISPLAY_ERROR_NOT_INITIALIZED,
    LLE_DISPLAY_ERROR_DISPLAY_FAILED,
    LLE_DISPLAY_ERROR_CONTENT_TOO_LARGE,
} lle_display_result_t;

/**
 * Display update flags
 */
typedef enum {
    LLE_DISPLAY_UPDATE_NORMAL = 0,       // Normal update
    LLE_DISPLAY_UPDATE_FORCE = (1 << 0), // Force refresh even if unchanged
    LLE_DISPLAY_UPDATE_CURSOR_ONLY = (1 << 1), // Only cursor position changed
} lle_display_update_flags_t;

/**
 * Initialize display integration
 * 
 * Connects LLE to Lusush display system through command_layer.
 * 
 * @param display Output: Display handle (allocated by function)
 * @param capabilities Terminal capabilities from Week 1
 * @param command_layer Lusush command_layer for rendering
 * @return LLE_DISPLAY_SUCCESS or error code
 * 
 * CRITICAL: This function does NOT write to terminal.
 *           All rendering goes through command_layer.
 */
lle_display_result_t lle_display_init(
    lle_display_t **display,
    lle_terminal_capabilities_t *capabilities,
    command_layer_t *command_layer
);

/**
 * Update display with new content
 * 
 * Sends content to Lusush command_layer for rendering.
 * The command_layer handles all terminal output.
 * 
 * @param display Display handle
 * @param content Text to display (NULL-terminated)
 * @param cursor_position Cursor position in content (0-based)
 * @param flags Update flags (see lle_display_update_flags_t)
 * @return LLE_DISPLAY_SUCCESS or error code
 * 
 * CRITICAL: This function does NOT write to terminal.
 *           All rendering goes through command_layer.
 */
lle_display_result_t lle_display_update(
    lle_display_t *display,
    const char *content,
    size_t cursor_position,
    lle_display_update_flags_t flags
);

/**
 * Clear display content
 * 
 * Clears the command_layer display.
 * 
 * @param display Display handle
 * @return LLE_DISPLAY_SUCCESS or error code
 */
lle_display_result_t lle_display_clear(
    lle_display_t *display
);

/**
 * Get current display content (for testing)
 * 
 * Returns the content currently set in the display.
 * 
 * @param display Display handle
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return LLE_DISPLAY_SUCCESS or error code
 */
lle_display_result_t lle_display_get_content(
    const lle_display_t *display,
    char *buffer,
    size_t buffer_size
);

/**
 * Get current cursor position (for testing)
 * 
 * @param display Display handle
 * @param cursor_position Output: Cursor position
 * @return LLE_DISPLAY_SUCCESS or error code
 */
lle_display_result_t lle_display_get_cursor_position(
    const lle_display_t *display,
    size_t *cursor_position
);

/**
 * Destroy display integration
 * 
 * @param display Display handle to destroy
 */
void lle_display_destroy(
    lle_display_t *display
);

/**
 * Get human-readable error message
 * 
 * @param result Result code
 * @return Error message string (static, do not free)
 */
const char *lle_display_error_string(
    lle_display_result_t result
);

#endif /* LLE_DISPLAY_H */
