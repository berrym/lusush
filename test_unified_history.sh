#!/bin/bash

# Unified History System Test for Lusush
# Tests the integration between readline history and POSIX history systems

echo "=== Lusush Unified History System Test ==="
echo ""

# Build if needed
if [ ! -f builddir/lusush ]; then
    echo "Building lusush..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "Build failed! Cannot proceed."
        exit 1
    fi
fi

echo "This test verifies that:"
echo "1. History is properly saved between sessions"
echo "2. The 'history' builtin shows the same entries as arrow key navigation"
echo "3. POSIX history and readline history are unified"
echo ""

echo "=== Phase 1: Clean History Test ==="
echo ""

# Remove existing history files
rm -f ~/.lusush_history ~/.lusushist .lusush_history .lusushist

echo "Removed existing history files for clean test"
echo ""

echo "=== Phase 2: Interactive History Population ==="
echo ""

echo "Starting lusush interactively to populate history..."
echo "You will need to:"
echo "1. Type several test commands"
echo "2. Verify 'history' builtin shows them"
echo "3. Verify UP arrow navigates them"
echo "4. Type 'exit' to save and continue test"
echo ""

echo "Commands to test:"
echo "  echo \"history entry 1\""
echo "  echo \"history entry 2\""
echo "  echo \"history entry 3\""
echo "  pwd"
echo "  whoami"
echo "  history"
echo "  exit"
echo ""

echo "Press ENTER to start interactive session..."
read

# Run interactive session
script -q -c './builddir/lusush' /dev/null

echo ""
echo "=== Phase 3: History Persistence Test ==="
echo ""

echo "Now starting a NEW lusush session to test history persistence..."
echo "The history from the previous session should be available."
echo ""

echo "Test in the new session:"
echo "1. Type 'history' - should show previous commands"
echo "2. Press UP arrow - should navigate through previous commands"
echo "3. Verify entries match between builtin and navigation"
echo ""

echo "Press ENTER to start new session..."
read

script -q -c './builddir/lusush' /dev/null

echo ""
echo "=== Phase 4: History File Analysis ==="
echo ""

echo "Checking history files that were created:"
echo ""

if [ -f ~/.lusush_history ]; then
    echo "✅ Found ~/.lusush_history:"
    echo "   Entries: $(wc -l < ~/.lusush_history)"
    echo "   Sample entries:"
    head -5 ~/.lusush_history | sed 's/^/     /'
elif [ -f ~/.lusushist ]; then
    echo "✅ Found ~/.lusushist:"
    echo "   Entries: $(wc -l < ~/.lusushist)"
    echo "   Sample entries:"
    head -5 ~/.lusushist | sed 's/^/     /'
elif [ -f .lusush_history ]; then
    echo "✅ Found .lusush_history (local):"
    echo "   Entries: $(wc -l < .lusush_history)"
    echo "   Sample entries:"
    head -5 .lusush_history | sed 's/^/     /'
else
    echo "❌ No history file found"
    echo "   Checked: ~/.lusush_history, ~/.lusushist, .lusush_history"
    echo "   This indicates history is not being saved properly"
fi

echo ""
echo "=== Phase 5: Non-Interactive History Test ==="
echo ""

echo "Testing if history command works in non-interactive mode:"
echo 'history' | ./builddir/lusush

echo ""
echo "Note: Non-interactive mode may not show history due to different"
echo "initialization path. This is normal shell behavior."

echo ""
echo "=== Results Analysis ==="
echo ""

echo "Expected behavior for WORKING unified history:"
echo ""
echo "✅ Interactive Session 1:"
echo "   - Commands get added to history as you type them"
echo "   - 'history' builtin shows numbered list of commands"
echo "   - UP arrow navigates through the same commands"
echo "   - History is saved to file on exit"
echo ""
echo "✅ Interactive Session 2:"
echo "   - 'history' shows commands from previous session"
echo "   - UP arrow can navigate through previous session commands"
echo "   - New commands get added to existing history"
echo ""
echo "✅ History File:"
echo "   - Contains readable command entries"
echo "   - Persists between sessions"
echo "   - Used by both builtin and navigation"
echo ""

echo "If you saw different behavior, the history unification needs work."
echo ""

echo "=== Debug Mode Test ==="
echo ""

echo "To see detailed history debugging, run:"
echo "  LUSUSH_DEBUG=1 script -q -c './builddir/lusush' /dev/null"
echo ""
echo "Look for these debug messages:"
echo "  [HISTORY_DEBUG] Added to unified history"
echo "  [HISTORY_DEBUG] Loaded N entries from POSIX history"
echo "  [HISTORY_DEBUG] Synced N POSIX entries to readline"
echo ""

echo "=== Manual Verification Complete ==="
echo ""
echo "Use the results above to determine if history unification is working correctly."
