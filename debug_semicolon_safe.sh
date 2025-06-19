#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== SEMICOLON PARSING INVESTIGATION ==="
echo

echo "Test 1: Semicolon outside control structure (should work)"
var1=ONE; var2=TWO
echo "var1=$var1 var2=$var2"

echo
echo "Test 2: Single command in for loop (known to work)"
for i in TEST; do
works=SUCCESS
done
echo "works=$works"

echo  
echo "Test 3: Newline separated in for loop (should work)"
for i in TEST; do
newline1=FIRST
newline2=SECOND  
done
echo "newline1=$newline1 newline2=$newline2"

echo
echo "Test 4: Mixed newline and simple commands"
for i in TEST; do
echo "Inside loop"
mixed=MIXED
done
echo "mixed=$mixed"

echo
echo "=== SAFE SEMICOLON INVESTIGATION COMPLETE ==="
