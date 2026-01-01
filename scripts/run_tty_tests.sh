#!/bin/bash
# Run TTY-required tests for manual verification
# These tests require a real interactive terminal and cannot run via meson test

echo "==================================================="
echo "LLE Phase 1 TTY-Required Test Runner"
echo "==================================================="
echo ""
echo "NOTE: These tests MUST run in a real terminal (TTY)"
echo "      They will fail if run via automation/CI"
echo ""

cd /home/mberry/Lab/c/lusush

echo "Test 1: editor_test_tty (basic editor operations)"
echo "---------------------------------------------------"
./build/src/lle/foundation/editor_test_tty
echo ""

echo "Test 2: editor_advanced_test_tty (word/line operations)"
echo "--------------------------------------------------------"
./build/src/lle/foundation/editor_advanced_test_tty
echo ""

echo "==================================================="
echo "Manual TTY tests complete"
echo "==================================================="
