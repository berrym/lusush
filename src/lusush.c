/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/linenoise/linenoise.h"
#include "../include/node.h"
#include "../include/scanner.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
    FILE *in = NULL;   // input file stream pointer
    char *line = NULL; // pointer to a line of input read

    // Initialize special shell variables
    shell_pid = getpid();
    shell_argc = argc;
    shell_argv = argv;
    last_exit_status = 0;

    // Perform startup tasks
    init(argc, argv, &in);

    // Read input one line at a time until user exits
    // or EOF is read from either stdin or input file
    while (!exit_flag) {
        // Read a line of input from the opened stream
        line = get_input(in);

        if (line == NULL) {
            exit_flag = true;
            continue;
        }

        // Create a source structure from input
        source_t src;
        src.buf = line;
        src.bufsize = strlen(line);
        src.pos = INIT_SRC_POS;

        // Check if this line contains pipes and use simple parser if so
        if (strchr(line, '|') != NULL) {
            execute_pipeline_simple(line);
        } else {
            // Parse then execute a command normally
            parse_and_execute(&src);
        }

        if (shell_type() != NORMAL_SHELL) {
            linenoiseFree(line);
        } else {
            free_input_buffers();
        }
    }

    if (in) {
        fclose(in);
    }
}

int parse_and_execute(source_t *src) {
    skip_whitespace(src);

    token_t *tok = tokenize(src);

    if (tok == &eof_token) {
        return 0;
    }

    while (tok && tok != &eof_token) {
        node_t *cmd = parse_command(tok);

        if (cmd == NULL) {
            break;
        }

        // Execute the parsed command using the appropriate handler
        execute_node(cmd);
        
        free_node_tree(cmd);
        tok = tokenize(src);
    }

    return 1;
}
