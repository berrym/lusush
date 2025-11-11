#!/bin/bash
# Interactive UTF-8 Test - Shows you what to type and lets you copy/paste

set -euo pipefail

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}=========================================="
echo "Phase 1 UTF-8 Interactive Test"
echo -e "==========================================${NC}"
echo ""
echo "This script will show you test strings that you can:"
echo "  1. Copy and paste into lusush"
echo "  2. Type using Ctrl+Shift+U + hex code"
echo ""

run_test() {
    local num=$1
    local desc=$2
    local test_str=$3
    local hex=$4

    echo -e "${YELLOW}Test $num: $desc${NC}"
    echo -e "${GREEN}Copy this → $test_str${NC}"
    if [ -n "$hex" ]; then
        echo -e "Or type: Ctrl+Shift+U then $hex"
    fi
    echo "Expected: Cursor moves by character, backspace deletes entire character"
    echo ""
    read -p "Press Enter for next test..."
    echo ""
}

run_test "1" "2-byte UTF-8 (é)" "café" "00e9 for é"
run_test "2" "3-byte CJK" "日本" "65e5 for 日"
run_test "3" "4-byte emoji" "🎉" "1f389"
run_test "4" "Complex grapheme (family)" "👨‍👩‍👧‍👦" "(complex, use copy/paste)"
run_test "5" "Flag emoji" "🇺🇸" "(use copy/paste)"
run_test "6" "Mixed content" "Hello 世界" "4e16 754c for 世界"
run_test "7" "Skin tone emoji" "👋🏽" "(use copy/paste)"

echo -e "${GREEN}=========================================="
echo "All test strings shown!"
echo "==========================================${NC}"
echo ""
echo "Now test in lusush:"
echo "  1. ./builddir/lusush"
echo "  2. Paste each test string"
echo "  3. Test LEFT/RIGHT arrows (move by character)"
echo "  4. Test BACKSPACE (delete entire character)"
echo "  5. Type more text to test buffer modifications"
echo ""
echo "Record results in: docs/development/PHASE1_TEST_RESULTS.md"
