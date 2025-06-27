#!/bin/bash

echo "=== DEBUG TOKENIZER TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "Testing tokenizer parsing for arithmetic expressions..."
echo

# Create a simple test to see how tokens are being parsed
echo "Test 1: Simple arithmetic with debug output"
echo 'echo $((5 + 3))' | "$LUSUSH" 2>&1
echo

echo "Test 2: Arithmetic with variables"
echo -e "a=10\necho \$((a + 5))" | "$LUSUSH" 2>&1
echo

echo "Test 3: Complex arithmetic"
echo -e "x=2\ny=3\necho \$((x * (y + 1)))" | "$LUSUSH" 2>&1
echo

echo "Test 4: Mixed arithmetic and command substitution"
echo 'echo $((5 + $(echo 3)))' | "$LUSUSH" 2>&1
echo

echo "Test 5: Simple command substitution (for comparison)"
echo 'echo $(echo hello)' | "$LUSUSH" 2>&1
echo

echo "Test 6: Variable expansion (for comparison)"
echo -e "VAR=test\necho \$VAR" | "$LUSUSH" 2>&1
echo

echo "Test 7: Testing if arithmetic is parsed as command substitution"
echo 'echo "Debug: $((2 + 2))"' | "$LUSUSH" 2>&1
echo

echo "Test 8: Checking what happens with whitespace in arithmetic"
echo 'echo $(( 5 + 3 ))' | "$LUSUSH" 2>&1
echo

echo "Test 9: Checking parentheses balancing"
echo 'echo $((5 + (3 * 2)))' | "$LUSUSH" 2>&1
echo

echo "Done!"
