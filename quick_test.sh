#!/bin/bash
# Lusush Shell - Quick Verification Test
# Simple test to verify core functionality works

set -e  # Exit on any error

echo "Lusush Shell - Quick Verification Test"
echo "======================================"

# Check if binary exists
if [ ! -f "builddir/lusush" ]; then
    echo "❌ ERROR: builddir/lusush not found"
    echo "Please run: ninja -C builddir"
    exit 1
fi

echo "✓ Binary found"

# Test 1: Basic execution
echo -n "Testing basic execution... "
if echo 'echo "hello"; exit' | timeout 5s ./builddir/lusush > /dev/null 2>&1; then
    echo "✓ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

# Test 2: Interactive mode
echo -n "Testing interactive mode... "
if echo 'exit' | timeout 5s ./builddir/lusush -i > /dev/null 2>&1; then
    echo "✓ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

# Test 3: Theme system
echo -n "Testing theme system... "
if echo 'theme list > /dev/null; exit' | timeout 5s ./builddir/lusush -i > /dev/null 2>&1; then
    echo "✓ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

# Test 4: Display integration
echo -n "Testing display integration... "
if echo 'display status > /dev/null; exit' | timeout 5s ./builddir/lusush -i > /dev/null 2>&1; then
    echo "✓ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

# Test 5: Multiline commands
echo -n "Testing multiline commands... "
if echo -e 'for i in 1 2\ndo\necho $i\ndone\nexit' | timeout 10s ./builddir/lusush -i > /dev/null 2>&1; then
    echo "✓ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

echo ""
echo "🎉 All core tests PASSED!"
echo ""
echo "For visual testing (colors, clear screen), run:"
echo "  ./builddir/lusush -i"
echo ""
echo "Quick visual test:"
echo "  echo 'clear; theme set dark; echo \"Dark theme test\"; exit' | ./builddir/lusush -i"
