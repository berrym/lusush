#!/bin/bash

# Test script to specifically test tab completion display behavior on Linux
# This script identifies and reproduces the tab completion display issues

set -e

echo "==============================================="
echo "Tab Completion Display Test for Linux"
echo "==============================================="
echo

# Build first
echo "1. Building LLE with latest fixes..."
echo "-----------------------------------"
scripts/lle_build.sh build

echo
echo "2. Platform and Environment Check"
echo "--------------------------------"
echo "Platform: $(uname -s)"
echo "Terminal: $TERM"
echo "LLE_DEBUG: ${LLE_DEBUG:-not set}"
echo "LLE_DEBUG_COMPLETION: ${LLE_DEBUG_COMPLETION:-not set}"

echo
echo "3. Basic Tab Completion Test"
echo "---------------------------"

echo "Testing basic tab completion behavior..."
echo "This will type 'test_' and press TAB to trigger completion"

# Create a test session that simulates tab completion
echo "Creating completion test files for consistent results..."
mkdir -p test_completion_debug
cd test_completion_debug
touch test_file1.txt test_file2.txt test_file3.txt
echo

echo "Running tab completion test..."
echo "Expected: Should show completion menu without debug output or prompt corruption"

# Test tab completion with manual input simulation
cat << 'EOF' > completion_test_input.txt
test_

exit
EOF

echo "Simulating tab completion (type 'test_' then TAB):"
echo "Command: echo -e 'test_\\t\\nexit' | timeout 5s ../builddir/lusush"

# Run the test and capture all output
echo "Output:"
echo "----"
OUTPUT=$(echo -e 'test_\t\nexit' | timeout 5s ../builddir/lusush 2>&1 || true)
echo "$OUTPUT"
echo "----"

cd ..
rm -rf test_completion_debug

echo
echo "4. Analysis of Tab Completion Issues"
echo "----------------------------------"

# Check for specific issues mentioned
echo "Checking for identified issues:"

# Issue 1: Debug output without LLE_DEBUG
if echo "$OUTPUT" | grep -q "\[.*COMPLETION.*\]" && [[ -z "${LLE_DEBUG}" ]]; then
    echo "❌ ISSUE 1: Debug output appearing without LLE_DEBUG=1"
    echo "   Found completion debug messages when debug mode is disabled"
else
    echo "✅ ISSUE 1: No unexpected debug output found"
fi

# Issue 2: Cursor math debug output
if echo "$OUTPUT" | grep -q "\[CURSOR_MATH\]"; then
    echo "❌ ISSUE 2: Cursor math debug output still appearing"
    echo "   This should be fixed by our recent changes"
else
    echo "✅ ISSUE 2: No cursor math debug output (fix working)"
fi

# Issue 3: Prompt redrawing/moving
PROMPT_COUNT=$(echo "$OUTPUT" | grep -c "mberry@.*lusush.*\$" || echo "0")
if [[ "$PROMPT_COUNT" -gt 3 ]]; then
    echo "❌ ISSUE 3: Excessive prompt redrawing detected ($PROMPT_COUNT instances)"
    echo "   Tab completion may be causing prompt to redraw/move"
else
    echo "✅ ISSUE 3: Prompt redrawing appears normal ($PROMPT_COUNT instances)"
fi

# Issue 4: Menu positioning
if echo "$OUTPUT" | grep -q "test_.*file"; then
    echo "✅ ISSUE 4: Tab completion menu appears to be working"

    # Check for menu positioning issues
    MENU_LINES=$(echo "$OUTPUT" | grep -c "test_.*file" || echo "0")
    echo "   Found $MENU_LINES completion menu items"

    if [[ "$MENU_LINES" -gt 0 ]]; then
        echo "   Checking menu positioning..."
        # Look for signs of menu appearing in wrong location
        if echo "$OUTPUT" | grep -A5 -B5 "test_.*file" | grep -q "\$.*test_.*file"; then
            echo "   ❌ Menu may be overlapping with prompt"
        else
            echo "   ✅ Menu positioning appears correct"
        fi
    fi
else
    echo "❌ ISSUE 4: Tab completion menu not detected in output"
fi

echo
echo "5. Debug Mode Test"
echo "-----------------"

echo "Testing with debug mode enabled to see detailed behavior..."
LLE_DEBUG=1 timeout 3s bash -c 'echo -e "test_\t\nexit" | ./builddir/lusush' > debug_output.txt 2>&1 || true

echo "Debug output sample (first 20 lines):"
head -20 debug_output.txt || true
echo "... (truncated)"

# Check specific debug patterns
if grep -q "Platform detected: Linux" debug_output.txt; then
    echo "✅ Linux platform detection working"
else
    echo "❌ Linux platform detection not found in debug output"
fi

if grep -q "cursor.*query.*failed\|using.*fallback" debug_output.txt; then
    echo "✅ Cursor queries properly disabled on Linux"
else
    echo "⚠️  Cursor query behavior unclear from debug output"
fi

echo
echo "6. Tab Completion Specific Debug Test"
echo "------------------------------------"

echo "Testing with completion debug enabled..."
LLE_DEBUG_COMPLETION=1 timeout 3s bash -c 'echo -e "test_\t\nexit" | ./builddir/lusush' > completion_debug.txt 2>&1 || true

echo "Completion debug output sample:"
grep "\[.*COMPLETION.*\]" completion_debug.txt | head -10 || echo "No completion debug output found"

echo
echo "7. Recommendations"
echo "-----------------"

echo "Based on the test results:"

# Provide specific recommendations based on findings
if grep -q "\[CURSOR_MATH\]" debug_output.txt 2>/dev/null; then
    echo "• CRITICAL: Cursor math debug output still appearing - need to verify fix"
fi

if [[ "$PROMPT_COUNT" -gt 3 ]]; then
    echo "• DISPLAY: Excessive prompt redrawing - investigate display update logic"
fi

if ! echo "$OUTPUT" | grep -q "test_.*file"; then
    echo "• COMPLETION: Tab completion not triggering - check input handling"
fi

echo "• TESTING: Run this test in a real Linux/Konsole terminal for accurate results"
echo "• VALIDATION: Compare behavior with/without debug modes"

echo
echo "8. Clean Up"
echo "----------"
rm -f debug_output.txt completion_debug.txt

echo "Tab completion display test completed!"
echo "==============================================="
