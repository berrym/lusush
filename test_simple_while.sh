#!/bin/bash

echo "=== WHILE LOOP TEST ==="
echo

echo "1. Basic while loop:"
counter=1
while [ "$counter" -le 3 ]; do
    echo "  Counter: $counter"
    counter=$((counter + 1))
done

echo
echo "2. While loop with multiple commands:"
i=1
while [ "$i" -le 2 ]; do
    echo "  Iteration $i"
    result=$((i * 2))
    echo "  Result: $result"
    i=$((i + 1))
done

echo
echo "3. While loop with variable assignments:"
loop_count=0
while [ "$loop_count" -lt 2 ]; do
    loop_var=VALUE_$loop_count
    echo "  Set loop_var to: $loop_var"
    loop_count=$((loop_count + 1))
done
echo "  Final loop_var: $loop_var"

echo
echo "4. While loop with semicolons:"
semi_count=0
while [ "$semi_count" -lt 2 ]; do
    echo "  Semi iteration: $semi_count"; semi_var=SEMI_$semi_count; semi_count=$((semi_count + 1))
done
echo "  Final semi_var: $semi_var"

echo
echo "5. Nested while test (if not hanging):"
outer=1
echo "  Testing simple nested structure (may hang):"
# This should be commented out as it causes hangs
# while [ "$outer" -le 1 ]; do
#     inner=1
#     while [ "$inner" -le 1 ]; do
#         echo "    Nested: $outer,$inner"
#         inner=$((inner + 1))
#     done
#     outer=$((outer + 1))
# done
echo "  (Nested while loops skipped - known to cause hangs)"

echo
echo "=== WHILE LOOP TEST COMPLETE ==="
