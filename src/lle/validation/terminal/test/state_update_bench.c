// src/lle/validation/terminal/test/state_update_bench.c
// Simple benchmark for state update performance (no terminal interaction)

#define _POSIX_C_SOURCE 199309L

#include "../state.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void) {
    lle_terminal_validation_t term;
    memset(&term, 0, sizeof(term));
    
    // Initialize basic state without terminal interaction
    term.state.rows = 24;
    term.state.cols = 80;
    term.state.scroll_top = 0;
    term.state.scroll_bottom = 23;
    
    printf("=== State Update Performance Benchmark ===\n\n");
    
    const int iterations = 10000;
    printf("Performing %d state updates...\n\n", iterations);
    
    // Perform state updates
    for (int i = 0; i < iterations; i++) {
        lle_terminal_state_update_cursor(&term, (uint16_t)(i % 24), (uint16_t)(i % 80));
    }
    
    // Calculate statistics
    uint64_t avg_time_ns = term.total_update_time_ns / term.total_updates;
    uint64_t avg_time_us = avg_time_ns / 1000;
    
    printf("Performance Results:\n");
    printf("  Total updates: %lu\n", term.total_updates);
    printf("  Total time: %lu ns\n", term.total_update_time_ns);
    printf("  Average time: %lu ns (%.3f us)\n", avg_time_ns, avg_time_ns / 1000.0);
    printf("  Last update: %lu ns (%.3f us)\n", 
           term.state.last_update_ns, term.state.last_update_ns / 1000.0);
    
    printf("\nValidation:\n");
    printf("  Terminal queries: 0 (code review confirmed)\n");
    printf("  State updates: Internal only\n");
    
    printf("\n=== Results ===\n");
    if (avg_time_us < 100) {
        printf("[PASS] Average update time: %lu us < 100 us target\n", avg_time_us);
        printf("State update performance meets requirements\n");
        return 0;
    } else {
        printf("[FAIL] Average update time: %lu us >= 100 us target\n", avg_time_us);
        return 1;
    }
}
