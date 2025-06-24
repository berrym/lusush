#!/bin/bash

echo "=== COMPREHENSIVE POSIX ARITHMETIC EXPANSION TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "Testing all POSIX arithmetic operators and features..."
echo "Current shell: $LUSUSH"
echo

# Function to test arithmetic and compare with expected result
test_arithmetic() {
    local description="$1"
    local expression="$2"
    local expected="$3"

    echo -n "Testing: $description... "

    # Test the expression
    local result=$(echo "echo \$((${expression}))" | "$LUSUSH" 2>/dev/null)

    if [ "$result" = "$expected" ]; then
        echo "✅ PASS ($result)"
    else
        echo "❌ FAIL (expected $expected, got $result)"
    fi
}

# Function to test if operator is supported
test_operator_support() {
    local description="$1"
    local expression="$2"

    echo -n "Testing: $description... "

    # Test if the expression produces any numeric result (not error)
    local result=$(echo "echo \$((${expression}))" | "$LUSUSH" 2>/dev/null)

    if [[ "$result" =~ ^-?[0-9]+$ ]]; then
        echo "✅ SUPPORTED ($result)"
    else
        echo "❌ NOT SUPPORTED or ERROR"
    fi
}

echo "=== BASIC ARITHMETIC OPERATORS ==="

# Addition
test_arithmetic "Addition" "5 + 3" "8"
test_arithmetic "Addition with spaces" " 10 + 2 " "12"
test_arithmetic "Multiple addition" "1 + 2 + 3 + 4" "10"

# Subtraction
test_arithmetic "Subtraction" "10 - 3" "7"
test_arithmetic "Negative result" "5 - 10" "-5"
test_arithmetic "Multiple subtraction" "20 - 5 - 3" "12"

# Multiplication
test_arithmetic "Multiplication" "6 * 7" "42"
test_arithmetic "Multiply by zero" "100 * 0" "0"
test_arithmetic "Multiply negative" "-5 * 3" "-15"

# Division
test_arithmetic "Division" "15 / 3" "5"
test_arithmetic "Integer division" "17 / 3" "5"
test_arithmetic "Division by one" "42 / 1" "42"

# Modulo
test_arithmetic "Modulo" "17 % 5" "2"
test_arithmetic "Modulo zero result" "15 % 3" "0"
test_arithmetic "Modulo larger divisor" "5 % 10" "5"

echo
echo "=== ADVANCED OPERATORS ==="

# Exponentiation (** operator)
test_operator_support "Exponentiation 2**3" "2**3"
test_operator_support "Exponentiation 5**2" "5**2"
test_operator_support "Exponentiation 3**4" "3**4"

# Increment/Decrement
test_operator_support "Pre-increment ++var" "++5"
test_operator_support "Post-increment var++" "5++"
test_operator_support "Pre-decrement --var" "--5"
test_operator_support "Post-decrement var--" "5--"

echo
echo "=== BITWISE OPERATORS ==="

# Bitwise AND
test_operator_support "Bitwise AND" "12 & 10"
test_operator_support "Bitwise AND zero" "5 & 0"

# Bitwise OR
test_operator_support "Bitwise OR" "12 | 10"
test_operator_support "Bitwise OR with zero" "5 | 0"

# Bitwise XOR
test_operator_support "Bitwise XOR" "12 ^ 10"
test_operator_support "Bitwise XOR same" "5 ^ 5"

# Bitwise NOT
test_operator_support "Bitwise NOT" "~5"
test_operator_support "Bitwise NOT zero" "~0"

# Bit shifts
test_operator_support "Left shift" "5 << 2"
test_operator_support "Right shift" "20 >> 2"

echo
echo "=== LOGICAL OPERATORS ==="

# Logical AND
test_operator_support "Logical AND true" "1 && 1"
test_operator_support "Logical AND false" "1 && 0"
test_operator_support "Logical AND short-circuit" "0 && 5"

# Logical OR
test_operator_support "Logical OR true" "0 || 1"
test_operator_support "Logical OR false" "0 || 0"
test_operator_support "Logical OR short-circuit" "1 || 5"

# Logical NOT
test_operator_support "Logical NOT true" "!1"
test_operator_support "Logical NOT false" "!0"
test_operator_support "Logical NOT number" "!5"

echo
echo "=== COMPARISON OPERATORS ==="

# Equality
test_arithmetic "Equal true" "5 == 5" "1"
test_arithmetic "Equal false" "5 == 3" "0"
test_arithmetic "Not equal true" "5 != 3" "1"
test_arithmetic "Not equal false" "5 != 5" "0"

# Relational
test_arithmetic "Less than true" "3 < 5" "1"
test_arithmetic "Less than false" "5 < 3" "0"
test_arithmetic "Less equal true" "5 <= 5" "1"
test_arithmetic "Less equal false" "6 <= 5" "0"

test_arithmetic "Greater than true" "5 > 3" "1"
test_arithmetic "Greater than false" "3 > 5" "0"
test_arithmetic "Greater equal true" "5 >= 5" "1"
test_arithmetic "Greater equal false" "4 >= 5" "0"

echo
echo "=== PARENTHESES AND PRECEDENCE ==="

test_arithmetic "Simple parentheses" "(5 + 3) * 2" "16"
test_arithmetic "Nested parentheses" "((2 + 3) * (4 + 1))" "25"
test_arithmetic "Precedence without parens" "2 + 3 * 4" "14"
test_arithmetic "Precedence with parens" "(2 + 3) * 4" "20"
test_arithmetic "Complex precedence" "2 * 3 + 4 * 5" "26"

echo
echo "=== VARIABLES IN ARITHMETIC ==="

echo "Setting up test variables..."
echo -e "a=10\nb=5\nc=2" | "$LUSUSH" > /dev/null

# Test variables in arithmetic (using separate shell invocations)
test_arithmetic "Variable addition" "a + b" "15"
test_arithmetic "Variable multiplication" "a * c" "20"
test_arithmetic "Mixed variables and literals" "a + 5" "15"
test_arithmetic "Variable in parentheses" "(a + b) * c" "30"

echo
echo "=== ASSIGNMENT OPERATORS ==="

test_operator_support "Simple assignment" "x = 5"
test_operator_support "Add assignment" "x += 5"
test_operator_support "Subtract assignment" "x -= 3"
test_operator_support "Multiply assignment" "x *= 2"
test_operator_support "Divide assignment" "x /= 2"
test_operator_support "Modulo assignment" "x %= 3"

echo
echo "=== TERNARY OPERATOR ==="

test_operator_support "Ternary true" "1 ? 10 : 20"
test_operator_support "Ternary false" "0 ? 10 : 20"
test_operator_support "Ternary with expression" "(5 > 3) ? 100 : 200"

echo
echo "=== OCTAL AND HEXADECIMAL ==="

test_arithmetic "Octal number" "010" "8"
test_arithmetic "Octal arithmetic" "010 + 2" "10"
test_operator_support "Hexadecimal number" "0x10"
test_operator_support "Hexadecimal arithmetic" "0x10 + 5"

echo
echo "=== SPECIAL CASES ==="

test_arithmetic "Unary minus" "-5" "-5"
test_arithmetic "Unary plus" "+5" "5"
test_arithmetic "Double negative" "--5" "5"
test_arithmetic "Zero operations" "0 + 0" "0"
test_arithmetic "Large numbers" "999999 + 1" "1000000"

echo
echo "=== ERROR HANDLING ==="

echo -n "Testing: Division by zero... "
result=$(echo "echo \$((5 / 0))" | "$LUSUSH" 2>&1)
if [[ "$result" =~ (error|Error|ERROR|division.*zero) ]]; then
    echo "✅ PROPERLY HANDLED (error detected)"
else
    echo "❌ NOT HANDLED ($result)"
fi

echo -n "Testing: Invalid syntax... "
result=$(echo "echo \$((5 +))" | "$LUSUSH" 2>&1)
if [[ "$result" =~ (error|Error|ERROR|syntax) ]] || [ -z "$result" ]; then
    echo "✅ PROPERLY HANDLED"
else
    echo "❌ NOT HANDLED ($result)"
fi

echo
echo "=== POSIX COMPLIANCE SUMMARY ==="
echo
echo "REQUIRED POSIX ARITHMETIC FEATURES:"
echo "✓ Basic operators: + - * / %"
echo "✓ Parentheses for grouping"
echo "✓ Variables in expressions"
echo "✓ Comparison operators: == != < <= > >="
echo "✓ Logical operators: && || !"
echo "✓ Unary operators: + -"
echo
echo "OPTIONAL/EXTENDED FEATURES:"
echo "? Exponentiation: **"
echo "? Bitwise operators: & | ^ ~ << >>"
echo "? Assignment operators: += -= *= /= %="
echo "? Increment/decrement: ++ --"
echo "? Ternary operator: ? :"
echo "? Octal/hex numbers: 0123 0x123"
echo
echo "=== TEST COMPLETE ==="
