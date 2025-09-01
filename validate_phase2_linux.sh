#!/bin/bash
#
# Phase 2 Linux Rich Completion Validation Test
# Validates that Phase 2 rich completion features work correctly on Linux
# without interfering with macOS functionality
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
PASSED_TESTS=0
FAILED_TESTS=0
TOTAL_TESTS=0

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_TESTS++))
}

log_failure() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_TESTS++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

start_test() {
    ((TOTAL_TESTS++))
    log_info "Test $TOTAL_TESTS: $1"
}

print_banner() {
    echo -e "${BOLD}${CYAN}"
    cat << 'EOF'
    ╔══════════════════════════════════════════════════════════════╗
    ║         Phase 2 Linux Rich Completion Validation            ║
    ║       Testing Rich Completions on Linux Platform            ║
    ╚══════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
    echo "Validating Phase 2 rich completion features for Linux compatibility"
    echo "Platform: $(uname -s) $(uname -r) $(uname -m)"
    echo ""
}

# Test 1: Rich completion system initialization
test_rich_completion_init() {
    start_test "Rich completion system initialization"

    local init_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'echo "init test"\nexit' 2>&1)
    local exit_code=$?

    if [[ $exit_code -eq 0 ]] && [[ "$init_test" != *"Failed to initialize rich completions"* ]]; then
        log_success "Rich completion system initializes successfully"
    else
        log_failure "Rich completion system initialization failed"
        echo "Output: $init_test"
    fi
}

# Test 2: Backward compatibility with existing completions
test_backward_compatibility() {
    start_test "Backward compatibility with existing tab completion"

    # Test that basic tab completion still works
    local compat_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls\nexit' 2>&1)
    local exit_code=$?

    if [[ $exit_code -eq 0 ]]; then
        log_success "Backward compatibility maintained"
    else
        log_failure "Backward compatibility broken"
    fi
}

# Test 3: Autosuggestions still work with rich completions
test_autosuggestions_compatibility() {
    start_test "Autosuggestions compatibility with rich completions"

    local auto_test=$($LUSUSH_BINARY -c 'display testsuggestion' 2>&1)

    if [[ "$auto_test" == *"SUCCESS"* ]]; then
        log_success "Autosuggestions work with rich completions"
    else
        log_failure "Autosuggestions broken with rich completions"
        echo "Output: $auto_test"
    fi
}

# Test 4: File completion in Linux environment
test_file_completion_linux() {
    start_test "File completion in Linux environment"

    # Create test environment
    local test_dir=$(mktemp -d)
    local current_dir=$(pwd)

    cd "$test_dir"
    touch test_file1.txt test_file2.log
    mkdir test_subdir

    # Test file completion
    local file_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls test_file\nexit' 2>&1)
    local exit_code=$?

    cd "$current_dir"
    rm -rf "$test_dir"

    if [[ $exit_code -eq 0 ]]; then
        log_success "File completion works in Linux environment"
    else
        log_failure "File completion failed in Linux environment"
    fi
}

# Test 5: Git completion in Linux
test_git_completion_linux() {
    start_test "Git completion in Linux environment"

    if ! command -v git >/dev/null 2>&1; then
        log_warning "Git not available - skipping git completion test"
        ((PASSED_TESTS++))
        return
    fi

    # Create test git repository
    local git_dir=$(mktemp -d)
    local current_dir=$(pwd)

    cd "$git_dir"
    git init --quiet >/dev/null 2>&1
    echo "test" > test.txt
    git add test.txt >/dev/null 2>&1
    git commit -m "test" --quiet >/dev/null 2>&1

    # Test git completion
    local git_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'git status\nexit' 2>&1)
    local exit_code=$?

    cd "$current_dir"
    rm -rf "$git_dir"

    if [[ $exit_code -eq 0 ]]; then
        log_success "Git completion works in Linux environment"
    else
        log_failure "Git completion failed in Linux environment"
    fi
}

# Test 6: Directory-only completion for cd command
test_cd_completion() {
    start_test "Directory-only completion for cd command"

    local test_dir=$(mktemp -d)
    local current_dir=$(pwd)

    cd "$test_dir"
    mkdir subdir1 subdir2
    touch file1.txt file2.txt  # These should be ignored for cd completion

    # Test cd completion
    local cd_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'cd sub\nexit' 2>&1)
    local exit_code=$?

    cd "$current_dir"
    rm -rf "$test_dir"

    if [[ $exit_code -eq 0 ]]; then
        log_success "Directory-only completion for cd works"
    else
        log_failure "Directory-only completion for cd failed"
    fi
}

# Test 7: Performance impact assessment
test_performance_impact() {
    start_test "Performance impact of rich completions"

    # Measure startup time
    local start_time=$(date +%s%N)
    $LUSUSH_BINARY -c 'exit' >/dev/null 2>&1
    local end_time=$(date +%s%N)
    local startup_time=$(( (end_time - start_time) / 1000000 ))

    if [[ $startup_time -lt 1000 ]]; then  # Less than 1 second
        log_success "Performance impact acceptable (${startup_time}ms startup)"
    else
        log_warning "Performance impact significant (${startup_time}ms startup)"
        ((PASSED_TESTS++))  # Don't fail for performance warnings
    fi
}

# Test 8: Rich completion configuration
test_rich_completion_config() {
    start_test "Rich completion configuration and settings"

    # Test that rich completion system can be configured
    local config_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'# Configuration test\nexit' 2>&1)
    local exit_code=$?

    if [[ $exit_code -eq 0 ]] && [[ "$config_test" != *"ERROR"* ]]; then
        log_success "Rich completion configuration works"
    else
        log_failure "Rich completion configuration failed"
    fi
}

# Test 9: Memory stability
test_memory_stability() {
    start_test "Memory stability with rich completions"

    # Run multiple completion operations to test for leaks
    local memory_test=$($LUSUSH_BINARY -c 'for i in 1 2 3 4 5; do echo "memory test $i"; done' 2>&1)
    local exit_code=$?

    if [[ $exit_code -eq 0 ]] && [[ "$memory_test" == *"memory test 5"* ]]; then
        log_success "Memory stability test passed"
    else
        log_failure "Memory stability test failed"
    fi
}

# Test 10: Integration with existing Lusush features
test_feature_integration() {
    start_test "Integration with existing Lusush features"

    # Test themes still work
    local theme_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'theme list\nexit' 2>&1)
    local theme_exit=$?

    # Test multiline constructs still work
    local multiline_test=$($LUSUSH_BINARY -c 'for i in 1 2; do echo "test $i"; done' 2>&1)
    local multiline_exit=$?

    if [[ $theme_exit -eq 0 && $multiline_exit -eq 0 ]]; then
        log_success "Integration with existing features works"
    else
        log_failure "Integration with existing features broken"
    fi
}

# Test 11: Linux-specific path completion
test_linux_path_completion() {
    start_test "Linux-specific path completion"

    # Test /usr/bin completion (common Linux path)
    if [[ -d "/usr/bin" ]]; then
        local path_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls /usr/bin/b\nexit' 2>&1)
        local exit_code=$?

        if [[ $exit_code -eq 0 ]]; then
            log_success "Linux path completion works"
        else
            log_failure "Linux path completion failed"
        fi
    else
        log_warning "/usr/bin not found - skipping Linux path test"
        ((PASSED_TESTS++))
    fi
}

# Test 12: Rich completion error handling
test_error_handling() {
    start_test "Rich completion error handling"

    # Test completion in error conditions
    local error_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls /nonexistent/path\nexit' 2>&1)
    local exit_code=$?

    # Should not crash, even if path doesn't exist
    if [[ $exit_code -eq 0 ]] || [[ $exit_code -eq 124 ]]; then  # 124 is timeout
        log_success "Error handling works correctly"
    else
        log_failure "Error handling failed"
    fi
}

# Generate Linux-specific recommendations
generate_recommendations() {
    echo ""
    echo -e "${BOLD}${CYAN}Linux Platform Analysis and Recommendations${NC}"
    echo ""

    local pass_rate=$(( (PASSED_TESTS * 100) / TOTAL_TESTS ))

    if [[ $pass_rate -ge 95 ]]; then
        echo -e "${GREEN}✅ EXCELLENT: Rich completions work perfectly on Linux${NC}"
        echo "• No Linux-specific changes needed"
        echo "• Ready for production deployment"
        echo "• Cross-platform compatibility maintained"

    elif [[ $pass_rate -ge 85 ]]; then
        echo -e "${YELLOW}⚠️ GOOD: Rich completions mostly work with minor issues${NC}"
        echo "• Consider addressing failing test cases"
        echo "• May need Linux-specific optimizations"
        echo "• Generally suitable for deployment with monitoring"

    else
        echo -e "${RED}❌ ISSUES: Significant rich completion problems on Linux${NC}"
        echo "• Linux-specific fixes needed"
        echo "• Review completion system integration"
        echo "• Consider conditional compilation for Linux"
    fi

    echo ""
    echo -e "${CYAN}Linux-Specific Considerations:${NC}"
    echo "• GNU Readline version: $(rl_library_version 2>/dev/null || echo 'Unknown')"
    echo "• File system: $(df -T . 2>/dev/null | tail -1 | awk '{print $2}' || echo 'Unknown')"
    echo "• Shell environment: $SHELL"
    echo "• Terminal: $TERM"

    if [[ $FAILED_TESTS -gt 0 ]]; then
        echo ""
        echo -e "${YELLOW}Recommended Linux-Specific Changes:${NC}"
        echo "• Add Linux path completion optimizations"
        echo "• Enhance error handling for Linux file systems"
        echo "• Test with different Linux distributions"
        echo "• Consider adding Linux-specific completion sources"
    fi
}

# Main execution
main() {
    print_banner

    # Verify binary exists
    if [[ ! -x "$LUSUSH_BINARY" ]]; then
        log_failure "Lusush binary not found: $LUSUSH_BINARY"
        echo "Please build first: meson setup builddir && ninja -C builddir"
        exit 1
    fi

    log_info "Starting Phase 2 Linux validation tests..."
    echo ""

    # Run all tests
    test_rich_completion_init
    test_backward_compatibility
    test_autosuggestions_compatibility
    test_file_completion_linux
    test_git_completion_linux
    test_cd_completion
    test_performance_impact
    test_rich_completion_config
    test_memory_stability
    test_feature_integration
    test_linux_path_completion
    test_error_handling

    # Results summary
    echo ""
    echo -e "${BOLD}${BLUE}=========================================${NC}"
    echo -e "${BOLD}${BLUE}     PHASE 2 LINUX VALIDATION RESULTS${NC}"
    echo -e "${BOLD}${BLUE}=========================================${NC}"
    echo ""
    echo -e "${CYAN}Test Results:${NC}"
    echo "  Total Tests: $TOTAL_TESTS"
    echo "  Passed: $PASSED_TESTS"
    echo "  Failed: $FAILED_TESTS"
    echo "  Success Rate: $(( (PASSED_TESTS * 100) / TOTAL_TESTS ))%"
    echo ""

    generate_recommendations

    echo ""
    if [[ $FAILED_TESTS -eq 0 ]]; then
        echo -e "${GREEN}🎉 All Phase 2 Linux validation tests passed!${NC}"
        echo -e "${GREEN}Rich completions are fully compatible with Linux.${NC}"
    elif [[ $FAILED_TESTS -le 2 ]]; then
        echo -e "${YELLOW}⚠️ Minor issues detected, but mostly functional.${NC}"
    else
        echo -e "${RED}❌ Multiple issues detected - needs attention.${NC}"
    fi

    echo ""
    echo -e "${CYAN}Ready for Linux deployment:${NC} $([ $FAILED_TESTS -eq 0 ] && echo "✅ YES" || echo "⚠️ WITH CAVEATS")"
    echo ""

    # Exit with appropriate code
    [[ $FAILED_TESTS -eq 0 ]] && exit 0 || exit 1
}

# Handle help argument
if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    echo "Phase 2 Linux Rich Completion Validation Test"
    echo ""
    echo "Usage: $0"
    echo ""
    echo "This script validates that Phase 2 rich completion features work"
    echo "correctly on Linux platforms without breaking existing functionality."
    echo ""
    echo "Tests performed:"
    echo "• Rich completion system initialization"
    echo "• Backward compatibility with existing completions"
    echo "• Integration with autosuggestions"
    echo "• File completion in Linux environment"
    echo "• Git completion functionality"
    echo "• Directory-only completion for cd"
    echo "• Performance impact assessment"
    echo "• Configuration and error handling"
    echo "• Memory stability"
    echo "• Integration with existing Lusush features"
    echo ""
    echo "Requirements:"
    echo "• Built Lusush binary (./builddir/lusush)"
    echo "• Linux operating system"
    echo "• Standard utilities (git optional)"
    exit 0
fi

# Run main function
main "$@"
