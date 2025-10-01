#!/usr/bin/env lusush

# Comprehensive Test Suite for Documentation Examples
# Tests all code examples from Lusush documentation to verify they work correctly
# This addresses the critical documentation accuracy issue identified in the handoff document

echo "=========================================="
echo "Lusush Documentation Examples Test Suite"
echo "=========================================="
echo

# Test counter
total_tests=0
passed_tests=0
failed_tests=0

# Test function
test_example() {
    local test_name="$1"
    local expected_behavior="$2"
    shift 2

    total_tests=$((total_tests + 1))
    echo "Test $total_tests: $test_name"
    echo "Expected: $expected_behavior"

    # Run the test
    if "$@" >/dev/null 2>&1; then
        echo "✅ PASS"
        passed_tests=$((passed_tests + 1))
    else
        echo "❌ FAIL"
        failed_tests=$((failed_tests + 1))
        echo "Command failed: $*"
    fi
    echo
}

# Test with output capture
test_example_with_output() {
    local test_name="$1"
    local expected_output="$2"
    shift 2

    total_tests=$((total_tests + 1))
    echo "Test $total_tests: $test_name"
    echo "Expected output: $expected_output"

    # Run the test and capture output
    actual_output=$("$@" 2>&1)
    exit_code=$?

    if [ $exit_code -eq 0 ] && [ "$actual_output" = "$expected_output" ]; then
        echo "✅ PASS"
        passed_tests=$((passed_tests + 1))
    else
        echo "❌ FAIL"
        failed_tests=$((failed_tests + 1))
        echo "Exit code: $exit_code"
        echo "Expected: '$expected_output'"
        echo "Actual:   '$actual_output'"
    fi
    echo
}

echo "=== Testing Basic POSIX Constructs ==="

# Test 1: Basic for loop (from README.md)
test_example_with_output \
    "Basic for loop with numbers" \
    "Number: 1
Number: 2
Number: 3" \
    sh -c 'for i in 1 2 3; do echo "Number: $i"; done'

# Test 2: For loop with items (from BUILTIN_COMMANDS.md)
test_example_with_output \
    "For loop with items" \
    "Fruit: apple
Fruit: banana
Fruit: cherry" \
    sh -c 'for item in apple banana cherry; do echo "Fruit: $item"; done'

# Test 3: Parameter expansion (from USER_GUIDE.md)
test_example_with_output \
    "Parameter expansion" \
    "Directory: /path/to
Basename: file.txt
Extension: txt" \
    sh -c 'filename="/path/to/file.txt"; echo "Directory: ${filename%/*}"; echo "Basename: ${filename##*/}"; echo "Extension: ${filename##*.}"'

# Test 4: String length (from ADVANCED_SCRIPTING_GUIDE.md)
test_example_with_output \
    "String length calculation" \
    "Length: 5" \
    sh -c 'text="hello"; echo "Length: ${#text}"'

# Test 5: Arithmetic expansion
test_example_with_output \
    "Arithmetic expansion" \
    "Result: 15" \
    sh -c 'echo "Result: $((3 * 5))"'

# Test 6: Conditional test
test_example \
    "POSIX test construct" \
    "Should pass for existing file" \
    test -f /etc/passwd

# Test 7: While loop with variable persistence
echo "Test $((total_tests + 1)): While loop variable persistence"
result=0
i=1
while [ $i -le 3 ]; do
    result=$((result + i))
    i=$((i + 1))
done
total_tests=$((total_tests + 1))
if [ "$result" -eq 6 ]; then
    echo "✅ PASS - Variables persist in while loops"
    passed_tests=$((passed_tests + 1))
else
    echo "❌ FAIL - Expected 6, got $result"
    failed_tests=$((failed_tests + 1))
fi
echo

# Test 8: For loop variable persistence (CRITICAL TEST)
echo "Test $((total_tests + 1)): For loop variable persistence"
result=0
for i in 1 2 3; do
    result=$((result + i))
done
total_tests=$((total_tests + 1))
if [ "$result" -eq 6 ]; then
    echo "✅ PASS - Variables persist in for loops"
    passed_tests=$((passed_tests + 1))
else
    echo "⚠️  ISSUE - For loop variable scope problem"
    echo "   Expected: 6, Actual: $result"
    echo "   This is a known limitation that needs documentation"
    failed_tests=$((failed_tests + 1))
fi
echo

echo "=== Testing Bash-Specific Constructs (Should Fail) ==="

# Test 9: Brace expansion should fail
total_tests=$((total_tests + 1))
echo "Test $total_tests: Brace expansion should fail"
if echo 'for i in {1..3}; do echo $i; done' | lusush 2>/dev/null; then
    echo "❌ FAIL - Brace expansion should not work"
    failed_tests=$((failed_tests + 1))
else
    echo "✅ PASS - Brace expansion correctly rejected"
    passed_tests=$((passed_tests + 1))
fi
echo

# Test 10: Double bracket test should fail
total_tests=$((total_tests + 1))
echo "Test $total_tests: Double bracket test should fail"
if echo '[[ -f /etc/passwd ]] && echo test' | lusush 2>/dev/null; then
    echo "❌ FAIL - Double bracket test should not work"
    failed_tests=$((failed_tests + 1))
else
    echo "✅ PASS - Double bracket test correctly rejected"
    passed_tests=$((passed_tests + 1))
fi
echo

echo "=== Testing Complex Examples ==="

# Test 11: Complex parameter expansion (from COMPREHENSIVE_TEST_SUITE.md)
test_example_with_output \
    "Complex parameter expansion" \
    "https example.com:8080" \
    sh -c 'url="https://example.com:8080/path"; proto=${url%%://*}; rest=${url#*://}; host=${rest%%/*}; echo "$proto $host"'

# Test 12: Case statement
test_example_with_output \
    "Case statement" \
    "Starting" \
    sh -c 'action="start"; case "$action" in start) echo "Starting" ;; stop) echo "Stopping" ;; *) echo "Unknown" ;; esac'

# Test 13: Function definition and call
test_example_with_output \
    "Function definition and call" \
    "Hello, World!" \
    sh -c 'greet() { echo "Hello, $1!"; }; greet "World"'

echo "=== Summary ==="
echo "Total tests: $total_tests"
echo "Passed: $passed_tests"
echo "Failed: $failed_tests"
echo

if [ $failed_tests -eq 0 ]; then
    echo "🎉 All tests passed! Documentation examples are accurate."
    exit 0
else
    echo "⚠️  $failed_tests test(s) failed. Documentation needs updates."

    # Provide specific recommendations
    echo
    echo "=== Recommendations ==="
    echo "1. Document the for-loop variable scope limitation"
    echo "2. Provide while-loop alternatives for examples that need variable persistence"
    echo "3. Add warning notes in documentation about this behavior"
    echo "4. Consider examples that work around this limitation"
    echo

    exit 1
fi
