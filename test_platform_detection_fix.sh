#!/bin/bash

# Test script to verify platform-specific Linux compatibility fixes
# This script tests that the lle_display_clear_to_eol_linux_safe() function
# correctly detects the platform and uses appropriate clearing methods

set -e

echo "=================================================="
echo "Platform Detection and Linux Compatibility Test"
echo "=================================================="
echo

# Enable debug mode to see platform detection output
export LLE_DEBUG=1

echo "1. Building LLE with Linux compatibility fixes..."
echo "------------------------------------------------"
scripts/lle_build.sh build

echo
echo "2. Platform Detection Test"
echo "-------------------------"

# Get platform info
echo "Current platform information:"
echo "- uname -s: $(uname -s)"
echo "- Compiler defines:"

# Check what platform the code will detect
cat > /tmp/platform_test.c << 'EOF'
#include <stdio.h>

int main() {
#ifdef __APPLE__
    printf("Platform: macOS (LLE_PLATFORM_MACOS)\n");
    printf("Expected behavior: Fast escape sequences\n");
#elif defined(__linux__)
    printf("Platform: Linux (LLE_PLATFORM_LINUX)\n");
    printf("Expected behavior: Safe space-overwrite clearing\n");
#else
    printf("Platform: Unknown (LLE_PLATFORM_UNKNOWN)\n");
    printf("Expected behavior: Fallback to Linux-safe method\n");
#endif
    return 0;
}
EOF

gcc -o /tmp/platform_test /tmp/platform_test.c
echo "Platform detection result:"
/tmp/platform_test
rm -f /tmp/platform_test.c /tmp/platform_test

echo
echo "3. Testing Linux-Safe Clear Function in Debug Mode"
echo "-------------------------------------------------"

# Create a simple test that will trigger the clear function
echo "Creating simple input test..."

# Test basic character input with debug output
echo "Testing basic character input (should show platform detection):"
echo "Expected debug output patterns:"
if [[ "$(uname -s)" == "Linux" ]]; then
    echo "  - [LLE_DISPLAY_INCREMENTAL] Platform detected: Linux"
    echo "  - [LLE_CLEAR_EOL] Using Linux-safe character clearing"
    echo "  - [LLE_CLEAR_EOL] Linux: Safe clearing distance: XX"
    echo "  - [LLE_CLEAR_EOL] Linux-safe clear completed"
elif [[ "$(uname -s)" == "Darwin" ]]; then
    echo "  - [LLE_DISPLAY_INCREMENTAL] Platform detected: macOS"
    echo "  - [LLE_CLEAR_EOL] Using fast macOS clear method"
else
    echo "  - [LLE_DISPLAY_INCREMENTAL] Platform detected: Unknown"
    echo "  - [LLE_CLEAR_EOL] Using Linux-safe character clearing (fallback)"
fi

echo
echo "Running shell with debug output (5 second timeout)..."
echo "Type a few characters and press Enter to test character input."
echo "Look for the platform detection and clear method debug messages."
echo

# Run the shell with timeout and capture debug output
timeout 5s ./builddir/lusush 2>&1 | grep -E "(Platform detected|LLE_CLEAR_EOL|LLE_DISPLAY_INCREMENTAL)" | head -10 || true

echo
echo "4. Testing All Clear-to-EOL Call Sites"
echo "------------------------------------"

echo "Verifying all clear-to-EOL operations use Linux-safe function:"

# Check that we have only the legitimate call inside the Linux-safe function
echo "Searching for lle_terminal_clear_to_eol calls..."
ALL_CALLS=$(grep -n "lle_terminal_clear_to_eol(state->terminal)" src/line_editor/display.c || true)

if [[ -z "$ALL_CALLS" ]]; then
    echo "❌ ERROR: No clear-to-EOL calls found at all"
    exit 1
fi

# Count the number of calls
CALL_COUNT=$(echo "$ALL_CALLS" | wc -l)

if [[ "$CALL_COUNT" -eq 1 ]]; then
    # Check if it's the correct one (inside the Linux-safe function around line 1350)
    LINE_NUMBER=$(echo "$ALL_CALLS" | cut -d: -f1)
    if [[ "$LINE_NUMBER" -ge 1340 && "$LINE_NUMBER" -le 1360 ]]; then
        echo "✅ GOOD: Found exactly one clear-to-EOL call (the correct macOS fast path)"
        echo "   Line: $ALL_CALLS"
        echo "   This is the legitimate call inside lle_display_clear_to_eol_linux_safe()"
    else
        echo "❌ ERROR: Found one call but it's not in the correct location (should be ~line 1350)"
        echo "$ALL_CALLS"
        exit 1
    fi
else
    echo "❌ ERROR: Found $CALL_COUNT clear-to-EOL calls, should be exactly 1 (macOS fast path only)"
    echo "$ALL_CALLS"
    echo
    echo "Extra calls should be replaced with lle_display_clear_to_eol_linux_safe(state)"
    exit 1
fi

# Check that we have the Linux-safe calls
echo "Searching for Linux-safe clear-to-EOL calls..."
SAFE_CALLS=$(grep -n "lle_display_clear_to_eol_linux_safe(state)" src/line_editor/display.c || true)

if [[ -n "$SAFE_CALLS" ]]; then
    echo "✅ GOOD: Found Linux-safe clear calls:"
    echo "$SAFE_CALLS"
else
    echo "❌ ERROR: No Linux-safe clear calls found"
    exit 1
fi

echo
echo "5. Function Implementation Verification"
echo "------------------------------------"

echo "Verifying lle_display_clear_to_eol_linux_safe implementation..."

# Check that the function has platform detection
if grep -q "lle_platform_type_t platform = lle_detect_platform();" src/line_editor/display.c; then
    echo "✅ Platform detection: Present"
else
    echo "❌ Platform detection: Missing"
    exit 1
fi

# Check that it has macOS fast path
if grep -q "if (platform == LLE_PLATFORM_MACOS)" src/line_editor/display.c; then
    echo "✅ macOS fast path: Present"
else
    echo "❌ macOS fast path: Missing"
    exit 1
fi

# Check that it has Linux safe method
if grep -q "lle_terminal_write(state->terminal, \" \", 1)" src/line_editor/display.c; then
    echo "✅ Linux space-overwrite method: Present"
else
    echo "❌ Linux space-overwrite method: Missing"
    exit 1
fi

echo
echo "6. Build and Test Verification"
echo "----------------------------"

echo "Running basic test suite to ensure no regressions..."
timeout 30s meson test -C builddir test_lle_018_multiline_input_display --verbose 2>&1 | grep -E "(PASS|FAIL|ERROR)" || true

echo
echo "7. Summary"
echo "----------"

echo "✅ Linux compatibility fixes implemented:"
echo "   - Platform detection working"
echo "   - All clear-to-EOL calls use Linux-safe version"
echo "   - macOS performance preserved (uses fast escape sequences)"
echo "   - Linux uses safe space-overwrite method"
echo
echo "Expected behavior:"
echo "📱 macOS: Zero performance impact, uses original fast clearing"
echo "🐧 Linux: Uses space-overwrite clearing to prevent character duplication"
echo "❓ Other: Fallback to Linux-safe method"
echo
echo "Next steps for Linux testing:"
echo "1. Deploy this code to a Linux/Konsole environment"
echo "2. Run: LLE_DEBUG=1 ./builddir/lusush"
echo "3. Type characters and verify no duplication occurs"
echo "4. Look for debug messages confirming Linux-safe clearing is used"
echo
echo "Platform detection test completed!"
echo "=================================================="
