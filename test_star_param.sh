#!/bin/bash

# Test script for $* parameter functionality in lusush shell

echo "=== Testing \$* Parameter Implementation ==="

# Test 1: Basic $* usage
echo "Test 1: Basic \$* parameter"
echo 'set -- a b c; echo "$*"' > /tmp/test_star.sh
chmod +x /tmp/test_star.sh
echo "Expected: a b c"
echo -n "Lusush:   "
./builddir/lusush /tmp/test_star.sh
echo -n "Bash:     "
bash /tmp/test_star.sh
echo ""

# Test 2: $* without quotes
echo "Test 2: \$* without quotes"
echo 'set -- a b c; echo $*' > /tmp/test_star2.sh
chmod +x /tmp/test_star2.sh
echo "Expected: a b c"
echo -n "Lusush:   "
./builddir/lusush /tmp/test_star2.sh
echo -n "Bash:     "
bash /tmp/test_star2.sh
echo ""

# Test 3: $* with different values
echo "Test 3: \$* with different values"
echo 'set -- hello world test; echo "$*"' > /tmp/test_star3.sh
chmod +x /tmp/test_star3.sh
echo "Expected: hello world test"
echo -n "Lusush:   "
./builddir/lusush /tmp/test_star3.sh
echo -n "Bash:     "
bash /tmp/test_star3.sh
echo ""

# Test 4: $* when no parameters set
echo "Test 4: \$* when no parameters"
echo 'echo "$*"' > /tmp/test_star4.sh
chmod +x /tmp/test_star4.sh
echo "Expected: (empty)"
echo -n "Lusush:   "
./builddir/lusush /tmp/test_star4.sh
echo -n "Bash:     "
bash /tmp/test_star4.sh
echo ""

# Test 5: $* with single parameter
echo "Test 5: \$* with single parameter"
echo 'set -- single; echo "$*"' > /tmp/test_star5.sh
chmod +x /tmp/test_star5.sh
echo "Expected: single"
echo -n "Lusush:   "
./builddir/lusush /tmp/test_star5.sh
echo -n "Bash:     "
bash /tmp/test_star5.sh
echo ""

# Test 6: Check if $* variable exists
echo "Test 6: Check \$* variable directly"
echo 'set -- a b c; echo "Star: ${*}"' > /tmp/test_star6.sh
chmod +x /tmp/test_star6.sh
echo "Expected: Star: a b c"
echo -n "Lusush:   "
./builddir/lusush /tmp/test_star6.sh
echo -n "Bash:     "
bash /tmp/test_star6.sh
echo ""

# Cleanup
rm -f /tmp/test_star*.sh

echo "=== \$* Parameter Tests Complete ==="
