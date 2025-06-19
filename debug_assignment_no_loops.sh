#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== ASSIGNMENT DEBUG - NO LOOPS ==="
echo

echo "1. Testing assignment outside any control structure:"
echo "Setting x=1"
x=1
echo "x is now: $x"
echo

echo "2. Testing assignment in simple if statement:"
echo "Setting y=2 inside if"
if true; then
    y=2
    echo "Inside if: y=$y"
fi
echo "Outside if: y=$y"
echo

echo "3. Testing assignment in for loop:"
echo "Setting z inside for loop"
for i in 1; do
    z=3
    echo "Inside for: z=$z"
done
echo "Outside for: z=$z"
echo

echo "4. Testing arithmetic assignment:"
echo "Setting w=\$((5+5))"
w=$((5+5))
echo "w is now: $w"
echo

echo "5. Testing what happens to variables in while context (NO LOOP):"
echo "Just parsing the while structure without execution"
# Let's see if we can trace the parsing without infinite loops
echo "i=0" 
i=0
echo "i is: $i"
echo
echo "Now let's try a single while iteration simulation:"
echo "if test \"\$i\" -le 0; then i=\$((i + 1)); fi"
if test "$i" -le 0; then 
    i=$((i + 1))
fi
echo "After if simulation: i=$i"

echo
echo "=== ASSIGNMENT DEBUG COMPLETE ==="
