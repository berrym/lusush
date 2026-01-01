/**
 * @file parser.h
 * @brief POSIX shell parser using recursive descent
 *
 * Implements a proper recursive descent parser for POSIX shell grammar.
 * Handles control structures, commands, pipelines, and proper token
 * boundary management.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include "tokenizer.h"

/** Parser state */
typedef struct parser {
    tokenizer_t *tokenizer;
    const char *error_message;
    bool has_error;
} parser_t;

/* ============================================================================
 * Parser Lifecycle
 * ============================================================================ */

/**
 * @brief Create a new parser for input string
 *
 * @param input Shell command string to parse
 * @return New parser instance or NULL on failure
 */
parser_t *parser_new(const char *input);

/**
 * @brief Free a parser and associated resources
 *
 * @param parser Parser to free
 */
void parser_free(parser_t *parser);

/* ============================================================================
 * Parsing Functions
 * ============================================================================ */

/**
 * @brief Parse input into an AST
 *
 * @param parser Parser context
 * @return Root AST node or NULL on error
 */
node_t *parser_parse(parser_t *parser);

/**
 * @brief Parse a complete command line
 *
 * @param parser Parser context
 * @return Root AST node or NULL on error
 */
node_t *parser_parse_command_line(parser_t *parser);

/* ============================================================================
 * Error Handling
 * ============================================================================ */

/**
 * @brief Check if parser has an error
 *
 * @param parser Parser context
 * @return True if a parse error occurred
 */
bool parser_has_error(parser_t *parser);

/**
 * @brief Get the parser error message
 *
 * @param parser Parser context
 * @return Error message string or NULL
 */
const char *parser_error(parser_t *parser);

#endif // PARSER_H
