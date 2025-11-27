/*
 * input_continuation.h - Shared Multiline Input Continuation System
 * 
 * This module provides shared multiline parsing functionality for both the
 * Line Editing Engine (LLE) and the main input system. It handles shell
 * construct detection, quote tracking, bracket counting, and continuation
 * line analysis.
 * 
 * This is NOT owned by LLE but is shared infrastructure used by multiple
 * components of Lusush.
 */

#ifndef INPUT_CONTINUATION_H
#define INPUT_CONTINUATION_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Context types for the context stack
 * Used to track nested shell constructs for proper prompt display
 */
typedef enum {
    CONTEXT_NONE = 0,
    CONTEXT_IF,
    CONTEXT_WHILE,
    CONTEXT_FOR,
    CONTEXT_UNTIL,
    CONTEXT_CASE,
    CONTEXT_FUNCTION,
    CONTEXT_BRACE_GROUP
} continuation_context_type_t;

/*
 * Maximum nesting depth for context stack
 */
#define CONTINUATION_MAX_CONTEXT_DEPTH 32

/*
 * Continuation state structure
 * 
 * Tracks the parsing state for multiline input constructs including:
 * - Quote tracking (single, double, backtick)
 * - Bracket/brace/parenthesis counting
 * - Control structure detection (if/then/fi, case, loops)
 * - Here document handling
 * - Function definitions
 * - Context stack for proper nested construct tracking
 */
typedef struct {
    // Quote tracking
    int quote_count;
    int double_quote_count;
    int backtick_count;
    bool in_single_quote;
    bool in_double_quote;
    bool in_backtick;
    
    // Bracket/brace/parenthesis tracking
    int paren_count;
    int brace_count;
    int bracket_count;
    
    // Escape and continuation
    bool escaped;
    bool has_continuation;
    
    // Here document handling
    bool in_here_doc;
    char *here_doc_delimiter;
    
    // Command substitution and arithmetic
    bool in_command_substitution;
    bool in_arithmetic;
    
    // Control structure tracking (legacy flags - kept for compatibility)
    bool in_function_definition;
    bool in_case_statement;
    bool in_if_statement;
    bool in_while_loop;
    bool in_for_loop;
    bool in_until_loop;
    int compound_command_depth;
    
    // Context stack for nested construct tracking
    // This allows proper prompt switching when entering/exiting nested constructs
    continuation_context_type_t context_stack[CONTINUATION_MAX_CONTEXT_DEPTH];
    int context_stack_depth;
} continuation_state_t;

/*
 * Initialize a continuation state structure
 * 
 * Parameters:
 *   state - Pointer to state structure to initialize
 */
void continuation_state_init(continuation_state_t *state);

/*
 * Clean up a continuation state structure
 * 
 * Frees any dynamically allocated memory (e.g., here_doc_delimiter)
 * and resets the structure to initial state.
 * 
 * Parameters:
 *   state - Pointer to state structure to clean up
 */
void continuation_state_cleanup(continuation_state_t *state);

/*
 * Analyze a line and update the continuation state
 * 
 * Parses the line for shell constructs, quotes, brackets, control
 * keywords, and updates the state accordingly. This is the core
 * parsing function.
 * 
 * Parameters:
 *   line  - The line to analyze (null-terminated string)
 *   state - Pointer to state structure to update
 */
void continuation_analyze_line(const char *line, continuation_state_t *state);

/*
 * Check if the current input is complete
 * 
 * Returns true if all constructs are closed and the input does not
 * require continuation. Returns false if the input needs more lines.
 * 
 * Parameters:
 *   state - Pointer to state structure to check
 * 
 * Returns:
 *   true if input is complete, false if continuation is needed
 */
bool continuation_is_complete(const continuation_state_t *state);

/*
 * Check if the current state needs continuation
 * 
 * Similar to continuation_is_complete but returns the opposite value.
 * This is a convenience function.
 * 
 * Parameters:
 *   state - Pointer to state structure to check
 * 
 * Returns:
 *   true if continuation is needed, false if input is complete
 */
bool continuation_needs_continuation(const continuation_state_t *state);

/*
 * Get an appropriate continuation prompt string
 * 
 * Returns a prompt string based on the current parsing state.
 * The returned string is a static or symbol table string and should
 * not be freed by the caller.
 * 
 * Parameters:
 *   state - Pointer to state structure
 * 
 * Returns:
 *   Prompt string (e.g., "> ", "quote> ", "if> ", etc.)
 */
const char *continuation_get_prompt(const continuation_state_t *state);

/*
 * Check if a word is a shell control keyword
 * 
 * Returns true for keywords like "if", "then", "while", "do", etc.
 * 
 * Parameters:
 *   word - The word to check (null-terminated string)
 * 
 * Returns:
 *   true if word is a control keyword, false otherwise
 */
bool continuation_is_control_keyword(const char *word);

/*
 * Check if a line is a control structure terminator
 * 
 * Returns true for terminators like "fi", "done", "esac", "}".
 * 
 * Parameters:
 *   line - The line to check (null-terminated string)
 * 
 * Returns:
 *   true if line is a terminator, false otherwise
 */
bool continuation_is_terminator(const char *line);

#endif /* INPUT_CONTINUATION_H */
