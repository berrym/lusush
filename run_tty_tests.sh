#!/bin/bash
# Run TTY-dependent tests for manual verification

echo "==================================================="
echo "LLE Phase 1 TTY-Dependent Test Runner"
echo "==================================================="
echo ""

cd /home/mberry/Lab/c/lusush

echo "Test 1: editor_test"
echo "-------------------"
./builddir/src/lle/foundation/editor_test
echo ""

echo "Test 2: editor_advanced_test_v2"
echo "--------------------------------"
./builddir/src/lle/foundation/editor_advanced_test_v2
echo ""

echo "==================================================="
echo "Manual TTY tests complete"
echo "==================================================="
