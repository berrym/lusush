#!/bin/bash

# Simple POSIX builtin test for debugging
SHELL_PATH="./builddir/lusush"

echo "Testing POSIX builtins in lusush..."
echo "Shell: $SHELL_PATH"
echo

# Test builtin existence
test_builtin() {
    local name="$1"
    local result
    result=$($SHELL_PATH -c "type $name" 2>/dev/null)
    local exit_code=$?

    if [[ $exit_code -eq 0 ]]; then
        echo "✓ $name: $result"
        return 0
    else
        echo "✗ $name: NOT FOUND (exit code: $exit_code)"
        return 1
    fi
}

echo "=== POSIX Required Builtins ==="
test_builtin ":"
test_builtin "."
test_builtin "break"
test_builtin "continue"
test_builtin "cd"
test_builtin "echo"
test_builtin "eval"
test_builtin "exec"
test_builtin "exit"
test_builtin "export"
test_builtin "false"
test_builtin "getopts"
test_builtin "hash"
test_builtin "pwd"
test_builtin "readonly"
test_builtin "return"
test_builtin "set"
test_builtin "shift"
test_builtin "test"
test_builtin "["
test_builtin "times"
test_builtin "trap"
test_builtin "true"
test_builtin "type"
test_builtin "ulimit"
test_builtin "umask"
test_builtin "unset"
test_builtin "wait"

echo
echo "=== Quick Functionality Tests ==="

# Test basic functionality
echo "Testing basic functionality:"

# Echo test
result=$($SHELL_PATH -c "echo hello world" 2>/dev/null)
if [[ "$result" == "hello world" ]]; then
    echo "✓ echo: works correctly"
else
    echo "✗ echo: failed (got: '$result')"
fi

# PWD test
result=$($SHELL_PATH -c "pwd" 2>/dev/null)
if [[ -n "$result" && -d "$result" ]]; then
    echo "✓ pwd: works correctly"
else
    echo "✗ pwd: failed (got: '$result')"
fi

# CD test
result=$($SHELL_PATH -c "cd /tmp && pwd" 2>/dev/null)
if [[ "$result" == "/tmp" ]]; then
    echo "✓ cd: works correctly"
else
    echo "✗ cd: failed (got: '$result')"
fi

# True/False test
$SHELL_PATH -c "true" 2>/dev/null
if [[ $? -eq 0 ]]; then
    echo "✓ true: returns 0"
else
    echo "✗ true: failed"
fi

$SHELL_PATH -c "false" 2>/dev/null
if [[ $? -eq 1 ]]; then
    echo "✓ false: returns 1"
else
    echo "✗ false: failed"
fi

# Export test
result=$($SHELL_PATH -c "export TEST_VAR=value; echo \$TEST_VAR" 2>/dev/null)
if [[ "$result" == "value" ]]; then
    echo "✓ export: works correctly"
else
    echo "✗ export: failed (got: '$result')"
fi

echo
echo "=== Missing Builtins Check ==="

missing_count=0
missing_builtins=()

for builtin in readonly hash; do
    if ! $SHELL_PATH -c "type $builtin" >/dev/null 2>&1; then
        echo "✗ MISSING: $builtin"
        missing_builtins+=("$builtin")
        ((missing_count++))
    else
        echo "✓ FOUND: $builtin"
    fi
done

echo
if [[ $missing_count -eq 0 ]]; then
    echo "✓ All critical POSIX builtins are implemented!"
else
    echo "⚠ $missing_count POSIX builtin(s) are missing:"
    for builtin in "${missing_builtins[@]}"; do
        echo "  - $builtin"
    done

    echo
    echo "Implementation recommendations:"
    echo "- readonly: Create read-only variables (POSIX required)"
    echo "- hash: Command path hashing for performance (POSIX required)"
fi

echo
echo "Test completed."
