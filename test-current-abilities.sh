# Test script for current lusush capabilities with comment support
# All features listed here work correctly

echo "=== LUSUSH CURRENT CAPABILITIES TEST ==="

# Basic echo functionality
echo "1. Basic echo works"

# Variable assignment and expansion  
TEST_VAR="Hello World"
echo "2. Variable assignment: $TEST_VAR"

# Command substitution with $() syntax
CURRENT_DATE=$(date '+%Y-%m-%d')
echo "3. Command substitution: Today is $CURRENT_DATE"

# Arithmetic expansion
A=10
B=5
echo "4. Arithmetic: $A + $B = $((A + B))"
echo "   Arithmetic: $A * $B = $((A * B))"

# Globbing test
echo "5. Globbing test - files:"
echo *

# Mixed operators (our main achievement)
echo "6. Mixed operators test:"
echo "success" | grep "success" && echo "   Pipeline + AND works!"
echo "fail" | grep "success" || echo "   Pipeline + OR works!"

# Comment support (now working!)
echo "7. Comment support: Working!" # This is an inline comment

echo "=== TEST COMPLETED ==="
