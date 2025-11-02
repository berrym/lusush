/**
 * keybinding.h - Keybinding Engine for GNU Readline Compatibility
 *
 * Implements fast key sequence lookup and binding management for interactive
 * line editing. Uses hashtable for O(1) lookup performance (<50μs requirement).
 *
 * Key Features:
 * - Fast key sequence lookup using libhashtable
 * - Multi-key sequences (chords) support
 * - GNU Readline key notation (C-a, M-f, etc.)
 * - Mode-specific bindings (emacs/vi)
 * - Function pointer dispatch
 *
 * GNU Readline Compatibility:
 * - C-a, C-e (beginning/end of line)
 * - M-f, M-b (forward/backward word)
 * - C-k, C-y, M-y (kill/yank)
 * - C-r, C-s (reverse/forward search)
 * - Multi-key sequences (C-x C-s, etc.)
 *
 * Specification: docs/lle_specification/critical_gaps/25_default_keybindings_complete.md
 * Implementation Plan: docs/lle_specification/critical_gaps/25_IMPLEMENTATION_PLAN.md
 * Date: 2025-11-02
 */

#ifndef LLE_KEYBINDING_H
#define LLE_KEYBINDING_H

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct lle_keybinding_manager lle_keybinding_manager_t;
typedef struct lle_editor lle_editor_t;

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/**
 * Maximum length of a key sequence string
 * Example: "C-x C-s" = 7 characters
 */
#define LLE_MAX_KEY_SEQUENCE_LENGTH 64

/**
 * Maximum number of keys in a sequence (chord)
 * Example: C-x C-s = 2 keys
 */
#define LLE_MAX_SEQUENCE_KEYS 4

/**
 * Performance requirement: key lookup must complete within 50 microseconds
 */
#define LLE_KEYBINDING_LOOKUP_MAX_US 50

/**
 * Default initial hashtable size for keybindings
 */
#define LLE_KEYBINDING_INITIAL_SIZE 128

/* ============================================================================
 * TYPES
 * ============================================================================ */

/**
 * Keybinding action function signature
 * 
 * All keybinding actions take an editor context and return a result code.
 * The editor context provides access to buffer, history, kill ring, etc.
 *
 * @param editor Editor context (contains buffer, history, kill ring, etc.)
 * @return LLE_SUCCESS or error code
 */
typedef lle_result_t (*lle_keybinding_action_t)(lle_editor_t *editor);

/**
 * Keybinding mode
 */
typedef enum {
    LLE_KEYMAP_EMACS,      /* GNU Emacs keybindings (default) */
    LLE_KEYMAP_VI_INSERT,  /* Vi insert mode */
    LLE_KEYMAP_VI_COMMAND, /* Vi command mode */
    LLE_KEYMAP_CUSTOM      /* User-defined keybindings */
} lle_keymap_mode_t;

/**
 * Key event structure
 * Represents a single keypress or special key
 */
typedef struct {
    uint32_t codepoint;    /* Unicode codepoint for regular keys */
    bool ctrl;             /* Ctrl modifier */
    bool alt;              /* Alt/Meta modifier */
    bool shift;            /* Shift modifier */
    bool is_special;       /* True for arrow keys, function keys, etc. */
    uint32_t special_key;  /* Special key code (if is_special) */
} lle_key_event_t;

/**
 * Special key codes
 */
typedef enum {
    LLE_KEY_NONE = 0,
    LLE_KEY_ENTER,
    LLE_KEY_TAB,
    LLE_KEY_BACKSPACE,
    LLE_KEY_DELETE,
    LLE_KEY_UP,
    LLE_KEY_DOWN,
    LLE_KEY_LEFT,
    LLE_KEY_RIGHT,
    LLE_KEY_HOME,
    LLE_KEY_END,
    LLE_KEY_PAGEUP,
    LLE_KEY_PAGEDOWN,
    LLE_KEY_INSERT,
    LLE_KEY_ESCAPE,
    LLE_KEY_F1,
    LLE_KEY_F2,
    LLE_KEY_F3,
    LLE_KEY_F4,
    LLE_KEY_F5,
    LLE_KEY_F6,
    LLE_KEY_F7,
    LLE_KEY_F8,
    LLE_KEY_F9,
    LLE_KEY_F10,
    LLE_KEY_F11,
    LLE_KEY_F12
} lle_special_key_t;

/**
 * Keybinding information structure
 * Used for introspection and debugging
 */
typedef struct {
    char key_sequence[LLE_MAX_KEY_SEQUENCE_LENGTH];
    lle_keybinding_action_t action;
    const char *function_name;
    lle_keymap_mode_t mode;
} lle_keybinding_info_t;

/* ============================================================================
 * LIFECYCLE FUNCTIONS
 * ============================================================================ */

/**
 * Create a new keybinding manager
 *
 * @param manager Output pointer for created manager
 * @param pool Memory pool for allocations (NULL = use global pool)
 * @return LLE_SUCCESS or error code
 *
 * @note Initial size is LLE_KEYBINDING_INITIAL_SIZE (128 bindings)
 * @note Hashtable will grow automatically as bindings are added
 */
lle_result_t lle_keybinding_manager_create(
    lle_keybinding_manager_t **manager
);

/**
 * Destroy a keybinding manager and free all resources
 *
 * @param manager Manager to destroy
 * @return LLE_SUCCESS or error code
 *
 * @note All registered bindings are freed
 * @note After this call, manager pointer is invalid
 */
lle_result_t lle_keybinding_manager_destroy(
    lle_keybinding_manager_t *manager
);

/* ============================================================================
 * KEYBINDING REGISTRATION
 * ============================================================================ */

/**
 * Bind a key sequence to an action function
 *
 * @param manager Keybinding manager
 * @param key_sequence Key sequence in GNU Readline notation
 * @param action Action function to execute
 * @param function_name Human-readable function name (for introspection)
 * @return LLE_SUCCESS or error code
 *
 * Key Sequence Format (GNU Readline notation):
 * - "C-a" = Ctrl-A
 * - "M-f" = Meta-F (Alt-F or ESC f)
 * - "C-x C-s" = Ctrl-X Ctrl-S (multi-key sequence)
 * - "UP", "DOWN", "LEFT", "RIGHT" = Arrow keys
 * - "RET", "TAB", "DEL" = Enter, Tab, Delete
 * - "F1" through "F12" = Function keys
 *
 * @note If key_sequence is already bound, overwrites previous binding
 * @note function_name is optional (can be NULL)
 * @note Multi-key sequences are separated by spaces
 */
lle_result_t lle_keybinding_manager_bind(
    lle_keybinding_manager_t *manager,
    const char *key_sequence,
    lle_keybinding_action_t action,
    const char *function_name
);

/**
 * Unbind a key sequence
 *
 * @param manager Keybinding manager
 * @param key_sequence Key sequence to unbind
 * @return LLE_SUCCESS, LLE_ERROR_NOT_FOUND, or other error code
 *
 * @note Returns LLE_ERROR_NOT_FOUND if sequence is not bound
 */
lle_result_t lle_keybinding_manager_unbind(
    lle_keybinding_manager_t *manager,
    const char *key_sequence
);

/**
 * Clear all keybindings
 *
 * @param manager Keybinding manager
 * @return LLE_SUCCESS or error code
 *
 * @note Removes all bindings from all modes
 */
lle_result_t lle_keybinding_manager_clear(
    lle_keybinding_manager_t *manager
);

/* ============================================================================
 * KEY PROCESSING
 * ============================================================================ */

/**
 * Process a key event and execute bound action
 *
 * @param manager Keybinding manager
 * @param editor Editor context
 * @param key_event Key event to process
 * @return LLE_SUCCESS, LLE_ERROR_NOT_FOUND, or action result code
 *
 * Behavior:
 * - Looks up key_event in current keymap
 * - If found, calls bound action function with editor context
 * - If not found, returns LLE_ERROR_NOT_FOUND
 * - For multi-key sequences, buffers keys until sequence completes
 *
 * @note Returns LLE_ERROR_NOT_FOUND if key is not bound
 * @note Multi-key sequences timeout after 1 second of inactivity
 */
lle_result_t lle_keybinding_manager_process_key(
    lle_keybinding_manager_t *manager,
    lle_editor_t *editor,
    const lle_key_event_t *key_event
);

/**
 * Reset multi-key sequence state
 *
 * @param manager Keybinding manager
 * @return LLE_SUCCESS or error code
 *
 * @note Call this after any non-key operation to reset sequence buffer
 * @note Automatically called on timeout (1 second)
 */
lle_result_t lle_keybinding_manager_reset_sequence(
    lle_keybinding_manager_t *manager
);

/* ============================================================================
 * KEYMAP MODE MANAGEMENT
 * ============================================================================ */

/**
 * Set the active keymap mode
 *
 * @param manager Keybinding manager
 * @param mode Keymap mode to activate
 * @return LLE_SUCCESS or error code
 *
 * @note Default mode is LLE_KEYMAP_EMACS
 * @note Switching modes changes which bindings are active
 */
lle_result_t lle_keybinding_manager_set_mode(
    lle_keybinding_manager_t *manager,
    lle_keymap_mode_t mode
);

/**
 * Get the active keymap mode
 *
 * @param manager Keybinding manager
 * @param mode_out Output pointer for current mode
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_keybinding_manager_get_mode(
    lle_keybinding_manager_t *manager,
    lle_keymap_mode_t *mode_out
);

/* ============================================================================
 * PRESET LOADING
 * ============================================================================ */

/**
 * Load GNU Emacs preset keybindings
 *
 * @param manager Keybinding manager
 * @return LLE_SUCCESS or error code
 *
 * Loads 40+ GNU Readline/Emacs bindings:
 * - Movement: C-a, C-e, C-f, C-b, M-f, M-b
 * - Editing: C-d, DEL, C-k, C-u, C-w, M-d
 * - Kill/Yank: C-k, C-y, M-y
 * - History: C-p, C-n, C-r, C-s
 * - Completion: TAB, M-?
 * - Special: C-l, C-g, RET
 *
 * @note Overwrites existing bindings if conflicts
 * @note Sets mode to LLE_KEYMAP_EMACS
 */
lle_result_t lle_keybinding_manager_load_emacs_preset(
    lle_keybinding_manager_t *manager
);

/**
 * Load Vi insert mode preset keybindings
 *
 * @param manager Keybinding manager
 * @return LLE_SUCCESS or error code
 *
 * @note Sets mode to LLE_KEYMAP_VI_INSERT
 * @note Vi command mode bindings will be added in future phase
 */
lle_result_t lle_keybinding_manager_load_vi_insert_preset(
    lle_keybinding_manager_t *manager
);

/* ============================================================================
 * INTROSPECTION
 * ============================================================================ */

/**
 * List all active keybindings
 *
 * @param manager Keybinding manager
 * @param bindings_out Output pointer for bindings array
 * @param count_out Output pointer for count
 * @return LLE_SUCCESS or error code
 *
 * @note Caller must free bindings array with lle_pool_free
 * @note Only returns bindings for current active mode
 */
lle_result_t lle_keybinding_manager_list_bindings(
    lle_keybinding_manager_t *manager,
    lle_keybinding_info_t **bindings_out,
    size_t *count_out
);

/**
 * Lookup action for a key sequence
 *
 * @param manager Keybinding manager
 * @param key_sequence Key sequence to lookup
 * @param action_out Output pointer for action function
 * @return LLE_SUCCESS, LLE_ERROR_NOT_FOUND, or error code
 *
 * @note Does not execute the action, only returns function pointer
 * @note Returns LLE_ERROR_NOT_FOUND if sequence is not bound
 */
lle_result_t lle_keybinding_manager_lookup(
    lle_keybinding_manager_t *manager,
    const char *key_sequence,
    lle_keybinding_action_t *action_out
);

/**
 * Get count of registered keybindings
 *
 * @param manager Keybinding manager
 * @param count_out Output pointer for count
 * @return LLE_SUCCESS or error code
 *
 * @note Returns count for current active mode only
 */
lle_result_t lle_keybinding_manager_get_count(
    lle_keybinding_manager_t *manager,
    size_t *count_out
);

/* ============================================================================
 * KEY SEQUENCE PARSING UTILITIES
 * ============================================================================ */

/**
 * Parse GNU Readline key sequence notation to key event
 *
 * @param key_sequence Key sequence string (e.g., "C-a", "M-f")
 * @param key_event_out Output pointer for parsed key event
 * @return LLE_SUCCESS, LLE_ERROR_INVALID_FORMAT, or error code
 *
 * Supported Formats:
 * - "C-x" = Ctrl+x
 * - "M-x" = Alt+x (Meta)
 * - "C-M-x" = Ctrl+Alt+x
 * - "UP", "DOWN", "LEFT", "RIGHT"
 * - "RET", "TAB", "DEL", "ESC"
 * - "F1" through "F12"
 * - "a" through "z" = Regular characters
 *
 * @note Only parses single key, not multi-key sequences
 * @note Multi-key sequences must be parsed key by key
 */
lle_result_t lle_key_sequence_parse(
    const char *key_sequence,
    lle_key_event_t *key_event_out
);

/**
 * Convert key event to GNU Readline notation string
 *
 * @param key_event Key event to convert
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return LLE_SUCCESS, LLE_ERROR_BUFFER_OVERFLOW, or error code
 *
 * @note Buffer should be at least LLE_MAX_KEY_SEQUENCE_LENGTH bytes
 */
lle_result_t lle_key_event_to_string(
    const lle_key_event_t *key_event,
    char *buffer,
    size_t buffer_size
);

/* ============================================================================
 * PERFORMANCE MONITORING
 * ============================================================================ */

/**
 * Get keybinding lookup statistics
 *
 * @param manager Keybinding manager
 * @param avg_lookup_time_us Output pointer for average lookup time (microseconds)
 * @param max_lookup_time_us Output pointer for maximum lookup time (microseconds)
 * @return LLE_SUCCESS or error code
 *
 * @note Statistics reset on manager creation
 * @note Useful for verifying <50μs performance requirement
 */
lle_result_t lle_keybinding_manager_get_stats(
    lle_keybinding_manager_t *manager,
    uint64_t *avg_lookup_time_us,
    uint64_t *max_lookup_time_us
);

/**
 * Reset performance statistics
 *
 * @param manager Keybinding manager
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_keybinding_manager_reset_stats(
    lle_keybinding_manager_t *manager
);

#endif /* LLE_KEYBINDING_H */
