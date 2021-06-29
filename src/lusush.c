/*
 * lusush.c - LUSUs' SHell
 */

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "errors.h"
#include "exec.h"
#include "init.h"
#include "input.h"
#include "lusush.h"
#include "node.h"
#include "parser.h"
#include "scanner.h"

int main(int argc, char **argv)
{
    FILE *in = stdin;                // input file stream pointer
    bool looping = true;             // boolean flag to keep looping
    char *line = NULL;               // pointer to a line of input read

    // Perform startup tasks
    init(argc, argv, &in);

    // Read input one line at a time until user exits
    // or EOF is read from either stdin or input file
    while (looping) {
        // Read a line of input from the opened stream
        line = get_input(in);

        if (!line)
            break;

        // Create a source structure from input
        struct source src;
        src.buf = line;
        src.bufsize = strnlen(line, MAXLINE);
        src.pos = INIT_SRC_POS;

        // Parse then execute a command
        parse_and_execute(&src);
    }
}

int parse_and_execute(struct source *src)
{
    skip_whitespace(src);

    struct token *tok = tokenize(src);

    if (tok == &eof_token)
        return 0;

    while (tok && tok != &eof_token) {
        struct node *cmd = parse_command(tok);

        if (!cmd)
            break;

        do_command(cmd);
        free_node_tree(cmd);
        tok = tokenize(src);
    }

    return 1;
}
