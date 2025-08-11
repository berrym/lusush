#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../include/readline_integration.h"
#include "../include/signals.h"

/**
 * Debug control builtin for lusush
 * Allows enabling/disabling debug modes from within the shell
 */

void print_debug_usage(void) {
    printf("debug - Control lusush debugging modes\n");
    printf("\n");
    printf("Usage:\n");
    printf("  debug on                    - Enable all debugging\n");
    printf("  debug off                   - Disable all debugging\n");
    printf("  debug interactive on/off    - Control interactive mode debugging\n");
    printf("  debug signals on/off        - Control signal debugging\n");
    printf("  debug readline on/off       - Control readline debugging\n");
    printf("  debug status                - Show current debug settings\n");
    printf("  debug test                  - Run quick debug tests\n");
    printf("\n");
    printf("Examples:\n");
    printf("  debug on                    # Enable all debugging\n");
    printf("  debug interactive on        # Enable interactive mode debugging only\n");
    printf("  debug status                # Show what's currently enabled\n");
}

void print_debug_status(void) {
    printf("=== Lusush Debug Status ===\n");
    printf("Interactive mode debugging: %s\n", 
           lusush_get_interactive_debug() ? "ENABLED" : "DISABLED");
    printf("Signal debugging: %s\n", 
           lusush_get_signal_debug() ? "ENABLED" : "DISABLED");
    printf("Readline debugging: %s\n", 
           lusush_readline_is_debug_enabled() ? "ENABLED" : "DISABLED");
    printf("Current mode: %s\n", 
           lusush_is_interactive_mode() ? "INTERACTIVE" : "NON-INTERACTIVE");
    printf("Current session: %s\n", 
           lusush_is_interactive_session() ? "INTERACTIVE" : "NON-INTERACTIVE");
    printf("\n");
    printf("Use 'debug on' to enable all debugging\n");
    printf("Use 'debug off' to disable all debugging\n");
}

void run_debug_tests(void) {
    printf("=== Running Quick Debug Tests ===\n");
    
    printf("Mode Detection:\n");
    printf("  lusush_is_interactive_mode(): %s\n", 
           lusush_is_interactive_mode() ? "TRUE" : "FALSE");
    printf("  lusush_is_interactive_session(): %s\n", 
           lusush_is_interactive_session() ? "TRUE" : "FALSE");
    
    printf("\nDebug Settings:\n");
    printf("  Interactive debugging: %s\n", 
           lusush_get_interactive_debug() ? "ON" : "OFF");
    printf("  Signal debugging: %s\n", 
           lusush_get_signal_debug() ? "ON" : "OFF");
    printf("  Readline debugging: %s\n", 
           lusush_readline_is_debug_enabled() ? "ON" : "OFF");
    
    printf("\nTo test specific issues:\n");
    printf("1. Type 'echo test' and press UP arrow (should navigate history, not show completion)\n");
    printf("2. Type 'echo hello' and press Ctrl+C (should clear line, not exit shell)\n");
    printf("3. Type 'echo hello | grep h' (should work in interactive mode)\n");
    printf("\n");
}

int builtin_debug(char **argv) {
    if (!argv[1]) {
        print_debug_usage();
        return 0;
    }
    
    if (strcmp(argv[1], "status") == 0) {
        print_debug_status();
        return 0;
    }
    
    if (strcmp(argv[1], "test") == 0) {
        run_debug_tests();
        return 0;
    }
    
    if (strcmp(argv[1], "on") == 0) {
        printf("Enabling all debugging modes...\n");
        lusush_set_interactive_debug(true);
        lusush_set_signal_debug(true);
        lusush_readline_set_debug(true);
        printf("All debugging enabled. Use 'debug status' to verify.\n");
        return 0;
    }
    
    if (strcmp(argv[1], "off") == 0) {
        printf("Disabling all debugging modes...\n");
        lusush_set_interactive_debug(false);
        lusush_set_signal_debug(false);
        lusush_readline_set_debug(false);
        printf("All debugging disabled.\n");
        return 0;
    }
    
    if (strcmp(argv[1], "interactive") == 0) {
        if (argv[2]) {
            if (strcmp(argv[2], "on") == 0) {
                lusush_set_interactive_debug(true);
                printf("Interactive mode debugging enabled.\n");
                return 0;
            } else if (strcmp(argv[2], "off") == 0) {
                lusush_set_interactive_debug(false);
                printf("Interactive mode debugging disabled.\n");
                return 0;
            }
        }
        printf("Usage: debug interactive on|off\n");
        return 1;
    }
    
    if (strcmp(argv[1], "signals") == 0) {
        if (argv[2]) {
            if (strcmp(argv[2], "on") == 0) {
                lusush_set_signal_debug(true);
                printf("Signal debugging enabled.\n");
                return 0;
            } else if (strcmp(argv[2], "off") == 0) {
                lusush_set_signal_debug(false);
                printf("Signal debugging disabled.\n");
                return 0;
            }
        }
        printf("Usage: debug signals on|off\n");
        return 1;
    }
    
    if (strcmp(argv[1], "readline") == 0) {
        if (argv[2]) {
            if (strcmp(argv[2], "on") == 0) {
                lusush_readline_set_debug(true);
                printf("Readline debugging enabled.\n");
                return 0;
            } else if (strcmp(argv[2], "off") == 0) {
                lusush_readline_set_debug(false);
                printf("Readline debugging disabled.\n");
                return 0;
            }
        }
        printf("Usage: debug readline on|off\n");
        return 1;
    }
    
    printf("Unknown debug command: %s\n", argv[1]);
    print_debug_usage();
    return 1;
}

/**
 * Register the debug builtin with the shell
 * Call this from builtins initialization
 */
void register_debug_builtin(void) {
    // This would be called from builtins.c to register the debug command
    // Implementation depends on how builtins are registered in lusush
}