#!/bin/bash

# Test script for until loop functionality in lusush shell

echo "=== Testing Until Loop Implementation ==="

# Test 1: Basic until loop with counter
echo "Test 1: Basic until loop"
./builddir/lusush -c '
i=1
until [ $i -gt 3 ]; do
    echo "Iteration $i"
    i=$((i + 1))
done
echo "Final value: $i"
'

echo ""

# Test 2: Until loop with command condition
echo "Test 2: Until loop with command condition"
./builddir/lusush -c '
count=0
until false; do
    echo "Count: $count"
    count=$((count + 1))
    if [ $count -ge 2 ]; then
        break
    fi
done
'

echo ""

# Test 3: Until loop that never executes (condition is true from start)
echo "Test 3: Until loop that never executes"
./builddir/lusush -c '
echo "Before until loop"
until true; do
    echo "This should never execute"
done
echo "After until loop"
'

echo ""

# Test 4: Until loop with variable assignment in condition
echo "Test 4: Until loop with variable assignment"
./builddir/lusush -c '
x=5
until [ $x -eq 0 ]; do
    echo "x is $x"
    x=$((x - 1))
done
echo "Loop finished, x is $x"
'

echo ""

# Test 5: Nested until and while loops
echo "Test 5: Nested loops"
./builddir/lusush -c '
outer=1
until [ $outer -gt 2 ]; do
    echo "Outer loop: $outer"
    inner=1
    while [ $inner -le 2 ]; do
        echo "  Inner loop: $inner"
        inner=$((inner + 1))
    done
    outer=$((outer + 1))
done
'

echo ""

# Test 6: Until loop with multiple commands in body
echo "Test 6: Multiple commands in until loop body"
./builddir/lusush -c '
n=3
until [ $n -eq 0 ]; do
    echo "n is $n"
    echo "Decreasing n"
    n=$((n - 1))
    echo "n is now $n"
done
'

echo ""

# Test 7: Until loop with pipeline condition
echo "Test 7: Until loop with pipeline condition"
./builddir/lusush -c '
counter=0
until echo "test" | grep -q "xyz"; do
    echo "Pipeline failed as expected, counter: $counter"
    counter=$((counter + 1))
    if [ $counter -ge 2 ]; then
        break
    fi
done
'

echo ""

echo "=== Until Loop Tests Complete ==="
