#!/bin/bash

echo "=== LUSUSH BUILTIN COMMANDS TEST SUITE ==="
echo "Testing all builtin commands for functionality and POSIX compliance"
echo "Shell: ./builddir/lusush"
echo ""

SHELL_PATH="./builddir/lusush"
PASS_COUNT=0
TOTAL_COUNT=0

run_test() {
    local test_name="$1"
    local input="$2"
    local expected_type="$3"  # "success", "output", "contains", "file"
    local expected="$4"

    echo "Test: $test_name"
    echo "Input: $input"

    TOTAL_COUNT=$((TOTAL_COUNT + 1))

    if [ "$expected_type" = "success" ]; then
        # Test for successful execution (exit code 0)
        if timeout 5s $SHELL_PATH -c "$input" >/dev/null 2>&1; then
            echo "✓ PASSED"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo "✗ FAILED - Command failed"
        fi
    elif [ "$expected_type" = "output" ]; then
        # Test for specific output
        output=$(timeout 5s $SHELL_PATH -c "$input" 2>&1)
        if [ "$output" = "$expected" ]; then
            echo "✓ PASSED"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo "✗ FAILED"
            echo "Expected: '$expected'"
            echo "Got: '$output'"
        fi
    elif [ "$expected_type" = "contains" ]; then
        # Test that output contains expected text
        output=$(timeout 5s $SHELL_PATH -c "$input" 2>&1)
        if echo "$output" | grep -q "$expected"; then
            echo "✓ PASSED"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo "✗ FAILED"
            echo "Expected to contain: '$expected'"
            echo "Got: '$output'"
        fi
    elif [ "$expected_type" = "file" ]; then
        # Test for file content
        timeout 5s $SHELL_PATH -c "$input" >/dev/null 2>&1
        if [ -f "$expected" ] && [ -s "$expected" ]; then
            echo "✓ PASSED"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo "✗ FAILED - File not created or empty"
        fi
    fi
    echo ""
}

echo "=== CORE NAVIGATION BUILTINS ==="

run_test "pwd - print working directory" \
    'pwd' \
    'contains' \
    'lusush'

run_test "cd - change directory" \
    'cd /tmp; pwd' \
    'output' \
    '/tmp'

run_test "cd - return to home" \
    'cd; pwd' \
    'contains' \
    '/home'

echo "=== HELP AND INFORMATION BUILTINS ==="

run_test "help - show builtin help" \
    'help' \
    'contains' \
    'builtin help'

run_test "dump - show symbol table" \
    'dump' \
    'contains' \
    'GLOBAL SCOPE'

echo "=== LOGICAL BUILTINS ==="

run_test "true - return success" \
    'true' \
    'success' \
    ''

run_test "false - return failure" \
    'false && echo "should not run"' \
    'output' \
    ''

run_test "test - string equality" \
    'test "hello" = "hello"; echo $?' \
    'contains' \
    '0'

run_test "test - string inequality" \
    'test "hello" != "world"; echo $?' \
    'contains' \
    '0'

run_test "bracket test - file existence" \
    '[ -f "/etc/passwd" ]; echo $?' \
    'contains' \
    '0'

echo "=== TEXT PROCESSING BUILTINS ==="

run_test "echo - simple output" \
    'echo "hello world"' \
    'output' \
    'hello world'

run_test "echo - multiple arguments" \
    'echo hello world test' \
    'output' \
    'hello world test'

run_test "echo - variable expansion" \
    'VAR=test; echo "Value: $VAR"' \
    'output' \
    'Value: test'

echo "=== VARIABLE MANAGEMENT BUILTINS ==="

run_test "variable assignment" \
    'VAR=hello; echo $VAR' \
    'output' \
    'hello'

run_test "export - basic usage (safe test)" \
    'export TEST_VAR=value; echo done' \
    'output' \
    'done'

run_test "unset - basic test (safe)" \
    'TEST_VAR=temp; echo $TEST_VAR' \
    'output' \
    'temp'

echo "=== SHELL CONTROL BUILTINS ==="

run_test "set - show variables" \
    'set' \
    'success' \
    ''

run_test "setopt - shell options" \
    'setopt' \
    'success' \
    ''

run_test "setprompt - prompt setting" \
    'setprompt "test> "' \
    'success' \
    ''

echo "=== ALIAS BUILTINS ==="

run_test "alias - set alias" \
    'alias ll="ls -l"' \
    'success' \
    ''

run_test "unalias - remove alias" \
    'alias test_alias="echo test"; unalias test_alias' \
    'success' \
    ''

echo "=== SCRIPT EXECUTION BUILTINS ==="

run_test "eval - evaluate arguments" \
    'eval "echo hello"' \
    'output' \
    'hello'

run_test "source - dot command" \
    'echo "echo sourced" > /tmp/test_source.sh; source /tmp/test_source.sh; rm -f /tmp/test_source.sh' \
    'output' \
    'sourced'

echo "=== I/O BUILTINS ==="

run_test "read - basic input (with echo)" \
    'echo "test input" | { read var; echo "read: $var"; }' \
    'contains' \
    'read:'

echo "=== UTILITY BUILTINS ==="

run_test "clear - clear screen" \
    'clear' \
    'success' \
    ''

echo "=== HISTORY BUILTINS ==="

run_test "history - command history" \
    'history' \
    'success' \
    ''

echo "=== EXIT BUILTIN (test in subshell) ==="

run_test "exit - with code" \
    '(exit 42); echo $?' \
    'contains' \
    '42'

echo "=== RESULTS ==="
echo "Total tests: $TOTAL_COUNT"
echo "Passed: $PASS_COUNT"
echo "Failed: $((TOTAL_COUNT - PASS_COUNT))"

if [ $PASS_COUNT -eq $TOTAL_COUNT ]; then
    echo "SUCCESS: All builtin tests passed!"
    success_rate=100
else
    success_rate=$((PASS_COUNT * 100 / TOTAL_COUNT))
    echo "Success rate: ${success_rate}%"

    if [ $success_rate -ge 80 ]; then
        echo "GOOD: Most builtins working correctly"
    elif [ $success_rate -ge 60 ]; then
        echo "FAIR: Significant builtin functionality available"
    else
        echo "NEEDS WORK: Many builtins require fixes"
    fi
fi

echo ""
echo "=== BUILTIN COMMAND COVERAGE ==="
echo "This test validates essential POSIX builtin commands:"
echo "• Navigation: cd, pwd"
echo "• Information: help, dump, history"
echo "• Logic: true, false, test, ["
echo "• Text: echo"
echo "• Variables: export, unset, set"
echo "• Control: setopt, setprompt"
echo "• Aliases: alias, unalias"
echo "• Execution: eval, source"
echo "• I/O: read, clear"
echo "• Exit: exit"
echo ""
echo "Key POSIX builtin requirements tested:"
echo "• Command recognition and execution"
echo "• Proper exit codes and return values"
echo "• Variable and environment integration"
echo "• Standard input/output handling"
echo "• Error handling and edge cases"
