#!/bin/bash
# Comprehensive test for new parameter expansion features

echo "=== LUSUSH PARAMETER EXPANSION TESTS ==="

# Test 1: ${var=value} - assign if unset
echo "1. ${var=value} - assign default if unset:"
echo "  Result: [${newvar1=default_value}]"
echo "  Check: newvar1=$newvar1"

# Test 2: ${var:-default} - use default if unset, don't assign
echo "2. ${var:-default} - use default without assignment:"
echo "  Result: [${newvar2:-fallback_value}]"
echo "  Check: newvar2=$newvar2 (should be empty)"

# Test 3: ${var:=value} - assign if unset or empty
echo "3. ${var:=value} - assign if unset or empty:"
echo "  Result: [${newvar3:=assigned_value}]"
echo "  Check: newvar3=$newvar3"

# Test 4: ${var:+alternate} - use alternate if set and non-empty
echo "4. ${var:+alternate} - use alternate if set:"
EXISTING="has_value"
echo "  With set var: [${EXISTING:+replacement_text}]"
echo "  With unset var: [${newvar4:+replacement_text}]"

# Test 5: ${var-default} vs ${var:-default} with empty variables
echo "5. Difference between - and :-:"
EMPTY=""
echo "  Empty with -: [${EMPTY-default}]"
echo "  Empty with :-: [${EMPTY:-default}]"

# Test 6: ${var+alternate} - use alternate if set (even if empty)
echo "6. ${var+alternate} - use alternate if set (even if empty):"
echo "  With set var: [${EXISTING+alternate}]"
echo "  With empty var: [${EMPTY+alternate}]"
echo "  With unset var: [${newvar5+alternate}]"

echo "=== TESTS COMPLETE ==="
