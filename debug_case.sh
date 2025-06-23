#!/bin/bash

# Debug script for case statement issues

echo "=== CASE STATEMENT DEBUG ==="
echo "Shell: ./builddir/lusush"
echo

# Test 1: Simple exact match (should work)
echo "Test 1: Simple exact match"
echo "Command: case hello in hello) echo match ;; esac"
./builddir/lusush -c 'case hello in hello) echo match ;; esac'
echo "Exit code: $?"
echo

# Test 2: Second pattern (failing)
echo "Test 2: Second pattern match"
echo "Command: case test in hello) echo no ;; test) echo yes ;; esac"
./builddir/lusush -c 'case test in hello) echo no ;; test) echo yes ;; esac'
echo "Exit code: $?"
echo

# Test 3: Wildcard pattern (failing)
echo "Test 3: Wildcard pattern"
echo "Command: case hello in h*) echo wildcard ;; esac"
./builddir/lusush -c 'case hello in h*) echo wildcard ;; esac'
echo "Exit code: $?"
echo

# Test 4: Variable expansion (failing)
echo "Test 4: Variable in test word"
echo "Command: var=hello; case \$var in hello) echo variable ;; esac"
./builddir/lusush -c 'var=hello; case $var in hello) echo variable ;; esac'
echo "Exit code: $?"
echo

# Test 5: Debug output with verbose mode
echo "Test 5: Debug output for second pattern"
echo "Command with debug: case test in hello) echo no ;; test) echo yes ;; esac"
echo "Enabling debug output..."
DEBUG=1 ./builddir/lusush -c 'case test in hello) echo no ;; test) echo yes ;; esac'
echo "Exit code: $?"
echo

echo "=== END DEBUG ==="
