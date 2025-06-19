#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== MULTI-COMMAND PARSING DIAGNOSTIC ==="
echo

echo "Test 1: Two assignments separated by newline in for loop"
echo "Expected: Two separate assignments"
echo "Actual result:"

for i in SINGLE; do
first=FIRST
second=SECOND
done

echo "first=$first"
echo "second=$second"

echo
echo "Test 2: Two assignments separated by semicolon in for loop"  
echo "Expected: Two separate assignments"
echo "Actual result:"

for i in SINGLE; do
third=THIRD; fourth=FOURTH
done

echo "third=$third"
echo "fourth=$fourth"

echo
echo "Test 3: Simple case that works for comparison"
for i in SINGLE; do
simple=WORKS
done

echo "simple=$simple"

echo
echo "=== DIAGNOSTIC COMPLETE ==="
