#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== NODE TYPE DEBUG ==="
echo

echo "Testing what node types assignments become..."
echo

echo "1. Simple assignment at top level:"
echo "About to execute: w=42"
w=42
echo "w=$w"
echo

echo "2. Assignment in if statement:"  
echo "About to execute if with assignment"
if true; then
    echo "Inside if block, about to assign x=100"
    x=100
    echo "Still inside if: x=$x"
fi
echo "Outside if: x=$x"
echo

echo "3. Assignment in for loop:"
echo "About to execute for with assignment"  
for num in 7; do
    echo "Inside for block, about to assign y=200"
    y=200
    echo "Still inside for: y=$y"
done
echo "Outside for: y=$y"
echo

echo "4. Testing variable scope across different contexts:"
echo "Setting global: global_var=GLOBAL"
global_var=GLOBAL
echo "Global: global_var=$global_var"

if true; then
    echo "In if, setting if_var=IF_SCOPE"
    if_var=IF_SCOPE
    echo "In if: if_var=$if_var, global_var=$global_var"
fi
echo "After if: if_var=$if_var, global_var=$global_var"

for val in SINGLE; do
    echo "In for, setting for_var=FOR_SCOPE"  
    for_var=FOR_SCOPE
    echo "In for: for_var=$for_var, global_var=$global_var"
done
echo "After for: for_var=$for_var, global_var=$global_var"

echo
echo "=== NODE TYPE DEBUG COMPLETE ==="
