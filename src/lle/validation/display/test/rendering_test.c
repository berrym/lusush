/*
 * LLE Display Client Rendering Test - Week 2 Day 6
 * Tests basic rendering functionality and architectural compliance
 */

#define _POSIX_C_SOURCE 199309L

#include "../client.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test 1: Basic initialization
static int test_init(void) {
    lle_display_client_t client;
    
    int ret = lle_display_client_init(&client, 24, 80);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Initialization failed\n");
        return -1;
    }
    
    // Verify initialization
    assert(client.buffer.rows == 24);
    assert(client.buffer.cols == 80);
    assert(client.buffer.cells != NULL);
    assert(client.cursor_row == 0);
    assert(client.cursor_col == 0);
    assert(client.render_count == 0);
    assert(client.escape_sequence_violations == 0);
    
    lle_display_client_cleanup(&client);
    
    printf("[PASS] Initialization test\n");
    return 0;
}

// Test 2: Single-line rendering
static int test_single_line_rendering(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    const char *test_line = "Hello, World!";
    int ret = lle_display_client_render(&client, test_line);
    
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Rendering failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify buffer contents
    for (size_t i = 0; i < strlen(test_line); i++) {
        assert(client.buffer.cells[i].codepoint == (uint32_t)test_line[i]);
    }
    
    // Verify performance tracking
    assert(client.render_count == 1);
    assert(client.last_render_time_ns > 0);
    
    // Verify no escape sequence violations
    assert(client.escape_sequence_violations == 0);
    
    lle_display_client_cleanup(&client);
    
    printf("[PASS] Single-line rendering test\n");
    return 0;
}

// Test 3: Multi-line rendering
static int test_multiline_rendering(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    // Create a line longer than 80 characters (should wrap)
    char long_line[200];
    memset(long_line, 'A', 199);
    long_line[199] = '\0';
    
    int ret = lle_display_client_render_multiline(&client, long_line, 0);
    
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Multi-line rendering failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify wrapping: first row should be full
    for (size_t i = 0; i < 80; i++) {
        assert(client.buffer.cells[i].codepoint == 'A');
    }
    
    // Second row should also have content
    assert(client.buffer.cells[80].codepoint == 'A');
    
    lle_display_client_cleanup(&client);
    
    printf("[PASS] Multi-line rendering test\n");
    return 0;
}

// Test 4: Syntax highlighting rendering
static int test_highlighted_rendering(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    const char *test_line = "echo hello";
    uint8_t attrs[10] = {
        LLE_ATTR_BOLD,        // 'e'
        LLE_ATTR_BOLD,        // 'c'
        LLE_ATTR_BOLD,        // 'h'
        LLE_ATTR_BOLD,        // 'o'
        0,                    // ' '
        LLE_ATTR_UNDERLINE,   // 'h'
        LLE_ATTR_UNDERLINE,   // 'e'
        LLE_ATTR_UNDERLINE,   // 'l'
        LLE_ATTR_UNDERLINE,   // 'l'
        LLE_ATTR_UNDERLINE    // 'o'
    };
    
    int ret = lle_display_client_render_highlighted(&client, test_line, attrs, 10);
    
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Highlighted rendering failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify attributes applied
    assert(client.buffer.cells[0].attrs == LLE_ATTR_BOLD);
    assert(client.buffer.cells[4].attrs == 0);
    assert(client.buffer.cells[5].attrs == LLE_ATTR_UNDERLINE);
    
    lle_display_client_cleanup(&client);
    
    printf("[PASS] Syntax highlighting rendering test\n");
    return 0;
}

// Test 5: Performance measurement
static int test_performance(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    // Perform 1000 render operations
    const char *test_line = "Performance test line";
    
    for (int i = 0; i < 1000; i++) {
        lle_display_client_render(&client, test_line);
    }
    
    // Check performance
    double avg_time_ms;
    uint64_t violations;
    lle_display_client_get_metrics(&client, &avg_time_ms, &violations);
    
    printf("  Performance: %d renders, avg %.3f ms\n", 
           (int)client.render_count, avg_time_ms);
    
    // Verify performance target (<10ms for Week 2 success criteria)
    if (avg_time_ms >= 10.0) {
        fprintf(stderr, "[FAIL] Average render time %.3f ms >= 10ms target\n", 
                avg_time_ms);
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify zero violations
    if (violations != 0) {
        fprintf(stderr, "[FAIL] Escape sequence violations detected: %lu\n", 
                violations);
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    lle_display_client_cleanup(&client);
    
    printf("[PASS] Performance test (avg %.3f ms < 10ms target)\n", avg_time_ms);
    return 0;
}

// Test 6: Cursor positioning
static int test_cursor_positioning(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    // Test valid cursor positions
    int ret = lle_display_client_set_cursor(&client, 5, 10);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Valid cursor positioning failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    assert(client.cursor_row == 5);
    assert(client.cursor_col == 10);
    
    // Test bounds checking
    ret = lle_display_client_set_cursor(&client, 100, 100);
    if (ret == 0) {
        fprintf(stderr, "[FAIL] Out-of-bounds cursor accepted\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    lle_display_client_cleanup(&client);
    
    printf("[PASS] Cursor positioning test\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("===================================================\n");
    printf("  LLE Display Client Test - Phase 0 Week 2 Day 6\n");
    printf("===================================================\n\n");
    
    printf("Testing display client interface...\n\n");
    
    if (test_init() != 0) failures++;
    if (test_single_line_rendering() != 0) failures++;
    if (test_multiline_rendering() != 0) failures++;
    if (test_highlighted_rendering() != 0) failures++;
    if (test_performance() != 0) failures++;
    if (test_cursor_positioning() != 0) failures++;
    
    printf("\n===================================================\n");
    printf("                    Results\n");
    printf("===================================================\n");
    
    if (failures == 0) {
        printf("[PASS] All tests passed\n\n");
        printf("Architecture Validation: CONFIRMED\n");
        printf("- Display client interface operational\n");
        printf("- Zero escape sequence violations\n");
        printf("- Performance targets met (<10ms)\n");
        printf("- Ready for Week 2 Day 7 rendering scenarios\n");
        return 0;
    } else {
        printf("[FAIL] %d test(s) failed\n", failures);
        return 1;
    }
}
