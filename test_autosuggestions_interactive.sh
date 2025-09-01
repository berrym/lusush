#!/bin/bash
#
# Interactive Autosuggestions Test for Lusush Fish-like Features
# This script provides a comprehensive test of the autosuggestions system
# specifically designed to work on Linux and verify fish-like behavior
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

LUSUSH_BINARY="./builddir/lusush"

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

print_banner() {
    echo -e "${BOLD}${BLUE}"
    echo "=================================================================="
    echo "         Lusush Fish-like Autosuggestions Interactive Test"
    echo "=================================================================="
    echo -e "${NC}"
    echo "This test will verify the autosuggestions functionality works"
    echo "properly on Linux with the feature/fish-enhancements branch."
    echo ""
}

print_test_instructions() {
    echo -e "${BOLD}${YELLOW}INTERACTIVE TEST INSTRUCTIONS:${NC}"
    echo ""
    echo "You will be presented with several test scenarios to verify"
    echo "that fish-like autosuggestions are working correctly:"
    echo ""
    echo "1. History-based suggestions (gray text appears as you type)"
    echo "2. Right arrow or Ctrl+F to accept full suggestion"
    echo "3. Ctrl+Right arrow to accept word-by-word"
    echo "4. Suggestions should appear after typing 2+ characters"
    echo "5. Suggestions should be context-aware and relevant"
    echo ""
    echo -e "${CYAN}Key bindings to test:${NC}"
    echo "  • Right Arrow (→)     - Accept full suggestion"
    echo "  • Ctrl+F              - Accept full suggestion"
    echo "  • Ctrl+Right Arrow    - Accept next word only"
    echo "  • Continue typing     - Dismiss suggestion"
    echo ""
    echo -e "${YELLOW}What to look for:${NC}"
    echo "  • Gray text appearing after your cursor"
    echo "  • Suggestions based on command history"
    echo "  • Smooth acceptance with right arrow/Ctrl+F"
    echo "  • Word-by-word acceptance with Ctrl+Right"
    echo ""
}

setup_test_history() {
    log_step "Setting up test history for autosuggestions..."

    # Create a temporary history file with useful commands
    local temp_history=$(mktemp)
    cat > "$temp_history" << 'EOF'
echo "Hello World"
ls -la /usr/bin
git status
git add .
git commit -m "test commit"
for i in 1 2 3; do echo "Number: $i"; done
cd /tmp
pwd
history | tail -10
theme set dark
theme list
find . -name "*.c" -type f
grep -r "function" src/
make clean && make
./builddir/lusush --help
ps aux | grep lusush
kill -9 1234
chmod +x script.sh
tar -czf backup.tar.gz *
wget https://example.com/file.txt
ssh user@server.com
scp file.txt user@server:/tmp/
rsync -av /local/ /remote/
EOF

    export HISTFILE="$temp_history"
    log_success "Test history created with common commands"
}

cleanup_test() {
    if [[ -n "${HISTFILE:-}" ]] && [[ -f "$HISTFILE" ]]; then
        rm -f "$HISTFILE"
    fi
}

test_autosuggestions_basic() {
    log_step "Starting basic autosuggestions test..."
    echo ""
    echo -e "${BOLD}TEST 1: Basic History Suggestions${NC}"
    echo "Try typing the beginning of these commands and look for gray suggestions:"
    echo ""
    echo -e "${CYAN}Commands to try:${NC}"
    echo "  • Type: echo"
    echo "  • Type: ls -"
    echo "  • Type: git s"
    echo "  • Type: for"
    echo "  • Type: theme"
    echo ""
    echo "Press Enter after testing to continue..."
    echo ""

    # Start lusush in interactive mode
    HISTFILE="$HISTFILE" $LUSUSH_BINARY -i

    read -p "Did you see gray autosuggestions appear as you typed? (y/n): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        log_success "Basic autosuggestions test passed!"
        return 0
    else
        log_warning "Basic autosuggestions test may have issues"
        return 1
    fi
}

test_suggestion_acceptance() {
    log_step "Testing suggestion acceptance mechanisms..."
    echo ""
    echo -e "${BOLD}TEST 2: Suggestion Acceptance${NC}"
    echo "Test the different ways to accept suggestions:"
    echo ""
    echo -e "${CYAN}Test sequence:${NC}"
    echo "1. Type: 'ec' (should suggest 'echo \"Hello World\"')"
    echo "2. Press Right Arrow (→) to accept full suggestion"
    echo "3. Press Enter to execute"
    echo ""
    echo "4. Type: 'git s' (should suggest 'git status')"
    echo "5. Press Ctrl+F to accept full suggestion"
    echo "6. Press Enter to execute"
    echo ""
    echo "Press Enter after testing to continue..."
    echo ""

    HISTFILE="$HISTFILE" $LUSUSH_BINARY -i

    read -p "Were you able to accept suggestions with Right Arrow and Ctrl+F? (y/n): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        log_success "Suggestion acceptance test passed!"
        return 0
    else
        log_warning "Suggestion acceptance test may have issues"
        return 1
    fi
}

test_word_acceptance() {
    log_step "Testing word-by-word suggestion acceptance..."
    echo ""
    echo -e "${BOLD}TEST 3: Word-by-word Acceptance${NC}"
    echo "Test accepting suggestions one word at a time:"
    echo ""
    echo -e "${CYAN}Test sequence:${NC}"
    echo "1. Type: 'l' (should suggest 'ls -la /usr/bin')"
    echo "2. Press Ctrl+Right Arrow to accept just 'ls'"
    echo "3. Press Ctrl+Right Arrow again to accept '-la'"
    echo "4. Continue until you have the full command"
    echo ""
    echo "Press Enter after testing to continue..."
    echo ""

    HISTFILE="$HISTFILE" $LUSUSH_BINARY -i

    read -p "Were you able to accept suggestions word-by-word with Ctrl+Right Arrow? (y/n): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        log_success "Word-by-word acceptance test passed!"
        return 0
    else
        log_warning "Word-by-word acceptance test may have issues"
        return 1
    fi
}

test_suggestion_context() {
    log_step "Testing contextual suggestion intelligence..."
    echo ""
    echo -e "${BOLD}TEST 4: Context-Aware Suggestions${NC}"
    echo "Test that suggestions are intelligent and context-aware:"
    echo ""
    echo -e "${CYAN}Test scenarios:${NC}"
    echo "1. Type: 'git' (should suggest git commands from history)"
    echo "2. Type: 'echo' (should suggest echo commands)"
    echo "3. Type: 'th' (should suggest 'theme' commands)"
    echo "4. Type random text that's not in history (no suggestions should appear)"
    echo ""
    echo "Press Enter after testing to continue..."
    echo ""

    HISTFILE="$HISTFILE" $LUSUSH_BINARY -i

    read -p "Did suggestions appear contextually relevant to what you typed? (y/n): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        log_success "Contextual suggestions test passed!"
        return 0
    else
        log_warning "Contextual suggestions test may have issues"
        return 1
    fi
}

test_visual_appearance() {
    log_step "Testing visual appearance and formatting..."
    echo ""
    echo -e "${BOLD}TEST 5: Visual Appearance${NC}"
    echo "Verify the visual aspects of autosuggestions:"
    echo ""
    echo -e "${CYAN}What to verify:${NC}"
    echo "• Suggestions appear in gray color (not interfering with command)"
    echo "• Suggestions appear immediately after cursor position"
    echo "• Suggestions don't corrupt the display or prompt"
    echo "• Cursor position remains correct after accepting suggestions"
    echo "• No visual artifacts or screen corruption"
    echo ""
    echo "Test by typing various commands and observing the display quality."
    echo ""
    echo "Press Enter after testing to continue..."
    echo ""

    HISTFILE="$HISTFILE" $LUSUSH_BINARY -i

    read -p "Did the suggestions appear properly formatted in gray without display issues? (y/n): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        log_success "Visual appearance test passed!"
        return 0
    else
        log_warning "Visual appearance test may have issues"
        return 1
    fi
}

test_performance() {
    log_step "Testing autosuggestions performance..."
    echo ""
    echo -e "${BOLD}TEST 6: Performance${NC}"
    echo "Test that autosuggestions don't cause performance issues:"
    echo ""
    echo -e "${CYAN}Performance test:${NC}"
    echo "• Type rapidly and observe if suggestions keep up"
    echo "• Try typing long commands quickly"
    echo "• Verify there's no noticeable lag or delay"
    echo "• Check that the shell remains responsive"
    echo ""
    echo "Press Enter after testing to continue..."
    echo ""

    HISTFILE="$HISTFILE" $LUSUSH_BINARY -i

    read -p "Did the shell remain responsive with good performance? (y/n): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        log_success "Performance test passed!"
        return 0
    else
        log_warning "Performance test may have issues"
        return 1
    fi
}

run_comprehensive_test() {
    local passed=0
    local total=6

    print_banner
    print_test_instructions

    read -p "Press Enter to begin interactive testing..."
    echo ""

    setup_test_history

    # Run all tests
    test_autosuggestions_basic && ((passed++))
    echo ""
    test_suggestion_acceptance && ((passed++))
    echo ""
    test_word_acceptance && ((passed++))
    echo ""
    test_suggestion_context && ((passed++))
    echo ""
    test_visual_appearance && ((passed++))
    echo ""
    test_performance && ((passed++))

    # Results summary
    echo ""
    echo -e "${BOLD}${BLUE}=================================================================="
    echo "                      TEST RESULTS SUMMARY"
    echo "==================================================================${NC}"
    echo ""
    echo -e "${CYAN}Tests completed: ${BOLD}$passed/$total${NC}"

    if [[ $passed -eq $total ]]; then
        echo ""
        log_success "🎉 ALL AUTOSUGGESTIONS TESTS PASSED!"
        echo ""
        log_info "Fish-like autosuggestions are working perfectly on Linux!"
        echo ""
        echo -e "${GREEN}✓ History-based suggestions working${NC}"
        echo -e "${GREEN}✓ Right arrow/Ctrl+F acceptance working${NC}"
        echo -e "${GREEN}✓ Word-by-word acceptance working${NC}"
        echo -e "${GREEN}✓ Context-aware suggestions working${NC}"
        echo -e "${GREEN}✓ Visual appearance correct${NC}"
        echo -e "${GREEN}✓ Performance acceptable${NC}"
        echo ""
        echo -e "${BOLD}${GREEN}Linux compatibility VERIFIED!${NC}"

    elif [[ $passed -gt $((total/2)) ]]; then
        echo ""
        log_warning "⚠️  Most autosuggestions features working, but some issues detected."
        echo ""
        log_info "The core functionality is working, but some refinements may be needed."

    else
        echo ""
        log_error "❌ Multiple autosuggestions features not working properly."
        echo ""
        log_info "Significant issues detected that need investigation."
    fi

    cleanup_test

    echo ""
    echo -e "${BLUE}Test completed. Check the results above for Linux compatibility status.${NC}"
    echo ""
}

# Handle script arguments
case "${1:-}" in
    "--help"|"-h")
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Interactive test for fish-like autosuggestions in Lusush on Linux"
        echo ""
        echo "This script provides comprehensive interactive testing of:"
        echo "  - History-based autosuggestions"
        echo "  - Suggestion acceptance mechanisms"
        echo "  - Word-by-word suggestion acceptance"
        echo "  - Context-aware suggestion intelligence"
        echo "  - Visual appearance and formatting"
        echo "  - Performance characteristics"
        echo ""
        echo "Options:"
        echo "  --help, -h    Show this help message"
        echo ""
        echo "The test requires manual interaction to verify visual and"
        echo "behavioral aspects of the autosuggestions system."
        exit 0
        ;;
esac

# Verify prerequisites
if [[ ! -f "$LUSUSH_BINARY" ]]; then
    log_error "Lusush binary not found at $LUSUSH_BINARY"
    log_info "Please build Lusush first: meson setup builddir && ninja -C builddir"
    exit 1
fi

if [[ ! -x "$LUSUSH_BINARY" ]]; then
    log_error "Lusush binary is not executable: $LUSUSH_BINARY"
    exit 1
fi

# Check if we're in interactive mode
if [[ ! -t 0 ]] || [[ ! -t 1 ]]; then
    log_error "This test requires an interactive terminal"
    log_info "Please run this script in an interactive shell session"
    exit 1
fi

# Run the comprehensive test
run_comprehensive_test
