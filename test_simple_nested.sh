#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== SIMPLE NESTED TEST ==="
echo

echo "Test: Simple nested if"
if true; then
    outer=OUTER
    echo "Before nested if"
    if true; then
        inner=INNER
    fi
    echo "After nested if"
    after=AFTER
fi
echo "Results: outer=$outer inner=$inner after=$after"

echo
echo "=== SIMPLE NESTED TEST COMPLETE ==="
