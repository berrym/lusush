// src/lle/validation/terminal/test/terminal_compatibility_test.c
// Comprehensive terminal compatibility test for Phase 0 Week 1 validation

#define _POSIX_C_SOURCE 199309L

#include "../state.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Test terminal initialization
static int test_init(void) {
    lle_terminal_validation_t term;
    
    printf("Test 1: Terminal Initialization\n");
    
    int ret = lle_terminal_validation_init(&term);
    if (ret != 0) {
        printf("  [FAIL] Initialization failed (terminal not available or not a tty)\n");
        return -1;
    }
    
    // Verify state initialized
    if (term.state.rows == 0 || term.state.cols == 0) {
        printf("  [FAIL] Invalid terminal dimensions: %dx%d\n", 
               term.state.cols, term.state.rows);
        lle_terminal_validation_cleanup(&term);
        return -1;
    }
    
    printf("  Terminal dimensions: %dx%d\n", term.state.cols, term.state.rows);
    printf("  Scroll region: %d-%d\n", term.state.scroll_top, term.state.scroll_bottom);
    
    // Verify capability detection completed
    if (term.caps.detection_time_ms >= 50) {
        printf("  [WARNING] Capability detection took %ums (target: <50ms)\n",
               term.caps.detection_time_ms);
    } else {
        printf("  Capability detection: %ums [PASS]\n", term.caps.detection_time_ms);
    }
    
    lle_terminal_validation_cleanup(&term);
    printf("  [PASS] Initialization\n\n");
    return 0;
}

// Test state updates
static int test_state_updates(void) {
    lle_terminal_validation_t term;
    
    printf("Test 2: State Update Performance\n");
    
    if (lle_terminal_validation_init(&term) != 0) {
        printf("  [SKIP] Terminal not available\n\n");
        return 0;  // Skip if terminal not available, don't fail
    }
    
    // Perform 10,000 state updates
    const int iterations = 10000;
    for (int i = 0; i < iterations; i++) {
        lle_terminal_state_update_cursor(&term, 
                                         (uint16_t)(i % term.state.rows), 
                                         (uint16_t)(i % term.state.cols));
    }
    
    // Verify performance
    uint64_t avg_time_ns = term.total_update_time_ns / term.total_updates;
    uint64_t avg_time_us = avg_time_ns / 1000;
    
    printf("  Updates performed: %lu\n", term.total_updates);
    printf("  Average update time: %lu ns (%.3f us)\n", avg_time_ns, avg_time_ns / 1000.0);
    
    lle_terminal_validation_cleanup(&term);
    
    if (avg_time_us >= 100) {
        printf("  [FAIL] Update time %lu us >= 100 us target\n\n", avg_time_us);
        return -1;
    }
    
    printf("  [PASS] State updates (%.3f us < 100 us)\n\n", avg_time_ns / 1000.0);
    return 0;
}

// Test capability detection
static int test_capabilities(void) {
    lle_terminal_validation_t term;
    
    printf("Test 3: Terminal Capability Detection\n");
    
    if (lle_terminal_validation_init(&term) != 0) {
        printf("  [SKIP] Terminal not available\n\n");
        return 0;
    }
    
    // Print detected capabilities
    printf("  Terminal type: ");
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
    printf("  Detection time: %ums\n", term.caps.detection_time_ms);
    
    lle_terminal_validation_cleanup(&term);
    printf("  [PASS] Capability detection\n\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("===================================================\n");
    printf("  Terminal Compatibility Test - Phase 0 Week 1\n");
    printf("===================================================\n\n");
    
    // Display environment information
    const char *term_env = getenv("TERM");
    const char *lang_env = getenv("LANG");
    printf("Environment:\n");
    printf("  TERM: %s\n", term_env ? term_env : "(not set)");
    printf("  LANG: %s\n", lang_env ? lang_env : "(not set)");
    printf("\n");
    
    // Run tests
    if (test_init() != 0) failures++;
    if (test_state_updates() != 0) failures++;
    if (test_capabilities() != 0) failures++;
    
    // Summary
    printf("===================================================\n");
    printf("                    Results\n");
    printf("===================================================\n");
    if (failures == 0) {
        printf("[PASS] All tests passed on this terminal\n");
        printf("\nCompatibility: CONFIRMED\n");
        printf("This terminal is compatible with LLE terminal abstraction.\n");
        return 0;
    } else {
        printf("[FAIL] %d test(s) failed\n", failures);
        printf("\nCompatibility: ISSUES DETECTED\n");
        printf("This terminal has compatibility issues.\n");
        return 1;
    }
}
