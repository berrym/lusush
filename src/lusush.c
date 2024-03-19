/*
 * lusush.c - LUSUs' SHell
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/errors.h"
#include "../include/exec.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/lusush.h"
#include "../include/node.h"
#include "../include/parser.h"
#include "../include/scanner.h"

int main(int argc, char **argv)
{
    FILE *in = stdin;                // input file stream pointer
    char *line = NULL;               // pointer to a line of input read

    // Perform startup tasks
    init(argc, argv, &in);

    // Read input one line at a time until user exits
    // or EOF is read from either stdin or input file
    for (;;) {
        // Read a line of input from the opened stream
        line = get_input(in);

        if (line == NULL)
            break;

        // Create a source structure from input
        source_s src;
        src.buf = line;
        src.bufsize = strlen(line);
        src.pos = INIT_SRC_POS;

        // Parse then execute a command
        parse_and_execute(&src);
    }
}

int parse_and_execute(source_s *src)
{
    skip_whitespace(src);

    token_s *tok = tokenize(src);

    if (tok == &eof_token)
        return 0;

    while (tok && tok != &eof_token) {
        node_s *cmd = parse_command(tok);

        if (cmd == NULL)
            break;

        do_basic_command(cmd);
        free_node_tree(cmd);
        tok = tokenize(src);
    }

    return 1;
}
