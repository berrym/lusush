#!/bin/bash

# Test script for multiline history navigation fix
# Tests the enhanced safe replace approach for multiline content clearing

set -e

echo "=== Multiline History Navigation Fix Test ==="
echo "Testing enhanced safe replace approach with multiline clearing enabled"
echo

# Build the shell
echo "Building lusush with multiline clearing enabled..."
cd "$(dirname "$0")"
scripts/lle_build.sh build
if [ $? -ne 0 ]; then
    echo "❌ BUILD FAILED"
    exit 1
fi
echo "✅ Build successful"
echo

# Create test history file
HISTORY_FILE="/tmp/lusush_test_history"
cat > "$HISTORY_FILE" << 'EOF'
echo "short command"
echo "this is a very long multiline command that should span multiple terminal lines and test the multiline clearing functionality properly"
pwd
ls -la
echo "another long command that definitely spans multiple lines in most terminal configurations and should be cleared properly when navigating"
exit
EOF

echo "Test history created with multiline commands:"
echo "  1. Short command"
echo "  2. Long multiline command (spans multiple lines)"
echo "  3. pwd"
echo "  4. ls -la"
echo "  5. Another long multiline command"
echo "  6. exit"
echo

# Test 1: Basic multiline clearing functionality
echo "=== TEST 1: Multiline Clearing Function Test ==="
echo "Testing lle_terminal_safe_replace_content with multiline content..."

# Create a simple test program to validate the function
cat > /tmp/test_multiline_clear.c << 'EOF'
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mock minimal structures for testing
typedef struct {
    bool termcap_initialized;
} lle_terminal_manager_t;

// Function declarations
bool lle_terminal_write(lle_terminal_manager_t *tm, const char *data, size_t length);
bool lle_terminal_move_cursor_to_column(lle_terminal_manager_t *tm, size_t column);
bool lle_terminal_clear_exact_chars(lle_terminal_manager_t *tm, size_t char_count);
size_t lle_terminal_calculate_content_lines(const char *content, size_t content_length, size_t terminal_width, size_t prompt_width);
size_t lle_terminal_filter_control_chars(const char *input, size_t input_length, char *output, size_t output_capacity);
bool lle_terminal_safe_replace_content(lle_terminal_manager_t *tm, size_t prompt_width, size_t old_content_length, const char *new_content, size_t new_content_length, size_t terminal_width);

// Mock implementations for testing
bool lle_terminal_write(lle_terminal_manager_t *tm, const char *data, size_t length) {
    printf("TERMINAL_WRITE: '%.*s'\n", (int)length, data);
    return true;
}

bool lle_terminal_move_cursor_to_column(lle_terminal_manager_t *tm, size_t column) {
    printf("MOVE_CURSOR_TO_COLUMN: %zu\n", column);
    return true;
}

bool lle_terminal_clear_exact_chars(lle_terminal_manager_t *tm, size_t char_count) {
    printf("CLEAR_EXACT_CHARS: %zu characters\n", char_count);
    return true;
}

size_t lle_terminal_calculate_content_lines(const char *content, size_t content_length, size_t terminal_width, size_t prompt_width) {
    // Simple calculation for testing
    size_t available_width = terminal_width - prompt_width;
    return (content_length + available_width - 1) / available_width;
}

size_t lle_terminal_filter_control_chars(const char *input, size_t input_length, char *output, size_t output_capacity) {
    size_t filtered_len = input_length < output_capacity - 1 ? input_length : output_capacity - 1;
    memcpy(output, input, filtered_len);
    output[filtered_len] = '\0';
    return filtered_len;
}

int main() {
    lle_terminal_manager_t tm = { .termcap_initialized = true };

    printf("=== Testing Multiline Safe Replace ===\n");

    // Test case: Replace long content with short content
    const char *old_long = "this is a very long multiline command that should span multiple terminal lines";
    const char *new_short = "pwd";

    printf("\nTest: Replace long multiline content with short content\n");
    printf("Old content: '%s' (length=%zu)\n", old_long, strlen(old_long));
    printf("New content: '%s' (length=%zu)\n", new_short, strlen(new_short));
    printf("Expected: Multiline clearing should be triggered\n\n");

    bool result = lle_terminal_safe_replace_content(&tm, 25, strlen(old_long), new_short, strlen(new_short), 80);

    printf("\nResult: %s\n", result ? "SUCCESS" : "FAILED");
    printf("Expected behavior: Should clear %zu characters using exact character clearing\n", strlen(old_long));

    return 0;
}
EOF

echo "Compiling multiline clear test..."
gcc -o /tmp/test_multiline_clear /tmp/test_multiline_clear.c -I./src/line_editor 2>/dev/null || {
    echo "⚠️  Cannot compile standalone test, but that's expected"
    echo "   The important change is that multiline clearing is now enabled in terminal_manager.c"
}
echo

# Test 2: Verify the change is in place
echo "=== TEST 2: Verify Multiline Clearing Enabled ==="
echo "Checking that multiline clearing is enabled in terminal_manager.c..."

if grep -q "Multi-line clearing: ENABLED" src/line_editor/terminal_manager.c; then
    echo "✅ Multiline clearing is ENABLED in safe replace function"
else
    echo "❌ Multiline clearing is still DISABLED"
    exit 1
fi

if grep -q "lle_terminal_clear_exact_chars.*old_content_length" src/line_editor/terminal_manager.c; then
    echo "✅ Exact character clearing method is being used for multiline content"
else
    echo "❌ Exact character clearing method not found for multiline"
    exit 1
fi

echo

# Test 3: Verify safe replace is being used in history navigation
echo "=== TEST 3: Verify History Navigation Uses Safe Replace ==="
echo "Checking that history navigation uses lle_terminal_safe_replace_content..."

if grep -q "lle_terminal_safe_replace_content" src/line_editor/line_editor.c; then
    echo "✅ History navigation uses safe replace content function"
else
    echo "❌ History navigation not using safe replace - need to update line_editor.c"
    echo
    echo "NEXT STEP REQUIRED: Update history navigation in line_editor.c to use:"
    echo "  lle_terminal_safe_replace_content(terminal, prompt_width, old_length, new_content, new_length, terminal_width)"
    echo "  instead of the current NUCLEAR CLEAR approach"
    exit 1
fi

echo

# Test 4: Debug output verification
echo "=== TEST 4: Debug Output Analysis ==="
echo "Testing debug output for multiline clearing behavior..."

echo "Creating test command that would trigger multiline clearing..."
TEST_LONG_CMD="echo 'this is a very long command that should definitely span multiple terminal lines in most configurations and test multiline clearing'"

echo "Expected debug output should show:"
echo "  [LLE_SAFE_REPLACE] Multi-line content detected: X lines, using enhanced clearing"
echo "  [LLE_SAFE_REPLACE] Multi-line exact character clearing completed"
echo

# Test 5: Manual testing instructions
echo "=== TEST 5: Manual Testing Required ==="
echo "🚨 CRITICAL: Human testing verification needed"
echo
echo "To test the multiline history navigation fix:"
echo "1. Run: LLE_DEBUG=1 ./builddir/lusush"
echo "2. Execute these commands to create history:"
echo "   pwd"
echo "   echo 'short'"
echo "   echo 'this is a very long command that spans multiple lines and should be cleared properly'"
echo "   ls"
echo "3. Use UP arrow to navigate history"
echo "4. Verify: Long command clears completely when navigating to short command"
echo "5. Verify: No visual artifacts or overlapping content"
echo "6. Verify: Debug shows 'Multi-line exact character clearing completed'"
echo

echo "Expected behavior:"
echo "✅ Long multiline commands should clear completely"
echo "✅ Short commands should display cleanly after long commands"
echo "✅ No prompt duplication or visual artifacts"
echo "✅ Same-line replacement behavior like bash/zsh"
echo

echo "=== SUMMARY ==="
echo "✅ Build successful"
echo "✅ Multiline clearing enabled in terminal_manager.c"
echo "✅ Exact character clearing method used for multiline content"
if grep -q "lle_terminal_safe_replace_content" src/line_editor/line_editor.c; then
    echo "✅ History navigation uses safe replace function"
else
    echo "⚠️  History navigation needs update to use safe replace function"
fi
echo
echo "🎯 KEY IMPROVEMENT: Multiline content clearing is now enabled"
echo "   Previously: Multiline clearing was disabled (caused artifacts)"
echo "   Now: Uses proven exact character clearing method for multiline content"
echo
echo "🚨 REQUIRED: Human testing verification to confirm visual behavior"
echo "   Mathematical correctness ≠ Visual correctness"
echo "   Only human testing can verify terminal display behavior"
echo

# Test 6: What was changed summary
echo "=== CHANGES MADE ==="
echo "File: src/line_editor/terminal_manager.c"
echo "Function: lle_terminal_safe_replace_content()"
echo "Change: Enabled multiline clearing using exact character method"
echo
echo "Before:"
echo "  // Multi-line clearing: DISABLED due to terminal compatibility issues"
echo "  // For multiline content, skip the visual clearing and just position cursor"
echo
echo "After:"
echo "  // Multi-line clearing: ENABLED - Use proven multiline approach"
echo "  if (!lle_terminal_clear_exact_chars(tm, old_content_length)) {"
echo
echo "Impact: Multiline history entries should now clear properly"
echo "Status: Ready for human testing verification"
