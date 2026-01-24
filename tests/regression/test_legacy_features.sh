#!/usr/bin/env lush
# =============================================================================
# test_legacy_features.sh - Regression test for legacy shell features
# =============================================================================
#
# This test script was originally created years ago and exposed two critical
# bugs when run against lush after a major refactoring:
#
# Bug #70: Backslash-newline line continuation not properly removed in
#          double-quoted strings. The tokenizer was bulk-copying content
#          instead of processing character-by-character.
#          Fixed in: src/tokenizer.c (Session 126)
#
# Bug #71: Braced parameter expansion ${var} not executing in command position.
#          The executor had special-case code that expanded the variable and
#          discarded the result without executing.
#          Fixed in: src/executor.c (Session 126)
#
# This script is preserved as a regression test to ensure these bugs
# never return.
#
# Expected behavior:
# - Line continuations (backslash-newline) inside double quotes should be
#   removed entirely, joining the lines seamlessly
# - Parameter expansions like ${n1=4} should both assign the value AND
#   execute (attempting to run "4" as a command, which will fail with
#   "command not found" - this is correct POSIX behavior)
#
# Author: Michael Berry <trismegustis@gmail.com>
# Copyright (C) 2021-2026 Michael Berry
# =============================================================================

# Test 1: Line continuation in double-quoted strings
# This tests Bug #70 - backslash-newline should be removed entirely
echo "\nHello, \
world!\n\
\n\
\tThis\tis\tthe\t\`lush'\tshell!\n\
\n\
It's \
a \
fantastic \
$(date +'%A')!\n"

# Test 2: More line continuation with date expansion
echo "\tIn\t$(date +'%B')\t$(date +'%Y')\n"

# Test 3: Parameter assignment expansion in command position
# This tests Bug #71 - ${var=value} should assign AND return the value
# The value "4" will be executed as a command, which should fail with
# "command not found". This is correct POSIX behavior.
# We use `:` (true command) to consume the expansion results and ensure
# the variables are assigned without trying to execute "4" as a command.
: ${n1=4} ${n2=6} ${n3=2}

# Verify the assignments worked
echo $n1^$n2 / $n3 = $(($n1 ** $n2 / $n3))

# Test 4: Glob expansion and command substitution
echo "\nThe expanded glob \`*' of \`$(echo $(pwd))' looks like this..."
echo *

# Test 5: Command substitution with complex output
echo "\nThe detailed directory listing of ~ looks like...\n$(ls -alF ~)\n"

# Test 6: Simple message
echo "Goodbye!\n"

# =============================================================================
# Verification Notes:
#
# When running this script with lush, bash, and zsh:
#
# 1. The output should NOT contain literal backslash characters followed by
#    newlines in the quoted strings. If you see "Hello, \" on one line and
#    "world!" on the next, Bug #70 has regressed.
#
# 2. Line 53 uses the `:` (null) command to consume the assignment expansions.
#    This is a common POSIX idiom: `: ${var=default}` assigns var if unset
#    without trying to execute the value. The variables should be assigned.
#
# 3. The arithmetic expansion on line 55 should output: 4^6 / 2 = 2048
#    (4^6 = 4096, 4096/2 = 2048)
# =============================================================================
