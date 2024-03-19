#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include "scanner.h"

node_s *parse_command(token_s *tok);

#endif
