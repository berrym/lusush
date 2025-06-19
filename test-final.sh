#!/bin/bash
# Simplified comprehensive test

echo "=== LUSUSH COMPREHENSIVE TEST ==="

echo "1. Parameter expansion:"
echo "   ${var1=default_value}"
echo "   ${var2:-fallback}"
echo "   var1 is now: $var1"

echo "2. Command substitution:"
echo "   Modern: $(date +'%Y-%m-%d')"
TIME_VAR=`date +'%H:%M'`
echo "   Legacy: $TIME_VAR"

echo "3. Arithmetic:"
A=10
B=3
echo "   $A + $B = $((A + B))"

echo "4. Enhanced echo:"
echo "   Line1\nLine2\tTabbed"

echo "5. Mixed operators:"
echo "test" | grep "test" && echo "   Pipeline + AND works"

echo "=== TEST COMPLETE ==="
