/**
 * Modern POSIX Shell Parser - Clean Recursive Descent Implementation
 * 
 * This parser uses the new tokenizer and implements a proper recursive descent
 * parser for POSIX shell grammar. It handles control structures, commands,
 * pipelines, and proper token boundary management.
 */

#ifndef PARSER_MODERN_H
#define PARSER_MODERN_H

#include "tokenizer.h"
#include "node.h"

// Parser state
typedef struct parser_modern {
    modern_tokenizer_t *tokenizer;
    const char *error_message;
    bool has_error;
} parser_modern_t;

// Parser interface
parser_modern_t *parser_modern_new(const char *input);
void parser_modern_free(parser_modern_t *parser);

// Main parsing functions
node_t *parser_modern_parse(parser_modern_t *parser);
node_t *parser_modern_parse_command_line(parser_modern_t *parser);

// Error handling
bool parser_modern_has_error(parser_modern_t *parser);
const char *parser_modern_error(parser_modern_t *parser);

#endif // PARSER_MODERN_H
