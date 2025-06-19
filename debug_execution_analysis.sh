#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== DETAILED EXECUTION ANALYSIS ==="
echo

echo "1. Global assignment (should work):"
global_test=GLOBAL
echo "global_test=$global_test"
echo

echo "2. If statement assignment (working now?):"
if true; then
    if_test=IF_SUCCESS
    echo "Inside if: if_test=$if_test"
fi
echo "Outside if: if_test=$if_test"
echo

echo "3. For loop assignment (still broken?):"
echo "Loop var check first:"
for val in TESTVAL; do
    echo "Loop var val=$val"
done
echo "After for: val=$val"
echo

echo "Assignment in for:"
for val in TESTVAL; do
    for_test=FOR_SUCCESS
    echo "Inside for: for_test=$for_test"  
done
echo "Outside for: for_test=$for_test"
