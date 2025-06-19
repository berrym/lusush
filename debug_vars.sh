#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== DEBUGGING VARIABLE ASSIGNMENTS ==="

echo "Test 1: Checking if variables are set in if statement"
if true; then
    debug_a=ALPHA
    debug_b=BETA
fi
echo "After if: debug_a=[$debug_a] debug_b=[$debug_b]"

echo
echo "Test 2: Checking variables after if"
echo "Check debug_a: [$debug_a]"
echo "Check debug_b: [$debug_b]"
