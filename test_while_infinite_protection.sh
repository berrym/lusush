#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== TESTING WHILE LOOP INFINITE LOOP PROTECTION ==="
echo ""

echo "Test 1: Intentional infinite loop (should terminate safely)"
echo "Running: while true; do echo 'infinite'; done"
while true; do 
    echo "infinite loop iteration"
done

echo ""
echo "Test 2: Loop that should terminate normally"
echo "Running: i=1; while test \$i -le 3; do echo \$i; i=\$((i+1)); done"
i=1
while test $i -le 3; do
    echo "Loop iteration: $i"
    i=$((i+1))
done

echo ""
echo "=== PROTECTION TESTS COMPLETE ==="
