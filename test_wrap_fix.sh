#!/bin/bash

# Test script for multiline wrapping backspace fix
# This tests the specific issue reported by the user

set -e

echo "=========================================="
echo "LLE Multiline Wrapping Backspace Fix Test"
echo "=========================================="

# Build first
echo "Building LLE..."
if ! scripts/lle_build.sh build; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "Build successful!"
echo ""

# Show terminal environment info
echo "Terminal Environment:"
echo "  Terminal width: $(tput cols) characters"
echo "  Expected prompt width: ~77 characters"
echo "  Available content space: ~3 characters"
echo "  Content will wrap for commands longer than 3 chars"
echo ""

echo "=========================================="
echo "SPECIFIC TEST CASE (from user report)"
echo "=========================================="
echo "This test replicates the exact scenario reported:"
echo ""
echo "User typed: 'echo Hello' (10 characters)"
echo "Then pressed backspace to get: 'echo Hell' (9 characters)"
echo "Expected: Content should rewrite cleanly on wrapped line"
echo "Previous issue: Content redrew incorrectly causing visual problems"
echo ""

echo "=========================================="
echo "TEST INSTRUCTIONS"
echo "=========================================="
echo "1. LLE will start with debug output enabled"
echo "2. Type exactly: echo Hello"
echo "3. Press backspace ONCE to delete the 'o'"
echo "4. Observe the visual behavior:"
echo "   ✅ SUCCESS: Content updates smoothly on wrapped line"
echo "   ❌ FAILURE: Visual artifacts, staircase effects, or corruption"
echo "5. Press Ctrl+C to exit"
echo ""

echo "Key debug values to watch for:"
echo "  - Prompt width: should be 77"
echo "  - Terminal width: should be 80"
echo "  - Content wraps: should be detected as YES"
echo "  - Cursor positioning: should use carriage return method"
echo ""

echo "=========================================="
echo "EXPECTED BEHAVIOR"
echo "=========================================="
echo "When you type 'echo Hello' (10 chars):"
echo "  Total width: 77 (prompt) + 10 (content) = 87 characters"
echo "  Since 87 > 80 (terminal width), content WILL wrap"
echo "  This is normal and correct terminal behavior"
echo ""
echo "When you press backspace to get 'echo Hell' (9 chars):"
echo "  Total width: 77 (prompt) + 9 (content) = 86 characters"
echo "  Since 86 > 80 (terminal width), content WILL still wrap"
echo "  The fix should handle this wrapping correctly"
echo ""

echo "Visual success criteria:"
echo "✅ Content appears on wrapped line (this is correct)"
echo "✅ Backspace updates content smoothly without artifacts"
echo "✅ No staircase effects or multiple content copies"
echo "✅ Cursor positioning works correctly after backspace"
echo ""

# Create debug log location
DEBUG_LOG="/tmp/wrap_fix_test_$(date +%s).log"
echo "Debug output will be saved to: $DEBUG_LOG"
echo ""

echo "Press ENTER to start the test..."
read -r

echo "Starting LLE with comprehensive debug output..."
echo "Remember: Type 'echo Hello', press backspace once, observe behavior"
echo ""

# Run LLE with all debug flags
LLE_DEBUG=1 LLE_DEBUG_DISPLAY=1 LLE_CURSOR_DEBUG=1 LLE_OBSERVER_DEBUG=1 ./builddir/lusush 2>"$DEBUG_LOG"

echo ""
echo "=========================================="
echo "POST-TEST ANALYSIS"
echo "=========================================="

if [ -f "$DEBUG_LOG" ]; then
    echo "Analyzing debug output..."

    # Check for backspace operation
    if grep -q "VISUAL_DEBUG.*BACKSPACE:" "$DEBUG_LOG"; then
        echo "✅ Backspace operation detected"

        # Show key debug values
        echo ""
        echo "Key debug values:"
        grep "Prompt width:\|Terminal width:\|Content wraps\|Total prompt" "$DEBUG_LOG" | head -5

        # Check cursor positioning method used
        if grep -q "Content wraps, using carriage return" "$DEBUG_LOG"; then
            echo "✅ Correct multiline handling: Carriage return method used"
        elif grep -q "Content fits on one line" "$DEBUG_LOG"; then
            echo "ℹ️  Single line handling: Cursor left method used"
        else
            echo "❓ Cursor positioning method unclear"
        fi

        # Check for success indicators
        success_count=$(grep -c "SUCCESS:" "$DEBUG_LOG")
        error_count=$(grep -c "ERROR:" "$DEBUG_LOG")

        echo "Operation results: $success_count successes, $error_count errors"

        if [ "$error_count" -eq 0 ]; then
            echo "✅ No errors in cursor positioning operations"
        else
            echo "❌ Errors detected:"
            grep "ERROR:" "$DEBUG_LOG" | head -3
        fi

    else
        echo "ℹ️  No backspace operations detected (test may not have run completely)"
    fi

    # Show terminal detection
    echo ""
    echo "Terminal detection:"
    grep "Size:.*x.*\|Terminal width:" "$DEBUG_LOG" | head -2

else
    echo "❌ Debug log not found: $DEBUG_LOG"
fi

echo ""
echo "=========================================="
echo "MANUAL VERIFICATION"
echo "=========================================="
echo "The key question: Did the backspace update content smoothly?"
echo ""
echo "Expected behavior in your terminal:"
echo "1. You typed 'echo Hello' and it appeared on a wrapped line"
echo "2. You pressed backspace and 'echo Hell' appeared cleanly"
echo "3. No visual artifacts, staircase effects, or content duplication"
echo ""
echo "If you observed smooth, clean content updates: ✅ FIX IS SUCCESSFUL"
echo "If you observed visual artifacts or issues: ❌ Further debugging needed"
echo ""

echo "Debug log saved to: $DEBUG_LOG"
echo "Use: grep 'VISUAL_DEBUG' $DEBUG_LOG to see all debug output"
echo ""
echo "Test completed!"
