#!/bin/bash

# Comprehensive Test Script for Lusush Input Methods and Loop Types
# Tests the fix for script sourcing and debug system integration

echo "=== LUSUSH COMPREHENSIVE INPUT METHOD TESTING ==="
echo "Testing all input methods with loop constructs after bin_source fix"
echo ""

# Build lusush first
echo "Building lusush..."
ninja -C builddir > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "FAILED: Build failed"
    exit 1
fi
echo "✅ Build successful"
echo ""

# Create test files
echo "Creating test files..."

# Test 1: Simple for loop
cat > test_for_loop.sh << 'EOF'
for i in 1 2 3; do
    echo "FOR: $i"
done
EOF

# Test 2: While loop
cat > test_while_loop.sh << 'EOF'
count=1
while [ $count -le 3 ]; do
    echo "WHILE: $count"
    count=$((count + 1))
done
EOF

# Test 3: Until loop
cat > test_until_loop.sh << 'EOF'
count=1
until [ $count -gt 3 ]; do
    echo "UNTIL: $count"
    count=$((count + 1))
done
EOF

# Test 4: Nested loops
cat > test_nested_loops.sh << 'EOF'
for outer in a b; do
    for inner in 1 2; do
        echo "NESTED: $outer$inner"
    done
done
EOF

# Test 5: Complex script with functions and loops
cat > test_complex.sh << 'EOF'
test_function() {
    local param=$1
    for i in 1 2; do
        echo "FUNCTION: $param-$i"
    done
}

for letter in x y; do
    test_function $letter
done
EOF

echo "✅ Test files created"
echo ""

# Test functions
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_lines="$3"

    echo -n "Testing $test_name: "

    # Run the command and capture output
    output=$(eval "$command" 2>&1)
    exit_code=$?

    # Check for errors
    if echo "$output" | grep -q "DEBUG: Unhandled keyword type 46 (DONE)"; then
        echo "❌ FAILED - DONE parsing error detected"
        echo "   Output: $output"
        return 1
    fi

    if [ $exit_code -ne 0 ]; then
        echo "❌ FAILED - Non-zero exit code: $exit_code"
        echo "   Output: $output"
        return 1
    fi

    # Count non-empty output lines
    actual_lines=$(echo "$output" | grep -v "^$" | wc -l)

    if [ "$actual_lines" -eq "$expected_lines" ]; then
        echo "✅ PASSED ($actual_lines lines)"
        return 0
    else
        echo "❌ FAILED - Expected $expected_lines lines, got $actual_lines"
        echo "   Output: $output"
        return 1
    fi
}

# =============================================================================
# TEST SECTION 1: DIRECT COMMAND INPUT
# =============================================================================
echo "=== TEST SECTION 1: DIRECT COMMAND INPUT ==="

run_test "Direct for loop" \
    "echo 'for i in 1 2 3; do echo \"DIRECT: \$i\"; done' | ./builddir/lusush" \
    3

run_test "Direct while loop" \
    "echo 'count=1; while [ \$count -le 3 ]; do echo \"DIRECT: \$count\"; count=\$((count + 1)); done' | ./builddir/lusush" \
    3

run_test "Direct until loop" \
    "echo 'count=1; until [ \$count -gt 3 ]; do echo \"DIRECT: \$count\"; count=\$((count + 1)); done' | ./builddir/lusush" \
    3

echo ""

# =============================================================================
# TEST SECTION 2: SCRIPT SOURCING
# =============================================================================
echo "=== TEST SECTION 2: SCRIPT SOURCING (THE FIXED ISSUE) ==="

run_test "Source for loop" \
    "echo 'source test_for_loop.sh' | ./builddir/lusush" \
    3

run_test "Source while loop" \
    "echo 'source test_while_loop.sh' | ./builddir/lusush" \
    3

run_test "Source until loop" \
    "echo 'source test_until_loop.sh' | ./builddir/lusush" \
    3

run_test "Source nested loops" \
    "echo 'source test_nested_loops.sh' | ./builddir/lusush" \
    4

run_test "Source complex script" \
    "echo 'source test_complex.sh' | ./builddir/lusush" \
    4

echo ""

# =============================================================================
# TEST SECTION 3: PIPED MULTILINE INPUT
# =============================================================================
echo "=== TEST SECTION 3: PIPED MULTILINE INPUT ==="

run_test "Piped multiline for loop" \
    "cat test_for_loop.sh | ./builddir/lusush" \
    3

run_test "Piped multiline while loop" \
    "cat test_while_loop.sh | ./builddir/lusush" \
    3

run_test "Piped multiline until loop" \
    "cat test_until_loop.sh | ./builddir/lusush" \
    3

run_test "Piped nested loops" \
    "cat test_nested_loops.sh | ./builddir/lusush" \
    4

run_test "Piped complex script" \
    "cat test_complex.sh | ./builddir/lusush" \
    4

echo ""

# =============================================================================
# TEST SECTION 4: DEBUG SYSTEM WITH LOOPS (ORIGINAL BUG CONTEXT)
# =============================================================================
echo "=== TEST SECTION 4: DEBUG SYSTEM WITH LOOPS ==="

# Test debug with for loop
cat > debug_test_for.sh << 'EOF'
for i in 1 2; do
    echo "DEBUG_TEST: $i"
done
EOF

echo -n "Testing debug system with for loop: "
output=$(echo 'debug on; debug break add debug_test_for.sh 2; source debug_test_for.sh' | ./builddir/lusush 2>&1)

if echo "$output" | grep -q "DEBUG: Unhandled keyword type 46 (DONE)"; then
    echo "❌ FAILED - DONE parsing error in debug mode"
elif echo "$output" | grep -q "DEBUG_TEST: 1" && echo "$output" | grep -q "DEBUG_TEST: 2"; then
    echo "✅ PASSED - Debug system works with loops"
else
    echo "❌ FAILED - Loop variables not working correctly in debug mode"
    echo "   Output: $output"
fi

echo ""

# =============================================================================
# TEST SECTION 5: REGRESSION TESTING
# =============================================================================
echo "=== TEST SECTION 5: REGRESSION TESTING ==="

run_test "Basic commands still work" \
    "echo 'echo hello; pwd > /dev/null; echo world' | ./builddir/lusush" \
    2

run_test "Variables still work" \
    "echo 'x=test; echo \$x' | ./builddir/lusush" \
    1

run_test "Conditionals still work" \
    "echo 'if [ 1 -eq 1 ]; then echo success; fi' | ./builddir/lusush" \
    1

echo ""

# =============================================================================
# SUMMARY
# =============================================================================
echo "=== TEST SUMMARY ==="
echo "All input methods tested:"
echo "  ✓ Direct command input"
echo "  ✓ Script sourcing (source command) - THE MAIN FIX"
echo "  ✓ Piped multiline input"
echo "  ✓ Debug system integration"
echo "  ✓ Basic regression testing"
echo ""

# Clean up test files
rm -f test_for_loop.sh test_while_loop.sh test_until_loop.sh test_nested_loops.sh test_complex.sh debug_test_for.sh

echo "✅ COMPREHENSIVE TESTING COMPLETE"
echo ""
echo "CONCLUSION: If all tests passed, the bin_source fix has resolved:"
echo "  1. Script sourcing regression (loops in sourced files)"
echo "  2. Original debug system loop bug"
echo "  3. All POSIX loop constructs work across all input methods"
