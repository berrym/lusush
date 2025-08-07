#!/bin/bash

# Test script for the simplified multiline backspace approach
# This tests the carriage return + line clear + prompt redraw method

set -e

echo "=========================================="
echo "Simplified Multiline Backspace Test"
echo "=========================================="

# Build first
echo "Building LLE..."
if ! scripts/lle_build.sh build; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "Build successful!"
echo ""

# Show environment
echo "Test Environment:"
echo "  Terminal width: $(tput cols) characters"
echo "  Test approach: Carriage return + line clear + prompt redraw"
echo ""

echo "=========================================="
echo "CRITICAL TEST: Simplified Multiline Approach"
echo "=========================================="
echo "This test validates the new simplified approach for wrapped content:"
echo ""
echo "New Method for Wrapped Content:"
echo "  1. \\r (carriage return to line start)"
echo "  2. \\x1b[2K (clear entire line)"
echo "  3. Redraw prompt from scratch"
echo "  4. Write new content (wraps naturally)"
echo ""

echo "Expected Benefits:"
echo "✅ No complex cursor positioning across wrapped lines"
echo "✅ Reliable across all terminal types"
echo "✅ Complete elimination of artifacts"
echo "✅ Natural content wrapping after prompt redraw"
echo ""

echo "=========================================="
echo "TEST INSTRUCTIONS"
echo "=========================================="
echo "1. Type exactly: echo Hello World This Is A Very Long Command That Will Wrap"
echo "2. Press backspace 5-10 times"
echo "3. Observe the behavior:"
echo "   ✅ SUCCESS: Clean content updates, no 'echecho' artifacts"
echo "   ✅ SUCCESS: Prompt remains intact and stable"
echo "   ✅ SUCCESS: Content wraps naturally after backspace"
echo "   ❌ FAILURE: Any visual artifacts or staircase effects"
echo "4. Press Ctrl+C when done"
echo ""

echo "Debug messages to look for:"
echo "  - 'Content wraps, using carriage return and absolute positioning'"
echo "  - 'SUCCESS: Sent carriage return to line start'"
echo "  - 'SUCCESS: Cleared entire line'"
echo "  - 'SUCCESS: Redrew prompt'"
echo ""

# Create debug log
DEBUG_LOG="/tmp/simplified_multiline_test_$(date +%s).log"
echo "Debug output will be saved to: $DEBUG_LOG"
echo ""

echo "=========================================="
echo "COMPARISON TO PREVIOUS APPROACHES"
echo "=========================================="
echo "Previous Failed Approaches:"
echo "❌ Cursor left movement: Failed on wrapped lines"
echo "❌ Move up + position: Affected previous shell output"
echo "❌ Forward clearing: Left artifacts on wrapped lines"
echo ""
echo "New Simplified Approach:"
echo "✅ Clear entire line: Removes all content including wrapped portions"
echo "✅ Prompt redraw: Ensures clean state"
echo "✅ Natural wrapping: Content wraps correctly after prompt"
echo ""

echo "Press ENTER to start the test..."
read -r

echo "Starting LLE with comprehensive debug output..."
echo "Type the long command and test backspace behavior!"
echo ""

# Run with full debug output
LLE_DEBUG=1 LLE_DEBUG_DISPLAY=1 ./builddir/lusush 2>"$DEBUG_LOG"

echo ""
echo "=========================================="
echo "POST-TEST ANALYSIS"
echo "=========================================="

if [ -f "$DEBUG_LOG" ]; then
    echo "Analyzing debug output from: $DEBUG_LOG"
    echo ""

    # Check for backspace operations
    backspace_count=$(grep -c "VISUAL_DEBUG.*BACKSPACE:" "$DEBUG_LOG" 2>/dev/null || echo "0")
    echo "Backspace operations detected: $backspace_count"

    if [ "$backspace_count" -gt 0 ]; then
        echo "✅ Backspace operations occurred"

        # Check for simplified multiline approach
        if grep -q "Content wraps, using carriage return and absolute positioning" "$DEBUG_LOG"; then
            echo "✅ Simplified multiline approach used"

            # Check individual steps
            if grep -q "SUCCESS: Sent carriage return to line start" "$DEBUG_LOG"; then
                echo "✅ Step 1: Carriage return successful"
            else
                echo "❌ Step 1: Carriage return failed"
            fi

            if grep -q "SUCCESS: Cleared entire line" "$DEBUG_LOG"; then
                echo "✅ Step 2: Line clearing successful"
            else
                echo "❌ Step 2: Line clearing failed"
            fi

            if grep -q "SUCCESS: Redrew prompt" "$DEBUG_LOG"; then
                echo "✅ Step 3: Prompt redraw successful"
            else
                echo "❌ Step 3: Prompt redraw failed"
            fi

            # Check for content writing
            if grep -q "SUCCESS: Wrote.*characters" "$DEBUG_LOG"; then
                echo "✅ Step 4: Content writing successful"
            else
                echo "❌ Step 4: Content writing failed"
            fi

        elif grep -q "Content fits on one line" "$DEBUG_LOG"; then
            echo "ℹ️  Single-line approach used (expected for short content)"
        else
            echo "❓ Approach detection unclear"
        fi

        # Check for errors
        error_count=$(grep -c "ERROR:" "$DEBUG_LOG" 2>/dev/null || echo "0")
        if [ "$error_count" -eq 0 ]; then
            echo "✅ No errors detected"
        else
            echo "❌ $error_count errors detected:"
            grep "ERROR:" "$DEBUG_LOG" | head -3
        fi

    else
        echo "ℹ️  No backspace operations detected"
    fi

    echo ""
    echo "Key debug output:"
    echo "=================="
    grep "Content wraps\|SUCCESS.*carriage\|SUCCESS.*Cleared\|SUCCESS.*Redrew" "$DEBUG_LOG" | head -10

else
    echo "❌ Debug log not found: $DEBUG_LOG"
fi

echo ""
echo "=========================================="
echo "VISUAL VERIFICATION CHECKLIST"
echo "=========================================="
echo "Answer these questions based on what you observed:"
echo ""
echo "1. Did backspace work smoothly on wrapped content? (YES/NO)"
echo "2. Were there any 'echecho' or similar artifacts? (YES/NO)"
echo "3. Did the prompt remain stable and intact? (YES/NO)"
echo "4. Did content wrap naturally after backspace? (YES/NO)"
echo "5. Was there any staircase or cascading effect? (YES/NO)"
echo ""
echo "Success Pattern: 1=YES, 2=NO, 3=YES, 4=YES, 5=NO"
echo ""

echo "=========================================="
echo "NEXT STEPS"
echo "=========================================="
echo "If this approach works successfully:"
echo "✅ The simplified method solves the multiline backspace issue"
echo "✅ No need for complex cursor positioning across wrapped lines"
echo "✅ Ready for integration into production"
echo ""
echo "If issues persist:"
echo "❌ May need to investigate terminal-specific behavior"
echo "❌ Consider alternative approaches or hybrid solutions"
echo ""

echo "Debug log saved to: $DEBUG_LOG"
echo ""
echo "Analysis commands:"
echo "  grep 'VISUAL_DEBUG' $DEBUG_LOG"
echo "  grep 'SUCCESS\\|ERROR' $DEBUG_LOG"
echo "  grep 'Content wraps' $DEBUG_LOG"
echo ""
echo "Test completed!"
