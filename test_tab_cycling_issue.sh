#!/bin/bash

# Test script to simulate interactive TAB completion cycling
# This script tests the exact issue described in the handoff documents

echo "=== Testing TAB Completion Cycling Issue ==="
echo "This test simulates the prompt redraw issue during TAB cycling"
echo ""

# Build the project first
echo "Building LLE..."
scripts/lle_build.sh build

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "=== Test 1: Basic TAB cycling (should show issue) ==="
echo "Simulating: echo [TAB][TAB][TAB][ESCAPE]"
echo "Expected: Menu cycles WITHOUT prompt redraws"
echo "Current issue: Extra prompts appear during cycling"
echo ""

# Use expect-like simulation with short delays
{
    printf "echo "
    sleep 0.1
    printf "\t"      # First TAB - should show menu
    sleep 0.2
    printf "\t"      # Second TAB - should cycle WITHOUT prompt redraw
    sleep 0.2
    printf "\t"      # Third TAB - should cycle WITHOUT prompt redraw
    sleep 0.2
    printf "\x1b"    # ESCAPE - cancel menu
    sleep 0.1
    printf "\n"      # Complete line
    printf "exit\n"  # Exit
} | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/tab_cycle_test.log

echo ""
echo "=== Visual Output Analysis ==="
echo "Looking for evidence of prompt redraws during cycling..."
echo ""

# Count prompt occurrences in debug log
PROMPT_COUNT=$(grep -c "Rendering prompt" /tmp/tab_cycle_test.log)
TAB_COUNT=$(grep -c "TAB case executed" /tmp/tab_cycle_test.log)
CYCLING_COUNT=$(grep -c "Cycled to completion" /tmp/tab_cycle_test.log)

echo "Debug Analysis:"
echo "- TAB events processed: $TAB_COUNT"
echo "- Completion cycling events: $CYCLING_COUNT"
echo "- Prompt renders: $PROMPT_COUNT"
echo ""

if [ $PROMPT_COUNT -gt 2 ]; then
    echo "❌ ISSUE DETECTED: More than 2 prompt renders detected"
    echo "   Expected: 2 (initial + final after ENTER)"
    echo "   Actual: $PROMPT_COUNT"
    echo "   This indicates prompt redraws during TAB cycling"
else
    echo "✅ GOOD: Only expected prompt renders detected"
fi

echo ""
echo "=== Test 2: Compare with working arrow navigation ==="
echo "Testing arrow key navigation (should work correctly)"
echo ""

{
    printf "echo "
    sleep 0.1
    printf "\t"        # First TAB - show menu
    sleep 0.2
    printf "\x1b[B"    # Down arrow - should work
    sleep 0.2
    printf "\x1b[B"    # Down arrow - should work
    sleep 0.2
    printf "\x0D"      # ENTER - accept selection
    sleep 0.1
    printf "\n"        # Complete line
    printf "exit\n"    # Exit
} | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/arrow_test.log

ARROW_PROMPT_COUNT=$(grep -c "Rendering prompt" /tmp/arrow_test.log)
echo "Arrow navigation prompt renders: $ARROW_PROMPT_COUNT"

if [ $ARROW_PROMPT_COUNT -le 2 ]; then
    echo "✅ Arrow navigation working correctly"
else
    echo "❌ Arrow navigation also has prompt redraw issues"
fi

echo ""
echo "=== Debug Log Analysis ==="
echo "Checking for specific issue indicators..."

# Check for the surgical fix messages
if grep -q "Tab cycling in active session" /tmp/tab_cycle_test.log; then
    echo "✅ Surgical fix is active and detecting cycling sessions"
else
    echo "❌ Surgical fix not detected - may not be working"
fi

# Check for display sync failures
SYNC_FAILURES=$(grep -c "Failed to sync display state" /tmp/tab_cycle_test.log)
echo "Display sync failures: $SYNC_FAILURES"

if [ $SYNC_FAILURES -gt 0 ]; then
    echo "⚠️  Display sync failures detected - this may be root cause"
fi

echo ""
echo "=== Conclusion ==="
if [ $PROMPT_COUNT -le 2 ] && [ $CYCLING_COUNT -gt 0 ]; then
    echo "✅ TAB cycling appears to be working correctly!"
    echo "   No excessive prompt redraws detected"
    echo "   Cycling is functioning ($CYCLING_COUNT cycles)"
else
    echo "❌ TAB cycling issue still present"
    echo "   Either excessive prompts or cycling not working"
fi

echo ""
echo "Full debug logs available at:"
echo "- /tmp/tab_cycle_test.log (TAB cycling test)"
echo "- /tmp/arrow_test.log (arrow navigation test)"
echo ""
echo "Use: grep -E 'TAB case|Cycled to|Rendering prompt|Tab cycling in active session' /tmp/tab_cycle_test.log"
