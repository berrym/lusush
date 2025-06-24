#!/bin/bash

# Comprehensive test for supposedly broken features in lusush shell
# This test diagnoses the actual current state of features reported as broken

echo "=== LUSUSH BROKEN FEATURES DIAGNOSIS ==="
echo "Testing features reported as broken to determine actual status"
echo "Shell: ./builddir/lusush"
echo

# Test command substitution thoroughly
echo "=== COMMAND SUBSTITUTION DIAGNOSIS ==="

echo -n "Basic command substitution: "
result=$(echo 'echo $(echo hello)' | ./builddir/lusush)
if [ "$result" = "hello" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'hello')"
fi

echo -n "Command substitution in quotes: "
result=$(echo 'echo "Today: $(date +%Y)"' | ./builddir/lusush)
expected_pattern="Today: [0-9][0-9][0-9][0-9]"
if [[ "$result" =~ ^Today:\ [0-9]{4}$ ]]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', pattern: '$expected_pattern')"
fi

echo -n "Nested command substitution: "
result=$(echo 'echo $(echo $(echo nested))' | ./builddir/lusush)
if [ "$result" = "nested" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'nested')"
fi

echo -n "Command substitution with pipes: "
result=$(echo 'echo "Count: $(echo "a b c" | wc -w)"' | ./builddir/lusush)
if [[ "$result" =~ ^Count:\ [0-9]+$ ]]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result')"
fi

echo -n "Command substitution assignment: "
result=$(echo 'x=$(echo test); echo $x' | ./builddir/lusush)
if [ "$result" = "test" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'test')"
fi

echo

# Test single quote literal protection
echo "=== SINGLE QUOTE LITERAL PROTECTION DIAGNOSIS ==="

echo -n "Variable in single quotes: "
result=$(echo 'echo '\''Value: $USER'\''' | ./builddir/lusush)
if [ "$result" = "Value: \$USER" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'Value: \$USER')"
fi

echo -n "Arithmetic in single quotes: "
result=$(echo 'echo '\''Calc: $((5+3))'\''' | ./builddir/lusush)
if [ "$result" = "Calc: \$((5+3))" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'Calc: \$((5+3))')"
fi

echo -n "Command substitution in single quotes: "
result=$(echo 'echo '\''Date: $(date)'\''' | ./builddir/lusush)
if [ "$result" = "Date: \$(date)" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'Date: \$(date)')"
fi

echo -n "Mixed quotes test: "
result=$(echo 'echo '\''literal $USER'\'' "expanded $USER"' | ./builddir/lusush)
expected_pattern="literal \\\$USER expanded [a-zA-Z0-9_-]+"
if [[ "$result" =~ ^literal\ \$USER\ expanded\ [a-zA-Z0-9_-]+$ ]]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', pattern: '$expected_pattern')"
fi

echo

# Test edge cases that might reveal actual problems
echo "=== EDGE CASE DIAGNOSIS ==="

echo -n "Empty command substitution: "
result=$(echo 'echo "Empty: $()"' | ./builddir/lusush 2>/dev/null)
echo "RESULT: '$result' (may be expected to fail)"

echo -n "Invalid command substitution: "
result=$(echo 'echo "Invalid: $(nonexistent_command)"' | ./builddir/lusush 2>/dev/null)
echo "RESULT: '$result' (error handling test)"

echo -n "Complex nesting: "
result=$(echo 'echo "$(echo "$(echo inner)")"' | ./builddir/lusush)
if [ "$result" = "inner" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'inner')"
fi

echo -n "Command substitution with variables: "
result=$(echo 'cmd=echo; echo $(${cmd} test)' | ./builddir/lusush)
if [ "$result" = "test" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'test')"
fi

echo

# Test with direct shell invocation vs script context
echo "=== CONTEXT COMPARISON ==="

echo -n "Direct arithmetic (no context): "
result=$(echo '$((5 + 3))' | ./builddir/lusush)
if [ "$result" = "8" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: '8')"
fi

echo -n "Direct command substitution: "
result=$(echo '$(echo direct)' | ./builddir/lusush)
if [ "$result" = "direct" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ BROKEN (got: '$result', expected: 'direct')"
fi

echo

# Test regression test compatibility
echo "=== REGRESSION TEST SUBSET ==="

echo -n "Basic echo: "
result=$(echo 'echo hello' | ./builddir/lusush)
if [ "$result" = "hello" ]; then
    echo "✅ WORKING"
else
    echo "❌ BROKEN"
fi

echo -n "Variable expansion: "
result=$(echo 'x=test; echo $x' | ./builddir/lusush)
if [ "$result" = "test" ]; then
    echo "✅ WORKING"
else
    echo "❌ BROKEN"
fi

echo -n "Pipeline: "
result=$(echo 'echo hello | cat' | ./builddir/lusush)
if [ "$result" = "hello" ]; then
    echo "✅ WORKING"
else
    echo "❌ BROKEN"
fi

echo

echo "=== DIAGNOSIS SUMMARY ==="
echo
echo "Based on this comprehensive test, we need to determine:"
echo "1. Are the 'broken' features actually working in most contexts?"
echo "2. Are there specific edge cases or contexts where they fail?"
echo "3. Is the issue with direct shell invocation vs script execution?"
echo "4. Do we need to focus on edge cases and error handling instead?"
echo
echo "If most tests above show ✅ WORKING, then these features may not"
echo "be as broken as initially assessed, and we should focus on:"
echo "- Polishing edge cases"
echo "- Improving error handling"
echo "- Finding the specific contexts where they fail"
echo "- Moving on to other development priorities"
echo
echo "=== TEST COMPLETE ==="
