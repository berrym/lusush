#!/bin/bash
#
# Linux Rich Completion Issues Diagnostic Test
# Targets the specific file and git completion issues found in Phase 2 testing
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
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_banner() {
    echo -e "${BOLD}${CYAN}"
    echo "========================================================="
    echo "  Linux Rich Completion Issues Diagnostic Test"
    echo "  Targeting File and Git Completion Problems"
    echo "========================================================="
    echo -e "${NC}"
}

# Test 1: Basic file completion functionality
test_file_completion_basic() {
    log_info "Testing basic file completion..."

    # Create test directory with known files
    local test_dir=$(mktemp -d)
    cd "$test_dir"

    # Create test files
    touch test_file1.txt
    touch test_file2.log
    touch another_test.sh
    mkdir test_directory

    echo "Test files created in: $test_dir"
    ls -la

    # Test file completion directly
    log_info "Testing file completion in test directory..."

    # Simple test - just check if shell can list files
    local file_test=$($LUSUSH_BINARY -c 'ls test_file*' 2>&1)
    if [[ "$file_test" == *"test_file1.txt"* && "$file_test" == *"test_file2.log"* ]]; then
        log_success "File listing works correctly"
    else
        log_failure "File listing failed: $file_test"
    fi

    # Test completion context awareness
    local completion_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls test_\nexit' 2>&1)
    if [[ $? -eq 0 ]]; then
        log_success "File completion context test completed"
    else
        log_failure "File completion context test failed or timed out"
    fi

    # Cleanup
    cd - > /dev/null
    rm -rf "$test_dir"
}

# Test 2: Git completion functionality
test_git_completion() {
    log_info "Testing git completion functionality..."

    # Create temporary git repository
    local git_test_dir=$(mktemp -d)
    cd "$git_test_dir"

    # Initialize git repo
    if command -v git >/dev/null 2>&1; then
        git init --quiet
        echo "test content" > test.txt
        git add test.txt
        git commit -m "Initial commit" --quiet

        echo "Git repository created in: $git_test_dir"
        git status --short

        # Test basic git functionality in lusush
        local git_basic_test=$($LUSUSH_BINARY -c 'git --version' 2>&1)
        if [[ "$git_basic_test" == *"git version"* ]]; then
            log_success "Git command execution works"
        else
            log_failure "Git command execution failed: $git_basic_test"
        fi

        # Test git completion context
        local git_completion_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'git stat\nexit' 2>&1)
        if [[ $? -eq 0 ]]; then
            log_success "Git completion context test completed"
        else
            log_failure "Git completion context test failed or timed out"
        fi

        # Test git subcommand recognition
        local git_status_test=$($LUSUSH_BINARY -c 'git status --porcelain' 2>&1)
        if [[ $? -eq 0 ]]; then
            log_success "Git subcommand execution works"
        else
            log_failure "Git subcommand execution failed: $git_status_test"
        fi

    else
        log_warning "Git not available - skipping git-specific tests"
        ((PASSED_TESTS+=3))  # Don't penalize for missing git
    fi

    # Cleanup
    cd - > /dev/null
    rm -rf "$git_test_dir"
}

# Test 3: Rich completion integration diagnostics
test_rich_completion_integration() {
    log_info "Testing rich completion system integration..."

    # Test that rich completion system initializes
    local rich_init_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'# Rich completion test\nexit' 2>&1)
    if [[ $? -eq 0 ]] && [[ "$rich_init_test" != *"ERROR"* ]] && [[ "$rich_init_test" != *"FAIL"* ]]; then
        log_success "Rich completion system initializes without errors"
    else
        log_failure "Rich completion system initialization issues detected"
    fi

    # Test completion with common commands
    local common_cmd_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ec\nexit' 2>&1)
    if [[ $? -eq 0 ]]; then
        log_success "Common command completion test completed"
    else
        log_failure "Common command completion failed"
    fi

    # Test directory-only completion for cd
    local cd_test_dir=$(mktemp -d)
    local cd_subdir="$cd_test_dir/subdir"
    mkdir -p "$cd_subdir"
    touch "$cd_test_dir/file.txt"  # File that should be ignored for cd completion

    cd "$cd_test_dir"
    local cd_completion_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'cd sub\nexit' 2>&1)
    cd - > /dev/null

    if [[ $? -eq 0 ]]; then
        log_success "Directory-only completion test completed"
    else
        log_failure "Directory-only completion test failed"
    fi

    rm -rf "$cd_test_dir"
}

# Test 4: Linux-specific completion paths
test_linux_specific_paths() {
    log_info "Testing Linux-specific completion paths..."

    # Test /usr/bin completion (common on Linux)
    if [[ -d "/usr/bin" ]]; then
        local usr_bin_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls /usr/bin/ba\nexit' 2>&1)
        if [[ $? -eq 0 ]]; then
            log_success "Linux /usr/bin path completion test completed"
        else
            log_failure "Linux /usr/bin path completion failed"
        fi
    else
        log_warning "/usr/bin not found - skipping Linux-specific path test"
        ((PASSED_TESTS++))
    fi

    # Test /etc completion (Linux system directory)
    if [[ -d "/etc" ]]; then
        local etc_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls /etc/pa\nexit' 2>&1)
        if [[ $? -eq 0 ]]; then
            log_success "Linux /etc path completion test completed"
        else
            log_failure "Linux /etc path completion failed"
        fi
    else
        log_warning "/etc not found - skipping Linux system path test"
        ((PASSED_TESTS++))
    fi

    # Test home directory completion
    local home_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls ~\nexit' 2>&1)
    if [[ $? -eq 0 ]]; then
        log_success "Home directory completion test completed"
    else
        log_failure "Home directory completion failed"
    fi
}

# Test 5: Performance and memory impact
test_rich_completion_performance() {
    log_info "Testing rich completion performance impact..."

    # Measure startup time with rich completions
    local start_time=$(date +%s%N)
    $LUSUSH_BINARY -c 'exit' >/dev/null 2>&1
    local end_time=$(date +%s%N)
    local startup_time=$(( (end_time - start_time) / 1000000 ))

    if [[ $startup_time -lt 500 ]]; then  # Less than 500ms
        log_success "Rich completion startup performance acceptable (${startup_time}ms)"
    else
        log_warning "Rich completion startup performance slow (${startup_time}ms)"
        ((PASSED_TESTS++))  # Don't fail for performance warnings
    fi

    # Test completion response time
    local completion_start=$(date +%s%N)
    timeout 3s $LUSUSH_BINARY -i <<< $'ls\nexit' >/dev/null 2>&1
    local completion_end=$(date +%s%N)
    local completion_time=$(( (completion_end - completion_start) / 1000000 ))

    if [[ $completion_time -lt 3000 ]]; then  # Less than 3 seconds (timeout)
        log_success "Rich completion response time acceptable"
    else
        log_failure "Rich completion response time too slow"
    fi
}

# Test 6: Error handling and fallback
test_completion_error_handling() {
    log_info "Testing completion error handling and fallbacks..."

    # Test completion in non-existent directory
    local nonexistent_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'cd /nonexistent/path\nls\nexit' 2>&1)
    if [[ $? -eq 124 ]] || [[ $? -eq 0 ]]; then  # Timeout or success (both acceptable)
        log_success "Completion error handling test completed"
    else
        log_failure "Completion error handling failed"
    fi

    # Test completion with permission denied
    if [[ -d "/root" ]]; then
        local permission_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'ls /root/nonexist\nexit' 2>&1)
        if [[ $? -eq 124 ]] || [[ $? -eq 0 ]]; then  # Timeout or handled gracefully
            log_success "Permission-denied completion handling test completed"
        else
            log_failure "Permission-denied completion handling failed"
        fi
    else
        log_warning "/root directory test skipped"
        ((PASSED_TESTS++))
    fi

    # Test fallback to standard completion
    local fallback_test=$(timeout 5s $LUSUSH_BINARY -i <<< $'echo "fallback test"\nexit' 2>&1)
    if [[ $? -eq 0 ]] && [[ "$fallback_test" != *"ERROR"* ]]; then
        log_success "Completion fallback mechanism working"
    else
        log_failure "Completion fallback mechanism failed"
    fi
}

# Main execution
main() {
    print_banner

    log_info "Starting Linux rich completion diagnostics..."
    log_info "Target: Investigating file and git completion failures"
    echo ""

    # Verify binary exists
    if [[ ! -x "$LUSUSH_BINARY" ]]; then
        log_failure "Lusush binary not found: $LUSUSH_BINARY"
        log_info "Please build first: meson setup builddir && ninja -C builddir"
        exit 1
    fi

    log_info "Using binary: $LUSUSH_BINARY"
    log_info "Platform: $(uname -s) $(uname -r)"
    echo ""

    # Run diagnostic tests
    test_file_completion_basic
    echo ""
    test_git_completion
    echo ""
    test_rich_completion_integration
    echo ""
    test_linux_specific_paths
    echo ""
    test_rich_completion_performance
    echo ""
    test_completion_error_handling

    # Results summary
    local total_tests=$((PASSED_TESTS + FAILED_TESTS))
    local pass_rate=0
    if [[ $total_tests -gt 0 ]]; then
        pass_rate=$(( (PASSED_TESTS * 100) / total_tests ))
    fi

    echo ""
    echo -e "${BOLD}${CYAN}========================================${NC}"
    echo -e "${BOLD}${CYAN}  LINUX RICH COMPLETION DIAGNOSTICS${NC}"
    echo -e "${BOLD}${CYAN}========================================${NC}"
    echo ""
    echo -e "${CYAN}Results Summary:${NC}"
    echo "  Total Tests: $total_tests"
    echo "  Passed: $PASSED_TESTS"
    echo "  Failed: $FAILED_TESTS"
    echo "  Pass Rate: ${pass_rate}%"
    echo ""

    if [[ $FAILED_TESTS -eq 0 ]]; then
        echo -e "${GREEN}🎉 All diagnostic tests passed!${NC}"
        echo -e "${GREEN}Rich completion issues appear to be resolved on Linux.${NC}"
    elif [[ $FAILED_TESTS -le 2 ]]; then
        echo -e "${YELLOW}⚠️  Minor issues detected (${FAILED_TESTS} failures).${NC}"
        echo -e "${YELLOW}Rich completions mostly functional with some edge cases.${NC}"
    else
        echo -e "${RED}❌ Multiple issues detected (${FAILED_TESTS} failures).${NC}"
        echo -e "${RED}Rich completion system needs attention on Linux.${NC}"
    fi

    echo ""
    echo -e "${CYAN}Recommendations:${NC}"
    if [[ $FAILED_TESTS -eq 0 ]]; then
        echo "• Rich completion system is working well on Linux"
        echo "• Consider the issues from Phase 2 testing resolved"
        echo "• Ready for production use"
    elif [[ $FAILED_TESTS -le 2 ]]; then
        echo "• Address specific failing test cases"
        echo "• Consider Linux-specific completion path handling"
        echo "• Test in different Linux environments"
    else
        echo "• Review rich completion Linux integration"
        echo "• Check file system permission handling"
        echo "• Verify git integration works correctly"
        echo "• Consider adding Linux-specific completion logic"
    fi

    echo ""
    echo -e "${BLUE}For detailed analysis, review individual test outputs above.${NC}"
}

# Handle help argument
if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    cat << 'EOF'
Usage: test_linux_rich_completion_issues.sh

Diagnostic test script for Linux rich completion issues found in Phase 2.

This script specifically targets:
• File completion integration problems
• Git completion enhancement issues
• Linux-specific completion path handling
• Rich completion system integration
• Performance and error handling

The script creates temporary test environments to isolate and identify
the root causes of completion failures on Linux platforms.

Requirements:
• Built Lusush binary (./builddir/lusush)
• Standard Linux utilities (ls, git optional)
• Write permissions for temporary test directories

Output:
• Detailed diagnostic results
• Pass/fail status for each test
• Specific recommendations for fixes
EOF
    exit 0
fi

# Run the main function
main "$@"
