#!/bin/bash

# Focused debug test for command substitution edge cases in lusush shell
# This test specifically targets the failing cases identified in diagnosis

echo "=== COMMAND SUBSTITUTION DEBUG TEST ==="
echo "Testing specific failing cases to diagnose root causes"
echo "Shell: ./builddir/lusush"
echo

# Add debug wrapper function
debug_test() {
    local description="$1"
    local test_command="$2"
    local expected="$3"

    echo -n "$description: "

    # Run the test and capture both stdout and stderr
    result=$(echo "$test_command" | ./builddir/lusush 2>&1)
    exit_code=$?

    if [ "$result" = "$expected" ]; then
        echo "✅ WORKING ($result)"
    else
        echo "❌ FAILED"
        echo "  Command: $test_command"
        echo "  Expected: '$expected'"
        echo "  Got: '$result'"
        echo "  Exit code: $exit_code"
        echo
    fi
}

# Test the working cases first (baseline)
echo "=== BASELINE WORKING CASES ==="
debug_test "Basic command substitution" 'echo $(echo hello)' "hello"
debug_test "Command substitution in quotes" 'echo "Result: $(echo test)"' "Result: test"
debug_test "Command substitution assignment" 'x=$(echo assigned); echo $x' "assigned"

echo

# Test the failing cases
echo "=== FAILING CASES ANALYSIS ==="

# Case 1: Direct invocation (no echo wrapper)
echo -n "Direct command substitution: "
result=$(echo '$(echo direct)' | ./builddir/lusush 2>&1)
echo "RESULT: '$result' (should be 'direct')"

# Case 2: Complex nesting
echo -n "Complex nesting: "
result=$(echo 'echo "$(echo "$(echo inner)")"' | ./builddir/lusush 2>&1)
echo "RESULT: '$result' (should be 'inner')"

# Case 3: Command substitution with pipes
echo -n "Command substitution with pipes: "
result=$(echo 'echo "Count: $(echo "a b c" | wc -w)"' | ./builddir/lusush 2>&1)
echo "RESULT: '$result' (should be 'Count: 3' or similar)"

# Case 4: Variable expansion in command substitution
echo -n "Variable expansion in command substitution: "
result=$(echo 'cmd=echo; echo $(${cmd} test)' | ./builddir/lusush 2>&1)
echo "RESULT: '$result' (should be 'test')"

echo

# Test incremental complexity to find the breaking point
echo "=== INCREMENTAL COMPLEXITY ANALYSIS ==="

debug_test "Simple nested quotes" 'echo "$(echo simple)"' "simple"
debug_test "Double nested" 'echo $(echo $(echo double))' "double"
debug_test "Triple nested" 'echo $(echo $(echo $(echo triple)))' "triple"

echo

# Test different command patterns
echo "=== COMMAND PATTERN ANALYSIS ==="

debug_test "Command sub with cat" 'echo $(echo hello | cat)' "hello"
debug_test "Command sub with grep" 'echo $(echo hello | grep hello)' "hello"
debug_test "Command sub with wc" 'echo test | wc -w' "1"
debug_test "Command sub wc in substitution" 'echo $(echo test | wc -w)' "1"

echo

# Test quote handling in command substitution
echo "=== QUOTE HANDLING ANALYSIS ==="

debug_test "Single quotes in cmd sub" 'echo $(echo '\''single'\'')' "single"
debug_test "Double quotes in cmd sub" 'echo $(echo "double")' "double"
debug_test "Mixed quotes in cmd sub" 'echo "$(echo '\''mixed'\'')"' "mixed"

echo

# Test parsing vs execution issues
echo "=== PARSING VS EXECUTION ANALYSIS ==="

# Test if the issue is parsing or execution
echo -n "Parser test - tokenization: "
echo 'echo "Debug tokenization: $(echo test)"' | ./builddir/lusush 2>&1 | head -1

echo -n "Execution test - simple command: "
echo 'echo "Debug execution: $(date +%s)"' | ./builddir/lusush 2>&1 | head -1

echo

echo "=== ROOT CAUSE ANALYSIS ==="
echo "Based on the results above:"
echo "1. If basic cases work but complex ones fail → parsing issue"
echo "2. If direct invocation fails but echo works → context issue"
echo "3. If pipes fail → command execution issue"
echo "4. If variables fail → expansion order issue"
echo
echo "This analysis will help identify the specific component to fix:"
echo "- Tokenizer: If quotes and nesting are broken"
echo "- Parser: If command structure recognition fails"
echo "- Executor: If command execution pipeline fails"
echo "- Expansion: If variable/substitution order is wrong"

echo
echo "=== DEBUG TEST COMPLETE ==="
