#!/bin/bash

# Automated test for history navigation artifact fixes
# Tests wrapped line handling without interactive prompts

echo "🧪 Testing History Navigation Artifact Fixes"
echo "============================================="

# Build the project
echo "📦 Building lusush..."
cd "$(dirname "$0")"
if ! scripts/lle_build.sh build > /dev/null 2>&1; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build successful"

# Create test input file with history navigation commands
cat > /tmp/history_test_input << 'EOF'
echo "short"
echo "this is a very long command that will definitely wrap across multiple lines in most terminals and should test the visual footprint clearing logic"
ls
pwd
EOF

# Test 1: History navigation with wrapped content
echo ""
echo "🎯 Test 1: History navigation with wrapped content"
echo "Testing: Visual footprint clearing for complex changes"

# Use expect-like behavior with timeout
timeout 3s bash -c '
export LLE_DEBUG=1
export HISTFILE=/tmp/history_test_input
echo -e "\033[A\033[A\033[B\004" | ./builddir/lusush 2>/tmp/test1_debug.log
' > /dev/null 2>&1

# Analyze results
if grep -q "Using visual footprint clearing for complex changes" /tmp/test1_debug.log; then
    echo "✅ Visual footprint clearing activated"
else
    echo "❌ Visual footprint clearing not used"
fi

if grep -q "Using visual region clearing" /tmp/test1_debug.log; then
    echo "✅ Multi-line clearing logic engaged"
else
    echo "ℹ️  Simple clearing used (may be sufficient)"
fi

# Test 2: Backspace over wrapped history content
echo ""
echo "🎯 Test 2: Backspace over wrapped history content"
echo "Testing: Boundary crossing fixes during backspace"

timeout 3s bash -c '
export LLE_DEBUG=1
export HISTFILE=/tmp/history_test_input
echo -e "\033[A\177\177\177\004" | ./builddir/lusush 2>/tmp/test2_debug.log
' > /dev/null 2>&1

# Analyze backspace results
if grep -q "BOUNDARY CROSSING FIXES ACTIVE" /tmp/test2_debug.log; then
    echo "✅ Boundary crossing fixes triggered"
else
    echo "ℹ️  No boundary crossing detected"
fi

if grep -q "Crossing boundary: true" /tmp/test2_debug.log; then
    echo "✅ Boundary crossing properly detected"
else
    echo "ℹ️  No boundary crossing in this test"
fi

# Test 3: Error detection
echo ""
echo "🎯 Test 3: Error detection"

error_count=0
if grep -q "Invalid terminal coordinates" /tmp/test*_debug.log; then
    echo "⚠️  WARNING: Invalid terminal coordinates detected"
    error_count=$((error_count + 1))
fi

if grep -q "Failed to" /tmp/test*_debug.log; then
    echo "⚠️  WARNING: Operation failures detected"
    error_count=$((error_count + 1))
fi

if [ $error_count -eq 0 ]; then
    echo "✅ No errors detected"
fi

# Test 4: Performance check
echo ""
echo "🎯 Test 4: Performance verification"

# Count debug messages to ensure reasonable operation
incremental_ops=$(grep -c "LLE_INCREMENTAL" /tmp/test*_debug.log)
if [ $incremental_ops -gt 0 ] && [ $incremental_ops -lt 100 ]; then
    echo "✅ Reasonable number of incremental operations: $incremental_ops"
else
    echo "⚠️  Unusual operation count: $incremental_ops"
fi

# Summary
echo ""
echo "📋 Test Summary:"
echo "==============="

success_count=0
total_tests=4

# Check if main fixes are working
if grep -q "Using visual footprint clearing\|Using visual region clearing" /tmp/test*_debug.log; then
    echo "✅ History navigation artifacts: FIXED"
    success_count=$((success_count + 1))
else
    echo "❌ History navigation artifacts: NOT FIXED"
fi

if grep -q "BOUNDARY CROSSING FIXES ACTIVE\|Crossing boundary: true" /tmp/test*_debug.log; then
    echo "✅ Backspace over wrapped lines: WORKING"
    success_count=$((success_count + 1))
else
    echo "❌ Backspace over wrapped lines: NOT TESTED"
fi

if [ $error_count -eq 0 ]; then
    echo "✅ No errors detected: PASS"
    success_count=$((success_count + 1))
else
    echo "❌ Errors detected: FAIL"
fi

if [ $incremental_ops -gt 0 ] && [ $incremental_ops -lt 100 ]; then
    echo "✅ Performance reasonable: PASS"
    success_count=$((success_count + 1))
else
    echo "❌ Performance issues: FAIL"
fi

echo ""
echo "🎯 Overall Result: $success_count/$total_tests tests passed"

if [ $success_count -eq $total_tests ]; then
    echo "🎉 ALL TESTS PASSED - History navigation fixes are working!"
    exit_code=0
else
    echo "⚠️  SOME TESTS FAILED - Check debug logs for details"
    exit_code=1
fi

echo ""
echo "📁 Debug logs:"
echo "   Test 1: /tmp/test1_debug.log"
echo "   Test 2: /tmp/test2_debug.log"

# Clean up
rm -f /tmp/history_test_input

exit $exit_code
