/*
 * MODERN LUSUSH - Clean, Modern-Only Shell Implementation
 * 
 * This is a simplified implementation that uses only the modern components:
 * - Modern tokenizer (tokenizer_new.c)
 * - Modern parser (parser_modern.c) 
 * - Modern executor (executor_modern.c)
 * - Modern symbol table (symtable_modern.c)
 * 
 * No complexity analysis, no old parser fallbacks, just clean modern execution.
 */

#include "../include/executor_modern.h"
#include "../include/linenoise/linenoise.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global modern executor for persistent variable state
static executor_modern_t *global_executor = NULL;

// Initialize the modern shell
static int init_modern_shell(void) {
    global_executor = executor_modern_new();
    if (!global_executor) {
        fprintf(stderr, "Error: Failed to create modern executor\n");
        return 1;
    }
    
    // Enable debug mode if requested
    if (getenv("LUSUSH_DEBUG")) {
        executor_modern_set_debug(global_executor, true);
    }
    
    return 0;
}

// Clean up the modern shell
static void cleanup_modern_shell(void) {
    if (global_executor) {
        executor_modern_free(global_executor);
        global_executor = NULL;
    }
}

// Execute a command line using the modern executor
static int execute_command_line(const char *line) {
    if (!line || strlen(line) == 0) {
        return 0;
    }
    
    if (!global_executor) {
        fprintf(stderr, "Error: Modern executor not initialized\n");
        return 1;
    }
    
    // Execute using modern execution engine
    int exit_status = executor_modern_execute_command_line(global_executor, line);
    
    if (executor_modern_has_error(global_executor)) {
        fprintf(stderr, "Execution error: %s\n", executor_modern_error(global_executor));
    }
    
    return exit_status;
}

// Main shell loop for interactive mode
static int run_interactive_shell(void) {
    char *line;
    int exit_status = 0;
    
    printf("Modern LUSUSH Shell (v0.6.0)\n");
    printf("Type 'exit' to quit.\n\n");
    
    while ((line = linenoise("lusush> ")) != NULL) {
        // Skip empty lines
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        
        // Add to history
        linenoiseHistoryAdd(line);
        
        // Handle built-in exit command
        if (strcmp(line, "exit") == 0) {
            free(line);
            break;
        }
        
        // Execute the command
        exit_status = execute_command_line(line);
        
        free(line);
    }
    
    return exit_status;
}

// Execute a script file
static int run_script_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open script file '%s'\n", filename);
        return 1;
    }
    
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    int exit_status = 0;
    int line_number = 0;
    
    while ((nread = getline(&line, &len, file)) != -1) {
        line_number++;
        
        // Remove trailing newline
        if (nread > 0 && line[nread-1] == '\n') {
            line[nread-1] = '\0';
        }
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }
        
        // Execute the command
        int result = execute_command_line(line);
        if (result != 0) {
            exit_status = result;
            // Continue execution even on errors (POSIX behavior)
        }
    }
    
    free(line);
    fclose(file);
    return exit_status;
}

// Main function
int main(int argc, char **argv) {
    int exit_status = 0;
    
    // Initialize modern shell
    if (init_modern_shell() != 0) {
        return 1;
    }
    
    if (argc == 1) {
        // Interactive mode
        exit_status = run_interactive_shell();
    } else if (argc == 2) {
        // Script mode
        exit_status = run_script_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: %s [script_file]\n", argv[0]);
        exit_status = 1;
    }
    
    // Clean up
    cleanup_modern_shell();
    
    return exit_status;
}
