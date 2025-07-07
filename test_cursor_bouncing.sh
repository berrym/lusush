#!/bin/bash

# Simple test to isolate cursor bouncing issue in LUSUSH
# This test will help identify what specific scenarios cause cursor bouncing

echo "==============================================================================="
echo "LUSUSH CURSOR BOUNCING ISOLATION TEST"
echo "==============================================================================="
echo ""

LUSUSH_BIN="./builddir/lusush"

if [ ! -f "$LUSUSH_BIN" ]; then
    echo "❌ LUSUSH binary not found at $LUSUSH_BIN"
    echo "Please build first with: ninja -C builddir"
    exit 1
fi

echo "🔍 Testing cursor behavior in different scenarios..."
echo ""

# Test 1: Basic character deletion
echo "📝 Test 1: Basic character deletion"
echo "   Starting lusush, type 'hello world', then backspace 5 times"
echo "   Expected: Clean deletion without cursor bouncing"
echo "   Command: echo -e 'hello world\b\b\b\b\b' | $LUSUSH_BIN"
echo ""

# Test 2: History navigation
echo "📝 Test 2: History navigation"
echo "   Testing history up/down navigation"
echo "   Expected: Smooth history navigation without cursor bouncing"
echo ""

# Test 3: Long line editing
echo "📝 Test 3: Long line editing"
echo "   Testing with long lines that might wrap"
echo "   Expected: Proper handling without cursor issues"
echo ""

# Test 4: Bottom-line scenario
echo "📝 Test 4: Bottom-line scenario"
echo "   Fill screen and test editing at bottom line"
echo "   Expected: No cursor jumping or line consumption"
echo ""

echo "==============================================================================="
echo "INTERACTIVE CURSOR BOUNCING TEST"
echo "==============================================================================="
echo ""
echo "The following tests require manual interaction:"
echo ""

echo "🧪 Manual Test 1: Character Deletion"
echo "   1. Run: $LUSUSH_BIN"
echo "   2. Type: echo hello world"
echo "   3. Press backspace 5 times"
echo "   4. Type: test"
echo "   5. Press Enter"
echo "   6. Observe: Does cursor bounce or stay stable?"
echo "   7. Type: exit"
echo ""

echo "🧪 Manual Test 2: History Navigation"
echo "   1. Run: $LUSUSH_BIN"
echo "   2. Type: echo first command (press Enter)"
echo "   3. Type: echo second command (press Enter)"
echo "   4. Press Up arrow key"
echo "   5. Observe: Does cursor bounce when history changes?"
echo "   6. Press Up arrow key again"
echo "   7. Observe: Does cursor bounce or stay stable?"
echo "   8. Type: exit"
echo ""

echo "🧪 Manual Test 3: Long Line Editing"
echo "   1. Run: $LUSUSH_BIN"
echo "   2. Type a very long line: echo this is a very long line that might exceed terminal width and cause issues with cursor positioning and line wrapping behavior"
echo "   3. Press backspace multiple times"
echo "   4. Observe: Does cursor bounce or move erratically?"
echo "   5. Type: exit"
echo ""

echo "🧪 Manual Test 4: Bottom-Line Scenario"
echo "   1. Resize terminal to ~20 lines"
echo "   2. Run: $LUSUSH_BIN"
echo "   3. Fill screen: for i in {1..30}; do echo \"Line \$i\"; done"
echo "   4. Prompt should now be at bottom line"
echo "   5. Type: echo test"
echo "   6. Press backspace 4 times"
echo "   7. Type: hello"
echo "   8. Press Up arrow to recall last command"
echo "   9. Observe: Does cursor bounce or jump around?"
echo "   10. Type: exit"
echo ""

echo "🧪 Manual Test 5: Hints System Impact"
echo "   1. Run: $LUSUSH_BIN"
echo "   2. Type: config set hints_enabled true"
echo "   3. Type: ec (without pressing Enter)"
echo "   4. Observe: Do hints cause cursor bouncing?"
echo "   5. Press backspace twice"
echo "   6. Observe: Does cursor bounce during deletion?"
echo "   7. Type: ho test"
echo "   8. Press Up arrow (if there's history)"
echo "   9. Observe: Does cursor bounce during history navigation?"
echo "   10. Type: config set hints_enabled false"
echo "   11. Repeat tests 3-9 with hints disabled"
echo "   12. Type: exit"
echo ""

echo "==============================================================================="
echo "AUTOMATED CURSOR POSITION VERIFICATION"
echo "==============================================================================="
echo ""

# Simple automated test to check basic functionality
echo "Running basic functionality test..."

# Test basic command execution
result=$(echo "echo test" | timeout 5 $LUSUSH_BIN 2>&1 | tail -n 1)
if [[ "$result" == "test" ]]; then
    echo "✓ Basic command execution: PASSED"
else
    echo "✗ Basic command execution: FAILED"
    echo "  Expected: test"
    echo "  Got: $result"
fi

# Test variable assignment
result=$(echo "VAR=hello; echo \$VAR" | timeout 5 $LUSUSH_BIN 2>&1 | tail -n 1)
if [[ "$result" == "hello" ]]; then
    echo "✓ Variable assignment: PASSED"
else
    echo "✗ Variable assignment: FAILED"
    echo "  Expected: hello"
    echo "  Got: $result"
fi

# Test configuration
result=$(echo "config get hints_enabled" | timeout 5 $LUSUSH_BIN 2>&1 | tail -n 1)
if [[ "$result" == "false" ]]; then
    echo "✓ Configuration system: PASSED"
else
    echo "✗ Configuration system: FAILED"
    echo "  Expected: false"
    echo "  Got: $result"
fi

echo ""
echo "==============================================================================="
echo "CURSOR BOUNCING ANALYSIS INSTRUCTIONS"
echo "==============================================================================="
echo ""
echo "To diagnose cursor bouncing issues:"
echo ""
echo "1. 🔍 OBSERVE THE BEHAVIOR:"
echo "   - Does cursor jump to different positions?"
echo "   - Does cursor move multiple lines up/down?"
echo "   - Does cursor position become misaligned with text?"
echo "   - Does screen flicker or redraw unnecessarily?"
echo ""
echo "2. 📊 IDENTIFY PATTERNS:"
echo "   - Does it happen with all operations or specific ones?"
echo "   - Is it worse with longer lines?"
echo "   - Does it only occur at bottom of terminal?"
echo "   - Is it affected by terminal size?"
echo ""
echo "3. 🛠️ DEBUGGING STEPS:"
echo "   - Test with different terminal sizes"
echo "   - Test with hints enabled/disabled"
echo "   - Test with different line lengths"
echo "   - Test at different cursor positions"
echo ""
echo "4. 🎯 POTENTIAL CAUSES:"
echo "   - Incorrect cursor positioning calculations"
echo "   - Terminal width/height detection issues"
echo "   - Line wrapping logic problems"
echo "   - Escape sequence conflicts"
echo "   - Buffer overflow or underflow"
echo ""
echo "5. 🔧 POTENTIAL FIXES:"
echo "   - Revert to always using refreshLine() for stability"
echo "   - Improve cursor positioning calculations"
echo "   - Add better bounds checking"
echo "   - Simplify terminal escape sequences"
echo "   - Add debug logging for cursor movements"
echo ""

echo "==============================================================================="
echo "TO START INTERACTIVE TESTING:"
echo "==============================================================================="
echo ""
echo "Run the following command to start interactive testing:"
echo "  $LUSUSH_BIN"
echo ""
echo "Then follow the manual test procedures above."
echo ""
echo "To quit lusush during testing, type: exit"
echo ""
echo "==============================================================================="
