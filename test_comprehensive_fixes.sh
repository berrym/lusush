#!/bin/bash

# Comprehensive test script for Linux/Konsole compatibility fixes
# Tests all reported issues: backspace, syntax highlighting, tab completion, line wrap, terminal resize

echo "=== Comprehensive Linux/Konsole Compatibility Test ==="
echo "Testing all reported issues with debug output"
echo

# Get current terminal size
COLS=$(tput cols)
ROWS=$(tput lines)

echo "Current terminal size: ${COLS}x${ROWS}"
echo "Platform: $(uname -s)"
echo

# Enable debug mode to see what's happening
export LLE_DEBUG=1

echo "Starting lusush with comprehensive testing..."
echo
echo "=== TEST PLAN ==="
echo "1. BACKSPACE BEHAVIOR TEST"
echo "   - Type 'hello' then backspace each character"
echo "   - Watch for: 'Linux true incremental: backspace' messages"
echo "   - Expected: Consistent backspace behavior, no artifacts"
echo
echo "2. SYNTAX HIGHLIGHTING WITH LINE WRAP TEST"
echo "   - Type a very long command that wraps lines"
echo "   - Include quotes and pipes: echo \"very long text here\" | grep pattern"
echo "   - Expected: Syntax highlighting works across line wraps"
echo
echo "3. TAB COMPLETION ARTIFACTS TEST"
echo "   - Type 'ls te' then press Tab multiple times"
echo "   - Test with files of different lengths"
echo "   - Expected: No artifacts when switching between completions"
echo
echo "4. LINE WRAP + TAB COMPLETION TEST"
echo "   - Type a long path then use tab completion"
echo "   - Expected: Completion works correctly with wrapped lines"
echo
echo "5. TERMINAL RESIZE TEST"
echo "   - Type some text, then resize terminal window"
echo "   - Continue typing after resize"
echo "   - Expected: Display adapts correctly to new size"
echo

echo "=== DEBUG OUTPUT LEGEND ==="
echo "✅ GOOD: 'Linux true incremental: appending' - fast character addition"
echo "✅ GOOD: 'Linux true incremental: backspace' - fast backspace"
echo "✅ GOOD: 'Linux: Using carriage return for simple case' - simple operations"
echo "✅ GOOD: 'Linux complex check' - intelligent complexity detection"
echo "⚠️  OK: 'Linux syntax-sensitive operation' - needed for syntax highlighting"
echo "❌ BAD: Frequent full rewrites for simple operations"
echo "❌ BAD: Character duplication or artifacts"
echo

echo "=== ISSUE-SPECIFIC TESTS ==="
echo
echo "TEST 1: BACKSPACE CONSISTENCY"
echo "Instructions:"
echo "  - Type: 'test123'"
echo "  - Backspace each character one by one"
echo "  - Watch debug output for 'Linux true incremental: backspace'"
echo "  - Result should be consistent cursor movement"
echo
echo "TEST 2: SYNTAX HIGHLIGHTING + LINE WRAP"
echo "Instructions:"
echo "  - Type this long command (let it wrap):"
echo "    echo \"this is a very long command that should wrap across multiple lines and still have proper syntax highlighting for quotes pipes and other elements\" | grep pattern | sort | uniq"
echo "  - Expected: Colors work correctly across line boundaries"
echo
echo "TEST 3: TAB COMPLETION ARTIFACTS"
echo "Instructions:"
echo "  - Create test files: touch test_short.txt test_very_long_filename.txt"
echo "  - Type: 'ls test_' then press Tab repeatedly"
echo "  - Watch for artifacts when switching from long to short names"
echo
echo "TEST 4: TERMINAL RESIZE BEHAVIOR"
echo "Instructions:"
echo "  - Type: 'echo hello world'"
echo "  - Resize terminal window (make it narrower, then wider)"
echo "  - Continue typing: ' and more text'"
echo "  - Expected: Display adjusts correctly"
echo

echo "=== EXPECTED SUCCESS INDICATORS ==="
echo "✅ Backspace shows 'Linux true incremental: backspace' for simple cases"
echo "✅ Character input shows 'Linux true incremental: appending' for most chars"
echo "✅ Complex operations use 'Linux complex check' and appropriate strategy"
echo "✅ No character duplication or visual artifacts"
echo "✅ Syntax highlighting works across line wraps"
echo "✅ Tab completion clears artifacts properly"
echo "✅ Terminal resize doesn't break display"
echo

echo "=== FAILURE INDICATORS ==="
echo "❌ Backspace always uses full rewrite instead of incremental"
echo "❌ Simple characters trigger unnecessary full rewrites"
echo "❌ Character duplication appears"
echo "❌ Tab completion leaves artifacts from previous completions"
echo "❌ Syntax highlighting breaks with line wrapping"
echo "❌ Terminal resize causes display corruption"
echo

echo "Press Enter to start testing, Ctrl+C to exit lusush when done"
read -r

# Create test files for tab completion
touch test_short.txt test_very_long_filename_for_testing.txt

echo "Running lusush with debug output..."
./builddir/lusush

echo
echo "=== POST-TEST ANALYSIS ==="
echo "Review the debug output above for:"
echo
echo "✅ SUCCESS PATTERNS:"
echo "  - Most character input: 'Linux true incremental: appending'"
echo "  - Backspace operations: 'Linux true incremental: backspace'"
echo "  - Complex operations: 'Linux complex check' with appropriate handling"
echo "  - Clean visual output without artifacts"
echo
echo "❌ FAILURE PATTERNS:"
echo "  - Excessive 'using full rewrite' messages for simple operations"
echo "  - Missing 'true incremental' messages for backspace"
echo "  - Visual artifacts or character duplication"
echo "  - Broken syntax highlighting or tab completion"
echo

# Cleanup test files
rm -f test_short.txt test_very_long_filename_for_testing.txt

echo "Test completed. Check above for results analysis."
echo
echo "KEY METRICS TO EVALUATE:"
echo "1. Backspace Consistency: Did backspace use incremental updates?"
echo "2. Syntax Highlighting: Did it work with line wrapping?"
echo "3. Tab Completion: Were artifacts properly cleared?"
echo "4. Terminal Resize: Did display adapt correctly?"
echo "5. Overall Performance: Were most operations incremental?"
