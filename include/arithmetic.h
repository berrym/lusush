#ifndef ARITHMETIC_MODERN_H
#define ARITHMETIC_MODERN_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

/**
 * Modern Arithmetic Expansion Module
 * 
 * Provides POSIX-compliant arithmetic expansion using the shunting yard algorithm.
 * Supports all POSIX arithmetic operators, variables, and proper error handling.
 * 
 * This module replaces the legacy wordexp.c arithmetic functionality with a
 * clean, modern implementation suitable for the modern shell architecture.
 */

/**
 * arithm_expand_modern:
 *      Evaluate arithmetic expression with full POSIX operator support
 * 
 * @param orig_expr: Arithmetic expression string (with or without $(( )) wrapper)
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
char *arithm_expand_modern(const char *orig_expr);

/**
 * arithm_init_modern:
 *      Initialize the arithmetic expansion module
 * 
 * Call this once during shell initialization to set up any required
 * state for arithmetic evaluation.
 */
void arithm_init_modern(void);

/**
 * arithm_cleanup_modern:
 *      Clean up arithmetic expansion module resources
 * 
 * Call this during shell shutdown to free any allocated resources.
 */
void arithm_cleanup_modern(void);

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

#endif /* ARITHMETIC_MODERN_H */