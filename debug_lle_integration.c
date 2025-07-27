#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "include/linenoise_replacement.h"
#include "src/line_editor/lle_integration.h"

int main(int argc, char **argv) {
    printf("=== LLE Integration Debug Test ===\n");
    
    // Check if we're in a terminal
    if (!isatty(STDIN_FILENO)) {
        printf("ERROR: Not running in a terminal (stdin is not a TTY)\n");
        return 1;
    }
    
    printf("Running in terminal: YES\n");
    printf("stdin fd: %d\n", STDIN_FILENO);
    printf("stdout fd: %d\n", STDOUT_FILENO);
    printf("stderr fd: %d\n", STDERR_FILENO);
    
    // Test LLE integration initialization
    printf("\n--- Testing LLE Integration Initialization ---\n");
    
    if (!lle_integration_init()) {
        printf("ERROR: LLE integration failed to initialize\n");
        const char *error = lle_integration_get_last_error();
        if (error) {
            printf("Error message: %s\n", error);
        }
        return 1;
    }
    
    printf("LLE integration initialized successfully\n");
    
    // Check if integration is initialized
    if (!lle_integration_is_initialized()) {
        printf("ERROR: LLE integration reports not initialized after init\n");
        return 1;
    }
    
    printf("LLE integration status check: OK\n");
    
    // Test basic statistics
    size_t readline_calls, history_ops, memory_allocated;
    if (lle_integration_get_statistics(&readline_calls, &history_ops, &memory_allocated)) {
        printf("Initial statistics - readline calls: %zu, history ops: %zu, memory: %zu\n", 
               readline_calls, history_ops, memory_allocated);
    }
    
    // Test the linenoise replacement macros
    printf("\n--- Testing Linenoise Replacement ---\n");
    
    // Enable debug mode
    lle_replacement_set_debug(true);
    
    // Try to read a line
    printf("About to call linenoise() with prompt...\n");
    fflush(stdout);
    
    char *line = linenoise("debug_test> ");
    
    if (line == NULL) {
        printf("linenoise() returned NULL\n");
        printf("errno: %d (%s)\n", errno, strerror(errno));
        
        const char *error = lle_integration_get_last_error();
        if (error && strlen(error) > 0) {
            printf("LLE integration error: %s\n", error);
        }
        
        const char *replacement_error = lle_replacement_get_last_error();
        if (replacement_error && strlen(replacement_error) > 0) {
            printf("Replacement layer error: %s\n", replacement_error);
        }
        
        return 1;
    }
    
    printf("SUCCESS: Read line: '%s'\n", line);
    printf("Line length: %zu\n", strlen(line));
    
    // Free the line
    linenoiseFree(line);
    
    // Get final statistics
    if (lle_integration_get_statistics(&readline_calls, &history_ops, &memory_allocated)) {
        printf("Final statistics - readline calls: %zu, history ops: %zu, memory: %zu\n", 
               readline_calls, history_ops, memory_allocated);
    }
    
    // Clean shutdown
    printf("\n--- Cleaning Up ---\n");
    lle_integration_shutdown();
    
    printf("Debug test completed successfully\n");
    return 0;
}