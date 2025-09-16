#!/bin/bash

# Test persistent debugging across commands
echo "=== Testing Persistent Debugging ==="

echo "Step 1: Enable debug mode"
./builddir/lusush -c 'debug on'

echo
echo "Step 2: Set breakpoint and verify persistence"
./builddir/lusush -c 'debug on; debug break add simple_test.sh 5; debug break list'

echo
echo "Step 3: Run script with existing breakpoint (should trigger)"
./builddir/lusush -c 'debug on; debug break add simple_test.sh 5; source simple_test.sh'

echo
echo "Step 4: Test variable inspection persistence"
./builddir/lusush -c 'x=42; y=100; debug on; debug print x; debug print y; debug vars' | head -10

echo
echo "Step 5: Test function debugging"
./builddir/lusush -c 'myfunc() { echo "Inside function"; return 0; }; debug on; debug functions; myfunc'

echo
echo "=== Persistent Debugging Test Complete ==="
