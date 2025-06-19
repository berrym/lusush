#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== SYMBOL TABLE TRACING ==="
echo

echo "Step 1: Test global assignment"
echo "Setting global_test=GLOBAL_VALUE"
global_test=GLOBAL_VALUE
echo "After assignment: global_test=$global_test"
echo

echo "Step 2: Test assignment inside simple command"
echo "Running: test_var=SIMPLE_VALUE; echo test_var=\$test_var"
test_var=SIMPLE_VALUE; echo "test_var=$test_var"
echo "After command: test_var=$test_var" 
echo

echo "Step 3: Test assignment inside if statement"
echo "Running if statement with assignment..."
if true; then
    echo "Inside if: setting if_test=IF_VALUE"
    if_test=IF_VALUE
    echo "Inside if: if_test=$if_test"
fi
echo "After if: if_test=$if_test"
echo

echo "Step 4: Test assignment inside for loop"
echo "Running for loop with assignment..."
for item in SINGLE_ITEM; do
    echo "Inside for: setting for_test=FOR_VALUE"
    for_test=FOR_VALUE  
    echo "Inside for: for_test=$for_test"
    echo "Inside for: checking global_test=$global_test"
done
echo "After for: for_test=$for_test"
echo "After for: checking global_test=$global_test"
echo

echo "Step 5: Test loop variable persistence"
echo "For loop variable should be: item=$item"
echo

echo "=== SYMBOL TABLE TRACING COMPLETE ==="
