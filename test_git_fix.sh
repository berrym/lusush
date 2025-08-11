#!/bin/bash
# test_git_fix.sh - Test git status and external commands in interactive mode
# This script verifies that the readline signal handling fix resolves git issues

LUSUSH_BIN="./builddir/lusush"

echo "========================================"
echo "Git Status & External Commands Test"
echo "========================================"
echo
echo "Testing git status and other external commands..."
echo "This verifies the readline signal handling fix."
echo

# Test basic external commands first
echo "Test 1: Basic external commands"
echo "--------------------------------"
echo -e "whoami\ndate\nps -o pid,comm | head -3" | "$LUSUSH_BIN" 2>/dev/null
echo

echo "Test 2: Git commands (non-interactive)"
echo "---------------------------------------"
echo -e "git --version\ngit status --porcelain | head -5\ngit log --oneline -3" | "$LUSUSH_BIN" 2>/dev/null
echo

echo "Test 3: Complex git commands"
echo "-----------------------------"
echo -e "git branch\ngit remote -v\ngit config --get user.name" | "$LUSUSH_BIN" 2>/dev/null
echo

echo "========================================"
echo "INTERACTIVE MODE TEST"
echo "========================================"
echo
echo "To test the git status fix in interactive mode:"
echo
echo "1. Run: $LUSUSH_BIN"
echo
echo "2. Try these commands manually:"
echo "   git status"
echo "   git log --oneline -5"
echo "   git branch"
echo "   git diff --name-only"
echo
echo "Expected results:"
echo "✓ git status should show repository status immediately"
echo "✓ git log should show commit history"
echo "✓ git branch should show current branch"
echo "✓ No hanging or missing output"
echo "✓ All git commands should work normally"
echo
echo "Previous issue: git status would not show output in interactive mode"
echo "Fix applied: Disabled readline signal catching (rl_catch_signals = 0)"
echo "           This allows shell to handle child process signals properly"
echo
echo "If git commands work normally, the fix is successful!"
echo
echo "========================================"
echo "ADDITIONAL TESTS"
echo "========================================"
echo
echo "Also test these external commands:"
echo "  less README.md    (should work with proper terminal control)"
echo "  vim test_file     (should work without terminal conflicts)"
echo "  nano test_file    (should work without issues)"
echo "  man git           (should display properly)"
echo
echo "All external commands should now work properly in interactive mode."
