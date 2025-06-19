#!/bin/bash
# lusush Shell v0.2.1 - Core Capabilities Test

echo "=== LUSUSH v0.2.1 CORE CAPABILITIES ==="

echo "1. Basic Operations:"
MSG="Hello lusush v0.2.1"
echo "   $MSG"

echo "2. Parameter Expansion:"
echo "   ${var1=default} = assigned"
echo "   ${var2:-fallback} = fallback"
echo "   var1 is now: $var1"

echo "3. Command Substitution:"
echo "   Modern: $(date '+%Y-%m-%d')"
echo "   Legacy: `date '+%H:%M'`"

echo "4. Arithmetic:"
A=10; B=3
echo "   $A + $B = $((A + B))"

echo "5. Enhanced Echo:"
echo "   Escape sequences: Line1\nLine2"

echo "6. Mixed Operators:"
echo "test" | grep "test" && echo "   Pipeline + AND works"

echo "7. Control Flow:"
for i in 1 2; do echo "   Loop: $i"; done

echo "8. Comments:" # inline comment works

echo "=== LUSUSH v0.2.1 READY ==="
