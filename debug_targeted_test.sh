#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== TARGETED CONTROL STRUCTURE TEST ==="
echo

echo "Test 1: Simple for loop with assignment"
for val in ONE; do
    simple_var=ASSIGNED_IN_FOR
done
echo "After for loop: simple_var=$simple_var"
echo

echo "Test 2: If statement with assignment"  
if true; then
    if_var=ASSIGNED_IN_IF
fi
echo "After if statement: if_var=$if_var"
echo

echo "Test 3: For loop with multiple commands"
for num in 42; do
    multi_var=FIRST_ASSIGNMENT
    echo "Inside for: multi_var=$multi_var"
done
echo "After for loop: multi_var=$multi_var"
echo

echo "Test 4: If statement with multiple commands"
if true; then
    if_multi=FIRST_IF_ASSIGNMENT
    echo "Inside if: if_multi=$if_multi"  
fi
echo "After if statement: if_multi=$if_multi"

echo "=== TEST COMPLETE ==="
