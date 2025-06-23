#!/bin/bash

# Test script for quoted here document delimiter issue

echo "=== Testing quoted here document delimiter ==="

SHELL_PATH="./builddir/lusush"

echo "Test 1: Unquoted delimiter (should work)"
$SHELL_PATH << 'SCRIPT'
cat <<END
This should work fine
Variables like $HOME should expand
END
SCRIPT

echo ""
echo "Test 2: Double-quoted delimiter (should NOT expand variables)"
$SHELL_PATH << 'SCRIPT'
cat <<"END"
This should not expand
Variables like $HOME should not expand
END
SCRIPT

echo ""
echo "Test 3: Single-quoted delimiter (should NOT expand variables)"
$SHELL_PATH << 'SCRIPT'
cat <<'END'
This should not expand either
Variables like $HOME should not expand
END
SCRIPT

echo ""
echo "=== End of tests ==="
