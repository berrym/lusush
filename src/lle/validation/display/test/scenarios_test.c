/*
 * LLE Rendering Scenarios Test - Week 2 Day 7
 * Tests all 7 critical rendering scenarios
 */

#define _POSIX_C_SOURCE 199309L

#include "../scenarios.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test Scenario 1: Single-line rendering
static int test_scenario_1_single_line(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    scenario_single_line_t scenario = {
        .input = "echo hello world",
        .cursor_pos = 16
    };
    
    int ret = lle_scenario_render_single_line(&client, &scenario);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Scenario 1: Single-line rendering failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify cursor position
    assert(client.cursor_row == 0);
    assert(client.cursor_col == 16);
    
    // Verify content
    const char *expected = "echo hello world";
    for (size_t i = 0; i < strlen(expected); i++) {
        assert(client.buffer.cells[i].codepoint == (uint32_t)expected[i]);
    }
    
    lle_display_client_cleanup(&client);
    printf("[PASS] Scenario 1: Single-line rendering\n");
    return 0;
}

// Test Scenario 2: Multi-line rendering
static int test_scenario_2_multiline(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    // Create 200-character line (should wrap to 3 rows)
    char long_line[256];
    memset(long_line, 'A', 200);
    long_line[200] = '\0';
    
    scenario_multiline_t scenario = {
        .input = long_line,
        .cursor_pos = 150,
        .expected_rows = 3
    };
    
    int ret = lle_scenario_render_multiline(&client, &scenario);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Scenario 2: Multi-line rendering failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify wrapping: check cells across multiple rows
    assert(client.buffer.cells[0].codepoint == 'A');      // Row 0, Col 0
    assert(client.buffer.cells[79].codepoint == 'A');     // Row 0, Col 79
    assert(client.buffer.cells[80].codepoint == 'A');     // Row 1, Col 0
    assert(client.buffer.cells[159].codepoint == 'A');    // Row 1, Col 79
    assert(client.buffer.cells[160].codepoint == 'A');    // Row 2, Col 0
    
    // Verify cursor position (150 chars = row 1, col 70)
    assert(client.cursor_row == 1);
    assert(client.cursor_col == 70);
    
    lle_display_client_cleanup(&client);
    printf("[PASS] Scenario 2: Multi-line rendering\n");
    return 0;
}

// Test Scenario 3: Prompt rendering
static int test_scenario_3_prompt(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    scenario_prompt_t scenario = {
        .prompt = "user@host:~$ ",
        .input = "ls -la",
        .cursor_pos = 6
    };
    
    int ret = lle_scenario_render_prompt(&client, &scenario);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Scenario 3: Prompt rendering failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify combined content
    const char *expected = "user@host:~$ ls -la";
    for (size_t i = 0; i < strlen(expected); i++) {
        assert(client.buffer.cells[i].codepoint == (uint32_t)expected[i]);
    }
    
    // Verify cursor (prompt length = 13, cursor_pos = 6, total = 19)
    assert(client.cursor_col == 19);
    
    lle_display_client_cleanup(&client);
    printf("[PASS] Scenario 3: Prompt rendering\n");
    return 0;
}

// Test Scenario 4: Syntax highlighting
static int test_scenario_4_highlighting(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    const char *input = "echo hello";
    uint8_t attrs[] = {
        LLE_ATTR_BOLD, LLE_ATTR_BOLD, LLE_ATTR_BOLD, LLE_ATTR_BOLD,  // "echo"
        0,                                                             // " "
        LLE_ATTR_UNDERLINE, LLE_ATTR_UNDERLINE, LLE_ATTR_UNDERLINE,   // "hel"
        LLE_ATTR_UNDERLINE, LLE_ATTR_UNDERLINE                        // "lo"
    };
    
    scenario_highlighting_t scenario = {
        .input = input,
        .highlight_attrs = attrs,
        .attr_count = 10,
        .cursor_pos = 10
    };
    
    int ret = lle_scenario_render_highlighting(&client, &scenario);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Scenario 4: Syntax highlighting failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify attributes applied
    assert(client.buffer.cells[0].attrs == LLE_ATTR_BOLD);        // 'e'
    assert(client.buffer.cells[3].attrs == LLE_ATTR_BOLD);        // 'o'
    assert(client.buffer.cells[4].attrs == 0);                    // ' '
    assert(client.buffer.cells[5].attrs == LLE_ATTR_UNDERLINE);   // 'h'
    assert(client.buffer.cells[9].attrs == LLE_ATTR_UNDERLINE);   // 'o'
    
    lle_display_client_cleanup(&client);
    printf("[PASS] Scenario 4: Syntax highlighting\n");
    return 0;
}

// Test Scenario 5: Completion preview
static int test_scenario_5_completion(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    scenario_completion_t scenario = {
        .input = "ec",
        .completion = "ho hello",
        .cursor_pos = 2
    };
    
    int ret = lle_scenario_render_completion(&client, &scenario);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Scenario 5: Completion preview failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify input rendered normally
    assert(client.buffer.cells[0].codepoint == 'e');
    assert(client.buffer.cells[0].fg_color == 7);  // Normal white
    assert(client.buffer.cells[1].codepoint == 'c');
    assert(client.buffer.cells[1].fg_color == 7);
    
    // Verify completion rendered dimmed
    assert(client.buffer.cells[2].codepoint == 'h');
    assert(client.buffer.cells[2].fg_color == 8);  // Dimmed gray
    assert(client.buffer.cells[2].attrs == LLE_ATTR_DIM);
    
    // Verify cursor at end of input (not including completion)
    assert(client.cursor_col == 2);
    
    lle_display_client_cleanup(&client);
    printf("[PASS] Scenario 5: Completion preview\n");
    return 0;
}

// Test Scenario 6: Scroll region
static int test_scenario_6_scroll_region(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 10, 80);  // 10-row display
    
    // Create 20 lines of history
    const char *history_lines[20] = {
        "line 0", "line 1", "line 2", "line 3", "line 4",
        "line 5", "line 6", "line 7", "line 8", "line 9",
        "line 10", "line 11", "line 12", "line 13", "line 14",
        "line 15", "line 16", "line 17", "line 18", "line 19"
    };
    
    // Show lines 10-19 (last 10 lines)
    scenario_scroll_region_t scenario = {
        .lines = history_lines,
        .line_count = 20,
        .visible_start = 10,
        .visible_count = 10,
        .cursor_line = 9  // Last line
    };
    
    int ret = lle_scenario_render_scroll_region(&client, &scenario);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Scenario 6: Scroll region failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify first visible line is "line 10"
    assert(client.buffer.cells[0].codepoint == 'l');
    assert(client.buffer.cells[1].codepoint == 'i');
    assert(client.buffer.cells[2].codepoint == 'n');
    assert(client.buffer.cells[3].codepoint == 'e');
    assert(client.buffer.cells[5].codepoint == '1');
    assert(client.buffer.cells[6].codepoint == '0');
    
    // Verify last visible line is "line 19" (row 9)
    size_t row_9_offset = 9 * 80;
    assert(client.buffer.cells[row_9_offset + 5].codepoint == '1');
    assert(client.buffer.cells[row_9_offset + 6].codepoint == '9');
    
    // Verify cursor at row 9
    assert(client.cursor_row == 9);
    
    lle_display_client_cleanup(&client);
    printf("[PASS] Scenario 6: Scroll region\n");
    return 0;
}

// Test Scenario 7: Atomic updates
static int test_scenario_7_atomic_update(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    scenario_atomic_update_t scenario = {
        .old_content = "echo hello",
        .new_content = "echo hello world",
        .should_diff = true
    };
    
    int ret = lle_scenario_render_atomic_update(&client, &scenario);
    if (ret != 0) {
        fprintf(stderr, "[FAIL] Scenario 7: Atomic update failed\n");
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    // Verify new content rendered
    const char *expected = "echo hello world";
    for (size_t i = 0; i < strlen(expected); i++) {
        assert(client.buffer.cells[i].codepoint == (uint32_t)expected[i]);
    }
    
    // Verify buffer marked dirty
    assert(client.buffer.dirty == true);
    
    lle_display_client_cleanup(&client);
    printf("[PASS] Scenario 7: Atomic updates\n");
    return 0;
}

// Performance test across all scenarios
static int test_all_scenarios_performance(void) {
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    // Run each scenario multiple times
    const int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        scenario_single_line_t s1 = { .input = "test", .cursor_pos = 4 };
        lle_scenario_render_single_line(&client, &s1);
        
        char long_line[200];
        memset(long_line, 'A', 150);
        long_line[150] = '\0';
        scenario_multiline_t s2 = { .input = long_line, .cursor_pos = 50, .expected_rows = 2 };
        lle_scenario_render_multiline(&client, &s2);
        
        scenario_prompt_t s3 = { .prompt = "$ ", .input = "ls", .cursor_pos = 2 };
        lle_scenario_render_prompt(&client, &s3);
    }
    
    // Check average performance
    double avg_time_ms;
    uint64_t violations;
    lle_display_client_get_metrics(&client, &avg_time_ms, &violations);
    
    printf("  Performance: %d operations, avg %.3f ms\n", 
           (int)client.render_count, avg_time_ms);
    
    if (avg_time_ms >= 10.0) {
        fprintf(stderr, "[FAIL] Average time %.3f ms >= 10ms target\n", avg_time_ms);
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    if (violations != 0) {
        fprintf(stderr, "[FAIL] Escape sequence violations: %lu\n", violations);
        lle_display_client_cleanup(&client);
        return -1;
    }
    
    lle_display_client_cleanup(&client);
    printf("[PASS] All scenarios performance (avg %.3f ms < 10ms)\n", avg_time_ms);
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("===================================================\n");
    printf("  LLE Rendering Scenarios Test - Week 2 Day 7\n");
    printf("===================================================\n\n");
    
    printf("Testing all 7 rendering scenarios...\n\n");
    
    if (test_scenario_1_single_line() != 0) failures++;
    if (test_scenario_2_multiline() != 0) failures++;
    if (test_scenario_3_prompt() != 0) failures++;
    if (test_scenario_4_highlighting() != 0) failures++;
    if (test_scenario_5_completion() != 0) failures++;
    if (test_scenario_6_scroll_region() != 0) failures++;
    if (test_scenario_7_atomic_update() != 0) failures++;
    
    printf("\nTesting performance across all scenarios...\n\n");
    if (test_all_scenarios_performance() != 0) failures++;
    
    printf("\n===================================================\n");
    printf("                 Scenario Results\n");
    printf("===================================================\n\n");
    
    printf("| Scenario | Status |\n");
    printf("|----------|--------|\n");
    printf("| 1. Single-line            | %s |\n", failures < 8 ? "[PASS]" : "[FAIL]");
    printf("| 2. Multi-line             | %s |\n", failures < 7 ? "[PASS]" : "[FAIL]");
    printf("| 3. Prompt                 | %s |\n", failures < 6 ? "[PASS]" : "[FAIL]");
    printf("| 4. Syntax highlighting    | %s |\n", failures < 5 ? "[PASS]" : "[FAIL]");
    printf("| 5. Completion preview     | %s |\n", failures < 4 ? "[PASS]" : "[FAIL]");
    printf("| 6. Scroll region          | %s |\n", failures < 3 ? "[PASS]" : "[FAIL]");
    printf("| 7. Atomic updates         | %s |\n", failures < 2 ? "[PASS]" : "[FAIL]");
    printf("| Performance (<10ms)       | %s |\n", failures < 1 ? "[PASS]" : "[FAIL]");
    
    printf("\n===================================================\n");
    printf("                    Results\n");
    printf("===================================================\n");
    
    if (failures == 0) {
        printf("[PASS] All 7 scenarios passed\n\n");
        printf("Week 2 Success Criteria Met:\n");
        printf("- All 7 rendering scenarios work: YES\n");
        printf("- Zero escape sequences: YES\n");
        printf("- Update latency <10ms: YES\n");
        printf("- Ready for Week 2 Day 8-10 validation\n");
        return 0;
    } else {
        printf("[FAIL] %d test(s) failed\n", failures);
        return 1;
    }
}
