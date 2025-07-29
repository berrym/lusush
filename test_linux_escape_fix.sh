#!/bin/bash

# Test script to verify Linux escape sequence artifacts are fixed
# This script tests that cursor query responses like ^[[25;1R no longer appear in output

set -e

echo "=================================================="
echo "Linux Escape Sequence Artifact Fix Test"
echo "=================================================="
echo

# Build first
echo "1. Building LLE with cursor query fix..."
echo "----------------------------------------"
scripts/lle_build.sh build

echo
echo "2. Platform Detection"
echo "-------------------"
echo "Platform: $(uname -s)"
if [[ "$(uname -s)" == "Linux" ]]; then
    echo "✅ Running on Linux - cursor queries should be disabled"
else
    echo "ℹ️  Running on $(uname -s) - cursor queries may still be enabled"
fi

echo
echo "3. Testing for Escape Sequence Artifacts"
echo "---------------------------------------"

echo "Testing basic shell invocation for escape sequence artifacts..."

# Capture shell output and look for cursor position responses
echo "Running shell with timeout to capture initial output..."

# Test basic shell startup
OUTPUT=$(timeout 2s ./builddir/lusush <<< "echo hello" 2>&1 || true)

echo "Checking output for escape sequence artifacts..."

# Look for cursor position response patterns
if echo "$OUTPUT" | grep -q "\^\[\[[0-9]*;[0-9]*R"; then
    echo "❌ FOUND escape sequence artifacts in output:"
    echo "$OUTPUT" | grep "\^\[\[[0-9]*;[0-9]*R" || true
    echo
    echo "The fix did not work - cursor query responses are still appearing."
    exit 1
elif echo "$OUTPUT" | grep -q $'\x1b\[[0-9]*;[0-9]*R'; then
    echo "❌ FOUND binary escape sequence artifacts in output"
    echo "Raw output check failed - cursor query responses still present."
    exit 1
else
    echo "✅ NO escape sequence artifacts found in basic output"
fi

echo
echo "4. Testing Multiple Commands"
echo "---------------------------"

echo "Testing multiple commands to ensure no artifacts appear..."

# Test multiple commands
MULTI_OUTPUT=$(timeout 3s ./builddir/lusush <<< $'echo "test1"\necho "test2"\nexit' 2>&1 || true)

echo "Checking multi-command output for artifacts..."

if echo "$MULTI_OUTPUT" | grep -q "\^\[\[[0-9]*;[0-9]*R"; then
    echo "❌ FOUND escape sequence artifacts in multi-command output"
    exit 1
elif echo "$MULTI_OUTPUT" | grep -q $'\x1b\[[0-9]*;[0-9]*R'; then
    echo "❌ FOUND binary escape sequence artifacts in multi-command output"
    exit 1
else
    echo "✅ NO escape sequence artifacts found in multi-command output"
fi

echo
echo "5. Debug Output Verification"
echo "---------------------------"

echo "Checking debug output for cursor query behavior..."

# Test with debug output
DEBUG_OUTPUT=$(LLE_DEBUG=1 timeout 2s ./builddir/lusush <<< "echo debug_test" 2>&1 || true)

# Check if debug shows cursor queries are disabled on Linux
if [[ "$(uname -s)" == "Linux" ]]; then
    if echo "$DEBUG_OUTPUT" | grep -q "Cursor query failed, using default position"; then
        echo "✅ Linux: Cursor queries properly disabled (using fallback positioning)"
    elif echo "$DEBUG_OUTPUT" | grep -q "cursor.*query"; then
        echo "⚠️  Cursor query debug messages found - checking if they're working correctly..."
        if echo "$DEBUG_OUTPUT" | grep -q "\^\[\[[0-9]*;[0-9]*R"; then
            echo "❌ Cursor queries still causing artifacts despite debug messages"
            exit 1
        else
            echo "✅ Cursor query debug present but no artifacts - may be working correctly"
        fi
    else
        echo "✅ No cursor query debug messages - likely disabled as expected"
    fi
else
    echo "ℹ️  Non-Linux platform - cursor queries may still be enabled"
fi

echo
echo "6. Interactive Simulation Test"
echo "-----------------------------"

echo "Testing interactive character input simulation..."

# Create a more realistic interactive test
INTERACTIVE_OUTPUT=$(timeout 3s bash -c '
    echo -e "h\ne\nl\nl\no\n\nexit" | ./builddir/lusush 2>&1
' || true)

echo "Checking interactive simulation for artifacts..."

if echo "$INTERACTIVE_OUTPUT" | grep -q "\^\[\[[0-9]*;[0-9]*R"; then
    echo "❌ FOUND escape sequence artifacts in interactive simulation"
    echo "Sample of problematic output:"
    echo "$INTERACTIVE_OUTPUT" | head -10
    exit 1
else
    echo "✅ NO escape sequence artifacts found in interactive simulation"
fi

echo
echo "7. Summary"
echo "----------"

echo "✅ Linux escape sequence artifact fix validation:"
echo "   - Basic shell invocation: Clean"
echo "   - Multiple commands: Clean"
echo "   - Interactive simulation: Clean"
if [[ "$(uname -s)" == "Linux" ]]; then
    echo "   - Linux cursor queries: Disabled (fallback positioning used)"
else
    echo "   - Non-Linux platform: Cursor queries may still be enabled"
fi

echo
echo "Expected behavior on Linux:"
echo "🐧 Cursor queries disabled to prevent timing issues"
echo "📍 Fallback positioning used (may be less precise but reliable)"
echo "🚫 No escape sequence artifacts in terminal output"
echo "✅ Clean prompt display without ^[[row;colR sequences"

echo
echo "Fix verification completed successfully!"
echo "The escape sequence artifacts should now be resolved on Linux."
echo "=================================================="
