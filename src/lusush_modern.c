/**
 * Minimal Modern-Only POSIX Shell
 * 
 * This is a clean implementation that uses ONLY the modern components:
 * - tokenizer_new.c
 * - parser_modern.c 
 * - executor_modern.c
 * - symtable_modern.c
 * 
 * No complexity analysis, no legacy code, pure modern execution path.
 */

#include "../include/tokenizer_new.h"
#include "../include/parser_modern.h"
#include "../include/executor_modern.h"
#include "../include/symtable_modern.h"
#include "../include/node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// Simple error handling for the modern shell
static void modern_error(const char *msg) {
    fprintf(stderr, "lusush: %s\n", msg);
}

// Process a single command line using only modern components
static int process_command_line(executor_modern_t *executor, const char *line) {
    if (!line || strlen(line) == 0) {
        return 0;
    }
    
    // Create modern parser for this line
    parser_modern_t *parser = parser_modern_new(line);
    if (!parser) {
        modern_error("failed to create parser");
        return 1;
    }
    
    // Parse the command line
    node_t *ast = parser_modern_parse(parser);
    if (!ast) {
        if (parser_modern_has_error(parser)) {
            fprintf(stderr, "lusush: parse error: %s\n", parser_modern_error(parser));
        }
        parser_modern_free(parser);
        return 1;
    }
    
    // Execute the AST
    int result = executor_modern_execute(executor, ast);
    
    // Clean up
    free_node_tree(ast);
    parser_modern_free(parser);
    
    return result;
}

// Interactive mode - read and execute commands
static int interactive_mode(executor_modern_t *executor) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    
    printf("lusush (modern-only)> ");
    fflush(stdout);
    
    while ((nread = getline(&line, &len, stdin)) != -1) {
        // Remove trailing newline
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(line) == 0) {
            printf("lusush (modern-only)> ");
            fflush(stdout);
            continue;
        }
        
        // Handle exit command
        if (strcmp(line, "exit") == 0) {
            break;
        }
        
        // Process the command
        process_command_line(executor, line);
        
        printf("lusush (modern-only)> ");
        fflush(stdout);
    }
    
    free(line);
    return 0;
}

// Script mode - execute commands from file or stdin
static int script_mode(executor_modern_t *executor, FILE *input) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    int exit_code = 0;
    
    while ((nread = getline(&line, &len, input)) != -1) {
        // Remove trailing newline
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }
        
        // Process the command
        int result = process_command_line(executor, line);
        if (result != 0) {
            exit_code = result;
        }
    }
    
    free(line);
    return exit_code;
}

int main(int argc, char *argv[]) {
    // Create modern executor
    executor_modern_t *executor = executor_modern_new();
    if (!executor) {
        modern_error("failed to initialize modern executor");
        return 1;
    }
    
    int exit_code = 0;
    
    if (argc > 1) {
        // Script mode with file argument
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "lusush: %s: %s\n", argv[1], strerror(errno));
            exit_code = 1;
        } else {
            exit_code = script_mode(executor, file);
            fclose(file);
        }
    } else if (isatty(STDIN_FILENO)) {
        // Interactive mode
        exit_code = interactive_mode(executor);
    } else {
        // Script mode from stdin
        exit_code = script_mode(executor, stdin);
    }
    
    // Clean up
    executor_modern_free(executor);
    
    return exit_code;
}
