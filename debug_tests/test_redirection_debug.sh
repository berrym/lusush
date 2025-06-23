#!/bin/bash

# Debug test for redirection issues

SHELL_PATH="./builddir/lusush"

echo "=== REDIRECTION DEBUG TEST ==="
echo "Testing specific redirection patterns to identify issues"
echo

# Test 1: Basic stderr redirection (should work)
echo "Test 1: Basic stderr redirection"
echo "Command: ls nonexistent 2>/tmp/test1.txt"
echo 'ls nonexistent 2>/tmp/test1.txt' | $SHELL_PATH
if [ -f /tmp/test1.txt ]; then
    echo "File created with content:"
    cat /tmp/test1.txt
    rm -f /tmp/test1.txt
else
    echo "File not created"
fi
echo

# Test 2: stdout to stderr redirection (>&2)
echo "Test 2: stdout to stderr redirection (>&2)"
echo "Command: echo 'to stderr' >&2"
echo 'echo "to stderr" >&2' | $SHELL_PATH 2>/tmp/test2.txt
if [ -f /tmp/test2.txt ]; then
    echo "File created with content:"
    cat /tmp/test2.txt
    rm -f /tmp/test2.txt
else
    echo "File not created"
fi
echo

# Test 3: Combined >&2 and 2> redirection
echo "Test 3: Combined >&2 and 2> redirection"
echo "Command: echo 'error' >&2 2>/tmp/test3.txt"
echo 'echo "error" >&2 2>/tmp/test3.txt' | $SHELL_PATH
if [ -f /tmp/test3.txt ]; then
    echo "File created with content:"
    cat /tmp/test3.txt
    rm -f /tmp/test3.txt
else
    echo "File not created"
fi
echo

# Test 4: stderr to stdout redirection (2>&1)
echo "Test 4: stderr to stdout redirection (2>&1)"
echo "Command: ls nonexistent 2>&1"
echo 'ls nonexistent 2>&1' | $SHELL_PATH >/tmp/test4.txt
if [ -f /tmp/test4.txt ]; then
    echo "File created with content:"
    cat /tmp/test4.txt
    rm -f /tmp/test4.txt
else
    echo "File not created"
fi
echo

# Test 5: Combined stdout/stderr redirection (&>)
echo "Test 5: Combined stdout/stderr redirection (&>)"
echo "Command: (echo 'stdout'; echo 'stderr' >&2) &>/tmp/test5.txt"
echo '(echo "stdout"; echo "stderr" >&2) &>/tmp/test5.txt' | $SHELL_PATH
if [ -f /tmp/test5.txt ]; then
    echo "File created with content:"
    cat /tmp/test5.txt
    rm -f /tmp/test5.txt
else
    echo "File not created"
fi
echo

# Test 6: Here string with variable expansion
echo "Test 6: Here string with variable expansion"
echo "Command: msg='hello'; cat <<<\"Message: \$msg\""
echo 'msg="hello"; cat <<<"Message: $msg"' | $SHELL_PATH
echo

# Test 7: Error suppression with 2>/dev/null
echo "Test 7: Error suppression with 2>/dev/null"
echo "Command: ls nonexistent 2>/dev/null || echo 'suppressed'"
echo 'ls nonexistent 2>/dev/null || echo "suppressed"' | $SHELL_PATH
echo

echo "=== TOKEN PARSING DEBUG ==="
echo "Testing what tokens are generated for complex redirections:"

# Simple debug output if debug mode is available
echo "For: echo test >&2"
echo 'echo test >&2' | $SHELL_PATH 2>&1

echo "For: echo test 2>&1"
echo 'echo test 2>&1' | $SHELL_PATH 2>&1

echo "=== END DEBUG TEST ==="
