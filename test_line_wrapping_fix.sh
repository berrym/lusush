#!/bin/bash

# Test script to validate line wrapping cursor math fix
# This script tests if the fundamental cursor positioning bug has been fixed

echo "=== Line Wrapping Cursor Math Fix Test ==="
echo "Testing fundamental cursor positioning in small terminals"
echo

# Get current terminal size
COLS=$(tput cols)
ROWS=$(tput lines)

echo "Current terminal size: ${COLS}x${ROWS}"
echo "Platform: $(uname -s)"
echo

# Check if terminal is small enough to test wrapping
if [ "$COLS" -gt 60 ]; then
    echo "⚠️  WARNING: Terminal is wide ($COLS columns)"
    echo "   For best testing, resize terminal to ~40-50 columns"
    echo "   This will force line wrapping during typing"
    echo
fi

# Enable debug mode to see cursor math calculations
export LLE_DEBUG=1

echo "=== FOCUSED LINE WRAPPING TESTS ==="
echo
echo "TEST 1: BASIC LINE WRAPPING"
echo "Instructions:"
echo "  - Type a command that will wrap: 'echo hello world this is a very long command'"
echo "  - Watch debug output for cursor math calculations"
echo "  - Look for: '[CURSOR_MATH] POSITION_CALC' messages"
echo "  - Expected: No cursor positioning errors or artifacts"
echo

echo "TEST 2: CURSOR POSITIONING AT WRAP BOUNDARY"
echo "Instructions:"
echo "  - Type exactly enough characters to reach terminal edge"
echo "  - Look for 'at_boundary' calculations in debug output"
echo "  - Expected: Cursor should stay at end of line, not jump to next line"
echo

echo "TEST 3: BACKSPACE ACROSS LINE WRAP"
echo "Instructions:"
echo "  - Type long text that wraps to next line"
echo "  - Backspace from second line back to first line"
echo "  - Expected: Smooth cursor movement across line boundary"
echo

echo "TEST 4: SYNTAX HIGHLIGHTING WITH WRAPPING"
echo "Instructions:"
echo "  - Type: 'echo \"this is a very long quoted string that should wrap\""
echo "  - Expected: Syntax highlighting works across wrapped lines"
echo

echo "=== DEBUG OUTPUT TO WATCH FOR ==="
echo "✅ GOOD SIGNS:"
echo "  - '[CURSOR_MATH] Position calculated successfully'"
echo "  - Consistent row/col calculations"
echo "  - 'at_boundary=true' only at actual terminal edges"
echo "  - 'Linux true incremental' for simple operations"
echo

echo "❌ BAD SIGNS:"
echo "  - '[CURSOR_MATH] BOUNDARY_ERROR' messages"
echo "  - '[CURSOR_MATH] VALIDATION_FAILED' messages"
echo "  - Inconsistent cursor positioning"
echo "  - Character duplication or artifacts during wrapping"
echo

echo "=== CURSOR MATH VALIDATION ==="
echo "The debug output will show detailed cursor math calculations:"
echo "  - INPUTS: buffer info, prompt width, terminal width"
echo "  - CALCULATION: how total width is computed"
echo "  - POSITION_CALC: final row/col calculations"
echo "  - Boundary condition handling"
echo

echo "Press Enter to start testing..."
read -r

echo "Starting lusush with cursor math debugging..."
echo "Type 'exit' to quit when testing is complete"
echo

# Run lusush with full debug output
./builddir/lusush

echo
echo "=== TEST RESULTS ANALYSIS ==="
echo "Review the debug output above for:"
echo

echo "✅ SUCCESS INDICATORS:"
echo "  - No '[CURSOR_MATH] BOUNDARY_ERROR' or 'VALIDATION_FAILED' messages"
echo "  - Cursor positioning appears consistent during line wrapping"
echo "  - Text display works correctly in wrapped lines"
echo "  - Backspace works smoothly across line boundaries"
echo

echo "❌ FAILURE INDICATORS:"
echo "  - Cursor math errors in debug output"
echo "  - Visual artifacts during line wrapping"
echo "  - Incorrect cursor positioning when text wraps"
echo "  - Broken backspace behavior across lines"
echo

echo "KEY QUESTION: Did the cursor math fix solve the fundamental line wrapping issues?"
echo "If issues persist, the problem is deeper than just cursor math and requires"
echo "a complete rewrite of the display system architecture."
