#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include "scanner_old.h"

/**
 * Parse a command from a token stream.
 * Returns a node tree representing the command.
 */
node_t *parse_command(token_t *tok);

/**
 * NEW PARSER ARCHITECTURE: Parse a complete command from source
 * This replaces the flawed token-by-token approach with proper command parsing
 */
node_t *parse_complete_command(source_t *src);

/**
 * Parse and execute a command from a source.
 * Returns 1 on success, 0 on EOF, -1 on error.
 */
int parse_and_execute(source_t *src);

#endif /* PARSER_H */
