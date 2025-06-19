# Test script for current lusush capabilities with comment support
# All features listed here work correctly

echo "=== LUSUSH CURRENT CAPABILITIES TEST ==="

# Basic echo functionality
echo "1. Basic echo works"

# Variable assignment and expansion  
TEST_VAR="Hello World"
echo "2. Variable assignment: $TEST_VAR"

# Parameter expansion (NEW FEATURE!)
echo "3. Parameter expansion:"
echo "   ${unset1=default} -> sets and returns 'default'"
echo "   unset1 is now: $unset1"
echo "   ${unset2:-fallback} -> returns 'fallback' without setting"
echo "   ${unset3:=assigned} -> sets and returns 'assigned'"
echo "   unset3 is now: $unset3"

# Command substitution with $() syntax
CURRENT_DATE=$(date '+%Y-%m-%d')
echo "4. Command substitution: Today is $CURRENT_DATE"

# Arithmetic expansion
A=10
B=5
echo "5. Arithmetic: $A + $B = $((A + B))"
echo "   Arithmetic: $A * $B = $((A * B))"

# Globbing test
echo "6. Globbing test - files:"
echo *

# Mixed operators (our main achievement)
echo "7. Mixed operators test:"
echo "success" | grep "success" && echo "   Pipeline + AND works!"
echo "fail" | grep "success" || echo "   Pipeline + OR works!"

# Comment support (now working!)
echo "8. Comment support: Working!" # This is an inline comment

echo "=== TEST COMPLETED ==="
