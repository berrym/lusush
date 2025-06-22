#!/bin/bash

# Automated verification of multiline history conversion
# This script tests that multiline commands are properly converted in the history file

set -e

SHELL_PATH="./builddir/lusush"
HISTORY_FILE="$HOME/.lusushist"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Lusush History Conversion Verification ==="
echo "Testing shell: $SHELL_PATH"
echo "History file: $HISTORY_FILE"
echo

# Check if shell exists
if [ ! -x "$SHELL_PATH" ]; then
    echo -e "${RED}Error: Shell binary not found at $SHELL_PATH${NC}"
    echo "Please build the shell first with: meson compile -C builddir"
    exit 1
fi

# Clean up existing history
echo "Cleaning up existing history..."
rm -f "$HISTORY_FILE"

# Function to add multiline command and check history
test_history_conversion() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"

    echo -e "${YELLOW}Test: $test_name${NC}"
    echo "Input (multiline):"
    echo "$input" | sed 's/^/  /'

    # Add the command to history by executing it
    echo "$input" | timeout 10s "$SHELL_PATH" >/dev/null 2>&1 || true

    # Check if history file was created and contains expected content
    if [ -f "$HISTORY_FILE" ]; then
        local last_entry
        last_entry=$(tail -1 "$HISTORY_FILE" 2>/dev/null || echo "")

        echo "History entry: $last_entry"
        echo "Expected pattern: $expected_pattern"

        if echo "$last_entry" | grep -q "$expected_pattern"; then
            echo -e "${GREEN}✓ PASSED${NC}"
        else
            echo -e "${RED}✗ FAILED - Pattern not found${NC}"
        fi
    else
        echo -e "${RED}✗ FAILED - History file not created${NC}"
    fi
    echo
}

# Test 1: Simple backslash continuation
test_history_conversion \
    "Backslash continuation" \
    "echo hello \\
world" \
    "echo hello world"

# Test 2: Basic for loop
test_history_conversion \
    "FOR loop" \
    "for i in 1 2
do
    echo \$i
done" \
    "for i in 1 2 do echo"

# Test 3: IF statement
test_history_conversion \
    "IF statement" \
    "if true
then
    echo success
fi" \
    "if true then echo success fi"

# Test 4: Multiple simple commands
test_history_conversion \
    "Multiple commands" \
    "echo first
echo second
echo third" \
    "echo first echo second echo third"

# Test 5: Command with quotes
test_history_conversion \
    "Quoted strings" \
    "echo 'line one'
echo \"line two\"" \
    "echo 'line one' echo \"line two\""

# Test 6: Brace group
test_history_conversion \
    "Brace group" \
    "{
    echo inside
    echo braces
}" \
    "{ echo inside echo braces }"

echo "=== History File Contents ==="
if [ -f "$HISTORY_FILE" ]; then
    echo "Full history file:"
    cat "$HISTORY_FILE" | nl
else
    echo "No history file found"
fi

echo
echo "=== Manual Verification ==="
echo "To manually test history recall:"
echo "1. Run: $SHELL_PATH"
echo "2. Press UP ARROW multiple times to see history entries"
echo "3. Verify that multiline commands appear as single lines"
echo "4. Check that they are editable and executable"

echo
echo "=== Analysis ==="
if [ -f "$HISTORY_FILE" ]; then
    local total_entries
    total_entries=$(wc -l < "$HISTORY_FILE")
    echo "Total history entries: $total_entries"

    local multiline_entries
    multiline_entries=$(grep -c $'\n' "$HISTORY_FILE" 2>/dev/null || echo "0")
    echo "Entries with embedded newlines: $multiline_entries"

    if [ "$multiline_entries" -eq 0 ]; then
        echo -e "${GREEN}✓ Good: No embedded newlines found in history${NC}"
    else
        echo -e "${RED}✗ Issue: Found entries with embedded newlines${NC}"
    fi

    echo
    echo "Longest line length:"
    awk '{print length}' "$HISTORY_FILE" | sort -nr | head -1

    echo "Average line length:"
    awk '{total += length} END {print int(total/NR)}' "$HISTORY_FILE"
else
    echo -e "${RED}No history file to analyze${NC}"
fi

echo
echo "Verification complete!"
