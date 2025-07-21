/**
 * @file edit_commands.h
 * @brief Basic editing commands for Lusush Line Editor (LLE)
 * 
 * This module provides basic editing command functionality for the line editor,
 * including character insertion, deletion, cursor movement, and line operations.
 * Commands operate on display state and update both text buffer and display.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef LLE_EDIT_COMMANDS_H
#define LLE_EDIT_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "display.h"
#include "input_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Command Type Definitions
// ============================================================================

/**
 * @brief Editing command types
 */
typedef enum {
    LLE_CMD_INSERT_CHAR = 0,    /**< Insert character at cursor position */
    LLE_CMD_DELETE_CHAR,        /**< Delete character at cursor position */
    LLE_CMD_BACKSPACE,          /**< Delete character before cursor position */
    LLE_CMD_MOVE_CURSOR,        /**< Move cursor to new position */
    LLE_CMD_ACCEPT_LINE,        /**< Accept current line (Enter pressed) */
    LLE_CMD_CANCEL_LINE,        /**< Cancel current line (Ctrl+C pressed) */
    LLE_CMD_CLEAR_LINE,         /**< Clear entire line content */
    LLE_CMD_MOVE_HOME,          /**< Move cursor to beginning of line */
    LLE_CMD_MOVE_END,           /**< Move cursor to end of line */
    LLE_CMD_WORD_LEFT,          /**< Move cursor one word left */
    LLE_CMD_WORD_RIGHT,         /**< Move cursor one word right */
    LLE_CMD_DELETE_WORD,        /**< Delete word forward */
    LLE_CMD_BACKSPACE_WORD,     /**< Delete word backward */
    LLE_CMD_KILL_LINE,          /**< Kill from cursor to end of line */
    LLE_CMD_KILL_BEGINNING,     /**< Kill from beginning to cursor */
    LLE_CMD_TYPE_COUNT          /**< Total number of command types */
} lle_command_type_t;

/**
 * @brief Cursor movement directions for editing commands
 */
typedef enum {
    LLE_CMD_CURSOR_LEFT = 0,    /**< Move cursor left */
    LLE_CMD_CURSOR_RIGHT,       /**< Move cursor right */
    LLE_CMD_CURSOR_UP,          /**< Move cursor up (multiline) */
    LLE_CMD_CURSOR_DOWN,        /**< Move cursor down (multiline) */
    LLE_CMD_CURSOR_HOME,        /**< Move cursor to beginning */
    LLE_CMD_CURSOR_END,         /**< Move cursor to end */
    LLE_CMD_CURSOR_WORD_LEFT,   /**< Move cursor one word left */
    LLE_CMD_CURSOR_WORD_RIGHT,  /**< Move cursor one word right */
    LLE_CMD_CURSOR_ABSOLUTE     /**< Move cursor to absolute position */
} lle_cmd_cursor_movement_t;

/**
 * @brief Command execution result codes
 */
typedef enum {
    LLE_CMD_SUCCESS = 0,        /**< Command executed successfully */
    LLE_CMD_ERROR_INVALID_PARAM = -1,  /**< Invalid parameter */
    LLE_CMD_ERROR_INVALID_STATE = -2,  /**< Invalid display state */
    LLE_CMD_ERROR_BUFFER_FULL = -3,    /**< Text buffer is full */
    LLE_CMD_ERROR_INVALID_POSITION = -4, /**< Invalid cursor position */
    LLE_CMD_ERROR_DISPLAY_UPDATE = -5,   /**< Display update failed */
    LLE_CMD_ERROR_UNKNOWN_COMMAND = -6   /**< Unknown command type */
} lle_command_result_t;

// ============================================================================
// Command Data Structures
// ============================================================================

/**
 * @brief Data for character insertion command
 */
typedef struct {
    char character;             /**< Character to insert */
    uint32_t unicode;          /**< Unicode codepoint (for future UTF-8 support) */
} lle_insert_char_data_t;

/**
 * @brief Data for cursor movement command
 */
typedef struct {
    lle_cmd_cursor_movement_t direction; /**< Movement direction */
    size_t count;              /**< Number of positions to move (default 1) */
    size_t absolute_position;  /**< Absolute position (for CURSOR_ABSOLUTE) */
} lle_cursor_move_data_t;

/**
 * @brief Data for line operations
 */
typedef struct {
    bool preserve_content;     /**< Whether to preserve content for undo */
    char *result_buffer;       /**< Buffer to store result line (for ACCEPT) */
    size_t buffer_size;        /**< Size of result buffer */
} lle_line_operation_data_t;

/**
 * @brief Command execution context
 */
typedef struct {
    lle_command_type_t command;     /**< Command type being executed */
    const void *data;               /**< Command-specific data */
    lle_display_state_t *state;     /**< Display state to operate on */
    lle_command_result_t result;    /**< Execution result */
    bool display_updated;           /**< Whether display was updated */
    size_t affected_offset;         /**< Offset of affected text */
    size_t affected_length;         /**< Length of affected text */
} lle_command_context_t;

// ============================================================================
// Core Command Execution Functions
// ============================================================================

/**
 * @brief Execute a basic editing command
 * @param state Display state to operate on
 * @param cmd Command type to execute
 * @param data Command-specific data (may be NULL for some commands)
 * @return Command execution result code
 */
lle_command_result_t lle_execute_command(
    lle_display_state_t *state,
    lle_command_type_t cmd,
    const void *data
);

/**
 * @brief Execute command with full context
 * @param context Command execution context
 * @return Command execution result code
 */
lle_command_result_t lle_execute_command_with_context(lle_command_context_t *context);

/**
 * @brief Check if command can be executed on current state
 * @param state Display state to check
 * @param cmd Command type to check
 * @return true if command can be executed, false otherwise
 */
bool lle_can_execute_command(const lle_display_state_t *state, lle_command_type_t cmd);

// ============================================================================
// Character Operation Functions
// ============================================================================

/**
 * @brief Insert character at cursor position
 * @param state Display state to operate on
 * @param character Character to insert
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_insert_char(lle_display_state_t *state, char character);

/**
 * @brief Delete character at cursor position
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_delete_char(lle_display_state_t *state);

/**
 * @brief Delete character before cursor (backspace)
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_backspace(lle_display_state_t *state);

// ============================================================================
// Cursor Movement Functions
// ============================================================================

/**
 * @brief Move cursor in specified direction
 * @param state Display state to operate on
 * @param direction Movement direction
 * @param count Number of positions to move (default 1)
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_move_cursor(
    lle_display_state_t *state,
    lle_cmd_cursor_movement_t direction,
    size_t count
);

/**
 * @brief Move cursor to absolute position
 * @param state Display state to operate on
 * @param position Absolute cursor position
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_set_cursor_position(
    lle_display_state_t *state,
    size_t position
);

/**
 * @brief Move cursor to beginning of line
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_move_home(lle_display_state_t *state);

/**
 * @brief Move cursor to end of line
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_move_end(lle_display_state_t *state);

// ============================================================================
// Word Operation Functions
// ============================================================================

/**
 * @brief Move cursor one word left
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_word_left(lle_display_state_t *state);

/**
 * @brief Move cursor one word right
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_word_right(lle_display_state_t *state);

/**
 * @brief Delete word forward from cursor
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_delete_word(lle_display_state_t *state);

/**
 * @brief Delete word backward from cursor
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_backspace_word(lle_display_state_t *state);

// ============================================================================
// Line Operation Functions
// ============================================================================

/**
 * @brief Accept current line (Enter key functionality)
 * @param state Display state to operate on
 * @param result_buffer Buffer to store accepted line
 * @param buffer_size Size of result buffer
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_accept_line(
    lle_display_state_t *state,
    char *result_buffer,
    size_t buffer_size
);

/**
 * @brief Cancel current line (Ctrl+C functionality)
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_cancel_line(lle_display_state_t *state);

/**
 * @brief Clear entire line content
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_clear_line(lle_display_state_t *state);

/**
 * @brief Kill text from cursor to end of line
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_kill_line(lle_display_state_t *state);

/**
 * @brief Kill text from beginning of line to cursor
 * @param state Display state to operate on
 * @return Command execution result code
 */
lle_command_result_t lle_cmd_kill_beginning(lle_display_state_t *state);

// ============================================================================
// Key-to-Command Mapping Functions
// ============================================================================

/**
 * @brief Map key event to editing command
 * @param event Key event to map
 * @param cmd Pointer to store mapped command type
 * @return true if key maps to a command, false otherwise
 */
bool lle_map_key_to_command(const lle_key_event_t *event, lle_command_type_t *cmd);

/**
 * @brief Check if key event represents a printable character
 * @param event Key event to check
 * @return true if key represents printable character, false otherwise
 */
bool lle_key_is_printable_character(const lle_key_event_t *event);

/**
 * @brief Extract character from key event for insertion
 * @param event Key event to extract from
 * @param character Pointer to store extracted character
 * @return true if character extracted successfully, false otherwise
 */
bool lle_extract_character_from_key(const lle_key_event_t *event, char *character);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Get human-readable name of command type
 * @param cmd Command type
 * @return Command name string, or "UNKNOWN" if invalid
 */
const char *lle_command_type_to_string(lle_command_type_t cmd);

/**
 * @brief Get human-readable description of command result
 * @param result Command result code
 * @return Result description string
 */
const char *lle_command_result_to_string(lle_command_result_t result);

/**
 * @brief Initialize command context structure
 * @param context Context to initialize
 * @param state Display state
 * @param cmd Command type
 * @param data Command data
 * @return true on success, false on invalid parameters
 */
bool lle_command_context_init(
    lle_command_context_t *context,
    lle_display_state_t *state,
    lle_command_type_t cmd,
    const void *data
);

/**
 * @brief Validate command parameters
 * @param state Display state
 * @param cmd Command type
 * @param data Command data
 * @return true if parameters are valid, false otherwise
 */
bool lle_validate_command_parameters(
    const lle_display_state_t *state,
    lle_command_type_t cmd,
    const void *data
);

// ============================================================================
// Constants and Defaults
// ============================================================================

/** @brief Maximum number of characters that can be inserted in one command */
#define LLE_MAX_INSERT_LENGTH 1024

/** @brief Maximum number of positions cursor can move in one command */
#define LLE_MAX_CURSOR_MOVE_COUNT 1000

/** @brief Default cursor movement count */
#define LLE_DEFAULT_CURSOR_MOVE_COUNT 1

/** @brief Success return value for boolean command functions */
#define LLE_CMD_SUCCESS_BOOL true

/** @brief Failure return value for boolean command functions */
#define LLE_CMD_FAILURE_BOOL false

#ifdef __cplusplus
}
#endif

#endif // LLE_EDIT_COMMANDS_H