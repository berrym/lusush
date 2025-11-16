/**
 * keybinding_actions.h - Default Keybinding Action Functions
 *
 * Implements all 42 GNU Readline compatible keybinding actions for
 * interactive line editing. Provides 100% compatibility with standard
 * Emacs-style keybindings used in bash, zsh, and other shells.
 *
 * Key Features:
 * - Complete GNU Readline function set
 * - Movement operations (beginning/end of line, word navigation)
 * - Editing operations (kill/yank, transpose, case changes)
 * - History navigation (previous/next, search)
 * - Completion operations (tab completion, listing)
 * - Shell-specific operations (EOF, suspend, clear screen)
 * - Multiline command support for all operations
 *
 * Performance:
 * - All actions complete within 100 microseconds
 * - Optimized for interactive use
 *
 * Specification: docs/lle_specification/critical_gaps/25_default_keybindings_complete.md
 * Date: 2025-11-02
 */

#ifndef LLE_KEYBINDING_ACTIONS_H
#define LLE_KEYBINDING_ACTIONS_H

#include "lle/lle_editor.h"
#include "lle/error_handling.h"

/* ============================================================================
 * MOVEMENT ACTIONS
 * ============================================================================ */

/**
 * Move cursor to beginning of line (Ctrl-A)
 * For multiline: move to beginning of current logical line
 */
lle_result_t lle_beginning_of_line(lle_editor_t *editor);

/**
 * Move cursor to end of line (Ctrl-E)
 * For multiline: move to end of current logical line
 */
lle_result_t lle_end_of_line(lle_editor_t *editor);

/**
 * Move cursor forward one character (Ctrl-F, RIGHT arrow)
 * Handles multi-byte UTF-8 characters correctly
 */
lle_result_t lle_forward_char(lle_editor_t *editor);

/**
 * Move cursor backward one character (Ctrl-B, LEFT arrow)
 * Handles multi-byte UTF-8 characters correctly
 */
lle_result_t lle_backward_char(lle_editor_t *editor);

/**
 * Move cursor forward one word (Meta-F)
 * Word boundary: whitespace and shell metacharacters
 */
lle_result_t lle_forward_word(lle_editor_t *editor);

/**
 * Move cursor backward one word (Meta-B)
 * Word boundary: whitespace and shell metacharacters
 */
lle_result_t lle_backward_word(lle_editor_t *editor);

/* ============================================================================
 * EDITING ACTIONS - DELETION AND KILLING
 * ============================================================================ */

/**
 * Delete character at cursor (Ctrl-D)
 * If buffer empty at end, sends EOF
 */
lle_result_t lle_delete_char(lle_editor_t *editor);

/**
 * Delete character before cursor (DEL, Backspace)
 */
lle_result_t lle_backward_delete_char(lle_editor_t *editor);

/**
 * Kill from cursor to end of line (Ctrl-K)
 * For multiline: kill to end of current logical line
 * Text added to kill ring
 */
lle_result_t lle_kill_line(lle_editor_t *editor);

/**
 * Kill from beginning of line to cursor (Ctrl-U)
 * For multiline: kill from beginning of current logical line
 * Text added to kill ring
 */
lle_result_t lle_backward_kill_line(lle_editor_t *editor);

/**
 * Kill from cursor to end of current word (Meta-D)
 * Text added to kill ring
 */
lle_result_t lle_kill_word(lle_editor_t *editor);

/**
 * Kill from cursor to beginning of current word (Meta-DEL, Ctrl-W)
 * Text added to kill ring
 */
lle_result_t lle_backward_kill_word(lle_editor_t *editor);

/* ============================================================================
 * EDITING ACTIONS - YANK AND TRANSPOSE
 * ============================================================================ */

/**
 * Yank (paste) most recent kill (Ctrl-Y)
 * Inserts text from kill ring at cursor
 */
lle_result_t lle_yank(lle_editor_t *editor);

/**
 * Cycle through kill ring (Meta-Y)
 * Must be preceded by yank or yank-pop
 */
lle_result_t lle_yank_pop(lle_editor_t *editor);

/**
 * Transpose characters (Ctrl-T)
 * Swaps character at cursor with previous character
 */
lle_result_t lle_transpose_chars(lle_editor_t *editor);

/**
 * Transpose words (Meta-T)
 * Swaps current word with previous word
 */
lle_result_t lle_transpose_words(lle_editor_t *editor);

/* ============================================================================
 * EDITING ACTIONS - CASE CHANGES
 * ============================================================================ */

/**
 * Convert word to uppercase (Meta-U)
 * Uppercases from cursor to end of word, moves cursor past word
 */
lle_result_t lle_upcase_word(lle_editor_t *editor);

/**
 * Convert word to lowercase (Meta-L)
 * Lowercases from cursor to end of word, moves cursor past word
 */
lle_result_t lle_downcase_word(lle_editor_t *editor);

/**
 * Capitalize word (Meta-C)
 * Capitalizes first letter, lowercases rest, moves cursor past word
 */
lle_result_t lle_capitalize_word(lle_editor_t *editor);

/* ============================================================================
 * HISTORY NAVIGATION
 * ============================================================================ */

/**
 * Previous history entry (Ctrl-P, UP arrow)
 * Loads previous command from history into buffer
 */
lle_result_t lle_history_previous(lle_editor_t *editor);

/**
 * Next history entry (Ctrl-N, DOWN arrow)
 * Loads next command from history into buffer
 */
lle_result_t lle_history_next(lle_editor_t *editor);

/**
 * Reverse incremental search (Ctrl-R)
 * Interactive search backward through history
 */
lle_result_t lle_reverse_search_history(lle_editor_t *editor);

/**
 * Forward incremental search (Ctrl-S)
 * Interactive search forward through history
 */
lle_result_t lle_forward_search_history(lle_editor_t *editor);

/**
 * History search backward (Meta-P)
 * Search backward for command starting with current buffer content
 */
lle_result_t lle_history_search_backward(lle_editor_t *editor);

/**
 * History search forward (Meta-N)
 * Search forward for command starting with current buffer content
 */
lle_result_t lle_history_search_forward(lle_editor_t *editor);

/* ============================================================================
 * LINE AND BUFFER NAVIGATION
 * ============================================================================ */

/**
 * Previous line (multiline buffer navigation)
 * Moves cursor up one line while preserving column position via sticky_column
 * Returns success with no-op if already on first line
 */
lle_result_t lle_previous_line(lle_editor_t *editor);

/**
 * Next line (multiline buffer navigation)
 * Moves cursor down one line while preserving column position via sticky_column
 * Returns success with no-op if already on last line
 */
lle_result_t lle_next_line(lle_editor_t *editor);

/**
 * Beginning of buffer (Alt-<)
 * Moves cursor to position 0 (start of entire buffer)
 * Clears sticky column
 */
lle_result_t lle_beginning_of_buffer(lle_editor_t *editor);

/**
 * End of buffer (Alt->)
 * Moves cursor to end of entire buffer
 * Clears sticky column
 */
lle_result_t lle_end_of_buffer(lle_editor_t *editor);

/**
 * Smart up arrow (context-aware navigation)
 * Single-line mode: Navigate history backward (like Ctrl-P)
 * Multi-line mode: Navigate to previous line in buffer
 */
lle_result_t lle_smart_up_arrow(lle_editor_t *editor);

/**
 * Smart down arrow (context-aware navigation)
 * Single-line mode: Navigate history forward (like Ctrl-N)
 * Multi-line mode: Navigate to next line in buffer
 */
lle_result_t lle_smart_down_arrow(lle_editor_t *editor);

/* ============================================================================
 * COMPLETION ACTIONS
 * ============================================================================ */

/**
 * Complete current word (TAB)
 * Attempts to complete command, filename, or variable
 */
lle_result_t lle_complete(lle_editor_t *editor);

/**
 * List possible completions (Meta-?)
 * Shows all possible completions for current word
 */
lle_result_t lle_possible_completions(lle_editor_t *editor);

/**
 * Insert all completions (Meta-*)
 * Inserts all possible completions at cursor
 */
lle_result_t lle_insert_completions(lle_editor_t *editor);

/* ============================================================================
 * SHELL-SPECIFIC OPERATIONS
 * ============================================================================ */

/**
 * Accept line (Return, Ctrl-M, Ctrl-J)
 * Accepts current line for execution
 * For multiline: checks if command is complete before accepting
 * 
 * NOTE: This is a legacy simple action that sets the line_accepted flag.
 * New code should use lle_accept_line_context() for direct readline integration.
 */
lle_result_t lle_accept_line(lle_editor_t *editor);

/**
 * Accept line with context (Return, Ctrl-M, Ctrl-J) - Context-Aware Action
 * 
 * Full readline-aware ENTER key handler with direct access to readline context.
 * Checks for incomplete input using continuation state, handles history integration,
 * and directly manages readline completion (done/final_line).
 * 
 * Behavior:
 * - Incomplete input: Inserts newline, syncs cursor, refreshes display, continues editing
 * - Complete input: Adds to history, sets done=true, returns final line
 * 
 * This is a context-aware action that requires readline_context_t access.
 * See docs/lle_implementation/DUAL_ACTION_ARCHITECTURE.md for architecture details.
 * 
 * @param ctx Readline context with full access to buffer, history, completion state
 * @return LLE_SUCCESS on successful handling
 */
lle_result_t lle_accept_line_context(struct readline_context *ctx);

/**
 * Abort current operation (Ctrl-G)
 * Clears buffer, cancels search/completion, resets to clean state
 * 
 * NOTE: This is a legacy simple action. New code should use
 * lle_abort_line_context() for direct readline integration.
 */
lle_result_t lle_abort_line(lle_editor_t *editor);

/**
 * Abort line with context (Ctrl-G) - Context-Aware Action
 * 
 * Emacs-style abort that cancels current input and returns empty line to shell.
 * Directly manages readline completion state (done/final_line) without using flags.
 * 
 * This eliminates the abort_requested flag pattern which caused state persistence
 * bugs across readline sessions.
 * 
 * @param ctx Readline context with full access to done/final_line
 * @return LLE_SUCCESS
 */
lle_result_t lle_abort_line_context(struct readline_context *ctx);

/**
 * Send EOF (Ctrl-D on empty line)
 * Signals end of input
 */
lle_result_t lle_send_eof(lle_editor_t *editor);

/**
 * Interrupt (Ctrl-C)
 * Sends SIGINT signal
 */
lle_result_t lle_interrupt(lle_editor_t *editor);

/**
 * Suspend shell (Ctrl-Z)
 * Sends SIGTSTP signal
 */
lle_result_t lle_suspend(lle_editor_t *editor);

/**
 * Clear screen (Ctrl-L)
 * Clears terminal screen and redraws prompt/buffer
 */
lle_result_t lle_clear_screen(lle_editor_t *editor);

/* ============================================================================
 * UTILITY ACTIONS
 * ============================================================================ */

/**
 * Quoted insert (Ctrl-Q, Ctrl-V)
 * Next character inserted literally (ignores keybindings)
 */
lle_result_t lle_quoted_insert(lle_editor_t *editor);

/**
 * Unix line discard (Ctrl-U in bash mode)
 * Kills entire line (alternative binding for backward-kill-line)
 */
lle_result_t lle_unix_line_discard(lle_editor_t *editor);

/**
 * Unix word rubout (Ctrl-W)
 * Kills word backward using Unix word boundaries (whitespace only)
 */
lle_result_t lle_unix_word_rubout(lle_editor_t *editor);

/**
 * Delete horizontal space (Meta-\)
 * Deletes all whitespace around cursor
 */
lle_result_t lle_delete_horizontal_space(lle_editor_t *editor);

/**
 * Self-insert (default action for printable characters)
 * Inserts the character into the buffer
 */
lle_result_t lle_self_insert(lle_editor_t *editor, uint32_t codepoint);

/**
 * Newline (Ctrl-J, in multiline editing)
 * Inserts a newline character into the buffer
 */
lle_result_t lle_newline(lle_editor_t *editor);

/**
 * Insert literal newline (Shift-Enter, Alt-Enter)
 * Inserts a newline at cursor position regardless of completion status.
 * Useful for editing complete multiline commands when adding lines in the middle.
 */
lle_result_t lle_insert_newline_literal(lle_editor_t *editor);

/**
 * Tab insert (Meta-TAB)
 * Inserts a literal tab character
 */
lle_result_t lle_tab_insert(lle_editor_t *editor);

/* ============================================================================
 * PRESET MANAGEMENT
 * ============================================================================ */

/**
 * Load Emacs preset keybindings
 * Binds all GNU Readline Emacs-style keybindings
 */
lle_result_t lle_keybinding_load_emacs_preset(lle_editor_t *editor);

/**
 * Load Vi preset keybindings
 * Binds all Vi-style keybindings (insert and command modes)
 */
lle_result_t lle_keybinding_load_vi_preset(lle_editor_t *editor);

#endif /* LLE_KEYBINDING_ACTIONS_H */
