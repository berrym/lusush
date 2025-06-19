#!/bin/bash
# Test script for parameter expansion features

echo "=== PARAMETER EXPANSION TESTS ==="

# Test ${var=value} - assign if unset
echo "1. Assignment if unset:"
echo "  ${unset1=default1} (should assign and return 'default1')"
echo "  $unset1 (should be 'default1')"

# Test ${var:-default} - use default if unset
echo "2. Default if unset:"
echo "  ${unset2:-fallback2} (should return 'fallback2' without assignment)"
echo "  $unset2 (should be empty)"

# Test ${var:=value} - assign if unset or empty
echo "3. Assignment if unset or empty:"
echo "  ${unset3:=assigned3} (should assign and return 'assigned3')"
echo "  $unset3 (should be 'assigned3')"

# Test ${var:+alternate} - use alternate if set and non-empty
echo "4. Alternate if set:"
SETVAR="exists"
echo "  ${SETVAR:+alternate_text} (should return 'alternate_text')"
echo "  ${unset4:+alternate_text} (should return empty)"

# Test ${var-default} vs ${var:-default}
echo "5. Difference between - and :-:"
EMPTY=""
echo "  Empty var with -: ${EMPTY-default} (should be empty)"
echo "  Empty var with :-: ${EMPTY:-default} (should be 'default')"

echo "=== TESTS COMPLETE ==="
