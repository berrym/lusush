#!/bin/bash

echo "=== ARITHMETIC WITH VARIABLES TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "Creating test script for lusush..."

# Create a temporary script to test arithmetic with variables
cat > temp_arith_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

# Test 1: Simple variable assignment and arithmetic
echo "Test 1: Simple variable and arithmetic"
a=10
b=5
echo "a=$a, b=$b"
echo "Sum: $((a + b))"

# Test 2: Arithmetic with command substitution
echo "Test 2: Mixed arithmetic and command substitution"
c=$(echo 3)
echo "c=$c"
echo "Result: $((5 + c))"

# Test 3: Complex nested case
echo "Test 3: Complex nested"
x=2
y=3
echo "x=$x, y=$y"
echo "Complex: $((x * (y + 1)))"

# Test 4: Variables in arithmetic expressions
echo "Test 4: Multiple variables"
num1=15
num2=7
echo "num1=$num1, num2=$num2"
echo "Addition: $((num1 + num2))"
echo "Subtraction: $((num1 - num2))"
echo "Multiplication: $((num1 * num2))"
echo "Division: $((num1 / num2))"

# Test 5: Error case
echo "Test 5: Undefined variable in arithmetic"
echo "Undefined var: $((undefined_var + 1))"
EOF

chmod +x temp_arith_test.sh

echo "Running test script with lusush..."
./temp_arith_test.sh
echo
echo "Exit code: $?"

echo
echo "Cleaning up..."
rm temp_arith_test.sh

echo "Done!"
