#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include LLE headers
#include "src/line_editor/display.h"

/**
 * Test boundary crossing math validation
 * 
 * This test validates that the visual footprint calculation is correct
 * for boundary crossing scenarios without requiring full terminal setup.
 */

static bool test_visual_footprint_boundary_math() {
    printf("🧪 Testing visual footprint boundary crossing math...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test case 1: Text that exactly hits boundary (should wrap)
    // 82 (prompt) + 38 (text) = 120 (exactly at terminal width)
    const char *boundary_text = "12345678901234567890123456789012345678";  // exactly 38 chars
    printf("🔍 Debug: boundary_text length = %zu\n", strlen(boundary_text));
    if (!lle_calculate_visual_footprint(boundary_text, strlen(boundary_text), 82, 120, &footprint)) {
        printf("❌ Failed to calculate footprint for boundary text\n");
        return false;
    }
    
    printf("📊 Boundary text (38 chars) footprint: rows=%zu, end_col=%zu, wraps=%s\n",
           footprint.rows_used, footprint.end_column, footprint.wraps_lines ? "true" : "false");
    
    // Should wrap to 2 rows, end at column 1
    if (footprint.rows_used != 2 || footprint.end_column != 1 || !footprint.wraps_lines) {
        printf("❌ Incorrect footprint for boundary text\n");
        printf("   Expected: rows=2, end_col=1, wraps=true\n");
        printf("   Got:      rows=%zu, end_col=%zu, wraps=%s\n",
               footprint.rows_used, footprint.end_column, footprint.wraps_lines ? "true" : "false");
        return false;
    }
    
    // Test case 2: Text one character shorter (should be single line)
    // 82 (prompt) + 37 (text) = 119 (just under terminal width)
    const char *single_line_text = "1234567890123456789012345678901234567";   // exactly 37 chars
    printf("🔍 Debug: single_line_text length = %zu\n", strlen(single_line_text));
    printf("🔍 Debug: expected total = 82 + %zu = %zu\n", strlen(single_line_text), 82 + strlen(single_line_text));
    if (!lle_calculate_visual_footprint(single_line_text, strlen(single_line_text), 82, 120, &footprint)) {
        printf("❌ Failed to calculate footprint for single line text\n");
        return false;
    }
    
    printf("📊 Single line text (37 chars) footprint: rows=%zu, end_col=%zu, wraps=%s\n",
           footprint.rows_used, footprint.end_column, footprint.wraps_lines ? "true" : "false");
    
    // Should be single row, end at column 119
    if (footprint.rows_used != 1 || footprint.end_column != 119 || footprint.wraps_lines) {
        printf("❌ Incorrect footprint for single line text\n");
        printf("   Expected: rows=1, end_col=119, wraps=false\n");
        printf("   Got:      rows=%zu, end_col=%zu, wraps=%s\n",
               footprint.rows_used, footprint.end_column, footprint.wraps_lines ? "true" : "false");
        return false;
    }
    
    printf("✅ Visual footprint boundary math test passed!\n");
    return true;
}

static bool test_cursor_position_calculation() {
    printf("🧪 Testing cursor position calculation for boundary crossing...\n");
    
    // Test the exact scenario from the debug logs
    size_t prompt_width = 82;
    size_t terminal_width = 120;
    
    // Before boundary crossing: 38 characters (82 + 38 = 120)
    size_t text_length_before = 38;
    size_t expected_total_before = prompt_width + text_length_before;
    printf("📊 Before deletion: prompt=%zu + text=%zu = %zu (terminal=%zu)\n", 
           prompt_width, text_length_before, expected_total_before, terminal_width);
    
    // After boundary crossing: 37 characters (82 + 37 = 119)
    size_t text_length_after = 37;
    size_t expected_total_after = prompt_width + text_length_after;
    printf("📊 After deletion: prompt=%zu + text=%zu = %zu (terminal=%zu)\n", 
           prompt_width, text_length_after, expected_total_after, terminal_width);
    
    // The issue was that after boundary crossing, cursor was at position 120 instead of 119
    // This test validates that our expected math is correct
    if (expected_total_after != 119) {
        printf("❌ Expected cursor position calculation is wrong\n");
        printf("   Expected: 119, Got: %zu\n", expected_total_after);
        return false;
    }
    
    // Test visual footprint for both cases
    lle_visual_footprint_t footprint_before, footprint_after;
    
    const char *test_text_38 = "12345678901234567890123456789012345678";  // exactly 38 chars
    const char *test_text_37 = "1234567890123456789012345678901234567";   // exactly 37 chars
    
    if (!lle_calculate_visual_footprint(test_text_38, 38, prompt_width, terminal_width, &footprint_before) ||
        !lle_calculate_visual_footprint(test_text_37, 37, prompt_width, terminal_width, &footprint_after)) {
        printf("❌ Failed to calculate visual footprints\n");
        return false;
    }
    
    printf("📊 Before: rows=%zu, end_col=%zu (boundary crossing detection)\n",
           footprint_before.rows_used, footprint_before.end_column);
    printf("📊 After:  rows=%zu, end_col=%zu (should be 119)\n",
           footprint_after.rows_used, footprint_after.end_column);
    
    // Validate boundary crossing detection
    bool boundary_crossing = (footprint_before.rows_used != footprint_after.rows_used) ||
                            (footprint_before.wraps_lines && !footprint_after.wraps_lines);
    
    if (!boundary_crossing) {
        printf("❌ Boundary crossing not detected\n");
        return false;
    }
    
    // Validate cursor position after boundary crossing
    if (footprint_after.end_column != 119) {
        printf("❌ Cursor position after boundary crossing is incorrect\n");
        printf("   Expected: 119, Got: %zu\n", footprint_after.end_column);
        printf("   This is the bug our fix is designed to address!\n");
        
        // Show what the fix should do
        size_t corrected_position = prompt_width + text_length_after;
        printf("🔧 Fix: Cursor should be positioned at %zu\n", corrected_position);
        
        return false;
    }
    
    printf("✅ Cursor position calculation test passed!\n");
    return true;
}

static bool test_boundary_crossing_detection() {
    printf("🧪 Testing boundary crossing detection logic...\n");
    
    size_t prompt_width = 82;
    size_t terminal_width = 120;
    
    lle_visual_footprint_t footprint_before, footprint_after;
    
    // Create test strings that cross boundary
    const char *before_text = "12345678901234567890123456789012345678";  // exactly 38 chars -> wraps
    const char *after_text = "1234567890123456789012345678901234567";   // exactly 37 chars -> single line
    
    printf("🔍 Debug: before_text length = %zu\n", strlen(before_text));
    printf("🔍 Debug: after_text length = %zu\n", strlen(after_text));
    
    if (!lle_calculate_visual_footprint(before_text, strlen(before_text), prompt_width, terminal_width, &footprint_before) ||
        !lle_calculate_visual_footprint(after_text, strlen(after_text), prompt_width, terminal_width, &footprint_after)) {
        printf("❌ Failed to calculate visual footprints for boundary detection\n");
        return false;
    }
    
    // Test the boundary crossing detection logic from the actual code
    bool crossing_wrap_boundary = (footprint_before.rows_used != footprint_after.rows_used) ||
                                 (footprint_before.wraps_lines && !footprint_after.wraps_lines);
    
    printf("📊 Boundary crossing detection:\n");
    printf("   Before: rows=%zu, wraps=%s\n", footprint_before.rows_used, footprint_before.wraps_lines ? "true" : "false");
    printf("   After:  rows=%zu, wraps=%s\n", footprint_after.rows_used, footprint_after.wraps_lines ? "true" : "false");
    printf("   Crossing detected: %s\n", crossing_wrap_boundary ? "true" : "false");
    
    if (!crossing_wrap_boundary) {
        printf("❌ Boundary crossing not detected when it should be\n");
        return false;
    }
    
    printf("✅ Boundary crossing detection test passed!\n");
    return true;
}

int main() {
    printf("🚀 Starting boundary crossing math validation\n\n");
    
    bool all_tests_passed = true;
    
    // Test 1: Visual footprint calculation
    if (!test_visual_footprint_boundary_math()) {
        printf("❌ Visual footprint boundary math test failed\n");
        all_tests_passed = false;
    }
    printf("\n");
    
    // Test 2: Cursor position calculation
    if (!test_cursor_position_calculation()) {
        printf("❌ Cursor position calculation test failed\n");
        all_tests_passed = false;
    }
    printf("\n");
    
    // Test 3: Boundary crossing detection
    if (!test_boundary_crossing_detection()) {
        printf("❌ Boundary crossing detection test failed\n");
        all_tests_passed = false;
    }
    printf("\n");
    
    if (all_tests_passed) {
        printf("🎉 All boundary crossing math tests passed!\n");
        printf("📋 Summary:\n");
        printf("   ✅ Visual footprint calculation is mathematically correct\n");
        printf("   ✅ Cursor position math is accurate (119 after 37 chars)\n");
        printf("   ✅ Boundary crossing detection works properly\n");
        printf("\n");
        printf("🔧 The surgical fix in display.c should correct any remaining cursor positioning issues.\n");
        return 0;
    } else {
        printf("💥 Some math validation tests failed.\n");
        printf("🔍 This indicates the root cause needs further investigation.\n");
        return 1;
    }
}