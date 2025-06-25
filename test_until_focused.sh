#!/bin/bash

# Focused test script for until loop functionality in lusush shell
# Tests basic until loop behavior without using break statements

echo "=== Focused Until Loop Tests ==="

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

# Test 2: Until loop that never executes (condition is true from start)
echo "Test 2: Until loop that never executes"
./builddir/lusush -c '
echo "Before until loop"
until true; do
    echo "This should never execute"
done
echo "After until loop"
'

echo ""

# Test 3: Until loop with variable assignment in condition
echo "Test 3: Until loop with countdown"
./builddir/lusush -c '
x=5
until [ $x -eq 0 ]; do
    echo "x is $x"
    x=$((x - 1))
done
echo "Loop finished, x is $x"
'

echo ""

# Test 4: Until loop with string comparison
echo "Test 4: Until loop with string comparison"
./builddir/lusush -c '
word="hello"
count=0
until [ "$word" = "done" ]; do
    echo "word is still $word, count: $count"
    count=$((count + 1))
    if [ $count -eq 3 ]; then
        word="done"
    fi
done
echo "Final: word=$word, count=$count"
'

echo ""

# Test 5: Until loop with multiple commands in body
echo "Test 5: Multiple commands in until loop body"
./builddir/lusush -c '
n=3
until [ $n -eq 0 ]; do
    echo "n is $n"
    echo "Decreasing n"
    n=$((n - 1))
    echo "n is now $n"
done
echo "All done!"
'

echo ""

# Test 6: Until loop with arithmetic condition
echo "Test 6: Until loop with arithmetic condition"
./builddir/lusush -c '
sum=0
i=1
until [ $sum -ge 10 ]; do
    echo "Adding $i to sum ($sum)"
    sum=$((sum + i))
    i=$((i + 1))
done
echo "Final sum: $sum"
'

echo ""

# Test 7: Nested until and while loops
echo "Test 7: Nested loops"
./builddir/lusush -c '
outer=1
until [ $outer -gt 2 ]; do
    echo "Outer until loop: $outer"
    inner=1
    while [ $inner -le 2 ]; do
        echo "  Inner while loop: $inner"
        inner=$((inner + 1))
    done
    outer=$((outer + 1))
done
echo "Nested loops complete"
'

echo ""

echo "=== Until Loop Tests Complete ==="
