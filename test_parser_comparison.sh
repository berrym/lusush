#!/bin/bash

# Comparison test between old and new parsers
echo "=== PARSER COMPARISON TEST ==="

tests=(
    "echo hello"
    "echo hello world"
    "echo hello | cat"
    "true && echo success"
    "false || echo fallback"
    "if true; then echo if works; fi"
    "while false; do echo never; done"
    "for i in a b c; do echo \$i; done"
)

for test in "${tests[@]}"; do
    echo ""
    echo "Testing: $test"
    echo "  OLD PARSER:"
    echo "$test" | ~/Lab/c/lusush/builddir/lusush 2>&1 | sed 's/^/    /'
    echo "  NEW PARSER:"
    LUSUSH_NEW_PARSER=1 echo "$test" | ~/Lab/c/lusush/builddir/lusush 2>&1 | sed 's/^/    /'
done

echo ""
echo "=== COMPARISON COMPLETE ==="
