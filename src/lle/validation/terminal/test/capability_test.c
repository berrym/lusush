// src/lle/validation/terminal/test/capability_test.c

#include "../state.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Declare the capability detection function
int lle_terminal_detect_capabilities(lle_terminal_validation_t *term,
                                      uint32_t timeout_ms);

int main(void) {
    lle_terminal_validation_t term;
    memset(&term, 0, sizeof(term));
    
    // Set file descriptors
    term.input_fd = STDIN_FILENO;
    term.output_fd = STDOUT_FILENO;
    
    printf("=== Terminal Capability Detection Test ===\n\n");
    
    // Test capability detection with 50ms timeout
    int ret = lle_terminal_detect_capabilities(&term, 50);
    
    printf("Detection result: %s\n", ret == 0 ? "SUCCESS" : "FAILURE");
    printf("Detection time: %u ms (target: <50ms)\n", term.caps.detection_time_ms);
    printf("Detection successful: %s\n\n", term.caps.detection_successful ? "Yes" : "No");
    
    printf("Terminal Capabilities:\n");
    printf("  Type: ");
    switch (term.caps.type) {
        case TERM_VT100: printf("VT100\n"); break;
        case TERM_XTERM: printf("xterm\n"); break;
        case TERM_XTERM_256: printf("xterm-256color\n"); break;
        case TERM_RXVT: printf("rxvt\n"); break;
        case TERM_SCREEN: printf("screen\n"); break;
        case TERM_TMUX: printf("tmux\n"); break;
        case TERM_UNKNOWN: printf("Unknown\n"); break;
        default: printf("Invalid\n"); break;
    }
    
    printf("  Color support: %s\n", term.caps.has_color ? "Yes" : "No");
    printf("  256 color: %s\n", term.caps.has_256_color ? "Yes" : "No");
    printf("  True color: %s\n", term.caps.has_true_color ? "Yes" : "No");
    printf("  Unicode (UTF-8): %s\n", term.caps.has_unicode ? "Yes" : "No");
    printf("  Mouse support: %s\n", term.caps.has_mouse ? "Yes" : "No");
    printf("  Bracketed paste: %s\n", term.caps.has_bracketed_paste ? "Yes" : "No");
    
    printf("\n");
    if (term.caps.detection_time_ms < 50) {
        printf("[PASS] Detection completed in <50ms\n");
        return 0;
    } else {
        printf("[WARNING] Detection took %ums (target: <50ms)\n", 
               term.caps.detection_time_ms);
        return 1;
    }
}
