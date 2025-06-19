#!/bin/bash

echo "=== CONDITION TESTING ==="
echo

echo "1. Test command conditions:"
if test -f README.md; then
    echo "  README.md exists (test command works)"
fi

if test -d builddir; then
    echo "  builddir directory exists"
fi

echo
echo "2. Bracket conditions:"
if [ -f "meson.build" ]; then
    echo "  meson.build exists (bracket test works)"
fi

if [ ! -f "nonexistent.file" ]; then
    echo "  nonexistent.file does not exist (negation works)"
fi

echo
echo "3. String comparisons:"
str1="hello"
str2="world"
str3="hello"

if [ "$str1" = "$str3" ]; then
    echo "  String equality works: '$str1' = '$str3'"
fi

if [ "$str1" != "$str2" ]; then
    echo "  String inequality works: '$str1' != '$str2'"
fi

echo
echo "4. Numeric comparisons:"
num1=5
num2=10
num3=5

if [ "$num1" -eq "$num3" ]; then
    echo "  Numeric equality works: $num1 -eq $num3"
fi

if [ "$num1" -lt "$num2" ]; then
    echo "  Numeric less-than works: $num1 -lt $num2"
fi

if [ "$num2" -gt "$num1" ]; then
    echo "  Numeric greater-than works: $num2 -gt $num1"
fi

echo
echo "5. Complex conditions:"
if [ "$num1" -eq 5 ] && [ "$str1" = "hello" ]; then
    echo "  AND condition works"
fi

if [ "$num1" -eq 999 ] || [ "$str1" = "hello" ]; then
    echo "  OR condition works"
fi

echo
echo "6. Conditions in loops:"
echo "Testing conditions in for loop:"
for val in 1 2 3 4 5; do
    if [ "$val" -eq 3 ]; then
        echo "  Found value 3 in for loop"
    fi
done

echo "Testing conditions in while loop:"
test_counter=1
while [ "$test_counter" -le 3 ]; do
    if [ "$test_counter" -eq 2 ]; then
        echo "  Found counter 2 in while loop"
    fi
    test_counter=$((test_counter + 1))
done

echo
echo "=== CONDITION TEST COMPLETE ==="
