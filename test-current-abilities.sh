# Test script for current lusush c# Arithmetic expansion
A=10
B=5
echo "6. Arithmetic: $A + $B = $((A + B))"
echo "   Arithmetic: $A * $B = $((A * B))"
echo "   Arithmetic: $A ** 2 = $((A ** 2))"

# Globbing test
echo "7. Globbing test - files:"
echo *.md

# Mixed operators (our main achievement)
echo "8. Mixed operators test:"
echo "success" | grep "success" && echo "   Pipeline + AND works!"
echo "fail" | grep "success" || echo "   Pipeline + OR works!"

# Comment support (now working!)
echo "9. Comment support: Working!" # This is an inline commentcomment support
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

# Command substitution - both modern $() and legacy backticks
CURRENT_DATE=$(date '+%Y-%m-%d')
CURRENT_TIME=`date '+%H:%M'`
CURRENT_DAY=`date '+%A'`
echo "4. Command substitution:"
echo "   Modern syntax: Today is $CURRENT_DATE"
echo "   Legacy syntax: Current time is $CURRENT_TIME"
echo "   Mixed test: Today is $(echo $CURRENT_DAY), $CURRENT_DATE"
PWD_RESULT=`pwd`
echo "   Backtick assignment: Working directory is $PWD_RESULT"
echo "   Nested: Files in $(basename `pwd`): $(echo *.md | wc -w) markdown files"

# Enhanced echo with escape sequences (now enabled by default)
echo "5. Enhanced echo with escape sequences:"
echo "   Line1\nLine2\tTabbed\rCarriage\aBeep"

# Arithmetic expansion
A=10
B=5
echo "6. Arithmetic: $A + $B = $((A + B))"
echo "   Arithmetic: $A * $B = $((A * B))"

# Globbing test
echo "7. Globbing test - files:"
echo *

# Mixed operators (our main achievement)
echo "8. Mixed operators test:"
echo "success" | grep "success" && echo "   Pipeline + AND works!"
echo "fail" | grep "success" || echo "   Pipeline + OR works!"

# Comment support (now working!)
echo "9. Comment support: Working!" # This is an inline comment

echo "=== TEST COMPLETED ==="
