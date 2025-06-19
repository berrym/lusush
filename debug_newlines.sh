#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== TESTING IF NEWLINES ARE PRESERVED ==="
echo

echo "Test 1: Simple newline separation"
var1=FIRST
var2=SECOND
echo "var1=$var1 var2=$var2"

echo
echo "Test 2: If statement with newlines"
if true; then
    if_var=IF_VALUE
    echo "Inside if: if_var=$if_var"
fi
echo "Outside if: if_var=$if_var"
