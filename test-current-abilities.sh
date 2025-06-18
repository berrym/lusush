echo "=== LUSUSH CURRENT CAPABILITIES TEST ==="

echo "1. Basic echo works"

TEST_VAR="Hello World"
echo "2. Variable assignment: $TEST_VAR"

CURRENT_DATE=$(date '+%Y-%m-%d')
echo "3. Command substitution: Today is $CURRENT_DATE"

A=10
B=5
echo "4. Arithmetic: $A + $B = $((A + B))"
echo "   Arithmetic: $A * $B = $((A * B))"

echo "5. Globbing test - files:"
echo *

echo "6. Mixed operators test:"
echo "success" | grep "success" && echo "   Pipeline + AND works!"
echo "fail" | grep "success" || echo "   Pipeline + OR works!"

echo "=== TEST COMPLETED ==="
