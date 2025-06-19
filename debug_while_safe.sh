#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== WHILE LOOP SIMULATION (NO ACTUAL WHILE LOOPS) ==="
echo

echo "Testing while loop assignment behavior by simulating iterations:"
echo

echo "Simulation 1: Manual while loop logic"
echo "Initial: i=0"
i=0
echo "i=$i"

echo "First iteration simulation:"
if test "$i" -le 2; then
    echo "  Inside simulated while: i=$i"
    echo "  Setting test_var=WHILE_VALUE"
    test_var=WHILE_VALUE
    echo "  Inside simulated while: test_var=$test_var"
    echo "  Incrementing: i=\$((i + 1))"
    i=$((i + 1))
    echo "  After increment: i=$i"
fi

echo "After simulated while iteration:"
echo "  i=$i"
echo "  test_var=$test_var"

echo
echo "Simulation 2: Test if while loop structure parsing works"
echo "Let's see how while loop syntax is tokenized (but not executed):"

echo "This would be the structure:"
echo "while test \"\$i\" -le 1; do"
echo "    echo \"iteration \$i\""
echo "    i=\$((i + 1))"
echo "done"

echo
echo "=== SIMULATION COMPLETE (NO INFINITE LOOPS RISKED) ==="
