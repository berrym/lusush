#!/bin/bash

# Comprehensive shift built-in test for lusush shell
# Tests all aspects of the shift built-in implementation

echo "=== SHIFT BUILT-IN COMPREHENSIVE TEST ==="
echo "Testing shift built-in functionality in lusush shell"
echo "Shell: ./builddir/lusush"
echo

# Test basic shift functionality
echo "=== BASIC SHIFT FUNCTIONALITY ==="

echo -n "Default shift (shift by 1): "
result=$(echo 'set -- a b c d; shift; echo "$# $1 $2 $3"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "3 b c d" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '3 b c d')"
fi

echo -n "Shift by 2: "
result=$(echo 'set -- a b c d e; shift 2; echo "$# $1 $2 $3"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "3 c d e" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '3 c d e')"
fi

echo -n "Shift by 3: "
result=$(echo 'set -- one two three four five; shift 3; echo "$# $1 $2"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 four five" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 four five')"
fi

echo

# Test edge cases
echo "=== EDGE CASES ==="

echo -n "Shift all parameters: "
result=$(echo 'set -- a b c; shift 3; echo "$# $1"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "0 " ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '0 ')"
fi

echo -n "Shift more than available (should not error): "
result=$(echo 'set -- a b; shift 5; echo "$# $1"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "0 " ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '0 ')"
fi

echo -n "Shift with no parameters: "
result=$(echo 'set --; shift; echo "$# $1"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "0 " ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '0 ')"
fi

echo -n "Shift by 0: "
result=$(echo 'set -- a b c; shift 0; echo "$# $1 $2 $3"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "3 a b c" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '3 a b c')"
fi

echo

# Test error handling
echo "=== ERROR HANDLING ==="

echo -n "Invalid numeric argument: "
result=$(echo 'shift abc 2>&1' | ./builddir/lusush)
if [[ "$result" =~ "numeric argument required" ]]; then
    echo "✅ WORKING (shows error message)"
else
    echo "❌ FAILED (got: '$result', expected numeric argument error)"
fi

echo -n "Negative shift count: "
result=$(echo 'shift -1 2>&1' | ./builddir/lusush)
if [[ "$result" =~ "numeric argument required" ]]; then
    echo "✅ WORKING (rejects negative numbers)"
else
    echo "❌ FAILED (got: '$result', should reject negative)"
fi

echo -n "Multiple arguments (should use first): "
result=$(echo 'set -- a b c d; shift 2 3 4; echo "$# $1"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 c" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 c')"
fi

echo

# Test return codes
echo "=== RETURN CODES ==="

echo -n "Successful shift return code: "
result=$(echo 'set -- a b c; shift; echo $?' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "0" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '0')"
fi

echo -n "Shift with no args return code: "
result=$(echo 'set --; shift; echo $?' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "0" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '0')"
fi

echo -n "Invalid argument return code: "
result=$(echo 'shift abc >/dev/null 2>&1; echo $?' | ./builddir/lusush)
if [ "$result" = "1" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '1')"
fi

echo

# Test with different parameter types
echo "=== PARAMETER TYPES ==="

echo -n "Shift with quoted parameters: "
result=$(echo 'set -- "hello world" "foo bar" baz; shift; echo "$# $1 $2"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 foo bar baz" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 foo bar baz')"
fi

echo -n "Shift with special characters: "
result=$(echo 'set -- "a*b" "c?d" "e[f]g"; shift; echo "$# $1"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 c?d" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 c?d')"
fi

echo -n "Shift with empty parameters: "
result=$(echo 'set -- "" a ""; shift; echo "$# $1 $2"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 a " ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 a ')"
fi

echo

# Test integration with shell features
echo "=== SHELL INTEGRATION ==="

echo -n "Shift in function: "
result=$(echo 'func() { shift; echo "$# $1"; }; func a b c d' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "3 b" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '3 b')"
fi

echo -n "Shift with getopts (if available): "
result=$(echo 'set -- -a -b value; shift; echo "$# $1 $2"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 -b value" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 -b value')"
fi

echo -n "Multiple shifts: "
result=$(echo 'set -- a b c d e; shift; shift; shift; echo "$# $1 $2"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 d e" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 d e')"
fi

echo

# Test practical usage scenarios
echo "=== PRACTICAL USAGE ==="

echo -n "Argument processing pattern: "
result=$(echo 'set -- --verbose --file config.txt arg1 arg2; shift 3; echo "$# $1 $2"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "2 arg1 arg2" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '2 arg1 arg2')"
fi

echo -n "Command line parsing simulation: "
result=$(echo 'set -- prog -v -f file.txt input.txt; shift; echo "prog: $# args: $*"' | ./builddir/lusush 2>/dev/null)
if [[ "$result" =~ "prog: 4 args:" ]]; then
    echo "✅ WORKING (argument processing)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo

# Test large parameter counts
echo "=== PERFORMANCE AND SCALE ==="

echo -n "Shift with many parameters: "
result=$(echo 'set -- {1..20}; shift 10; echo "$# $1"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "10 11" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ NEEDS WORK (got: '$result', expected: '10 11')"
fi

echo -n "Large shift count: "
result=$(echo 'set -- {1..5}; shift 100; echo "$# $1"' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "0 " ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '0 ')"
fi

echo

echo "=== SHIFT TEST SUMMARY ==="
echo "This comprehensive test covers:"
echo "✓ Basic shift functionality (default and custom counts)"
echo "✓ Edge cases (shift all, shift more than available, etc.)"
echo "✓ Error handling (invalid arguments, negative numbers)"
echo "✓ Return codes (success and error conditions)"
echo "✓ Parameter types (quoted, special characters, empty)"
echo "✓ Shell integration (functions, multiple shifts)"
echo "✓ Practical usage (argument processing patterns)"
echo "✓ Performance (large parameter counts)"
echo
echo "The shift built-in is essential for:"
echo "- Command line argument processing in scripts"
echo "- Option parsing loops"
echo "- Function parameter management"
echo "- Script argument validation and processing"
echo
echo "=== TEST COMPLETE ==="
