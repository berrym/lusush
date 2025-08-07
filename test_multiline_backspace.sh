#!/bin/bash

# Interactive test for multiline backspace functionality in LLE
# This test simulates the exact scenario described in the handoff document

set -e

echo "=========================================="
echo "LLE Multiline Backspace Interactive Test"
echo "=========================================="

# Build the project first
echo "Building LLE..."
if ! scripts/lle_build.sh build; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "Build successful!"
echo ""

# Display terminal information
echo "Terminal Environment:"
echo "  TERM: $TERM"
echo "  Terminal size: $(tput cols)x$(tput lines) characters"
echo "  Expected behavior: Characters disappear on same line (no staircase)"
echo ""

# Test Instructions
echo "=========================================="
echo "CRITICAL TEST SCENARIO"
echo "=========================================="
echo "This test replicates the exact issue from the handoff document:"
echo ""
echo "1. Type a long command that wraps to the next line:"
echo "   echo hello world this is a very long command that definitely wraps to next line and keeps going"
echo ""
echo "2. Press BACKSPACE multiple times (10-15 times)"
echo ""
echo "3. OBSERVE the visual behavior:"
echo "   ✅ SUCCESS: Characters disappear on SAME line where they were typed"
echo "   ❌ FAILURE: Visual 'staircase' effect - each backspace creates new line"
echo ""
echo "4. Press Ctrl+C to exit when done testing"
echo ""

# Success criteria
echo "=========================================="
echo "SUCCESS CRITERIA"
echo "=========================================="
echo "✅ Characters disappear smoothly on the same line"
echo "✅ NO new lines created during backspace operations"
echo "✅ NO visual 'staircase' or cascading effect"
echo "✅ Prompt remains intact and stable"
echo "✅ Terminal cursor position matches visual display"
echo ""

# Failure indicators
echo "FAILURE INDICATORS:"
echo "❌ Each backspace creates content on a new line below"
echo "❌ Staircase pattern showing progressive buffer states"
echo "❌ Prompt duplication or corruption"
echo "❌ Terminal context loss"
echo ""

# Pre-test setup
echo "Setting up debug logging..."
DEBUG_LOG="/tmp/multiline_backspace_test_$(date +%s).log"
echo "Debug output will be saved to: $DEBUG_LOG"
echo ""

# Critical reminder
echo "=========================================="
echo "CRITICAL REMINDER"
echo "=========================================="
echo "This test addresses the core issue described in the handoff document:"
echo "  'Each backspace creates a visual staircase effect'"
echo "  'Terminal limitation where \\b \\b cannot cross line boundaries'"
echo ""
echo "The fix implemented:"
echo "  - Uses ANSI cursor left movement (\\x1b[%zuD) instead of \\b"
echo "  - Moves cursor to content start position"
echo "  - Clears and rewrites content (content rewrite strategy)"
echo ""

# Start the test
echo "Press ENTER to start the interactive test..."
read -r

echo "Starting LLE with enhanced debug output..."
echo "Remember: Type long command, press backspace multiple times, observe behavior"
echo ""

# Run LLE with comprehensive debug output
LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ./builddir/lusush 2>"$DEBUG_LOG"

# Post-test analysis
echo ""
echo "=========================================="
echo "POST-TEST ANALYSIS"
echo "=========================================="

if [ -f "$DEBUG_LOG" ]; then
    echo "Analyzing debug output from: $DEBUG_LOG"
    echo ""

    # Count backspace operations
    backspace_count=$(grep -c "VISUAL_DEBUG.*BACKSPACE:" "$DEBUG_LOG" 2>/dev/null || echo "0")
    echo "Backspace operations detected: $backspace_count"

    if [ "$backspace_count" -gt 0 ]; then
        echo "✅ Backspace operations were performed"

        # Check cursor positioning
        cursor_success=$(grep -c "SUCCESS: Moved cursor left to content start" "$DEBUG_LOG" 2>/dev/null || echo "0")
        echo "Successful cursor positioning: $cursor_success"

        # Check clearing operations
        clear_success=$(grep -c "SUCCESS: Cleared from cursor to end of line" "$DEBUG_LOG" 2>/dev/null || echo "0")
        echo "Successful line clearing: $clear_success"

        # Check content writing
        write_success=$(grep -c "SUCCESS: Wrote.*characters" "$DEBUG_LOG" 2>/dev/null || echo "0")
        echo "Successful content writing: $write_success"

        echo ""
        echo "Key debug messages:"
        echo "-------------------"
        grep "VISUAL_DEBUG.*Moving cursor left\|SUCCESS: Moved cursor\|SUCCESS: Cleared\|SUCCESS: Wrote" "$DEBUG_LOG" | head -10

        # Check for errors
        error_count=$(grep -c "ERROR:" "$DEBUG_LOG" 2>/dev/null || echo "0")
        if [ "$error_count" -gt 0 ]; then
            echo ""
            echo "❌ Errors detected ($error_count):"
            grep "ERROR:" "$DEBUG_LOG" | head -5
        else
            echo ""
            echo "✅ No errors detected in debug log"
        fi

    else
        echo "ℹ️  No backspace operations detected"
        echo "   Either no backspace was pressed, or debug output not captured"
    fi

    # Show terminal information from log
    echo ""
    echo "Terminal Information from test:"
    grep "Size:.*x\|Terminal.*width\|Prompt width" "$DEBUG_LOG" | head -3

else
    echo "❌ Debug log not found: $DEBUG_LOG"
fi

echo ""
echo "=========================================="
echo "MANUAL VERIFICATION REQUIRED"
echo "=========================================="
echo "The ultimate test is VISUAL BEHAVIOR during the interactive session."
echo ""
echo "Questions to answer:"
echo "1. Did characters disappear smoothly on the same line? (YES/NO)"
echo "2. Did you see any 'staircase' or cascading effects? (YES/NO)"
echo "3. Did the prompt remain stable throughout? (YES/NO)"
echo "4. Did backspace feel natural and responsive? (YES/NO)"
echo ""

echo "If you answered:"
echo "  1=YES, 2=NO, 3=YES, 4=YES  →  ✅ FIX IS SUCCESSFUL!"
echo "  Any other combination        →  ❌ Further debugging needed"
echo ""

# Analysis tools
echo "=========================================="
echo "ANALYSIS TOOLS"
echo "=========================================="
echo "Debug log location: $DEBUG_LOG"
echo ""
echo "Useful commands for analysis:"
echo "  grep 'VISUAL_DEBUG' $DEBUG_LOG"
echo "  grep 'SUCCESS\\|ERROR' $DEBUG_LOG"
echo "  grep 'Moving cursor\\|Positioned cursor' $DEBUG_LOG"
echo ""
echo "Full log: cat $DEBUG_LOG"
echo ""

echo "Test completed! Review the visual behavior and debug output to determine success."
