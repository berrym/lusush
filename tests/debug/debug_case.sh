#!/bin/bash

# Debug script for case statement wildcard pattern matching issue

SHELL_PATH="./builddir/lusush"

echo "=== Case Statement Debug Test ==="
echo "Testing case statement patterns to identify wildcard issue"
echo

# Test 1: Simple exact match (should work)
echo "Test 1: Simple exact match"
echo "Command: case test in test) echo match;; esac"
echo 'case test in test) echo match;; esac' | $SHELL_PATH
echo "Expected: match"
echo

# Test 2: Simple wildcard (currently failing)
echo "Test 2: Simple wildcard"
echo "Command: case test in *) echo wildcard;; esac"
echo 'case test in *) echo wildcard;; esac' | $SHELL_PATH
echo "Expected: wildcard"
echo

# Test 3: The exact failing test case
echo "Test 3: Exact failing test case"
echo "Command: case test in test) echo match;; *) echo nomatch;; esac"
echo 'case test in test) echo match;; *) echo nomatch;; esac' | $SHELL_PATH
echo "Expected: match"
echo

# Test 4: Non-matching case with wildcard fallback
echo "Test 4: Non-matching case with wildcard fallback"
echo "Command: case other in test) echo no;; *) echo wildcard;; esac"
echo 'case other in test) echo no;; *) echo wildcard;; esac' | $SHELL_PATH
echo "Expected: wildcard"
echo

# Test 5: Question mark wildcard
echo "Test 5: Question mark wildcard"
echo "Command: case a in ?) echo single;; esac"
echo 'case a in ?) echo single;; esac' | $SHELL_PATH
echo "Expected: single"
echo

# Test 6: Multiple character wildcard
echo "Test 6: Multiple character wildcard"
echo "Command: case hello in h*o) echo partial;; esac"
echo 'case hello in h*o) echo partial;; esac' | $SHELL_PATH
echo "Expected: partial"
echo

# Test 7: Multiple patterns with wildcard
echo "Test 7: Multiple patterns with wildcard"
echo "Command: case test in foo|*) echo multi;; esac"
echo 'case test in foo|*) echo multi;; esac' | $SHELL_PATH
echo "Expected: multi"
echo

# Test 8: Variable in case
echo "Test 8: Variable in case"
echo "Command: word=test; case \$word in test) echo var;; esac"
echo 'word=test; case $word in test) echo var;; esac' | $SHELL_PATH
echo "Expected: var"
echo

# Test 9: Debug pattern matching logic by testing simple patterns
echo "Test 9: Pattern variations"
echo "Command: case abc in a*) echo prefix;; esac"
echo 'case abc in a*) echo prefix;; esac' | $SHELL_PATH
echo "Expected: prefix"
echo

echo "Command: case abc in *c) echo suffix;; esac"
echo 'case abc in *c) echo suffix;; esac' | $SHELL_PATH
echo "Expected: suffix"
echo

echo "Command: case abc in *b*) echo middle;; esac"
echo 'case abc in *b*) echo middle;; esac' | $SHELL_PATH
echo "Expected: middle"
echo

# Test 10: Check if semicolons are parsed correctly
echo "Test 10: Multiple semicolons"
echo "Command: case test in test) echo one;; two) echo two;; esac"
echo 'case test in test) echo one;; two) echo two;; esac' | $SHELL_PATH
echo "Expected: one"
echo

# Test 11: Check execution order
echo "Test 11: Execution order check"
echo "Command: case test in *) echo first;; test) echo second;; esac"
echo 'case test in *) echo first;; test) echo second;; esac' | $SHELL_PATH
echo "Expected: first (should match first pattern)"
echo

echo "=== Manual pattern testing ==="
echo "If possible, test pattern matching function directly:"

# Test the basic pattern matching components that should work
echo "Testing basic cases that work vs wildcards that don't..."

echo "=== END DEBUG TEST ==="
