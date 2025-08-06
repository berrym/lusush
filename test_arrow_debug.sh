#!/bin/bash

# Minimal Arrow Key Debug Test for Lusush
# Tests if arrow keys reach the correct switch cases

echo "=== ARROW KEY DEBUG TEST ==="
echo ""
echo "This test will check if arrow keys are properly reaching the switch cases."
echo ""
echo "Steps:"
echo "1. Start shell with debug logging"
echo "2. Type 'echo test' and press ENTER (creates history)"
echo "3. Press UP arrow key"
echo "4. Check debug output"
echo ""

read -p "Press ENTER to start test..."

echo ""
echo "Starting Lusush with debug logging..."
echo "Debug output will be saved to /tmp/arrow_debug.log"
echo ""

# Start with debug mode
LLE_DEBUG=1 ./builddir/lusush 2>/tmp/arrow_debug.log

echo ""
echo "=== CHECKING RESULTS ==="
echo ""

# Check if arrow keys were detected
if grep -q "ARROW_UP case executed" /tmp/arrow_debug.log; then
    echo "✅ UP arrow key case was reached!"
else
    echo "❌ UP arrow key case was NOT reached"
fi

if grep -q "ARROW_DOWN case executed" /tmp/arrow_debug.log; then
    echo "✅ DOWN arrow key case was reached!"
else
    echo "❌ DOWN arrow key case was NOT reached"
fi

# Check if nuclear clear was executed
if grep -q "Executing NUCLEAR CLEAR approach" /tmp/arrow_debug.log; then
    echo "✅ Nuclear clear approach was executed!"
else
    echo "❌ Nuclear clear approach was NOT executed"
fi

# Check if escape sequences were parsed
if grep -q "Found match.*\[A.*type=8" /tmp/arrow_debug.log; then
    echo "✅ UP arrow escape sequence parsed correctly"
else
    echo "❌ UP arrow escape sequence parsing failed"
fi

echo ""
echo "Full arrow key debug output:"
grep -E "(ARROW|ESCAPE|NUCLEAR)" /tmp/arrow_debug.log

echo ""
echo "Test complete. Check above results."
