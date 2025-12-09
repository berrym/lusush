/**
 * @file builtin_widgets.c
 * @brief LLE Built-in Widget Implementations
 * 
 * Provides core widgets that are automatically registered when the editor
 * is created. These widgets provide essential editing operations that can
 * be bound to keys, triggered by hooks, or invoked programmatically.
 * 
 * Specification: docs/lle_specification/07_extensibility_framework_complete.md
 * Section: 4 - Widget System Architecture
 * 
 * Widget Categories:
 * - Movement widgets: cursor navigation
 * - Editing widgets: text manipulation
 * - History widgets: command history navigation
 * - Display widgets: screen refresh and clearing
 * - Completion widgets: tab completion operations
 * 
 * Naming Convention (ZSH-style):
 * - lowercase-with-hyphens for all widget names
 * - e.g., "forward-char", "backward-word", "kill-line"
 */

#include "lle/widget_system.h"
#include "lle/lle_editor.h"
#include "lle/buffer_management.h"
#include "lle/error_handling.h"
#include "lle/keybinding_actions.h"
#include <string.h>

/* ============================================================================
 * MOVEMENT WIDGETS
 * ============================================================================ */

/**
 * @brief Move cursor forward one character
 */
static lle_result_t widget_forward_char(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_forward_char(editor);
}

/**
 * @brief Move cursor backward one character
 */
static lle_result_t widget_backward_char(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_backward_char(editor);
}

/**
 * @brief Move cursor forward one word
 */
static lle_result_t widget_forward_word(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_forward_word(editor);
}

/**
 * @brief Move cursor backward one word
 */
static lle_result_t widget_backward_word(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_backward_word(editor);
}

/**
 * @brief Move cursor to beginning of line
 */
static lle_result_t widget_beginning_of_line(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_beginning_of_line(editor);
}

/**
 * @brief Move cursor to end of line
 */
static lle_result_t widget_end_of_line(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_end_of_line(editor);
}

/**
 * @brief Move cursor to beginning of buffer
 */
static lle_result_t widget_beginning_of_buffer(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_beginning_of_buffer(editor);
}

/**
 * @brief Move cursor to end of buffer
 */
static lle_result_t widget_end_of_buffer(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_end_of_buffer(editor);
}

/* ============================================================================
 * EDITING WIDGETS
 * ============================================================================ */

/**
 * @brief Delete character at cursor (delete-char)
 */
static lle_result_t widget_delete_char(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_delete_char(editor);
}

/**
 * @brief Delete character before cursor (backward-delete-char)
 */
static lle_result_t widget_backward_delete_char(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_backward_delete_char(editor);
}

/**
 * @brief Kill from cursor to end of line
 */
static lle_result_t widget_kill_line(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_kill_line(editor);
}

/**
 * @brief Kill from beginning of line to cursor (unix-line-discard)
 */
static lle_result_t widget_backward_kill_line(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_backward_kill_line(editor);
}

/**
 * @brief Kill word forward
 */
static lle_result_t widget_kill_word(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_kill_word(editor);
}

/**
 * @brief Kill word backward (unix-word-rubout)
 */
static lle_result_t widget_backward_kill_word(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_backward_kill_word(editor);
}

/**
 * @brief Yank (paste) from kill ring
 */
static lle_result_t widget_yank(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_yank(editor);
}

/**
 * @brief Transpose characters at cursor
 */
static lle_result_t widget_transpose_chars(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_transpose_chars(editor);
}

/**
 * @brief Transpose words at cursor
 */
static lle_result_t widget_transpose_words(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_transpose_words(editor);
}

/* ============================================================================
 * CASE CHANGE WIDGETS
 * ============================================================================ */

/**
 * @brief Capitalize word at cursor
 */
static lle_result_t widget_capitalize_word(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_capitalize_word(editor);
}

/**
 * @brief Convert word to uppercase
 */
static lle_result_t widget_upcase_word(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_upcase_word(editor);
}

/**
 * @brief Convert word to lowercase
 */
static lle_result_t widget_downcase_word(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_downcase_word(editor);
}

/* ============================================================================
 * HISTORY WIDGETS
 * ============================================================================ */

/**
 * @brief Navigate to previous history entry
 */
static lle_result_t widget_previous_history(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_history_previous(editor);
}

/**
 * @brief Navigate to next history entry
 */
static lle_result_t widget_next_history(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_history_next(editor);
}

/* Note: beginning-of-history and end-of-history widgets not yet implemented
 * These would require lle_beginning_of_history() and lle_end_of_history()
 * functions in keybinding_actions.c - planned for future implementation */

/* ============================================================================
 * COMPLETION WIDGETS
 * ============================================================================ */

/**
 * @brief Trigger tab completion
 */
static lle_result_t widget_complete(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_complete(editor);
}

/* ============================================================================
 * LINE CONTROL WIDGETS
 * ============================================================================ */

/**
 * @brief Clear screen and redraw
 */
static lle_result_t widget_clear_screen(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_clear_screen(editor);
}

/* ============================================================================
 * MULTILINE WIDGETS
 * ============================================================================ */

/**
 * @brief Smart up arrow - line navigation or history
 */
static lle_result_t widget_smart_up(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_smart_up_arrow(editor);
}

/**
 * @brief Smart down arrow - line navigation or history
 */
static lle_result_t widget_smart_down(lle_editor_t *editor, void *user_data)
{
    (void)user_data;
    return lle_smart_down_arrow(editor);
}

/* ============================================================================
 * REGISTRATION
 * ============================================================================ */

/**
 * @brief Register all built-in widgets
 * 
 * Called during editor initialization to register the standard set of
 * widgets that provide basic editing functionality.
 * 
 * @param registry Widget registry to register widgets in
 * @return LLE_SUCCESS on success, error code if any registration fails
 */
lle_result_t lle_register_builtin_widgets(lle_widget_registry_t *registry)
{
    if (!registry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result;
    
    /* Movement widgets */
    result = lle_widget_register(registry, "forward-char", widget_forward_char,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "backward-char", widget_backward_char,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "forward-word", widget_forward_word,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "backward-word", widget_backward_word,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "beginning-of-line", widget_beginning_of_line,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "end-of-line", widget_end_of_line,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "beginning-of-buffer", widget_beginning_of_buffer,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "end-of-buffer", widget_end_of_buffer,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    /* Editing widgets */
    result = lle_widget_register(registry, "delete-char", widget_delete_char,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "backward-delete-char", widget_backward_delete_char,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "kill-line", widget_kill_line,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "backward-kill-line", widget_backward_kill_line,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "kill-word", widget_kill_word,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "backward-kill-word", widget_backward_kill_word,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "yank", widget_yank,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "transpose-chars", widget_transpose_chars,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "transpose-words", widget_transpose_words,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    /* Case change widgets */
    result = lle_widget_register(registry, "capitalize-word", widget_capitalize_word,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "upcase-word", widget_upcase_word,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "downcase-word", widget_downcase_word,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    /* History widgets */
    result = lle_widget_register(registry, "previous-history", widget_previous_history,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "next-history", widget_next_history,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    /* Note: beginning-of-history and end-of-history widgets pending
     * implementation of underlying action functions */
    
    /* Completion widgets */
    result = lle_widget_register(registry, "complete", widget_complete,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    /* Line control widgets */
    result = lle_widget_register(registry, "clear-screen", widget_clear_screen,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    /* Multiline/smart navigation widgets */
    result = lle_widget_register(registry, "smart-up", widget_smart_up,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    result = lle_widget_register(registry, "smart-down", widget_smart_down,
                                 LLE_WIDGET_BUILTIN, NULL);
    if (result != LLE_SUCCESS) return result;
    
    return LLE_SUCCESS;
}
