#!/bin/bash
# UTF-8/Grapheme Manual Testing Helper
# Generates test inputs and provides verification commands

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

echo_test() {
    echo -e "${YELLOW}[TEST]${NC} $*"
}

echo_pass() {
    echo -e "${GREEN}[PASS]${NC} $*"
}

echo_fail() {
    echo -e "${RED}[FAIL]${NC} $*"
}

# Function to show byte sequence
show_bytes() {
    local str="$1"
    echo -n "$str" | od -A n -t x1 | tr -d ' \n'
}

# Function to count bytes
count_bytes() {
    local str="$1"
    echo -n "$str" | wc -c
}

echo_info "Phase 1 UTF-8/Grapheme Manual Testing Helper"
echo_info "=============================================="
echo ""

# Level 1: Basic UTF-8 Input
echo_test "LEVEL 1: Basic UTF-8 Input Tests"
echo ""

echo_test "Test 1.1: Single Multi-Byte Character (é)"
TEST_CHAR="é"
echo "  Character: $TEST_CHAR"
echo "  Bytes: $(show_bytes "$TEST_CHAR")"
echo "  Byte count: $(count_bytes "$TEST_CHAR")"
echo "  Expected: 2 bytes (c3 a9), 1 grapheme"
echo "  To test: Type '$TEST_CHAR' in lusush, press backspace"
echo ""

echo_test "Test 1.2: 3-Byte Character (日)"
TEST_CHAR="日"
echo "  Character: $TEST_CHAR"
echo "  Bytes: $(show_bytes "$TEST_CHAR")"
echo "  Byte count: $(count_bytes "$TEST_CHAR")"
echo "  Expected: 3 bytes (e6 97 a5), 1 grapheme, 2 columns wide"
echo "  To test: Type '$TEST_CHAR' in lusush, check cursor position"
echo ""

echo_test "Test 1.3: 4-Byte Character (🎉)"
TEST_CHAR="🎉"
echo "  Character: $TEST_CHAR"
echo "  Bytes: $(show_bytes "$TEST_CHAR")"
echo "  Byte count: $(count_bytes "$TEST_CHAR")"
echo "  Expected: 4 bytes (f0 9f 8e 89), 1 grapheme, 2 columns wide"
echo "  To test: Type '$TEST_CHAR' in lusush, press backspace"
echo ""

echo_test "Test 1.4: Mixed ASCII and UTF-8 (Hello 世界)"
TEST_STR="Hello 世界"
echo "  String: $TEST_STR"
echo "  Bytes: $(show_bytes "$TEST_STR")"
echo "  Byte count: $(count_bytes "$TEST_STR")"
echo "  Expected: 13 bytes, 8 graphemes, 10 display columns"
echo "  To test: Type '$TEST_STR' in lusush, check cursor shows position 8"
echo ""

# Level 2: Combining Characters
echo_test "LEVEL 2: Combining Characters Tests"
echo ""

echo_test "Test 2.1: Single Combining Mark (e + acute)"
# e + combining acute accent
TEST_CHAR="é"  # Note: This might be precomposed, need decomposed form
echo "  Character: $TEST_CHAR (precomposed for display)"
echo "  Decomposed form: e + U+0301"
echo "  To generate: printf 'e\\u0301'"
DECOMPOSED=$(printf 'e\u0301')
echo "  Bytes: $(show_bytes "$DECOMPOSED")"
echo "  Expected: 1 grapheme cluster (base + combining mark)"
echo "  To test: Type 'e' then combining accent, backspace should delete both"
echo ""

echo_test "Test 2.2: Multiple Combining Marks"
# e + grave + circumflex
MULTI_COMBINING=$(printf 'e\u0300\u0302')
echo "  Character: $MULTI_COMBINING"
echo "  Bytes: $(show_bytes "$MULTI_COMBINING")"
echo "  Expected: 1 grapheme cluster (e + 2 combining marks)"
echo "  To test: Arrow keys should skip entire cluster"
echo ""

echo_test "Test 2.3: Devanagari Sequence (क्ष)"
TEST_CHAR="क्ष"
echo "  Character: $TEST_CHAR"
echo "  Bytes: $(show_bytes "$TEST_CHAR")"
echo "  Expected: 1 grapheme cluster (ka + virama + sha)"
echo "  To test: Cursor should treat as single unit"
echo ""

# Level 3: Emoji Sequences
echo_test "LEVEL 3: Emoji Sequences Tests"
echo ""

echo_test "Test 3.1: Family Emoji (ZWJ Sequence)"
TEST_CHAR="👨‍👩‍👧‍👦"
echo "  Character: $TEST_CHAR"
echo "  Bytes: $(show_bytes "$TEST_CHAR")"
echo "  Byte count: $(count_bytes "$TEST_CHAR")"
echo "  Expected: 1 grapheme cluster (ZWJ sequence)"
echo "  To test: Backspace should delete entire family emoji"
echo ""

echo_test "Test 3.2: Flag Emoji (🇺🇸)"
TEST_CHAR="🇺🇸"
echo "  Character: $TEST_CHAR"
echo "  Bytes: $(show_bytes "$TEST_CHAR")"
echo "  Byte count: $(count_bytes "$TEST_CHAR")"
echo "  Expected: 1 grapheme cluster (Regional Indicator pair)"
echo "  To test: Should display as single flag"
echo ""

echo_test "Test 3.3: Skin Tone Modifier (👋🏽)"
TEST_CHAR="👋🏽"
echo "  Character: $TEST_CHAR"
echo "  Bytes: $(show_bytes "$TEST_CHAR")"
echo "  Byte count: $(count_bytes "$TEST_CHAR")"
echo "  Expected: 1 grapheme cluster (base + modifier)"
echo "  To test: Backspace removes entire modified emoji"
echo ""

# Generate test input file
echo_info "Generating test input file: utf8_test_inputs.txt"
cat > utf8_test_inputs.txt <<'EOF'
# Level 1: Basic UTF-8
é
日
🎉
Hello 世界

# Level 2: Combining Characters
EOF

printf 'e\u0301\n' >> utf8_test_inputs.txt
printf 'e\u0300\u0302\n' >> utf8_test_inputs.txt
echo 'क्ष' >> utf8_test_inputs.txt

cat >> utf8_test_inputs.txt <<'EOF'

# Level 3: Emoji Sequences
👨‍👩‍👧‍👦
🇺🇸
👋🏽
EOF

echo_pass "Test input file created: utf8_test_inputs.txt"
echo ""

echo_info "Ready to start manual testing"
echo_info "Run: ./build/lusush"
echo_info "Then copy/paste test inputs from above or utf8_test_inputs.txt"
echo ""
echo_info "Record results in: docs/development/PHASE1_TEST_RESULTS.md"
