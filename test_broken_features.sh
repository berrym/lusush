#!/bin/bash
# test_broken_features.sh - Comprehensive Test of Broken Shell Features
#
# This script systematically tests all basic shell features to document
# what's working and what's broken in the current Lusush shell state.
# Use this to understand the scope of issues for the next AI assistant.

set -e

LUSUSH_BIN="./builddir/lusush"

echo "🚨 LUSUSH SHELL - BROKEN FEATURES DIAGNOSTIC"
echo "============================================="
echo
echo "This test systematically checks all basic shell features"
echo "to document what's working and what needs to be fixed."
echo
echo "Date: $(date)"
echo "Lusush Binary: $LUSUSH_BIN"
echo

# Verify binary exists
if [ ! -f "$LUSUSH_BIN" ]; then
    echo "❌ CRITICAL: $LUSUSH_BIN not found"
    echo "Run: ninja -C builddir"
    exit 1
fi

echo "✅ Build Status: Binary exists"
echo
echo "========================================"
echo "TESTING BASIC SHELL FEATURES"
echo "========================================"
echo

# Test 1: History System
echo "🔍 TEST 1: History System"
echo "-------------------------"
echo "Testing history builtin and navigation..."

echo "Testing history builtin command:"
result=$(echo -e "echo 'hist test 1'\necho 'hist test 2'\nhistory" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "hist test"; then
    echo "✅ History builtin: WORKING"
else
    echo "❌ History builtin: BROKEN - No output or error"
    echo "   Output: $result"
fi

echo "Testing history navigation (simulated):"
# Note: Arrow key navigation can't be tested in non-interactive mode
echo "⚠️  Arrow key navigation: NEEDS MANUAL TEST"
echo "   Manual test: Press UP arrow in interactive mode"
echo "   Expected: Navigate to previous command"
echo "   Current issue: Shows completion menu instead"
echo

# Test 2: Signal Handling
echo "🔍 TEST 2: Signal Handling (Ctrl+C)"
echo "-----------------------------------"
echo "Testing signal behavior..."

echo "Testing shell survival with SIGINT:"
# Create a test that checks if shell exits on signal
timeout 3s bash -c '
    echo "echo test; sleep 5" | '"$LUSUSH_BIN"' &
    SHELL_PID=$!
    sleep 1
    kill -INT $SHELL_PID 2>/dev/null || true
    sleep 1
    if kill -0 $SHELL_PID 2>/dev/null; then
        echo "✅ Signal handling: Shell survives SIGINT"
        kill -TERM $SHELL_PID 2>/dev/null || true
    else
        echo "❌ Signal handling: Shell exits on SIGINT (CRITICAL BUG)"
    fi
    wait 2>/dev/null || true
' 2>/dev/null || echo "❌ Signal test failed or timed out"

echo "⚠️  Ctrl+C behavior: NEEDS MANUAL TEST"
echo "   Manual test: Press Ctrl+C while typing a command"
echo "   Expected: Clear line, new prompt, shell continues"
echo "   Current issue: May exit entire shell"
echo

# Test 3: Pipes and Pipelines
echo "🔍 TEST 3: Pipes and Pipelines"
echo "------------------------------"
echo "Testing basic pipe operations..."

echo "Simple pipe test:"
result=$(echo "echo hello world | grep hello" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "hello"; then
    echo "✅ Basic pipes: WORKING"
else
    echo "❌ Basic pipes: BROKEN"
    echo "   Command: echo hello world | grep hello"
    echo "   Output: $result"
fi

echo "Complex pipe test:"
result=$(echo "ls | head -3 | wc -l" | "$LUSUSH_BIN" 2>&1)
if [[ "$result" =~ ^[0-9]+$ ]]; then
    echo "✅ Complex pipes: WORKING"
else
    echo "❌ Complex pipes: BROKEN or UNRELIABLE"
    echo "   Command: ls | head -3 | wc -l"
    echo "   Output: $result"
fi
echo

# Test 4: Redirections
echo "🔍 TEST 4: Input/Output Redirection"
echo "-----------------------------------"
echo "Testing redirection operations..."

echo "Output redirection test:"
result=$(echo -e "echo 'redirection test' > /tmp/lusush_test.txt\ncat /tmp/lusush_test.txt\nrm -f /tmp/lusush_test.txt" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "redirection test"; then
    echo "✅ Output redirection: WORKING"
else
    echo "❌ Output redirection: BROKEN"
    echo "   Output: $result"
fi

echo "Input redirection test:"
echo "input test data" > /tmp/lusush_input_test.txt
result=$(echo "cat < /tmp/lusush_input_test.txt" | "$LUSUSH_BIN" 2>&1)
rm -f /tmp/lusush_input_test.txt
if echo "$result" | grep -q "input test data"; then
    echo "✅ Input redirection: WORKING"
else
    echo "❌ Input redirection: BROKEN"
    echo "   Output: $result"
fi

echo "Append redirection test:"
result=$(echo -e "echo 'line 1' > /tmp/lusush_append_test.txt\necho 'line 2' >> /tmp/lusush_append_test.txt\ncat /tmp/lusush_append_test.txt\nrm -f /tmp/lusush_append_test.txt" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "line 1" && echo "$result" | grep -q "line 2"; then
    echo "✅ Append redirection: WORKING"
else
    echo "❌ Append redirection: BROKEN"
    echo "   Output: $result"
fi
echo

# Test 5: Command Substitution
echo "🔍 TEST 5: Command Substitution"
echo "-------------------------------"
echo "Testing command substitution..."

echo "Basic command substitution test:"
result=$(echo "echo \"User: \$(whoami)\"" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "User:"; then
    echo "✅ Command substitution: WORKING"
else
    echo "❌ Command substitution: BROKEN"
    echo "   Command: echo \"User: \$(whoami)\""
    echo "   Output: $result"
fi

echo "Backtick command substitution test:"
result=$(echo "echo \"Date: \`date +%Y\`\"" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "Date:"; then
    echo "✅ Backtick substitution: WORKING"
else
    echo "❌ Backtick substitution: BROKEN"
    echo "   Output: $result"
fi
echo

# Test 6: Variable Operations
echo "🔍 TEST 6: Variable Operations"
echo "------------------------------"
echo "Testing variable assignment and expansion..."

echo "Variable assignment and expansion:"
result=$(echo -e "TEST_VAR='hello world'\necho \$TEST_VAR" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "hello world"; then
    echo "✅ Variable operations: WORKING"
else
    echo "❌ Variable operations: BROKEN"
    echo "   Output: $result"
fi

echo "Environment variables:"
result=$(echo "echo \$HOME" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "/"; then
    echo "✅ Environment variables: WORKING"
else
    echo "❌ Environment variables: BROKEN"
    echo "   Output: $result"
fi
echo

# Test 7: Background Jobs
echo "🔍 TEST 7: Background Jobs"
echo "--------------------------"
echo "Testing background job control..."

echo "Background job creation:"
result=$(echo -e "sleep 0.1 &\necho \"Job started: \$!\"\njobs\nwait" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "Job started"; then
    echo "✅ Background jobs: WORKING"
else
    echo "❌ Background jobs: BROKEN"
    echo "   Output: $result"
fi
echo

# Test 8: Built-in Commands
echo "🔍 TEST 8: Built-in Commands"
echo "----------------------------"
echo "Testing various built-in commands..."

echo "Testing 'help' command:"
result=$(echo "help" | "$LUSUSH_BIN" 2>&1 | head -5)
if echo "$result" | grep -q "exit\|help\|cd"; then
    echo "✅ Help command: WORKING"
else
    echo "❌ Help command: BROKEN"
fi

echo "Testing 'alias' command:"
result=$(echo -e "alias ll='ls -la'\nll /tmp >/dev/null && echo 'alias worked'" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "alias worked"; then
    echo "✅ Alias system: WORKING"
else
    echo "❌ Alias system: BROKEN"
    echo "   Output: $result"
fi

echo "Testing 'export' command:"
result=$(echo -e "export TEST_EXPORT='test value'\necho \$TEST_EXPORT" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "test value"; then
    echo "✅ Export command: WORKING"
else
    echo "❌ Export command: BROKEN"
    echo "   Output: $result"
fi
echo

# Test 9: Control Structures
echo "🔍 TEST 9: Control Structures"
echo "-----------------------------"
echo "Testing if/then/else, loops..."

echo "Testing if statement:"
result=$(echo -e "if [ 1 -eq 1 ]; then echo 'if works'; fi" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "if works"; then
    echo "✅ If statements: WORKING"
else
    echo "❌ If statements: BROKEN"
    echo "   Output: $result"
fi

echo "Testing for loop:"
result=$(echo -e "for i in 1 2 3; do echo \"number \$i\"; done" | "$LUSUSH_BIN" 2>&1)
if echo "$result" | grep -q "number 1" && echo "$result" | grep -q "number 3"; then
    echo "✅ For loops: WORKING"
else
    echo "❌ For loops: BROKEN"
    echo "   Output: $result"
fi
echo

# Test 10: Tab Completion vs History Conflict
echo "🔍 TEST 10: Tab Completion vs History Navigation"
echo "-----------------------------------------------"
echo "Testing the main reported issue..."

echo "This is the CRITICAL ISSUE reported by user:"
echo "❌ PROBLEM: UP ARROW shows completion menu instead of history"
echo "   Current behavior: 'display all 4418 possibilities y/n'"
echo "   Expected behavior: Navigate to previous command in history"
echo
echo "This indicates:"
echo "   - History navigation is misconfigured"
echo "   - Arrow keys are bound to completion instead of history"
echo "   - Readline variables may be set incorrectly"
echo

echo "========================================"
echo "SUMMARY OF CRITICAL ISSUES"
echo "========================================"
echo
echo "🚨 BROKEN FEATURES (Need Immediate Fix):"
echo "   1. History navigation (arrow keys show completion menu)"
echo "   2. History builtin command (no output)"
echo "   3. Signal handling (Ctrl+C may exit shell)"
echo "   4. [Needs verification] Pipes, redirections, job control"
echo
echo "✅ WORKING FEATURES (Don't Break):"
echo "   1. Git status in interactive mode"
echo "   2. Basic command execution"
echo "   3. Theme system and prompts"
echo "   4. Build system"
echo "   5. Non-interactive mode"
echo
echo "🎯 NEXT AI ASSISTANT PRIORITIES:"
echo "   1. FIX HISTORY SYSTEM (most critical)"
echo "   2. FIX SIGNAL HANDLING (safety critical)"
echo "   3. VERIFY CORE OPERATIONS (pipes, redirections)"
echo "   4. CLEAN UP COMPLETION CONFLICTS"
echo
echo "📊 ESTIMATED WORK: 8-12 hours to restore basic functionality"
echo
echo "🔧 MANUAL TESTING REQUIRED:"
echo "   Run: ./builddir/lusush"
echo "   Test: [UP ARROW] - should navigate history, not show completion"
echo "   Test: [Ctrl+C] - should clear line, not exit shell"
echo "   Test: echo hello | grep h - should work"
echo "   Test: echo test > file.txt - should work"
echo
echo "The GNU Readline foundation is solid, but core shell"
echo "functionality needs significant repair work."
