#!/bin/bash

# Line Wrapping Test Script for Lusush Line Editor
# Tests line wrapping behavior with different terminal widths and long inputs

echo "=== Line Wrapping Test for Lusush Line Editor ==="
echo
echo "This script tests how LLE handles long lines that exceed terminal width:"
echo "  - Cursor positioning in wrapped lines"
echo "  - Ctrl+A/Ctrl+E behavior with wrapped text"
echo "  - Visual feedback consistency"
echo "  - Backspace behavior across line boundaries"
echo "  - Tab completion in wrapped lines"
echo

# Create test directory
echo "Setting up test environment..."
mkdir -p line_wrapping_test
cd line_wrapping_test

# Create some files for tab completion testing
touch very_long_filename_that_will_definitely_cause_wrapping.txt
touch another_extremely_long_filename_for_testing_purposes.txt
touch short.txt

echo "Created test files for tab completion:"
ls -1

echo
echo "=== Line Wrapping Test Cases ==="
echo

echo "TEST 1: Basic Long Line Input"
echo "  1. Type a very long command that exceeds terminal width"
echo "  2. Example: 'echo this is a very long line that should wrap around the terminal width and continue on the next line'"
echo "  3. Expected: Text wraps naturally, cursor follows correctly"
echo "  4. Issue: Check if cursor positioning is mathematically correct"
echo

echo "TEST 2: Ctrl+A in Wrapped Line"
echo "  1. Type long text that wraps (from TEST 1)"
echo "  2. Press Ctrl+A"
echo "  3. Expected: Cursor moves to very beginning (after prompt)"
echo "  4. Issue: Does cursor actually move visually?"
echo

echo "TEST 3: Ctrl+E in Wrapped Line"
echo "  1. From TEST 2, press Ctrl+E"
echo "  2. Expected: Cursor moves to end of wrapped text"
echo "  3. Issue: Does cursor end up at correct position in wrapped line?"
echo

echo "TEST 4: Backspace Across Wrap Boundary"
echo "  1. Type text that just barely wraps: 'echo ' + (terminal_width - 5) characters"
echo "  2. Backspace to unwrap the line"
echo "  3. Expected: Line unwraps smoothly, cursor repositions correctly"
echo "  4. Issue: Does backspace handle wrap boundary correctly?"
echo

echo "TEST 5: Tab Completion in Wrapped Line"
echo "  1. Type: 'echo very_long_command_prefix_that_wraps very'"
echo "  2. Press Tab to complete filename"
echo "  3. Expected: Completion works correctly despite wrapping"
echo "  4. Issue: Does tab completion handle wrapped lines?"
echo

echo "TEST 6: Terminal Resize During Edit"
echo "  1. Type a long line"
echo "  2. Resize terminal to be narrower"
echo "  3. Continue editing"
echo "  4. Expected: Editor adapts to new width"
echo "  5. Issue: Does resize handling work with wrapped content?"
echo

echo "=== Diagnostic Information ==="
echo "Current terminal dimensions:"
echo "  Columns: $(tput cols)"
echo "  Lines: $(tput lines)"
echo

# Test different terminal widths
echo "=== Testing Different Terminal Widths ==="

test_width() {
    local width=$1
    echo
    echo "--- Testing with width=$width ---"
    echo "Setting COLUMNS=$width"
    export COLUMNS=$width

    # Create test string that should wrap
    local test_string="echo "
    for i in $(seq 1 $((width - 10))); do
        test_string="${test_string}x"
    done
    test_string="${test_string}_END"

    echo "Test string length: ${#test_string} (should wrap at width $width)"
    echo "Test string: $test_string"

    if command -v expect >/dev/null 2>&1; then
        echo "Running automated test with expect..."
        expect << EOF
set timeout 5
spawn stty cols $width
spawn ../builddir/lusush
expect "$ "

# Type the test string
send "$test_string"
sleep 0.5

# Test Ctrl+A
send "\001"
sleep 0.5

# Test Ctrl+E
send "\005"
sleep 0.5

# Test backspace
send "\b\b\b"
sleep 0.5

send "\r"
expect "$ "

send "exit\r"
expect eof
EOF
    else
        echo "Expect not available for automated testing"
    fi
}

# Test with different widths
for width in 40 60 80 120; do
    test_width $width
done

echo
echo "=== Manual Testing Instructions ==="
echo "To manually test line wrapping issues:"
echo
echo "1. Start lusush with debug output:"
echo "   LLE_DEBUG=1 ../builddir/lusush"
echo
echo "2. Test long line input:"
echo "   echo this_is_a_very_long_line_that_should_definitely_wrap_around_the_terminal_and_continue_on_next_line"
echo
echo "3. Watch for debug output showing:"
echo "   - [CURSOR_MATH] calculations"
echo "   - [LLE_DISPLAY_INCREMENTAL] wrapping messages"
echo "   - Cursor positioning coordinates"
echo
echo "4. Test cursor movement in wrapped line:"
echo "   - Type long line"
echo "   - Press Ctrl+A (should go to beginning)"
echo "   - Press Ctrl+E (should go to end)"
echo "   - Check if cursor visually moves correctly"
echo
echo "5. Test backspace across wrap boundary:"
echo "   - Type text that just wraps"
echo "   - Backspace to unwrap"
echo "   - Check if display updates correctly"
echo

echo "=== Expected Issues ==="
echo "Based on the keybinding problems reported:"
echo
echo "❌ LIKELY ISSUES:"
echo "  - Ctrl+A: Buffer position changes but cursor doesn't move visually"
echo "  - Ctrl+E: Cursor positioning calculation wrong in wrapped lines"
echo "  - Backspace: Unwrapping doesn't update display correctly"
echo "  - Cursor math: Mathematical errors in wrap boundary calculations"
echo
echo "✅ ROOT CAUSE ANALYSIS:"
echo "  - Cursor math calculations may be correct in buffer"
echo "  - Visual cursor positioning may not match buffer position"
echo "  - Display update system may not handle wrapped lines properly"
echo "  - Terminal positioning commands may not account for wrapping"
echo

echo "=== Starting Manual Test Session ==="
echo "Environment configured for line wrapping tests"
echo "Terminal width: $(tput cols)"
echo "Debug output enabled"
echo

# Export debug flags
export LLE_DEBUG=1
export LLE_DEBUG_CURSOR=1

echo "Starting lusush with line wrapping debug enabled..."
echo "Try the test cases above and observe cursor behavior"
echo "Type 'exit' when finished testing"
echo

# Start lusush for manual testing
../builddir/lusush

# Cleanup
echo
echo "=== Cleanup ==="
cd ..
rm -rf line_wrapping_test
echo "Line wrapping test completed!"

echo
echo "=== Analysis Questions ==="
echo "After testing, consider these questions:"
echo "1. Does the cursor visually move when using Ctrl+A/Ctrl+E in wrapped lines?"
echo "2. Are cursor math calculations correct according to debug output?"
echo "3. Does backspace properly unwrap lines and reposition cursor?"
echo "4. Are there discrepancies between buffer position and visual position?"
echo "5. Does the display system handle wrap boundaries correctly?"
echo
echo "These answers will help identify whether the issue is in:"
echo "- Cursor mathematics (cursor_math.c)"
echo "- Display rendering (display.c)"
echo "- Terminal positioning (terminal_manager.c)"
echo "- Command handling (edit_commands.c)"
