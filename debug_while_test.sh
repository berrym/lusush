#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== WHILE LOOP TEST ==="
echo

echo "Testing while loop with assignment (the original problem):"
i=1
echo "Starting with i=$i"

while test "$i" -le 2; do
    echo "Iteration $i"
    i=$((i + 1))
    echo "Updated i to $i"
done

echo "After while: i=$i"
echo
echo "Expected: i should be 3"
