#!/bin/bash
# debug_git_interactive.sh - Interactive Git Debugging Test
# This script helps diagnose the git status issue in interactive mode

LUSUSH_BIN="./builddir/lusush"

echo "========================================"
echo "Git Interactive Mode Debugging Test"
echo "========================================"
echo
echo "This script will help diagnose why git status isn't working"
echo "in interactive mode by testing different scenarios."
echo

# Test 1: Basic verification that git works outside lusush
echo "Test 1: Git works in regular shell"
echo "-----------------------------------"
git --version
echo "Git status (first 3 lines):"
git status | head -3
echo

# Test 2: Test git in non-interactive lusush
echo "Test 2: Git in non-interactive lusush"
echo "--------------------------------------"
echo -e "git --version\ngit status | head -3" | "$LUSUSH_BIN" 2>&1
echo

# Test 3: Test other external commands in non-interactive mode
echo "Test 3: Other external commands"
echo "--------------------------------"
echo -e "whoami\ndate\nps -o pid,comm | head -3" | "$LUSUSH_BIN" 2>&1
echo

# Test 4: Test with explicit output redirection
echo "Test 4: Git with explicit output handling"
echo "------------------------------------------"
echo -e "git status 2>&1\necho 'Exit code:' \$?" | "$LUSUSH_BIN" 2>&1
echo

# Test 5: Test git with different options
echo "Test 5: Different git commands"
echo "------------------------------"
echo -e "git log --oneline -2\ngit branch\ngit remote -v" | "$LUSUSH_BIN" 2>&1
echo

echo "========================================"
echo "INTERACTIVE MODE MANUAL TEST"
echo "========================================"
echo
echo "Now test manually in interactive mode:"
echo
echo "1. Run: $LUSUSH_BIN"
echo
echo "2. Try each command and report what happens:"
echo
echo "   Command: git --version"
echo "   Expected: Shows git version"
echo "   What happens: ___________________"
echo
echo "   Command: git status"
echo "   Expected: Shows repository status"
echo "   What happens: ___________________"
echo
echo "   Command: echo 'before'; git status; echo 'after'"
echo "   Expected: Shows before, git status, after"
echo "   What happens: ___________________"
echo
echo "   Command: git status > /tmp/git_test.txt && cat /tmp/git_test.txt"
echo "   Expected: Shows git status output"
echo "   What happens: ___________________"
echo
echo "   Command: whoami"
echo "   Expected: Shows username"
echo "   What happens: ___________________"
echo
echo "   Command: date"
echo "   Expected: Shows current date/time"
echo "   What happens: ___________________"
echo
echo "========================================"
echo "DIAGNOSTIC QUESTIONS"
echo "========================================"
echo
echo "Please answer these questions about the interactive behavior:"
echo
echo "1. Does the shell hang when you type 'git status'? (Y/N)"
echo "2. Does it return to prompt immediately with no output? (Y/N)"
echo "3. Do you see any error messages? (Y/N)"
echo "4. Do other commands (whoami, date, ls) work normally? (Y/N)"
echo "5. Does 'git --version' work? (Y/N)"
echo "6. Does 'git status > /tmp/test.txt' create the file? (Y/N)"
echo "7. Can you interrupt with Ctrl+C if it hangs? (Y/N)"
echo
echo "========================================"
echo "ADDITIONAL DEBUG TESTS"
echo "========================================"
echo
echo "If git status doesn't work, try these debug commands:"
echo
echo "In the interactive shell, run:"
echo "  strace -e trace=write git status 2>&1 | head -20"
echo "  timeout 5 git status"
echo "  git status 2>&1 | cat"
echo "  env | grep -E '(TERM|TTY|GIT)'"
echo
echo "This will help identify if it's:"
echo "- A hanging issue (timeout test)"
echo "- A TTY/terminal issue (strace test)"
echo "- A buffering issue (pipe to cat test)"
echo "- An environment issue (env test)"
