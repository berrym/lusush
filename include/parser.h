#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include "scanner.h"

/**
 * Parse a command from a token stream.
 * Returns a node tree representing the command.
 */
node_t *parse_command(token_t *tok);

/**
 * Parse and execute a command from a source.
 * Returns 1 on success, 0 on EOF, -1 on error.
 */
int parse_and_execute(source_t *src);

#endif /* PARSER_H */
