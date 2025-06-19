#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== FOR LOOP VARIABLE CHECK ==="

echo "Before for loop:"
echo "check_a=[$check_a] check_b=[$check_b]"

for item in single; do
    check_a=WORKS_A
    check_b=WORKS_B
done

echo "After for loop:"
echo "check_a=[$check_a] check_b=[$check_b]"
