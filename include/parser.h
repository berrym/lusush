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
#include "shell_error.h"
#include "tokenizer.h"

/** Parser state */
typedef struct parser {
    tokenizer_t *tokenizer;
    const char *error_message;
    bool has_error;

    /* Structured error collection (Phase 2 error management) */
    shell_error_collector_t *error_collector;
    const char *source_name;  /* Script name for error display */
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

/* ============================================================================
 * Structured Error Collection (Phase 2)
 * ============================================================================ */

/**
 * @brief Convert a token to a source location
 *
 * Creates a source_location_t from token position information.
 *
 * @param token Token to extract location from
 * @param filename Source filename (or NULL for default)
 * @return Source location structure
 */
source_location_t token_to_source_location(token_t *token, const char *filename);

/**
 * @brief Add a structured error to the parser's error collector
 *
 * Creates and adds a structured error with full source location information.
 * Falls back to the legacy error system if collector is not initialized.
 *
 * @param parser Parser context
 * @param code Error code from shell_error_code_t
 * @param fmt Printf-style format string for error message
 * @param ... Format arguments
 */
void parser_error_add(parser_t *parser, shell_error_code_t code,
                      const char *fmt, ...);

/**
 * @brief Display all collected parser errors
 *
 * Outputs all errors from the collector with source context.
 *
 * @param parser Parser context
 * @param out Output stream (typically stderr)
 * @param use_color Whether to use ANSI color codes
 */
void parser_display_errors(parser_t *parser, FILE *out, bool use_color);

/**
 * @brief Get the error collector from parser
 *
 * @param parser Parser context
 * @return Error collector or NULL if not initialized
 */
shell_error_collector_t *parser_get_error_collector(parser_t *parser);

#endif // PARSER_H
