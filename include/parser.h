/**
 * POSIX Shell Parser - Clean Recursive Descent Implementation
 *
 * This parser uses the tokenizer and implements a proper recursive descent
 * parser for POSIX shell grammar. It handles control structures, commands,
 * pipelines, and proper token boundary management.
 */

#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include "tokenizer.h"

// Parser state
typedef struct parser {
    tokenizer_t *tokenizer;
    const char *error_message;
    bool has_error;
} parser_t;

// Parser interface
parser_t *parser_new(const char *input);
void parser_free(parser_t *parser);

// Main parsing functions
node_t *parser_parse(parser_t *parser);
node_t *parser_parse_command_line(parser_t *parser);

// Error handling
bool parser_has_error(parser_t *parser);
const char *parser_error(parser_t *parser);

#endif // PARSER_H
