#!/bin/bash

# Comprehensive Test Suite for Modern Parameter Expansion Features
# Tests advanced ${var} parameter expansion patterns

echo "=== LUSUSH PARAMETER EXPANSION TEST SUITE ==="
echo

# Build the project first
echo "Building lusush..."
meson setup builddir --reconfigure > /dev/null 2>&1
ninja -C builddir > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build successful"
echo

# Test function
test_expansion() {
    local description="$1"
    local command="$2"
    local expected="$3"

    echo "Testing: $description"
    echo "Command: $command"

    # Run the command and capture output
    result=$(echo "$command" | ./builddir/lusush 2>&1 | tail -n 1)

    if [ "$result" = "$expected" ]; then
        echo "✅ PASS: Got '$result'"
    else
        echo "❌ FAIL: Expected '$expected', got '$result'"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    echo
}

FAILED_TESTS=0

echo "=== BASIC VARIABLE EXPANSION ==="
test_expansion "Simple variable" "name=John; echo \$name" "John"
test_expansion "Braced variable" "name=John; echo \${name}" "John"
test_expansion "Undefined variable" "echo \$undefined" ""

echo "=== DEFAULT VALUES (:-) ==="
test_expansion "Default for unset variable" "echo \${unset:-default}" "default"
test_expansion "Default for empty variable" "empty=; echo \${empty:-default}" "default"
test_expansion "No default for set variable" "name=John; echo \${name:-default}" "John"

echo "=== ALTERNATIVE VALUES (:+) ==="
test_expansion "Alternative for set variable" "name=John; echo \${name:+alternative}" "alternative"
test_expansion "No alternative for unset variable" "echo \${unset:+alternative}" ""
test_expansion "No alternative for empty variable" "empty=; echo \${empty:+alternative}" ""

echo "=== UNSET vs EMPTY DISTINCTION ==="
test_expansion "Default (-) for unset only" "echo \${unset-default}" "default"
test_expansion "No default (-) for empty" "empty=; echo \${empty-default}" ""
test_expansion "Alternative (+) for set even if empty" "empty=; echo \${empty+alternative}" "alternative"

echo "=== LENGTH EXPANSION ==="
test_expansion "Length of string" "name=John; echo \${#name}" "4"
test_expansion "Length of empty string" "empty=; echo \${#empty}" "0"
test_expansion "Length of unset variable" "echo \${#unset}" "0"
test_expansion "Length of longer string" "message='Hello World'; echo \${#message}" "11"

echo "=== SUBSTRING EXPANSION ==="
test_expansion "Substring from start" "text=Hello; echo \${text:1:3}" "ell"
test_expansion "Substring to end" "text=Hello; echo \${text:2}" "llo"
test_expansion "Substring from offset 0" "text=Hello; echo \${text:0:2}" "He"
test_expansion "Substring beyond length" "text=Hi; echo \${text:1:10}" "i"

echo "=== COMPLEX COMBINATIONS ==="
test_expansion "Default with spaces" "echo \${unset:-default value}" "default value"
test_expansion "Alternative with spaces" "name=John; echo \${name:+Hello \$name}" "Hello John"
test_expansion "Nested variable in default" "default=backup; echo \${unset:-\$default}" "backup"

echo "=== INTEGRATION WITH EXISTING FEATURES ==="
test_expansion "Parameter expansion in quotes" "name=John; echo \"Hello \${name:-Guest}\"" "Hello John"
test_expansion "Parameter expansion with arithmetic" "num=5; echo \${num:-0}" "5"
test_expansion "Multiple expansions" "first=A; second=B; echo \${first:-X}\${second:-Y}" "AB"

echo "=== EDGE CASES ==="
test_expansion "Empty default" "echo \${unset:-}" ""
test_expansion "Colon in default value" "echo \${unset:-http://example.com}" "http://example.com"
test_expansion "Braces in default value" "echo \${unset:-{test}}" "{test}"

echo "=== ADVANCED PATTERNS ==="
test_expansion "Substring with variables" "text=Hello; start=1; len=3; echo \${text:\$start:\$len}" "ell"
test_expansion "Default with command substitution" "echo \${unset:-\$(echo default)}" "default"

echo "=== RESULTS ==="
if [ $FAILED_TESTS -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    echo "Modern parameter expansion is working correctly."
else
    echo "❌ $FAILED_TESTS tests failed"
    echo "Parameter expansion needs debugging."
fi

echo
echo "Test completed: $(date)"
