#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== FOCUSED PARSING TEST ==="
echo

echo "Test 1: Top-level commands (parsed by main parser)"
echo "Two assignments with newline:"
first_top=TOP1
second_top=TOP2
echo "first_top=$first_top second_top=$second_top"

echo  
echo "Test 2: Top-level commands with semicolon:"
echo "Two assignments with semicolon:"
third_top=TOP3; fourth_top=TOP4
echo "third_top=$third_top fourth_top=$fourth_top"

echo
echo "=== FOCUSED TEST COMPLETE ==="
