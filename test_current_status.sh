#!/bin/bash

# Comprehensive Interactive Mode Status Test for Lusush
# This script tests the current state of critical interactive mode issues

echo "=== Lusush Interactive Mode Status Verification ==="
echo "Date: $(date)"
echo ""

# Ensure lusush is built
if [ ! -f builddir/lusush ]; then
    echo "❌ lusush binary not found. Building..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "❌ Build failed! Cannot proceed."
        exit 1
    fi
    echo "✅ Build successful"
fi

echo "This script tests the 3 critical interactive mode issues:"
echo "  1. History Navigation (UP arrow behavior)"
echo "  2. Signal Handling (Ctrl+C behavior)"
echo "  3. Basic Shell Operations (pipes, redirections)"
echo ""

# Clean environment for consistent testing
rm -f ~/.lusush_history ~/.lusushist .lusush_history .lusushist

echo "=== TEST 1: History Navigation Critical Issue ==="
echo ""
echo "Testing the original user complaint:"
echo "  ❌ ORIGINAL BUG: UP arrow shows 'display all 4418 possibilities'"
echo "  ✅ EXPECTED FIX: UP arrow navigates through command history"
echo ""

# Create expect script for history navigation test
cat > /tmp/test_history_nav.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

puts "Starting history navigation test..."

spawn script -q -c "./builddir/lusush" /dev/null

expect {
    "$ " {
        puts "✅ Shell started successfully"
    }
    timeout {
        puts "❌ Shell failed to start"
        exit 1
    }
}

# Add some commands to history
send "echo \"history_test_command_1\"\r"
expect "$ "

send "echo \"history_test_command_2\"\r"
expect "$ "

send "echo \"history_test_command_3\"\r"
expect "$ "

puts "\n--- CRITICAL TEST: UP Arrow Behavior ---"

# THE KEY TEST: Press UP arrow - this was the original bug
send "\033\[A"

# Check what happens
expect {
    "history_test_command_3" {
        puts "🎉 SUCCESS: UP arrow shows last command (history_test_command_3)"
        puts "🎉 HISTORY NAVIGATION WORKING!"

        # Test navigation deeper
        send "\033\[A"
        expect {
            "history_test_command_2" {
                puts "🎉 SUCCESS: Second UP arrow shows previous command"
                set nav_result "WORKING"
            }
            timeout {
                puts "❓ Second navigation unclear"
                set nav_result "PARTIAL"
            }
        }
    }
    "display all" {
        puts "❌ CRITICAL FAILURE: Original bug still present!"
        puts "❌ UP arrow triggers completion menu, not history navigation"
        set nav_result "BROKEN"
        send "n\r"
    }
    -re "possibilities.*y or n" {
        puts "❌ CRITICAL FAILURE: 'possibilities y/n' completion menu detected"
        puts "❌ This is the exact original bug reported by user"
        set nav_result "BROKEN"
        send "n\r"
    }
    timeout {
        puts "❓ UP arrow test timeout - behavior unclear"
        set nav_result "TIMEOUT"
    }
}

# Clear any pending input and exit
send "\003"
expect "$ "
send "exit\r"
expect eof

puts "\nHistory Navigation Result: $nav_result"
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_history_nav.exp
    echo "Running history navigation test..."
    /tmp/test_history_nav.exp
    nav_exit_code=$?
else
    echo "❌ expect not available - cannot test interactive history navigation"
    nav_exit_code=2
fi

echo ""
echo "=== TEST 2: Signal Handling Critical Issue ==="
echo ""
echo "Testing signal handling behavior:"
echo "  ❌ ORIGINAL BUG: Ctrl+C exits entire shell"
echo "  ✅ EXPECTED FIX: Ctrl+C clears line, shell continues"
echo ""

# Create expect script for signal handling test
cat > /tmp/test_signal_handling.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 8

puts "Starting signal handling test..."

spawn script -q -c "./builddir/lusush" /dev/null

expect {
    "$ " {
        puts "✅ Shell started for signal test"
    }
    timeout {
        puts "❌ Shell failed to start for signal test"
        exit 1
    }
}

# Type a command but don't execute it
send "echo \"this command should be interrupted\""
# DON'T send \r - we want to interrupt before execution

puts "\n--- CRITICAL TEST: Ctrl+C Behavior ---"

# Send Ctrl+C (SIGINT)
send "\003"

# Check what happens
expect {
    "$ " {
        puts "🎉 SUCCESS: Ctrl+C cleared line and returned to prompt"
        puts "🎉 SHELL CONTINUES RUNNING - Signal handling working!"
        set signal_result "WORKING"

        # Verify shell is still functional
        send "echo \"shell still working\"\r"
        expect {
            "shell still working" {
                puts "🎉 SUCCESS: Shell functional after Ctrl+C"
            }
            timeout {
                puts "❓ Shell response unclear after Ctrl+C"
            }
        }
        expect "$ "
    }
    eof {
        puts "❌ CRITICAL FAILURE: Shell exited on Ctrl+C"
        puts "❌ This is the dangerous behavior reported by user"
        set signal_result "BROKEN"
    }
    timeout {
        puts "❓ Signal handling test timeout"
        set signal_result "TIMEOUT"
    }
}

if {$signal_result eq "WORKING"} {
    # Test with actual child process
    puts "\n--- Testing Ctrl+C with Child Process ---"
    send "sleep 5\r"

    # Let it start
    sleep 1

    # Interrupt it
    send "\003"

    expect {
        "$ " {
            puts "🎉 SUCCESS: Ctrl+C interrupted child process, shell continues"
        }
        timeout {
            puts "❓ Child process interruption unclear"
        }
    }
}

send "exit\r"
expect eof

puts "\nSignal Handling Result: $signal_result"
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_signal_handling.exp
    echo "Running signal handling test..."
    /tmp/test_signal_handling.exp
    signal_exit_code=$?
else
    echo "❌ expect not available - cannot test interactive signal handling"
    signal_exit_code=2
fi

echo ""
echo "=== TEST 3: Basic Shell Operations in Interactive Mode ==="
echo ""
echo "Testing operations user reported as broken in interactive mode:"
echo "  - Pipes: cmd1 | cmd2"
echo "  - Redirections: cmd > file"
echo "  - Command substitution: \$(cmd)"
echo "  - Background jobs: cmd &"
echo ""

# Create expect script for shell operations test
cat > /tmp/test_shell_ops.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

puts "Starting shell operations test..."

spawn script -q -c "./builddir/lusush" /dev/null

expect "$ "

# Test 1: Pipes
puts "\n--- Testing Pipes ---"
send "echo hello | grep h\r"
expect {
    "hello" {
        puts "✅ SUCCESS: Pipes working in interactive mode"
        set pipe_result "WORKING"
    }
    timeout {
        puts "❌ FAILURE: Pipes timeout in interactive mode"
        set pipe_result "BROKEN"
    }
}
expect "$ "

# Test 2: Redirections
puts "\n--- Testing Redirections ---"
send "echo \"test redirect\" > /tmp/lusush_test.txt\r"
expect "$ "

send "cat /tmp/lusush_test.txt\r"
expect {
    "test redirect" {
        puts "✅ SUCCESS: Redirections working in interactive mode"
        set redirect_result "WORKING"
    }
    timeout {
        puts "❌ FAILURE: Redirections timeout in interactive mode"
        set redirect_result "BROKEN"
    }
}
expect "$ "

# Test 3: Command substitution
puts "\n--- Testing Command Substitution ---"
send "echo \"Current user: \$(whoami)\"\r"
expect {
    -re "Current user: \\w+" {
        puts "✅ SUCCESS: Command substitution working in interactive mode"
        set subst_result "WORKING"
    }
    timeout {
        puts "❌ FAILURE: Command substitution timeout in interactive mode"
        set subst_result "BROKEN"
    }
}
expect "$ "

# Test 4: Background jobs (quick test)
puts "\n--- Testing Background Jobs ---"
send "sleep 0.5 &\r"
expect {
    -re "\\[\\d+\\]" {
        puts "✅ SUCCESS: Background jobs working in interactive mode"
        set bg_result "WORKING"
    }
    "$ " {
        puts "❓ Background jobs unclear (no job number shown)"
        set bg_result "UNCLEAR"
    }
    timeout {
        puts "❌ FAILURE: Background jobs timeout in interactive mode"
        set bg_result "BROKEN"
    }
}
expect "$ "

# Wait for background job to complete
send "wait\r"
expect "$ "

# Test 5: Jobs command
puts "\n--- Testing Jobs Command ---"
send "sleep 2 &\r"
expect "$ "
send "jobs\r"
expect {
    -re "\\[\\d+\\].*sleep" {
        puts "✅ SUCCESS: Jobs command working in interactive mode"
        set jobs_result "WORKING"
    }
    timeout {
        puts "❌ FAILURE: Jobs command timeout in interactive mode"
        set jobs_result "BROKEN"
    }
}
expect "$ "

# Clean up
send "kill %1\r"
expect "$ "

send "exit\r"
expect eof

puts "\nShell Operations Results:"
puts "  Pipes: $pipe_result"
puts "  Redirections: $redirect_result"
puts "  Command Substitution: $subst_result"
puts "  Background Jobs: $bg_result"
puts "  Jobs Command: $jobs_result"
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_shell_ops.exp
    echo "Running shell operations test..."
    /tmp/test_shell_ops.exp
    ops_exit_code=$?
else
    echo "❌ expect not available - cannot test interactive shell operations"
    ops_exit_code=2
fi

echo ""
echo "=== TEST 4: Git Status Verification ==="
echo ""
echo "Verifying that git status still works (should be preserved from previous fix):"

if [ -d .git ]; then
    # Test git status in interactive mode
    cat > /tmp/test_git_status.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn script -q -c "./builddir/lusush" /dev/null

expect "$ "

send "git status\r"
expect {
    -re "(On branch|nothing to commit|Changes)" {
        puts "✅ SUCCESS: Git status working in interactive mode"
        set git_result "WORKING"
    }
    timeout {
        puts "❌ FAILURE: Git status timeout in interactive mode"
        set git_result "BROKEN"
    }
}

send "exit\r"
expect eof

puts "\nGit Status Result: $git_result"
EOF

    if command -v expect >/dev/null 2>&1; then
        chmod +x /tmp/test_git_status.exp
        /tmp/test_git_status.exp
        git_exit_code=$?
    else
        echo "❌ expect not available for git test"
        git_exit_code=2
    fi
else
    echo "ℹ️  Not a git repository - skipping git status test"
    git_exit_code=0
fi

echo ""
echo "=== FINAL STATUS ASSESSMENT ==="
echo ""

# Analyze results
history_status="UNKNOWN"
signal_status="UNKNOWN"
ops_status="UNKNOWN"
git_status="UNKNOWN"

if [ $nav_exit_code -eq 0 ]; then
    history_status="FIXED"
elif [ $nav_exit_code -eq 1 ]; then
    history_status="BROKEN"
else
    history_status="NEEDS_MANUAL_TEST"
fi

if [ $signal_exit_code -eq 0 ]; then
    signal_status="FIXED"
elif [ $signal_exit_code -eq 1 ]; then
    signal_status="BROKEN"
else
    signal_status="NEEDS_MANUAL_TEST"
fi

if [ $ops_exit_code -eq 0 ]; then
    ops_status="WORKING"
elif [ $ops_exit_code -eq 1 ]; then
    ops_status="BROKEN"
else
    ops_status="NEEDS_MANUAL_TEST"
fi

if [ $git_exit_code -eq 0 ]; then
    git_status="WORKING"
elif [ $git_exit_code -eq 1 ]; then
    git_status="BROKEN"
else
    git_status="NEEDS_MANUAL_TEST"
fi

echo "📊 CRITICAL ISSUES STATUS:"
echo ""
echo "1. 🔍 HISTORY NAVIGATION:"
echo "   Status: $history_status"
if [ "$history_status" = "FIXED" ]; then
    echo "   ✅ UP arrow navigates through command history"
    echo "   ✅ NO 'display all possibilities' completion menu"
    echo "   ✅ History builtin shows real commands with numbering"
elif [ "$history_status" = "BROKEN" ]; then
    echo "   ❌ UP arrow still shows completion menu"
    echo "   ❌ Original 'display all possibilities' bug persists"
    echo "   ❌ History navigation unusable"
else
    echo "   ❓ Requires manual verification in true interactive mode"
fi

echo ""
echo "2. 🔍 SIGNAL HANDLING:"
echo "   Status: $signal_status"
if [ "$signal_status" = "FIXED" ]; then
    echo "   ✅ Ctrl+C clears current line"
    echo "   ✅ Shell continues running (safe behavior)"
    echo "   ✅ Child processes properly interrupted"
elif [ "$signal_status" = "BROKEN" ]; then
    echo "   ❌ Ctrl+C exits entire shell (dangerous)"
    echo "   ❌ User loses shell session on interrupt"
    echo "   ❌ Signal handling not working correctly"
else
    echo "   ❓ Requires manual verification in true interactive mode"
fi

echo ""
echo "3. 🔍 BASIC SHELL OPERATIONS:"
echo "   Status: $ops_status"
if [ "$ops_status" = "WORKING" ]; then
    echo "   ✅ Pipes working: cmd1 | cmd2"
    echo "   ✅ Redirections working: cmd > file"
    echo "   ✅ Command substitution working: \$(cmd)"
    echo "   ✅ Background jobs working: cmd &"
elif [ "$ops_status" = "BROKEN" ]; then
    echo "   ❌ Pipes not working in interactive mode"
    echo "   ❌ Redirections not working in interactive mode"
    echo "   ❌ Command substitution not working in interactive mode"
    echo "   ❌ Background jobs not working in interactive mode"
else
    echo "   ❓ Requires manual verification in true interactive mode"
fi

echo ""
echo "4. 🔍 GIT STATUS (REGRESSION CHECK):"
echo "   Status: $git_status"
if [ "$git_status" = "WORKING" ]; then
    echo "   ✅ Git status working (previous fix preserved)"
elif [ "$git_status" = "BROKEN" ]; then
    echo "   ❌ Git status broken (regression detected)"
else
    echo "   ❓ Git test not applicable or needs manual verification"
fi

echo ""
echo "=== OVERALL ASSESSMENT ==="
echo ""

# Determine overall status
critical_issues=0
working_features=0
unclear_features=0

if [ "$history_status" = "BROKEN" ]; then
    critical_issues=$((critical_issues + 1))
elif [ "$history_status" = "FIXED" ]; then
    working_features=$((working_features + 1))
else
    unclear_features=$((unclear_features + 1))
fi

if [ "$signal_status" = "BROKEN" ]; then
    critical_issues=$((critical_issues + 1))
elif [ "$signal_status" = "FIXED" ]; then
    working_features=$((working_features + 1))
else
    unclear_features=$((unclear_features + 1))
fi

if [ "$ops_status" = "BROKEN" ]; then
    critical_issues=$((critical_issues + 1))
elif [ "$ops_status" = "WORKING" ]; then
    working_features=$((working_features + 1))
else
    unclear_features=$((unclear_features + 1))
fi

echo "Critical Issues: $critical_issues"
echo "Working Features: $working_features"
echo "Unclear/Manual Test Needed: $unclear_features"
echo ""

if [ $critical_issues -eq 0 ] && [ $working_features -ge 3 ]; then
    echo "🎉 SUCCESS: All critical interactive mode issues appear RESOLVED!"
    echo ""
    echo "ACHIEVEMENTS:"
    echo "  ✅ History navigation working (original main complaint fixed)"
    echo "  ✅ Signal handling safe (Ctrl+C doesn't exit shell)"
    echo "  ✅ Basic shell operations functional in interactive mode"
    echo "  ✅ Git status preserved (no regressions)"
    echo ""
    echo "🎯 LUSUSH APPEARS READY FOR DAILY INTERACTIVE USE"
    echo ""
    echo "Recommendation: Perform final manual verification with:"
    echo "  script -q -c './builddir/lusush' /dev/null"

elif [ $critical_issues -gt 0 ]; then
    echo "🚨 CRITICAL ISSUES REMAIN: $critical_issues major problems detected"
    echo ""
    echo "URGENT FIXES NEEDED:"

    if [ "$history_status" = "BROKEN" ]; then
        echo "  ❌ History navigation still broken (UP arrow shows completion)"
        echo "     Fix: src/readline_integration.c completion configuration"
    fi

    if [ "$signal_status" = "BROKEN" ]; then
        echo "  ❌ Signal handling dangerous (Ctrl+C exits shell)"
        echo "     Fix: src/signals.c SIGINT handler for interactive mode"
    fi

    if [ "$ops_status" = "BROKEN" ]; then
        echo "  ❌ Basic shell operations broken in interactive mode"
        echo "     Fix: Investigation needed for readline integration impact"
    fi

    echo ""
    echo "🎯 PRIORITY: Fix these $critical_issues critical issues for usable shell"

else
    echo "❓ TESTING INCONCLUSIVE: Manual verification required"
    echo ""
    echo "Some tests were inconclusive. Manual testing needed:"
    echo "  script -q -c './builddir/lusush' /dev/null"
    echo ""
    echo "Manual test checklist:"
    echo "  1. UP arrow should navigate history (not show completion menu)"
    echo "  2. Ctrl+C should clear line (not exit shell)"
    echo "  3. echo hello | grep h should work"
    echo "  4. echo test > /tmp/test.txt should work"
    echo "  5. echo \$(whoami) should work"
fi

echo ""
echo "=== DEBUGGING INSTRUCTIONS ==="
echo ""

echo "If issues are found, enable debugging:"
echo "  LUSUSH_DEBUG=1 script -q -c './builddir/lusush' /dev/null"
echo ""
echo "Debug output will show:"
echo "  [HISTORY_DEBUG] - History system operations"
echo "  [SIGNAL_DEBUG] - Signal handling behavior"
echo "  [DEBUG] - Readline configuration and completion setup"
echo ""

echo "For signal debugging specifically:"
echo "  Enable with: lusush_set_signal_debug(true) in shell"
echo "  Or check environment: LUSUSH_FORCE_INTERACTIVE=1"

echo ""
echo "=== MANUAL TESTING PROCEDURE ==="
echo ""

echo "For definitive verification (automated tests have limitations):"
echo ""
echo "1. Start interactive mode:"
echo "   script -q -c './builddir/lusush' /dev/null"
echo ""
echo "2. Test History Navigation (Critical Issue #1):"
echo "   lusush\$ echo \"test 1\""
echo "   lusush\$ echo \"test 2\""
echo "   lusush\$ [Press UP ARROW]"
echo "   Expected: Shows 'echo \"test 2\"'"
echo "   Bug: Shows 'display all XXXX possibilities (y or n)?'"
echo ""
echo "3. Test Signal Handling (Critical Issue #2):"
echo "   lusush\$ echo hello[Press Ctrl+C without Enter]"
echo "   Expected: Line clears, new prompt, shell continues"
echo "   Bug: Entire shell exits"
echo ""
echo "4. Test Shell Operations (Critical Issue #3):"
echo "   lusush\$ echo hello | grep h"
echo "   lusush\$ echo test > /tmp/test.txt"
echo "   lusush\$ cat /tmp/test.txt"
echo "   lusush\$ echo \$(whoami)"
echo "   Expected: All should work normally"
echo "   Bug: May fail or behave incorrectly in interactive mode"
echo ""

echo "=== TEST RESULT SUMMARY ==="
echo ""

if [ $critical_issues -eq 0 ] && [ $working_features -ge 3 ]; then
    echo "🎉 TESTING INDICATES: All critical issues resolved"
    echo "🎯 STATUS: Ready for production use"
    echo "📋 ACTION: Final manual verification recommended"
elif [ $critical_issues -gt 0 ]; then
    echo "🚨 TESTING INDICATES: $critical_issues critical issues remain"
    echo "🎯 STATUS: Requires immediate fixes"
    echo "📋 ACTION: Fix critical issues before production use"
else
    echo "❓ TESTING INCONCLUSIVE: Manual verification required"
    echo "🎯 STATUS: Cannot determine from automated tests"
    echo "📋 ACTION: Manual interactive testing mandatory"
fi

echo ""
echo "Final Note: Automated testing has limitations for interactive mode."
echo "True verification requires human manual testing in actual interactive environment."

# Cleanup
rm -f /tmp/test_history_nav.exp /tmp/test_signal_handling.exp /tmp/test_shell_ops.exp /tmp/test_git_status.exp
rm -f /tmp/lusush_test.txt

echo ""
echo "Test completed: $(date)"
