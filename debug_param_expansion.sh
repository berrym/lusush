#!/bin/bash

echo "=== PARAMETER EXPANSION DIAGNOSTIC TEST ==="
echo "Shell: ./builddir/lusush"
echo

# Build first
echo "Building lusush..."
ninja -C builddir > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi
echo

# Test each critical failure case individually
echo "=== CRITICAL FAILURE ANALYSIS ==="

echo "Test 1: Multiple parameter expansions (spacing issue)"
echo "Command: first=A; second=B; echo \${first:-X}\${second:-Y}"
result=$(echo 'first=A; second=B; echo ${first:-X}${second:-Y}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'AB'"
if [ "$result" = "AB" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Spacing/concatenation issue"
fi
echo

echo "Test 2: Braces in default value"
echo "Command: echo \${unset:-{test}}"
result=$(echo 'echo ${unset:-{test}}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: '{test}'"
if [ "$result" = "{test}" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Brace parsing issue"
fi
echo

echo "Test 3: Command substitution in default"
echo "Command: echo \${unset:-\$(echo default)}"
result=$(echo 'echo ${unset:-$(echo default)}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'default'"
if [ "$result" = "default" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Command substitution not expanded in default"
fi
echo

echo "Test 4: Pattern matching with colon (URL case)"
echo "Command: url=https://example.com/path; echo \${url#*://}"
result=$(echo 'url=https://example.com/path; echo ${url#*:/./}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'example.com/path'"
if [ "$result" = "example.com/path" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Pattern matching with colon broken"
fi
echo

echo "Test 5: Pattern matching with @ symbol"
echo "Command: email=user@example.com; echo \${email#*@}"
result=$(echo 'email=user@example.com; echo ${email#*@}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'example.com'"
if [ "$result" = "example.com" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Pattern matching with @ symbol broken"
fi
echo

echo "Test 6: Pattern matching with @ symbol (suffix)"
echo "Command: email=user@example.com; echo \${email%@*}"
result=$(echo 'email=user@example.com; echo ${email%@*}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'user'"
if [ "$result" = "user" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Suffix pattern matching with @ symbol broken"
fi
echo

echo "=== SPECIAL CHARACTER PATTERN TESTS ==="

echo "Test 7: Colon in pattern (simple)"
echo "Command: text=a:b:c; echo \${text#*:}"
result=$(echo 'text=a:b:c; echo ${text#*:}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'b:c'"
if [ "$result" = "b:c" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Simple colon pattern broken"
fi
echo

echo "Test 8: @ symbol in pattern (simple)"
echo "Command: text=a@b@c; echo \${text#*@}"
result=$(echo 'text=a@b@c; echo ${text#*@}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'b@c'"
if [ "$result" = "b@c" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Simple @ pattern broken"
fi
echo

echo "=== TOKENIZER DIAGNOSTIC ==="

echo "Test 9: Check if patterns are properly tokenized"
echo "Command with debug: url=https://example.com; echo \${url#https://}"
result=$(echo 'url=https://example.com; echo ${url#https://}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'example.com'"
if [ "$result" = "example.com" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Literal pattern matching broken"
fi
echo

echo "Test 10: Wildcard with special chars"
echo "Command: path=http://site.com/page; echo \${path#*://}"
result=$(echo 'path=http://site.com/page; echo ${path#*://}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'site.com/page'"
if [ "$result" = "site.com/page" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Wildcard with :// pattern broken"
fi
echo

echo "=== COMPLEX NESTED CASES ==="

echo "Test 11: Parameter expansion with quoted strings"
echo "Command: name=John; echo \"\${name:-Guest}\""
result=$(echo 'name=John; echo "${name:-Guest}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'John'"
if [ "$result" = "John" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Parameter expansion in quotes broken"
fi
echo

echo "Test 12: Multiple parameters in one string"
echo "Command: a=hello; b=world; echo \${a} \${b}"
result=$(echo 'a=hello; b=world; echo ${a} ${b}' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'hello world'"
if [ "$result" = "hello world" ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL - Multiple parameter expansion broken"
fi
echo

echo "=== SUMMARY ==="
echo "This diagnostic identifies the specific broken cases in parameter expansion."
echo "Focus areas for fixes:"
echo "1. Multiple parameter concatenation (spacing)"
echo "2. Brace handling in default values"
echo "3. Command substitution in defaults"
echo "4. Pattern matching with special characters (: and @)"
echo "5. Tokenizer handling of complex patterns"
