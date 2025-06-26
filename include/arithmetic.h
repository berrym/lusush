#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

/**
 * Arithmetic Expansion Module
 *
 * Provides POSIX-compliant arithmetic expansion using the shunting yard
 * algorithm. Supports all POSIX arithmetic operators, variables, and proper
 * error handling.
 *
 * This module provides arithmetic functionality for the shell architecture.
 */

/**
 * arithm_expand:
 *      Evaluate arithmetic expression with full POSIX operator support
 *
 * @param orig_expr: Arithmetic expression string (with or without $(( ))
 * wrapper)
 * @return: String representation of the result, or NULL on error
 *
 * Supported operators:
 * - Basic: + - * / %
 * - Comparison: == != < <= > >=
 * - Logical: && || !
 * - Bitwise: & | ^ ~ << >>
 * - Assignment: = += -= *= /= %= <<= >>= &= ^= |=
 * - Increment/Decrement: ++ --
 * - Exponentiation: **
 * - Ternary: ? :
 * - Parentheses for grouping
 * - Variables and numeric literals
 * - Octal (0123) and hexadecimal (0x123) numbers
 */
char *arithm_expand(const char *orig_expr);

/**
 * arithm_expand_with_executor:
 *      Evaluate arithmetic expression with executor context for scoped
 * variables
 *
 * @param executor: Executor context for scoped variable resolution
 * @param orig_expr: Arithmetic expression string (with or without $(( ))
 * wrapper)
 * @return: String representation of the result, or NULL on error
 *
 * This function is identical to arithm_expand but uses the executor's
 * symbol table for variable resolution, allowing access to function
 * parameters like $1, $2, etc.
 */
typedef struct executor executor_t;
char *arithm_expand_with_executor(executor_t *executor, const char *orig_expr);

/**
 * arithm_init:
 *      Initialize the arithmetic expansion module
 *
 * Call this once during shell initialization to set up any required
 * state for arithmetic evaluation.
 */
void arithm_init(void);

/**
 * arithm_cleanup:
 *      Clean up arithmetic expansion module resources
 *
 * Call this during shell shutdown to free any allocated resources.
 */
void arithm_cleanup(void);

/**
 * Error handling for arithmetic operations
 */
extern bool arithm_error_flag;
extern char *arithm_error_message;

/**
 * arithm_get_last_error:
 *      Get the last arithmetic error message
 *
 * @return: Error message string, or NULL if no error
 */
const char *arithm_get_last_error(void);

/**
 * arithm_set_error:
 *      Set an arithmetic error message
 *
 * @param message: Error message to set
 */
void arithm_set_error(const char *message);

/**
 * arithm_clear_error:
 *      Clear the current arithmetic error state
 */
void arithm_clear_error(void);

#endif /* ARITHMETIC_H */
