#!/bin/bash
# Week 8 Simple Integration Test
# Lusush Display Integration Validation

echo "Week 8 Display Integration - Simple Validation Test"
echo "=================================================="

# Test counters
PASS=0
FAIL=0

test_result() {
    if [ $1 -eq 0 ]; then
        echo "✅ PASS: $2"
        ((PASS++))
    else
        echo "❌ FAIL: $2"
        ((FAIL++))
    fi
}

# Test 1: Build verification
echo -n "Testing build... "
if [ -f "builddir/lusush" ]; then
    test_result 0 "Lusush binary exists"
else
    test_result 1 "Lusush binary missing"
    exit 1
fi

# Test 2: Basic shell execution
echo -n "Testing basic execution... "
if echo 'echo "test"; exit' | timeout 5s ./builddir/lusush > /dev/null 2>&1; then
    test_result 0 "Shell executes successfully"
else
    test_result 1 "Shell execution failed"
fi

# Test 3: Interactive mode
echo -n "Testing interactive mode... "
if echo 'exit' | timeout 5s ./builddir/lusush -i > /dev/null 2>&1; then
    test_result 0 "Interactive mode works"
else
    test_result 1 "Interactive mode failed"
fi

# Test 4: Display command availability
echo -n "Testing display command... "
if echo 'display help; exit' | timeout 5s ./builddir/lusush -i > /tmp/display_test.out 2>&1; then
    if grep -q "Display Integration System" /tmp/display_test.out; then
        test_result 0 "Display command available"
    else
        test_result 1 "Display command not working"
    fi
else
    test_result 1 "Display command test failed"
fi

# Test 5: Display status
echo -n "Testing display status... "
if echo 'display status; exit' | timeout 5s ./builddir/lusush -i > /tmp/status_test.out 2>&1; then
    if grep -q "Display Integration:" /tmp/status_test.out; then
        test_result 0 "Display status works"
    else
        test_result 1 "Display status not working"
    fi
else
    test_result 1 "Display status test failed"
fi

# Test 6: Clear screen integration
echo -n "Testing clear screen... "
if echo 'clear; echo "after clear"; exit' | timeout 5s ./builddir/lusush -i > /tmp/clear_test.out 2>&1; then
    if grep -q "after clear" /tmp/clear_test.out; then
        test_result 0 "Clear screen integration works"
    else
        test_result 1 "Clear screen integration failed"
    fi
else
    test_result 1 "Clear screen test failed"
fi

# Test 7: Environment variable support
echo -n "Testing environment variables... "
if LUSUSH_LAYERED_DISPLAY=0 echo 'display status; exit' | timeout 5s ./builddir/lusush -i > /tmp/env_test.out 2>&1; then
    if grep -q "INACTIVE\|disabled" /tmp/env_test.out; then
        test_result 0 "Environment variables work"
    else
        test_result 1 "Environment variables not working"
    fi
else
    test_result 1 "Environment variable test failed"
fi

# Test 8: Basic command execution with display integration
echo -n "Testing command execution... "
if echo 'echo "hello"; pwd; exit' | timeout 5s ./builddir/lusush -i > /tmp/cmd_test.out 2>&1; then
    if grep -q "hello" /tmp/cmd_test.out; then
        test_result 0 "Command execution works"
    else
        test_result 1 "Command execution failed"
    fi
else
    test_result 1 "Command execution test failed"
fi

# Cleanup
rm -f /tmp/*_test.out

# Summary
echo "=================================================="
echo "Summary: $PASS passed, $FAIL failed"

if [ $FAIL -eq 0 ]; then
    echo "🎉 Week 8 Integration: ALL TESTS PASSED!"
    echo "Display integration system is working correctly."
    exit 0
elif [ $FAIL -le 2 ]; then
    echo "⚠️  Week 8 Integration: MOSTLY WORKING (minor issues)"
    exit 1
else
    echo "❌ Week 8 Integration: MULTIPLE FAILURES"
    exit 2
fi
