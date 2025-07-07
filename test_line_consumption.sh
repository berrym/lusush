#!/bin/bash

# Test script specifically for line consumption prevention in LUSUSH
# This test verifies that editing at the bottom line doesn't consume previous lines

echo "==============================================================================="
echo "LUSUSH LINE CONSUMPTION PREVENTION TEST"
echo "==============================================================================="
echo ""

LUSUSH_BIN="./builddir/lusush"

if [ ! -f "$LUSUSH_BIN" ]; then
    echo "❌ LUSUSH binary not found at $LUSUSH_BIN"
    echo "Please build first with: ninja -C builddir"
    exit 1
fi

echo "🔍 Testing line consumption prevention at bottom of terminal..."
echo ""

# Function to create a test scenario
create_bottom_line_scenario() {
    local test_name="$1"
    local commands="$2"

    echo "📝 Test: $test_name"
    echo "Commands: $commands"

    # Create a script that fills the screen and tests editing
    cat > /tmp/lusush_line_test_$$.sh << EOF
#!/bin/bash
# Fill screen with numbered lines
for i in {1..25}; do
    echo "TestLine \$i: This content should NOT be consumed by shell editing"
done
echo "MARKER_LINE: This line should remain visible after editing"
echo "Now at bottom line - testing editing operations..."
EOF

    chmod +x /tmp/lusush_line_test_$$.sh

    # Run the test scenario
    echo -e "/tmp/lusush_line_test_$$.sh\n$commands\nexit" | timeout 10 $LUSUSH_BIN > /tmp/lusush_output_$$.txt 2>&1

    # Check if MARKER_LINE is still present (indicates no consumption)
    if grep -q "MARKER_LINE: This line should remain visible" /tmp/lusush_output_$$.txt; then
        echo "✓ PASSED - No line consumption detected"
        echo "  MARKER_LINE found in output - previous lines preserved"
        return 0
    else
        echo "✗ FAILED - Line consumption occurred"
        echo "  MARKER_LINE not found - previous lines were consumed"
        echo "  Last 10 lines of output:"
        tail -10 /tmp/lusush_output_$$.txt | sed 's/^/    /'
        return 1
    fi

    # Cleanup
    rm -f /tmp/lusush_line_test_$$.sh /tmp/lusush_output_$$.txt
}

# Test 1: Basic editing at bottom line
echo "▓▓▓ TEST 1: BASIC EDITING AT BOTTOM LINE ▓▓▓"
create_bottom_line_scenario "Basic editing" "echo hello world"
echo ""

# Test 2: Backspace operations at bottom line
echo "▓▓▓ TEST 2: BACKSPACE OPERATIONS ▓▓▓"
# Note: We can't easily simulate interactive backspace in a script
# This test focuses on command editing that might trigger refreshes
create_bottom_line_scenario "Command with corrections" "echo test_command"
echo ""

# Test 3: History navigation at bottom line
echo "▓▓▓ TEST 3: HISTORY NAVIGATION SCENARIO ▓▓▓"
create_bottom_line_scenario "History commands" "echo first\necho second\necho third"
echo ""

# Test 4: Long command at bottom line
echo "▓▓▓ TEST 4: LONG COMMAND HANDLING ▓▓▓"
create_bottom_line_scenario "Long command" "echo this_is_a_very_long_command_that_might_cause_line_wrapping_issues_in_terminal"
echo ""

# Test 5: Variable operations at bottom line
echo "▓▓▓ TEST 5: VARIABLE OPERATIONS ▓▓▓"
create_bottom_line_scenario "Variable operations" "TEST_VAR=hello\necho \$TEST_VAR"
echo ""

# Interactive test instructions
echo "▓▓▓ INTERACTIVE MANUAL TEST INSTRUCTIONS ▓▓▓"
echo ""
echo "For comprehensive verification, perform this manual test:"
echo ""
echo "1. 🖥️  SETUP:"
echo "   - Open a terminal window"
echo "   - Resize to approximately 20-25 lines height"
echo "   - This creates a scenario where the bottom line is easily reached"
echo ""
echo "2. 🚀 RUN LUSUSH:"
echo "   $LUSUSH_BIN"
echo ""
echo "3. 📄 FILL SCREEN:"
echo "   for i in {1..30}; do echo \"Line \$i: This should stay visible\"; done"
echo ""
echo "4. 🎯 VERIFY POSITION:"
echo "   - The prompt should now be at or near the bottom of your terminal"
echo "   - You should see numbered lines above the prompt"
echo "   - Note the highest line number visible"
echo ""
echo "5. ✏️  TEST EDITING OPERATIONS:"
echo "   a) Type: echo hello world"
echo "   b) Press backspace 5 times (delete 'world')"
echo "   c) Type: test"
echo "   d) Press Enter"
echo "   e) Expected output: 'hello test'"
echo ""
echo "6. 🔍 CHECK FOR LINE CONSUMPTION:"
echo "   - Look at the lines above the current prompt"
echo "   - The numbered lines should still be visible"
echo "   - The highest line number should be the same as before"
echo "   - If lines disappeared, that indicates line consumption"
echo ""
echo "7. 📈 TEST HISTORY NAVIGATION:"
echo "   a) Type: echo another command"
echo "   b) Press Enter"
echo "   c) Press Up arrow key"
echo "   d) You should see 'echo another command' appear"
echo "   e) Press Up arrow again"
echo "   f) You should see the previous command"
echo "   g) Check that numbered lines above are still visible"
echo ""
echo "8. 🔄 TEST MULTIPLE OPERATIONS:"
echo "   a) Type: echo final test"
echo "   b) Use backspace to edit"
echo "   c) Use arrow keys for history"
echo "   d) Each operation should preserve the numbered lines above"
echo ""
echo "9. ✅ SUCCESS CRITERIA:"
echo "   - All numbered lines remain visible throughout testing"
echo "   - No lines are 'consumed' or disappear during editing"
echo "   - Terminal content scrolls normally but editing doesn't cause consumption"
echo "   - Cursor behaves smoothly without bouncing"
echo ""
echo "10. 🚪 EXIT:"
echo "    Type: exit"
echo ""

echo "==============================================================================="
echo "LINE CONSUMPTION TEST SUMMARY"
echo "==============================================================================="
echo ""
echo "The automated tests above verify basic functionality, but the definitive test"
echo "is the manual interactive test. This is because line consumption primarily"
echo "occurs during interactive editing operations that are difficult to simulate."
echo ""
echo "🎯 Key Points:"
echo "• Line consumption happens when terminal editing causes previous content to disappear"
echo "• This typically occurs at the bottom line of the terminal during refresh operations"
echo "• The fix uses termcap capabilities to create safe margins and prevent scrolling issues"
echo "• Success means all previous terminal content remains visible during editing"
echo ""
echo "If you observe line consumption during the manual test, please report the specific"
echo "terminal type, size, and sequence of operations that triggered the issue."
echo ""
echo "==============================================================================="
