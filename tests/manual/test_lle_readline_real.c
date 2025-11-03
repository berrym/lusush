/*
 * Real-World Test for lle_readline()
 * 
 * This is a minimal REPL (Read-Eval-Print Loop) to actually test lle_readline()
 * in practice. It demonstrates real interactive usage of the LLE system.
 * 
 * Usage: ./test_lle_readline_real
 * 
 * Commands:
 *   - Type any text and press Enter
 *   - Try Ctrl-A, Ctrl-E for beginning/end of line
 *   - Try Ctrl-K to kill to end of line
 *   - Try Ctrl-Y to yank (paste)
 *   - Try arrow keys for cursor movement
 *   - Type 'quit' or 'exit' to quit
 *   - Ctrl-D on empty line to quit
 *   - Ctrl-C to interrupt
 * 
 * NOTE: lle_readline() creates its own terminal, buffer, and event system
 *       instances, so no external initialization is required.
 */

#include "../../include/lle/lle_readline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  LLE Readline - Real World Test                               ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("This is a minimal REPL to test lle_readline() interactively.\n");
    printf("\n");
    printf("Commands:\n");
    printf("  - Type text and press Enter to echo it back\n");
    printf("  - 'quit' or 'exit' to exit\n");
    printf("  - Ctrl-D on empty line to exit\n");
    printf("  - Ctrl-C to interrupt\n");
    printf("\n");
    printf("Emacs keybindings:\n");
    printf("  Ctrl-A: Beginning of line    Ctrl-K: Kill to end\n");
    printf("  Ctrl-E: End of line          Ctrl-Y: Yank (paste)\n");
    printf("  Ctrl-B: Back one char        Ctrl-W: Kill word back\n");
    printf("  Ctrl-F: Forward one char     Ctrl-U: Kill entire line\n");
    printf("  Ctrl-L: Clear screen\n");
    printf("\n");
    printf("Arrow keys: ← → for cursor movement\n");
    printf("            ↑ ↓ for history (if implemented)\n");
    printf("\n");
    printf("Starting REPL...\n");
    printf("(lle_readline() will initialize its own subsystems)\n");
    printf("\n");
    
    // Main REPL loop
    int line_count = 0;
    bool running = true;
    
    while (running) {
        // Read a line
        char *line = lle_readline("lle> ");
        
        // Handle EOF or interrupt
        if (line == NULL) {
            printf("\n");
            printf("Received EOF or interrupt. Exiting...\n");
            break;
        }
        
        line_count++;
        
        // Check for quit commands
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            printf("Exiting...\n");
            free(line);
            break;
        }
        
        // Check for empty line
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        
        // Echo the line back
        printf("Line %d: [%s]\n", line_count, line);
        printf("Length: %zu characters\n", strlen(line));
        
        // Free the line
        free(line);
    }
    
    // Cleanup
    printf("\n");
    printf("Total lines entered: %d\n", line_count);
    printf("\n");
    printf("Thank you for testing LLE!\n");
    printf("\n");
    
    return 0;
}
