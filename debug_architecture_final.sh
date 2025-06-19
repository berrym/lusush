#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== FINAL ARCHITECTURE TEST ==="
echo

echo "Understanding exactly what happens:"

echo
echo "Test: For loop with semicolon-separated commands"
for val in TEST; do
    multi1=FIRST; multi2=SECOND; echo "Values: multi1=$multi1 multi2=$multi2"
done  
echo "After for: multi1=$multi1 multi2=$multi2"

echo
echo "Test: For loop with newline-separated commands" 
for val in TEST; do
    sep1=FIRST
    sep2=SECOND  
    echo "Values: sep1=$sep1 sep2=$sep2"
done
echo "After for: sep1=$sep1 sep2=$sep2"

echo
echo "Test: If with newline-separated commands"
if true; then
    if1=FIRST
    if2=SECOND
    echo "Values: if1=$if1 if2=$if2"  
fi
echo "After if: if1=$if1 if2=$if2"

echo "=== ARCHITECTURE TEST COMPLETE ==="
