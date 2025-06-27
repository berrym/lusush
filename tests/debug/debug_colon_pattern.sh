#!/bin/bash

echo "=== COLON PATTERN MATCHING DIAGNOSTIC ==="
echo "Testing colon character in parameter expansion patterns"
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

echo "=== BASIC COLON TESTS ==="

echo "Test 1: Simple colon in string"
echo "Command: text=a:b; echo \"\$text\""
result=$(echo 'text=a:b; echo "$text"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'a:b'"
if [ "$result" = "a:b" ]; then
    echo "✅ PASS - Basic colon handling works"
else
    echo "❌ FAIL - Basic colon handling broken"
fi
echo

echo "Test 2: Literal colon removal"
echo "Command: text=a:b:c; echo \"\${text#a:}\""
result=$(echo 'text=a:b:c; echo "${text#a:}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'b:c'"
if [ "$result" = "b:c" ]; then
    echo "✅ PASS - Literal colon pattern works"
else
    echo "❌ FAIL - Literal colon pattern broken"
fi
echo

echo "Test 3: Wildcard with colon"
echo "Command: text=a:b:c; echo \"\${text#*:}\""
result=$(echo 'text=a:b:c; echo "${text#*:}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'b:c'"
if [ "$result" = "b:c" ]; then
    echo "✅ PASS - Wildcard colon pattern works"
else
    echo "❌ FAIL - Wildcard colon pattern broken"
fi
echo

echo "Test 4: URL-style pattern"
echo "Command: url=http://example.com; echo \"\${url#*://}\""
result=$(echo 'url=http://example.com; echo "${url#*://}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'example.com'"
if [ "$result" = "example.com" ]; then
    echo "✅ PASS - URL pattern parsing works"
else
    echo "❌ FAIL - URL pattern parsing broken"
fi
echo

echo "Test 5: Complex URL pattern"
echo "Command: url=https://user:pass@host.com:8080/path; echo \"\${url#*://}\""
result=$(echo 'url=https://user:pass@host.com:8080/path; echo "${url#*://}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'user:pass@host.com:8080/path'"
if [ "$result" = "user:pass@host.com:8080/path" ]; then
    echo "✅ PASS - Complex URL pattern works"
else
    echo "❌ FAIL - Complex URL pattern broken"
fi
echo

echo "=== OPERATOR DETECTION TESTS ==="

echo "Test 6: Default value with colon in value"
echo "Command: echo \"\${unset:-http://default.com}\""
result=$(echo 'echo "${unset:-http://default.com}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'http://default.com'"
if [ "$result" = "http://default.com" ]; then
    echo "✅ PASS - Colon in default value works"
else
    echo "❌ FAIL - Colon in default value broken"
fi
echo

echo "Test 7: Alternative with colon in value"
echo "Command: var=set; echo \"\${var:+http://alt.com}\""
result=$(echo 'var=set; echo "${var:+http://alt.com}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'http://alt.com'"
if [ "$result" = "http://alt.com" ]; then
    echo "✅ PASS - Colon in alternative value works"
else
    echo "❌ FAIL - Colon in alternative value broken"
fi
echo

echo "Test 8: Substring with colon values"
echo "Command: text=a:b:c:d:e; echo \"\${text:2:3}\""
result=$(echo 'text=a:b:c:d:e; echo "${text:2:3}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'b:c'"
if [ "$result" = "b:c" ]; then
    echo "✅ PASS - Substring with colons works"
else
    echo "❌ FAIL - Substring with colons broken"
fi
echo

echo "=== EDGE CASE TESTS ==="

echo "Test 9: Multiple colons in pattern"
echo "Command: text=a::b::c; echo \"\${text#*::}\""
result=$(echo 'text=a::b::c; echo "${text#*::}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'b::c'"
if [ "$result" = "b::c" ]; then
    echo "✅ PASS - Double colon pattern works"
else
    echo "❌ FAIL - Double colon pattern broken"
fi
echo

echo "Test 10: Colon at start of pattern"
echo "Command: text=:abc; echo \"\${text#:}\""
result=$(echo 'text=:abc; echo "${text#:}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'abc'"
if [ "$result" = "abc" ]; then
    echo "✅ PASS - Leading colon removal works"
else
    echo "❌ FAIL - Leading colon removal broken"
fi
echo

echo "Test 11: Colon at end of pattern"
echo "Command: text=abc:; echo \"\${text%:}\""
result=$(echo 'text=abc:; echo "${text%:}"' | ./builddir/lusush 2>&1)
echo "Result: '$result'"
echo "Expected: 'abc'"
if [ "$result" = "abc" ]; then
    echo "✅ PASS - Trailing colon removal works"
else
    echo "❌ FAIL - Trailing colon removal broken"
fi
echo

echo "=== DIAGNOSIS SUMMARY ==="
echo "This test isolates colon-specific pattern matching issues."
echo "Key areas to check:"
echo "1. Operator precedence (is ':' being treated as substring operator?)"
echo "2. Pattern parsing (are colons in patterns being escaped/modified?)"
echo "3. Wildcard matching (does '*:' work differently than '*a'?)"
echo "4. Context sensitivity (colon behavior in different expansion types)"
