#!/home/mberry/Lab/c/lusush/builddir/lusush

# Test script for persistent variable state
echo "=== Testing Script Execution Mode ==="

# Test 1: Simple variable assignment and usage
VAR1="hello"
echo "Test 1: VAR1 = $VAR1"

# Test 2: Multiple variables
VAR2="world"
echo "Test 2: VAR1 $VAR2"

# Test 3: Arithmetic
X=5
Y=3
echo "Test 3: X=$X, Y=$Y"

# Test 4: Variable in same line (should still work)
Z=10; echo "Test 4: Z=$Z"

# Test 5: Complex variable usage
MESSAGE="Script variables work!"
echo "Test 5: $MESSAGE"

echo "=== Script Test Complete ==="
