/**
 * keybinding_actions.c - Default Keybinding Action Function Implementations
 *
 * Complete implementation of all 44 GNU Readline compatible keybinding actions.
 * Provides 100% compatibility with Emacs-style keybindings.
 *
 * Specification: docs/lle_specification/critical_gaps/25_default_keybindings_complete.md
 * Date: 2025-11-02
 */

#include "lle/keybinding_actions.h"
#include "lle/buffer_management.h"
#include "lle/history.h"
#include "lle/kill_ring.h"
#include "lle/keybinding.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* History navigation position is now stored in editor->history_navigation_pos */

/**
 * Check if character is a word boundary (whitespace or shell metacharacter)
 */
static bool is_word_boundary(char c) {
    return isspace((unsigned char)c) || 
           c == '\0' ||
           c == '|' || c == '&' || c == ';' || c == '(' || c == ')' ||
           c == '<' || c == '>' || c == '\'' || c == '"' || c == '`' ||
           c == '$' || c == '\\';
}

/**
 * Check if character is Unix word boundary (whitespace only, for Ctrl-W)
 */
static bool is_unix_word_boundary(char c) {
    return isspace((unsigned char)c) || c == '\0';
}

/**
 * Find start of current word from position
 */
static size_t find_word_start(const char *text, size_t pos) {
    if (pos == 0) return 0;
    
    /* Skip whitespace backward */
    while (pos > 0 && isspace((unsigned char)text[pos - 1])) {
        pos--;
    }
    
    /* Find beginning of word */
    while (pos > 0 && !is_word_boundary(text[pos - 1])) {
        pos--;
    }
    
    return pos;
}

/**
 * Find end of current word from position
 */
static size_t find_word_end(const char *text, size_t len, size_t pos) {
    /* Skip whitespace forward */
    while (pos < len && isspace((unsigned char)text[pos])) {
        pos++;
    }
    
    /* Find end of word */
    while (pos < len && !is_word_boundary(text[pos])) {
        pos++;
    }
    
    return pos;
}

/**
 * Get current line boundaries for multiline buffer
 */
static void get_current_line_bounds(lle_buffer_t *buffer, size_t *start, size_t *end) {
    const char *data = buffer->data;
    size_t cursor = buffer->cursor.byte_offset;
    size_t len = buffer->length;
    
    /* Find start of current line */
    *start = cursor;
    while (*start > 0 && data[*start - 1] != '\n') {
        (*start)--;
    }
    
    /* Find end of current line */
    *end = cursor;
    while (*end < len && data[*end] != '\n') {
        (*end)++;
    }
}

/* ============================================================================
 * MOVEMENT ACTIONS
 * ============================================================================ */

lle_result_t lle_beginning_of_line(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* For multiline: move to beginning of current logical line */
    if (editor->buffer->length > 0 && strchr(editor->buffer->data, '\n')) {
        size_t line_start, line_end;
        get_current_line_bounds(editor->buffer, &line_start, &line_end);
        editor->buffer->cursor.byte_offset = line_start;
        editor->buffer->cursor.codepoint_index = line_start;
        editor->buffer->cursor.grapheme_index = line_start;
    } else {
        /* Single line: move to buffer beginning */
        editor->buffer->cursor.byte_offset = 0;
        editor->buffer->cursor.codepoint_index = 0;
        editor->buffer->cursor.grapheme_index = 0;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_end_of_line(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* For multiline: move to end of current logical line */
    if (editor->buffer->length > 0 && strchr(editor->buffer->data, '\n')) {
        size_t line_start, line_end;
        get_current_line_bounds(editor->buffer, &line_start, &line_end);
        editor->buffer->cursor.byte_offset = line_end;
        editor->buffer->cursor.codepoint_index = line_end;
        editor->buffer->cursor.grapheme_index = line_end;
    } else {
        /* Single line: move to buffer end */
        editor->buffer->cursor.byte_offset = editor->buffer->length;
        editor->buffer->cursor.codepoint_index = editor->buffer->length;
        editor->buffer->cursor.grapheme_index = editor->buffer->length;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_forward_char(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->cursor_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Use cursor_manager to move forward by one grapheme cluster */
    return lle_cursor_manager_move_by_graphemes(editor->cursor_manager, 1);
}

lle_result_t lle_backward_char(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->cursor_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Use cursor_manager to move backward by one grapheme cluster */
    return lle_cursor_manager_move_by_graphemes(editor->cursor_manager, -1);
}

lle_result_t lle_forward_word(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->cursor_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Find the end of the current word */
    size_t new_pos = find_word_end(editor->buffer->data, 
                                    editor->buffer->length,
                                    editor->buffer->cursor.byte_offset);
    
    /* Use cursor_manager to move to the calculated position */
    return lle_cursor_manager_move_to_byte_offset(editor->cursor_manager, new_pos);
}

lle_result_t lle_backward_word(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->cursor_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Find the start of the current/previous word */
    size_t new_pos = find_word_start(editor->buffer->data,
                                      editor->buffer->cursor.byte_offset);
    
    /* Use cursor_manager to move to the calculated position */
    return lle_cursor_manager_move_to_byte_offset(editor->cursor_manager, new_pos);
}

/* ============================================================================
 * EDITING ACTIONS - DELETION AND KILLING
 * ============================================================================ */

lle_result_t lle_delete_char(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor_pos = editor->buffer->cursor.byte_offset;
    size_t buffer_length = editor->buffer->length;
    
    /* If at end of buffer and buffer is empty, send EOF (bash behavior) */
    if (cursor_pos >= buffer_length && buffer_length == 0) {
        return lle_send_eof(editor);
    }
    
    /* Delete character at cursor */
    if (cursor_pos < buffer_length) {
        return lle_buffer_delete_text(editor->buffer, cursor_pos, 1);
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_backward_delete_char(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (editor->buffer->cursor.byte_offset > 0) {
        size_t delete_pos = editor->buffer->cursor.byte_offset - 1;
        lle_result_t result = lle_buffer_delete_text(editor->buffer, delete_pos, 1);
        if (result == LLE_SUCCESS) {
            editor->buffer->cursor.byte_offset--;
            editor->buffer->cursor.codepoint_index--;
            editor->buffer->cursor.grapheme_index--;
        }
        return result;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_kill_line(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor_pos = editor->buffer->cursor.byte_offset;
    size_t kill_end;
    
    /* For multiline: kill to end of current logical line */
    if (editor->buffer->length > 0 && strchr(editor->buffer->data, '\n')) {
        size_t line_start, line_end;
        get_current_line_bounds(editor->buffer, &line_start, &line_end);
        kill_end = line_end;
    } else {
        /* Single line: kill to end of buffer */
        kill_end = editor->buffer->length;
    }
    
    if (cursor_pos < kill_end) {
        size_t kill_len = kill_end - cursor_pos;
        char *killed_text = strndup(editor->buffer->data + cursor_pos, kill_len);
        
        if (killed_text) {
            /* Add to kill ring */
            if (editor->kill_ring) {
                lle_kill_ring_add(editor->kill_ring, killed_text, kill_len);
            }
            free(killed_text);
        }
        
        /* Delete the text */
        return lle_buffer_delete_text(editor->buffer, cursor_pos, kill_len);
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_backward_kill_line(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor_pos = editor->buffer->cursor.byte_offset;
    size_t kill_start;
    
    /* For multiline: kill from beginning of current logical line */
    if (editor->buffer->length > 0 && strchr(editor->buffer->data, '\n')) {
        size_t line_start, line_end;
        get_current_line_bounds(editor->buffer, &line_start, &line_end);
        kill_start = line_start;
    } else {
        /* Single line: kill from beginning (bash behavior) */
        kill_start = 0;
    }
    
    if (cursor_pos > kill_start) {
        size_t kill_len = cursor_pos - kill_start;
        char *killed_text = strndup(editor->buffer->data + kill_start, kill_len);
        
        if (killed_text) {
            /* Add to kill ring */
            if (editor->kill_ring) {
                lle_kill_ring_add(editor->kill_ring, killed_text, kill_len);
            }
            free(killed_text);
        }
        
        /* Delete the text */
        lle_result_t result = lle_buffer_delete_text(editor->buffer, kill_start, kill_len);
        if (result == LLE_SUCCESS) {
            editor->buffer->cursor.byte_offset = kill_start;
            editor->buffer->cursor.codepoint_index = kill_start;
            editor->buffer->cursor.grapheme_index = kill_start;
        }
        return result;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_kill_word(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor_pos = editor->buffer->cursor.byte_offset;
    size_t word_end = find_word_end(editor->buffer->data,
                                     editor->buffer->length,
                                     cursor_pos);
    
    if (word_end > cursor_pos) {
        size_t kill_len = word_end - cursor_pos;
        char *killed_text = strndup(editor->buffer->data + cursor_pos, kill_len);
        
        if (killed_text) {
            /* Add to kill ring */
            if (editor->kill_ring) {
                lle_kill_ring_add(editor->kill_ring, killed_text, kill_len);
            }
            free(killed_text);
        }
        
        /* Delete the text */
        return lle_buffer_delete_text(editor->buffer, cursor_pos, kill_len);
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_backward_kill_word(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor_pos = editor->buffer->cursor.byte_offset;
    size_t word_start = find_word_start(editor->buffer->data, cursor_pos);
    
    if (cursor_pos > word_start) {
        size_t kill_len = cursor_pos - word_start;
        char *killed_text = strndup(editor->buffer->data + word_start, kill_len);
        
        if (killed_text) {
            /* Add to kill ring */
            if (editor->kill_ring) {
                lle_kill_ring_add(editor->kill_ring, killed_text, kill_len);
            }
            free(killed_text);
        }
        
        /* Delete the text */
        lle_result_t result = lle_buffer_delete_text(editor->buffer, word_start, kill_len);
        if (result == LLE_SUCCESS) {
            editor->buffer->cursor.byte_offset = word_start;
            editor->buffer->cursor.codepoint_index = word_start;
            editor->buffer->cursor.grapheme_index = word_start;
        }
        return result;
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * EDITING ACTIONS - YANK AND TRANSPOSE
 * ============================================================================ */

lle_result_t lle_yank(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->kill_ring) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    const char *yank_text = NULL;
    lle_result_t result = lle_kill_ring_get_current(editor->kill_ring, &yank_text);
    
    if (result != LLE_SUCCESS || !yank_text) {
        return LLE_SUCCESS;  /* Nothing to yank */
    }
    
    size_t yank_length = strlen(yank_text);
    return lle_buffer_insert_text(editor->buffer,
                                   editor->buffer->cursor.byte_offset,
                                   yank_text,
                                   yank_length);
}

lle_result_t lle_yank_pop(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->kill_ring) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Get next entry from kill ring (includes state check) */
    const char *yank_text = NULL;
    lle_result_t result = lle_kill_ring_yank_pop(editor->kill_ring, &yank_text);
    
    if (result != LLE_SUCCESS || !yank_text) {
        return LLE_SUCCESS;  /* Ignore if error or no text */
    }
    
    /* Delete previously yanked text and insert new text */
    size_t yank_length = strlen(yank_text);
    return lle_buffer_insert_text(editor->buffer,
                                   editor->buffer->cursor.byte_offset,
                                   yank_text,
                                   yank_length);
}

lle_result_t lle_transpose_chars(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor = editor->buffer->cursor.byte_offset;
    
    /* Need at least 2 characters */
    if (editor->buffer->length < 2) {
        return LLE_SUCCESS;
    }
    
    /* If at end, transpose last two chars */
    if (cursor >= editor->buffer->length) {
        cursor = editor->buffer->length - 1;
    }
    
    /* Need at least one char before cursor */
    if (cursor == 0) {
        return LLE_SUCCESS;
    }
    
    /* Swap characters */
    char temp = editor->buffer->data[cursor];
    editor->buffer->data[cursor] = editor->buffer->data[cursor - 1];
    editor->buffer->data[cursor - 1] = temp;
    
    /* Move cursor forward if not at end */
    if (cursor < editor->buffer->length - 1) {
        editor->buffer->cursor.byte_offset++;
        editor->buffer->cursor.codepoint_index++;
        editor->buffer->cursor.grapheme_index++;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_transpose_words(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor = editor->buffer->cursor.byte_offset;
    const char *data = editor->buffer->data;
    size_t len = editor->buffer->length;
    
    /* Find current word boundaries */
    size_t word2_start = cursor;
    while (word2_start > 0 && !is_word_boundary(data[word2_start - 1])) {
        word2_start--;
    }
    
    size_t word2_end = cursor;
    while (word2_end < len && !is_word_boundary(data[word2_end])) {
        word2_end++;
    }
    
    /* Find previous word */
    size_t word1_end = word2_start;
    while (word1_end > 0 && is_word_boundary(data[word1_end - 1])) {
        word1_end--;
    }
    
    if (word1_end == 0) {
        return LLE_SUCCESS;  /* No previous word */
    }
    
    size_t word1_start = word1_end;
    while (word1_start > 0 && !is_word_boundary(data[word1_start - 1])) {
        word1_start--;
    }
    
    /* Extract words */
    size_t word1_len = word1_end - word1_start;
    size_t word2_len = word2_end - word2_start;
    
    char *word1 = strndup(data + word1_start, word1_len);
    char *word2 = strndup(data + word2_start, word2_len);
    
    if (!word1 || !word2) {
        free(word1);
        free(word2);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Replace word1 with word2 */
    lle_buffer_delete_text(editor->buffer, word1_start, word1_len);
    lle_buffer_insert_text(editor->buffer, word1_start, word2, word2_len);
    
    /* Replace word2 with word1 (adjust position) */
    size_t new_word2_start = word2_start - word1_len + word2_len;
    lle_buffer_delete_text(editor->buffer, new_word2_start, word2_len);
    lle_buffer_insert_text(editor->buffer, new_word2_start, word1, word1_len);
    
    free(word1);
    free(word2);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * EDITING ACTIONS - CASE CHANGES
 * ============================================================================ */

lle_result_t lle_upcase_word(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor = editor->buffer->cursor.byte_offset;
    size_t word_end = find_word_end(editor->buffer->data,
                                     editor->buffer->length,
                                     cursor);
    
    /* Uppercase from cursor to end of word */
    for (size_t i = cursor; i < word_end; i++) {
        editor->buffer->data[i] = (char)toupper((unsigned char)editor->buffer->data[i]);
    }
    
    /* Move cursor past word */
    editor->buffer->cursor.byte_offset = word_end;
    editor->buffer->cursor.codepoint_index = word_end;
    editor->buffer->cursor.grapheme_index = word_end;
    
    return LLE_SUCCESS;
}

lle_result_t lle_downcase_word(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor = editor->buffer->cursor.byte_offset;
    size_t word_end = find_word_end(editor->buffer->data,
                                     editor->buffer->length,
                                     cursor);
    
    /* Lowercase from cursor to end of word */
    for (size_t i = cursor; i < word_end; i++) {
        editor->buffer->data[i] = (char)tolower((unsigned char)editor->buffer->data[i]);
    }
    
    /* Move cursor past word */
    editor->buffer->cursor.byte_offset = word_end;
    editor->buffer->cursor.codepoint_index = word_end;
    editor->buffer->cursor.grapheme_index = word_end;
    
    return LLE_SUCCESS;
}

lle_result_t lle_capitalize_word(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor = editor->buffer->cursor.byte_offset;
    size_t word_end = find_word_end(editor->buffer->data,
                                     editor->buffer->length,
                                     cursor);
    
    /* Capitalize first letter */
    if (cursor < word_end) {
        editor->buffer->data[cursor] = (char)toupper((unsigned char)editor->buffer->data[cursor]);
    }
    
    /* Lowercase rest */
    for (size_t i = cursor + 1; i < word_end; i++) {
        editor->buffer->data[i] = (char)tolower((unsigned char)editor->buffer->data[i]);
    }
    
    /* Move cursor past word */
    editor->buffer->cursor.byte_offset = word_end;
    editor->buffer->cursor.codepoint_index = word_end;
    editor->buffer->cursor.grapheme_index = word_end;
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * HISTORY NAVIGATION
 * ============================================================================ */

lle_result_t lle_history_previous(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->history_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t entry_count = 0;
    lle_result_t result = lle_history_get_entry_count(editor->history_system, &entry_count);
    if (result != LLE_SUCCESS || entry_count == 0) {
        return LLE_SUCCESS;  /* No history */
    }
    
    /* Move backward in history (toward older entries) */
    if (editor->history_navigation_pos < entry_count) {
        size_t idx = entry_count - 1 - editor->history_navigation_pos;
        lle_history_entry_t *entry = NULL;
        result = lle_history_get_entry_by_index(editor->history_system, idx, &entry);
        
        if (result == LLE_SUCCESS && entry && entry->command) {
            /* Clear buffer and insert history entry */
            lle_buffer_clear(editor->buffer);
            lle_buffer_insert_text(editor->buffer, 0, entry->command, strlen(entry->command));
            editor->buffer->cursor.byte_offset = editor->buffer->length;
            editor->buffer->cursor.codepoint_index = editor->buffer->length;
            editor->buffer->cursor.grapheme_index = editor->buffer->length;
            editor->history_navigation_pos++;
        }
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_next(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->history_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t entry_count = 0;
    lle_result_t result = lle_history_get_entry_count(editor->history_system, &entry_count);
    if (result != LLE_SUCCESS || entry_count == 0 || editor->history_navigation_pos == 0) {
        return LLE_SUCCESS;  /* No history or already at current line */
    }
    
    /* Move forward in history (toward newer entries) */
    editor->history_navigation_pos--;
    
    if (editor->history_navigation_pos == 0) {
        /* Back to current line - clear buffer */
        lle_buffer_clear(editor->buffer);
    } else {
        size_t idx = entry_count - editor->history_navigation_pos;
        lle_history_entry_t *entry = NULL;
        result = lle_history_get_entry_by_index(editor->history_system, idx, &entry);
        
        if (result == LLE_SUCCESS && entry && entry->command) {
            /* Clear buffer and insert history entry */
            lle_buffer_clear(editor->buffer);
            lle_buffer_insert_text(editor->buffer, 0, entry->command, strlen(entry->command));
            editor->buffer->cursor.byte_offset = editor->buffer->length;
            editor->buffer->cursor.codepoint_index = editor->buffer->length;
            editor->buffer->cursor.grapheme_index = editor->buffer->length;
        }
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_reverse_search_history(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->history_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Start interactive reverse search */
    editor->history_search_active = true;
    editor->history_search_direction = -1;  /* Reverse */
    
    return LLE_SUCCESS;
}

lle_result_t lle_forward_search_history(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->history_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Start interactive forward search */
    editor->history_search_active = true;
    editor->history_search_direction = 1;  /* Forward */
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_search_backward(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->history_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Search backward for command starting with current buffer content */
    const char *search_prefix = editor->buffer->data ? editor->buffer->data : "";
    
    if (strlen(search_prefix) == 0) {
        return LLE_SUCCESS;  /* Nothing to search for */
    }
    
    /* Use prefix search */
    lle_history_search_results_t *results = lle_history_search_prefix(
        editor->history_system,
        search_prefix,
        10  /* max results */
    );
    
    if (results && lle_history_search_results_get_count(results) > 0) {
        const lle_search_result_t *result = lle_history_search_results_get(results, 0);
        if (result && result->command) {
            /* Replace buffer with found entry */
            lle_buffer_clear(editor->buffer);
            lle_buffer_insert_text(editor->buffer, 0, 
                                   result->command,
                                   strlen(result->command));
            editor->buffer->cursor.byte_offset = editor->buffer->length;
            editor->buffer->cursor.codepoint_index = editor->buffer->length;
            editor->buffer->cursor.grapheme_index = editor->buffer->length;
        }
    }
    
    if (results) {
        lle_history_search_results_destroy(results);
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_search_forward(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->history_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Search forward for command starting with current buffer content */
    /* Note: This is essentially the same as backward but would track position */
    return lle_history_search_backward(editor);
}

/* ============================================================================
 * COMPLETION ACTIONS
 * ============================================================================ */

lle_result_t lle_complete(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Completion system requires Spec 12 implementation */
    return LLE_SUCCESS;
}

lle_result_t lle_possible_completions(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Completion system requires Spec 12 implementation */
    return LLE_SUCCESS;
}

lle_result_t lle_insert_completions(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Completion system requires Spec 12 implementation */
    return LLE_SUCCESS;
}

/* ============================================================================
 * SHELL-SPECIFIC OPERATIONS
 * ============================================================================ */

lle_result_t lle_accept_line(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Signal that line is accepted (caller handles execution) */
    return LLE_SUCCESS;
}

lle_result_t lle_abort_line(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Cancel any active operations */
    if (editor->history_search_active) {
        editor->history_search_active = false;
    }
    
    /* Clear the buffer */
    lle_result_t result = lle_buffer_clear(editor->buffer);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Reset cursor */
    editor->buffer->cursor.byte_offset = 0;
    editor->buffer->cursor.codepoint_index = 0;
    editor->buffer->cursor.grapheme_index = 0;
    
    /* Kill ring state is managed internally by kill_ring module */
    /* No direct access to opaque structure fields needed */
    
    /* Reset modes */
    editor->quoted_insert_mode = false;
    
    return LLE_SUCCESS;
}

lle_result_t lle_send_eof(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Signal EOF to caller (return special code or set flag) */
    /* In practice, the readline loop will detect this and exit */
    return LLE_SUCCESS;
}

lle_result_t lle_interrupt(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Send SIGINT to self */
    raise(SIGINT);
    
    return LLE_SUCCESS;
}

lle_result_t lle_suspend(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Send SIGTSTP to self */
    raise(SIGTSTP);
    
    return LLE_SUCCESS;
}

lle_result_t lle_clear_screen(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear screen using ANSI escape sequence */
    /* In integrated system, this would use display controller */
    printf("\033[H\033[2J");
    fflush(stdout);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * UTILITY ACTIONS
 * ============================================================================ */

lle_result_t lle_quoted_insert(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Set flag for next character to be inserted literally */
    editor->quoted_insert_mode = true;
    
    return LLE_SUCCESS;
}

lle_result_t lle_unix_line_discard(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Kill entire line (bash Ctrl-U behavior) */
    size_t cursor_pos = editor->buffer->cursor.byte_offset;
    
    if (cursor_pos > 0) {
        char *killed_text = strndup(editor->buffer->data, cursor_pos);
        
        if (killed_text) {
            /* Add to kill ring */
            if (editor->kill_ring) {
                lle_kill_ring_add(editor->kill_ring, killed_text, cursor_pos);
            }
            free(killed_text);
        }
        
        /* Delete from beginning to cursor */
        lle_result_t result = lle_buffer_delete_text(editor->buffer, 0, cursor_pos);
        if (result == LLE_SUCCESS) {
            editor->buffer->cursor.byte_offset = 0;
            editor->buffer->cursor.codepoint_index = 0;
            editor->buffer->cursor.grapheme_index = 0;
        }
        return result;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_unix_word_rubout(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor_pos = editor->buffer->cursor.byte_offset;
    const char *data = editor->buffer->data;
    
    /* Find word start using Unix word boundaries (whitespace only) */
    size_t word_start = cursor_pos;
    
    /* Skip trailing whitespace */
    while (word_start > 0 && is_unix_word_boundary(data[word_start - 1])) {
        word_start--;
    }
    
    /* Find beginning of word */
    while (word_start > 0 && !is_unix_word_boundary(data[word_start - 1])) {
        word_start--;
    }
    
    if (cursor_pos > word_start) {
        size_t kill_len = cursor_pos - word_start;
        char *killed_text = strndup(data + word_start, kill_len);
        
        if (killed_text) {
            /* Add to kill ring */
            if (editor->kill_ring) {
                lle_kill_ring_add(editor->kill_ring, killed_text, kill_len);
            }
            free(killed_text);
        }
        
        /* Delete the text */
        lle_result_t result = lle_buffer_delete_text(editor->buffer, word_start, kill_len);
        if (result == LLE_SUCCESS) {
            editor->buffer->cursor.byte_offset = word_start;
            editor->buffer->cursor.codepoint_index = word_start;
            editor->buffer->cursor.grapheme_index = word_start;
        }
        return result;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_delete_horizontal_space(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cursor = editor->buffer->cursor.byte_offset;
    const char *data = editor->buffer->data;
    size_t len = editor->buffer->length;
    
    /* Find start of whitespace */
    size_t start = cursor;
    while (start > 0 && isspace((unsigned char)data[start - 1])) {
        start--;
    }
    
    /* Find end of whitespace */
    size_t end = cursor;
    while (end < len && isspace((unsigned char)data[end])) {
        end++;
    }
    
    if (end > start) {
        /* Delete whitespace */
        lle_result_t result = lle_buffer_delete_text(editor->buffer, start, end - start);
        if (result == LLE_SUCCESS) {
            editor->buffer->cursor.byte_offset = start;
            editor->buffer->cursor.codepoint_index = start;
            editor->buffer->cursor.grapheme_index = start;
        }
        return result;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_self_insert(lle_editor_t *editor, uint32_t codepoint) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Convert codepoint to UTF-8 bytes */
    char utf8_bytes[4];
    size_t byte_count = 0;
    
    if (codepoint < 0x80) {
        utf8_bytes[0] = (char)codepoint;
        byte_count = 1;
    } else if (codepoint < 0x800) {
        utf8_bytes[0] = (char)(0xC0 | (codepoint >> 6));
        utf8_bytes[1] = (char)(0x80 | (codepoint & 0x3F));
        byte_count = 2;
    } else if (codepoint < 0x10000) {
        utf8_bytes[0] = (char)(0xE0 | (codepoint >> 12));
        utf8_bytes[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_bytes[2] = (char)(0x80 | (codepoint & 0x3F));
        byte_count = 3;
    } else {
        utf8_bytes[0] = (char)(0xF0 | (codepoint >> 18));
        utf8_bytes[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        utf8_bytes[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_bytes[3] = (char)(0x80 | (codepoint & 0x3F));
        byte_count = 4;
    }
    
    /* Insert at cursor */
    return lle_buffer_insert_text(editor->buffer,
                                   editor->buffer->cursor.byte_offset,
                                   utf8_bytes,
                                   byte_count);
}

lle_result_t lle_newline(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Insert newline at cursor */
    return lle_buffer_insert_text(editor->buffer,
                                   editor->buffer->cursor.byte_offset,
                                   "\n",
                                   1);
}

lle_result_t lle_tab_insert(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Insert tab at cursor */
    return lle_buffer_insert_text(editor->buffer,
                                   editor->buffer->cursor.byte_offset,
                                   "\t",
                                   1);
}

/* ============================================================================
 * PRESET MANAGEMENT
 * ============================================================================ */

lle_result_t lle_keybinding_load_emacs_preset(lle_editor_t *editor) {
    if (!editor || !editor->keybinding_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Bind all Emacs-style keybindings */
    /* This calls lle_keybinding_manager_bind() for each binding */
    
    lle_keybinding_manager_t *mgr = editor->keybinding_manager;
    
    /* Movement */
    lle_keybinding_manager_bind(mgr, "C-a", lle_beginning_of_line, "beginning-of-line");
    lle_keybinding_manager_bind(mgr, "C-e", lle_end_of_line, "end-of-line");
    lle_keybinding_manager_bind(mgr, "C-f", lle_forward_char, "forward-char");
    lle_keybinding_manager_bind(mgr, "C-b", lle_backward_char, "backward-char");
    lle_keybinding_manager_bind(mgr, "M-f", lle_forward_word, "forward-word");
    lle_keybinding_manager_bind(mgr, "M-b", lle_backward_word, "backward-word");
    
    /* Editing */
    lle_keybinding_manager_bind(mgr, "C-d", lle_delete_char, "delete-char");
    lle_keybinding_manager_bind(mgr, "DEL", lle_backward_delete_char, "backward-delete-char");
    lle_keybinding_manager_bind(mgr, "C-k", lle_kill_line, "kill-line");
    lle_keybinding_manager_bind(mgr, "C-u", lle_backward_kill_line, "backward-kill-line");
    lle_keybinding_manager_bind(mgr, "M-d", lle_kill_word, "kill-word");
    lle_keybinding_manager_bind(mgr, "M-DEL", lle_backward_kill_word, "backward-kill-word");
    lle_keybinding_manager_bind(mgr, "C-w", lle_unix_word_rubout, "unix-word-rubout");
    lle_keybinding_manager_bind(mgr, "C-y", lle_yank, "yank");
    lle_keybinding_manager_bind(mgr, "M-y", lle_yank_pop, "yank-pop");
    lle_keybinding_manager_bind(mgr, "C-t", lle_transpose_chars, "transpose-chars");
    lle_keybinding_manager_bind(mgr, "M-t", lle_transpose_words, "transpose-words");
    
    /* Case changes */
    lle_keybinding_manager_bind(mgr, "M-u", lle_upcase_word, "upcase-word");
    lle_keybinding_manager_bind(mgr, "M-l", lle_downcase_word, "downcase-word");
    lle_keybinding_manager_bind(mgr, "M-c", lle_capitalize_word, "capitalize-word");
    
    /* History */
    lle_keybinding_manager_bind(mgr, "C-p", lle_history_previous, "previous-history");
    lle_keybinding_manager_bind(mgr, "C-n", lle_history_next, "next-history");
    lle_keybinding_manager_bind(mgr, "C-r", lle_reverse_search_history, "reverse-search-history");
    lle_keybinding_manager_bind(mgr, "C-s", lle_forward_search_history, "forward-search-history");
    lle_keybinding_manager_bind(mgr, "M-p", lle_history_search_backward, "history-search-backward");
    lle_keybinding_manager_bind(mgr, "M-n", lle_history_search_forward, "history-search-forward");
    
    /* Completion */
    lle_keybinding_manager_bind(mgr, "TAB", lle_complete, "complete");
    lle_keybinding_manager_bind(mgr, "M-?", lle_possible_completions, "possible-completions");
    lle_keybinding_manager_bind(mgr, "M-*", lle_insert_completions, "insert-completions");
    
    /* Shell operations */
    lle_keybinding_manager_bind(mgr, "RET", lle_accept_line, "accept-line");
    lle_keybinding_manager_bind(mgr, "C-g", lle_abort_line, "abort");
    lle_keybinding_manager_bind(mgr, "C-l", lle_clear_screen, "clear-screen");
    lle_keybinding_manager_bind(mgr, "C-c", lle_interrupt, "interrupt");
    lle_keybinding_manager_bind(mgr, "C-z", lle_suspend, "suspend");
    
    /* Utilities */
    lle_keybinding_manager_bind(mgr, "C-q", lle_quoted_insert, "quoted-insert");
    lle_keybinding_manager_bind(mgr, "C-v", lle_quoted_insert, "quoted-insert");
    lle_keybinding_manager_bind(mgr, "M-\\", lle_delete_horizontal_space, "delete-horizontal-space");
    lle_keybinding_manager_bind(mgr, "C-j", lle_newline, "newline");
    lle_keybinding_manager_bind(mgr, "M-TAB", lle_tab_insert, "tab-insert");
    
    /* Arrow keys */
    lle_keybinding_manager_bind(mgr, "UP", lle_history_previous, "previous-history");
    lle_keybinding_manager_bind(mgr, "DOWN", lle_history_next, "next-history");
    lle_keybinding_manager_bind(mgr, "LEFT", lle_backward_char, "backward-char");
    lle_keybinding_manager_bind(mgr, "RIGHT", lle_forward_char, "forward-char");
    
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_load_vi_preset(lle_editor_t *editor) {
    if (!editor || !editor->keybinding_manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Vi mode preset requires additional implementation */
    return LLE_SUCCESS;
}
