#!/bin/bash

# Focused Test for Syntax Highlighting and Tab Completion Wrapping Fixes
# Tests the specific issues that emerged after the line wrapping fix

echo "=== Lusush Line Editor: Wrapping Fixes Validation Test ==="
echo
echo "This test validates fixes for:"
echo "  1. Syntax highlighting stopping on wrapped lines"
echo "  2. Tab completion rendering issues across wrapped lines"
echo

# Get current terminal dimensions
TERM_WIDTH=$(tput cols)
TERM_HEIGHT=$(tput lines)

echo "Current terminal: ${TERM_WIDTH}x${TERM_HEIGHT}"
echo

# Create test environment
echo "Setting up test environment..."
mkdir -p wrapping_fixes_test
cd wrapping_fixes_test

# Create files for tab completion testing
touch very_long_filename_that_should_cause_wrapping_in_most_terminals.txt
touch another_extremely_long_filename_for_testing_tab_completion.txt
touch short.txt
touch test_file.txt

echo "Test files created:"
ls -1
echo

echo "=== TEST 1: Terminal Width Detection Verification ==="
echo
echo "OBJECTIVE: Verify that LLE detects actual terminal width instead of hardcoded 80"
echo
echo "Expected behavior:"
echo "  - Debug output should show terminal width as ${TERM_WIDTH}, not 80"
echo "  - Line wrapping should occur at column ${TERM_WIDTH}, not 80"
echo
echo "Commands to test:"
echo "  1. Start lusush with LLE_DEBUG=1"
echo "  2. Look for '[LLE_DISPLAY_INIT]' messages showing terminal size"
echo "  3. Type a long line and observe where it wraps"
echo

echo "=== TEST 2: Syntax Highlighting Across Wrapped Lines ==="
echo
echo "OBJECTIVE: Verify syntax highlighting continues correctly on wrapped lines"
echo
echo "Test command (adjust length for your terminal width):"
# Create a command that will wrap at current terminal width
LONG_STRING="'this is a very long string that should definitely wrap to the next line and maintain syntax highlighting throughout the entire wrapped text'"
echo "echo ${LONG_STRING}"
echo
echo "Expected behavior:"
echo "  - 'echo' should be highlighted in blue (command color)"
echo "  - String quotes and content should be highlighted in green"
echo "  - Highlighting should continue on the wrapped portion"
echo "  - No color reset at wrap boundary"
echo
echo "Failure indicators:"
echo "  - Highlighting stops at wrap boundary"
echo "  - Wrapped text appears in default color"
echo "  - Color codes visible in output"
echo

echo "=== TEST 3: Tab Completion on Wrapped Lines ==="
echo
echo "OBJECTIVE: Verify tab completion works correctly when cursor is on wrapped line"
echo
echo "Test sequence:"
echo "  1. Type a long prefix that fills most of terminal width"
echo "  2. Add partial filename and press Tab"
echo "  3. Verify completion list appears correctly"
echo
LONG_PREFIX=$(printf 'echo %*s' $((TERM_WIDTH - 15)) '' | tr ' ' 'x')
echo "Example prefix: ${LONG_PREFIX} very<TAB>"
echo
echo "Expected behavior:"
echo "  - Completion list appears below current line"
echo "  - List is properly formatted within terminal width"
echo "  - Cursor returns to correct position after completion"
echo "  - No display corruption"
echo
echo "Failure indicators:"
echo "  - Completion list appears at wrong position"
echo "  - List extends beyond terminal width"
echo "  - Cursor positioning incorrect after completion"
echo "  - Display corruption or extra newlines"
echo

echo "=== TEST 4: Multiple Completions with Wrapping ==="
echo
echo "OBJECTIVE: Test completion list display when multiple matches exist"
echo
echo "Test sequence:"
echo "  1. Type long prefix + ambiguous completion"
echo "  2. Press Tab to see multiple matches"
echo "  3. Verify list formatting"
echo
echo "Example: ${LONG_PREFIX} test<TAB>"
echo
echo "Expected behavior:"
echo "  - All matching files shown in formatted list"
echo "  - List respects terminal width boundaries"
echo "  - Selection indicator works correctly"
echo "  - Clean return to command line"
echo

echo "=== AUTOMATED DETECTION TEST ==="
echo
echo "Testing terminal width detection..."

# Set debug flags
export LLE_DEBUG=1
export LLE_DEBUG_DISPLAY=1

echo "Starting lusush to capture terminal width detection..."
echo "Look for these debug messages:"
echo "  '[LLE_DISPLAY_INIT] Terminal size detected: ${TERM_WIDTH}x${TERM_HEIGHT}'"
echo "  '[LLE_DISPLAY_RENDER] Terminal resized: ...' (if any)"
echo

# Test if we can capture the debug output
echo "=== STARTING LUSUSH WITH DEBUG OUTPUT ==="
echo "You should see terminal size detection messages below:"
echo "Then test the syntax highlighting and tab completion manually."
echo

# Run a quick test command in lusush
expect << 'EOF' 2>/dev/null || {
echo "Expect not available for automated testing"
echo "Please run manually: LLE_DEBUG=1 ../builddir/lusush"
echo
}
set timeout 10
spawn env LLE_DEBUG=1 ../builddir/lusush
expect {
    "Terminal size detected" {
        puts "✅ Terminal size detection working"
        send "echo 'syntax highlighting test with a very long string that should wrap'\r"
        expect "$ "
        send "very\t"
        expect "$ "
        send "exit\r"
        expect eof
    }
    timeout {
        puts "❌ No terminal size detection message seen"
        send "exit\r"
        expect eof
    }
}
EOF

echo
echo "=== MANUAL TESTING INSTRUCTIONS ==="
echo
echo "1. Start lusush with debug output:"
echo "   LLE_DEBUG=1 ../builddir/lusush"
echo
echo "2. Verify terminal width detection:"
echo "   Look for: '[LLE_DISPLAY_INIT] Terminal size detected: ${TERM_WIDTH}x${TERM_HEIGHT}'"
echo
echo "3. Test syntax highlighting on wrapped lines:"
echo "   Type: echo 'very long string that wraps to test syntax highlighting continuation'"
echo "   Expected: Command and string should stay highlighted across wrap"
echo
echo "4. Test tab completion on wrapped lines:"
echo "   Type: echo very_long_command_that_fills_most_of_terminal_width very<TAB>"
echo "   Expected: Completion list appears properly formatted"
echo
echo "5. Test multiple completions:"
echo "   Type: ${LONG_PREFIX} test<TAB>"
echo "   Expected: Multiple matches shown in clean list format"
echo

echo "=== SUCCESS CRITERIA ==="
echo
echo "✅ PASS CONDITIONS:"
echo "  - Debug output shows correct terminal width (${TERM_WIDTH}, not 80)"
echo "  - Syntax highlighting continues across wrapped lines"
echo "  - Tab completion lists appear at correct positions"
echo "  - No display corruption during completion"
echo "  - Cursor positioning accurate after all operations"
echo
echo "❌ FAIL CONDITIONS:"
echo "  - Debug shows 80x24 instead of actual terminal size"
echo "  - Syntax highlighting stops at wrap boundary"
echo "  - Completion lists appear at wrong positions"
echo "  - Display corruption or positioning errors"
echo

echo "=== STARTING INTERACTIVE TEST ==="
echo "Press Enter to start lusush for manual testing..."
read -r

# Change back to parent directory for testing
cd ..

# Start lusush for interactive testing
LLE_DEBUG=1 ./builddir/lusush

# Cleanup
echo
echo "=== CLEANUP ==="
rm -rf wrapping_fixes_test
echo "Test environment cleaned up"

echo
echo "=== TEST COMPLETION ==="
echo "If all tests passed, the line wrapping fixes are working correctly."
echo "If any tests failed, additional fixes may be needed for:"
echo "  - Syntax highlighting segment rendering"
echo "  - Completion list positioning calculations"
echo "  - Terminal geometry integration"
