/**
 * LLE Display Integration - Implementation
 * 
 * Week 2 Implementation: Minimal viable display integration
 * 
 * CRITICAL DESIGN PRINCIPLE:
 * This implementation does NOT write to terminal directly.
 * All rendering goes through Lusush command_layer.
 * 
 * The command_layer is part of Lusush's proven display system
 * that handles all terminal output, escape sequences, and rendering.
 * 
 * Spec Reference: 08_display_integration_complete.md (Week 2 subset)
 */

#include <lle/display.h>
#include <lle/terminal.h>
#include <display/command_layer.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Display integration structure (opaque)
 */
struct lle_display_s {
    // Terminal capabilities (from Week 1)
    lle_terminal_capabilities_t *capabilities;
    
    // Lusush display system connection
    command_layer_t *command_layer;
    
    // Current display state
    char content[4096];           // Current displayed text
    size_t content_length;        // Text length
    size_t cursor_position;       // Cursor position in text
    
    // Status
    bool initialized;
};

/**
 * Initialize display integration
 */
lle_display_result_t lle_display_init(
    lle_display_t **display,
    lle_terminal_capabilities_t *capabilities,
    command_layer_t *command_layer)
{
    if (!display || !capabilities || !command_layer) {
        return LLE_DISPLAY_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate display structure */
    lle_display_t *disp = calloc(1, sizeof(lle_display_t));
    if (!disp) {
        return LLE_DISPLAY_ERROR_MEMORY;
    }
    
    /* Store references */
    disp->capabilities = capabilities;
    disp->command_layer = command_layer;
    
    /* Initialize state */
    disp->content[0] = '\0';
    disp->content_length = 0;
    disp->cursor_position = 0;
    disp->initialized = true;
    
    *display = disp;
    return LLE_DISPLAY_SUCCESS;
}

/**
 * Update display with new content
 * 
 * CRITICAL: This function does NOT write to terminal.
 *           All rendering goes through command_layer.
 */
lle_display_result_t lle_display_update(
    lle_display_t *display,
    const char *content,
    size_t cursor_position,
    lle_display_update_flags_t flags)
{
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERROR_NOT_INITIALIZED;
    }
    
    if (!content) {
        return LLE_DISPLAY_ERROR_INVALID_PARAMETER;
    }
    
    size_t content_len = strlen(content);
    
    /* Check content size */
    if (content_len >= sizeof(display->content)) {
        return LLE_DISPLAY_ERROR_CONTENT_TOO_LARGE;
    }
    
    /* Check cursor position validity */
    if (cursor_position > content_len) {
        return LLE_DISPLAY_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if content actually changed (unless force flag set) */
    bool content_changed = (strcmp(display->content, content) != 0);
    bool cursor_changed = (display->cursor_position != cursor_position);
    
    if (!content_changed && !cursor_changed && 
        !(flags & LLE_DISPLAY_UPDATE_FORCE)) {
        /* Nothing to update */
        return LLE_DISPLAY_SUCCESS;
    }
    
    /* Store new content */
    strcpy(display->content, content);
    display->content_length = content_len;
    display->cursor_position = cursor_position;
    
    /* Update command_layer with new content
     * 
     * CRITICAL: This is where we hand off to Lusush display system.
     * The command_layer handles all terminal output, escape sequences,
     * syntax highlighting, and rendering.
     * 
     * We do NOT write to terminal ourselves.
     */
    command_layer_error_t result = command_layer_set_command(
        display->command_layer,
        content,
        cursor_position
    );
    
    if (result != COMMAND_LAYER_SUCCESS) {
        return LLE_DISPLAY_ERROR_DISPLAY_FAILED;
    }
    
    /* Trigger command_layer update if content changed
     * (cursor-only changes don't need full update) */
    if (content_changed || (flags & LLE_DISPLAY_UPDATE_FORCE)) {
        result = command_layer_update(display->command_layer);
        
        if (result != COMMAND_LAYER_SUCCESS) {
            return LLE_DISPLAY_ERROR_DISPLAY_FAILED;
        }
    }
    
    return LLE_DISPLAY_SUCCESS;
}

/**
 * Clear display content
 */
lle_display_result_t lle_display_clear(
    lle_display_t *display)
{
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERROR_NOT_INITIALIZED;
    }
    
    /* Clear internal state */
    display->content[0] = '\0';
    display->content_length = 0;
    display->cursor_position = 0;
    
    /* Clear command_layer */
    command_layer_error_t result = command_layer_set_command(
        display->command_layer,
        "",
        0
    );
    
    if (result != COMMAND_LAYER_SUCCESS) {
        return LLE_DISPLAY_ERROR_DISPLAY_FAILED;
    }
    
    result = command_layer_update(display->command_layer);
    
    if (result != COMMAND_LAYER_SUCCESS) {
        return LLE_DISPLAY_ERROR_DISPLAY_FAILED;
    }
    
    return LLE_DISPLAY_SUCCESS;
}

/**
 * Get current display content (for testing)
 */
lle_display_result_t lle_display_get_content(
    const lle_display_t *display,
    char *buffer,
    size_t buffer_size)
{
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERROR_NOT_INITIALIZED;
    }
    
    if (!buffer || buffer_size == 0) {
        return LLE_DISPLAY_ERROR_INVALID_PARAMETER;
    }
    
    /* Copy content to buffer */
    size_t copy_len = display->content_length;
    if (copy_len >= buffer_size) {
        copy_len = buffer_size - 1;
    }
    
    memcpy(buffer, display->content, copy_len);
    buffer[copy_len] = '\0';
    
    return LLE_DISPLAY_SUCCESS;
}

/**
 * Get current cursor position (for testing)
 */
lle_display_result_t lle_display_get_cursor_position(
    const lle_display_t *display,
    size_t *cursor_position)
{
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERROR_NOT_INITIALIZED;
    }
    
    if (!cursor_position) {
        return LLE_DISPLAY_ERROR_INVALID_PARAMETER;
    }
    
    *cursor_position = display->cursor_position;
    
    return LLE_DISPLAY_SUCCESS;
}

/**
 * Destroy display integration
 */
void lle_display_destroy(
    lle_display_t *display)
{
    if (!display) {
        return;
    }
    
    /* Clear command_layer (best effort) */
    if (display->initialized && display->command_layer) {
        command_layer_set_command(display->command_layer, "", 0);
        command_layer_update(display->command_layer);
    }
    
    /* Free display structure */
    free(display);
}

/**
 * Get human-readable error message
 */
const char *lle_display_error_string(
    lle_display_result_t result)
{
    switch (result) {
        case LLE_DISPLAY_SUCCESS:
            return "Success";
        case LLE_DISPLAY_ERROR_MEMORY:
            return "Memory allocation failed";
        case LLE_DISPLAY_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case LLE_DISPLAY_ERROR_NOT_INITIALIZED:
            return "Display not initialized";
        case LLE_DISPLAY_ERROR_DISPLAY_FAILED:
            return "Display operation failed";
        case LLE_DISPLAY_ERROR_CONTENT_TOO_LARGE:
            return "Content too large";
        default:
            return "Unknown error";
    }
}
