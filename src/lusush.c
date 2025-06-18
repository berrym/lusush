/*
 * lusush - POSIX-compliant shell
 */

#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/input.h"
#include "../include/linenoise/linenoise.h"
#include "../include/node.h"
#include "../include/parser.h"
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

    // Read input (buffering complete syntactic units) until user exits
    // or EOF is read from either stdin or input file
    while (!exit_flag) {
        // Read complete command(s) using unified input system
        // This ensures consistent parsing behavior between interactive and non-interactive modes
        line = get_unified_input(in);

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
    while (true) {
        skip_whitespace(src);
        
        // Use new parser architecture to get complete commands
        node_t *cmd = parse_complete_command(src);
        
        if (cmd == NULL) {
            break;  // EOF or error
        }

        // Execute the parsed command using the appropriate handler
        execute_node(cmd);
        
        free_node_tree(cmd);
        
        // Check for more commands (semicolon or newline separated)
        skip_whitespace(src);
        token_t *delimiter = tokenize(src);
        if (delimiter == &eof_token) {
            break;
        }
        if (delimiter->type == TOKEN_SEMI || delimiter->type == TOKEN_NEWLINE) {
            free_token(delimiter);
            continue;  // More commands to process
        } else {
            // Push back non-delimiter token
            unget_token(delimiter);
            break;
        }
    }

    return 1;
}
