#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== WHILE LOOP VARIABLE CHECK ==="

echo "Before while loop:"
echo "while_a=[$while_a] while_b=[$while_b]"

counter=1
while test $counter -eq 1; do
    while_a=WHILE_A
    while_b=WHILE_B
    counter=2
done

echo "After while loop:"
echo "while_a=[$while_a] while_b=[$while_b]"
