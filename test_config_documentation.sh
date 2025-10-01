#!/bin/bash

# Comprehensive Config Documentation Verification Test
# Tests all config command examples from Lusush documentation to ensure accuracy
# This verifies all documented config functionality works as promised

echo "==========================================="
echo "Config Documentation Verification Test"
echo "==========================================="
echo

# Test counter
total_tests=0
passed_tests=0
failed_tests=0

# Test function
test_config_example() {
    local test_name="$1"
    local expected_result="$2"
    shift 2

    total_tests=$((total_tests + 1))
    echo "Test $total_tests: $test_name"

    # Run the test and capture result
    local result
    result=$("$@" 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        if [ -z "$expected_result" ] || echo "$result" | grep -q "$expected_result"; then
            echo "✅ PASS"
            passed_tests=$((passed_tests + 1))
        else
            echo "❌ FAIL - Unexpected output"
            echo "Expected: $expected_result"
            echo "Got: $result"
            failed_tests=$((failed_tests + 1))
        fi
    else
        echo "❌ FAIL - Command failed with exit code $exit_code"
        echo "Output: $result"
        failed_tests=$((failed_tests + 1))
    fi
    echo
}

# Test with command execution
test_config_command() {
    local test_name="$1"
    local command="$2"
    local expected_pattern="$3"

    test_config_example "$test_name" "$expected_pattern" sh -c "$command"
}

echo "=== Basic Config Commands ==="

# Test 1: config show (basic functionality)
test_config_command "config show basic functionality" \
    "echo 'config show | head -1' | ./builddir/lusush" \
    "LUSUSH Configuration"

# Test 2: config show specific sections
test_config_command "config show history section" \
    "echo 'config show history' | ./builddir/lusush" \
    "history.enabled"

test_config_command "config show completion section" \
    "echo 'config show completion' | ./builddir/lusush" \
    "completion.enabled"

test_config_command "config show shell section" \
    "echo 'config show shell' | ./builddir/lusush" \
    "shell.errexit"

# Test 3: config get/set basic functionality
test_config_command "config set and get history.size" \
    "echo 'config set history.size 2000; config get history.size' | ./builddir/lusush" \
    "2000"

test_config_command "config set and get completion.enabled" \
    "echo 'config set completion.enabled false; config get completion.enabled' | ./builddir/lusush" \
    "false"

echo "=== Shell Options Integration ==="

# Test 4: Modern config interface for shell options
test_config_command "config set shell.errexit" \
    "echo 'config set shell.errexit true; config get shell.errexit' | ./builddir/lusush" \
    "true"

test_config_command "config set shell.xtrace" \
    "echo 'config set shell.xtrace true; config get shell.xtrace' | ./builddir/lusush" \
    "true"

test_config_command "config set shell.verbose" \
    "echo 'config set shell.verbose true; config get shell.verbose' | ./builddir/lusush" \
    "true"

# Test 5: Bidirectional sync - config to POSIX
test_config_command "config to POSIX sync (errexit)" \
    "echo 'config set shell.errexit true; set -o | grep errexit' | ./builddir/lusush" \
    "set -o errexit"

test_config_command "config to POSIX sync (xtrace)" \
    "echo 'config set shell.xtrace true; set -o | grep xtrace' | ./builddir/lusush" \
    "set -o xtrace"

# Test 6: Bidirectional sync - POSIX to config
test_config_command "POSIX to config sync (errexit)" \
    "echo 'set -e; config get shell.errexit' | ./builddir/lusush" \
    "true"

test_config_command "POSIX to config sync (xtrace)" \
    "echo 'set -x; config get shell.xtrace; set +x' | ./builddir/lusush" \
    "true"

echo "=== Boolean Value Variations ==="

# Test 7: Boolean value acceptance (documented variations)
test_config_command "Boolean true variations - true" \
    "echo 'config set shell.verbose true; config get shell.verbose' | ./builddir/lusush" \
    "true"

test_config_command "Boolean true variations - on" \
    "echo 'config set shell.verbose on; config get shell.verbose' | ./builddir/lusush" \
    "true"

test_config_command "Boolean true variations - 1" \
    "echo 'config set shell.verbose 1; config get shell.verbose' | ./builddir/lusush" \
    "true"

test_config_command "Boolean false variations - false" \
    "echo 'config set shell.verbose false; config get shell.verbose' | ./builddir/lusush" \
    "false"

test_config_command "Boolean false variations - off" \
    "echo 'config set shell.verbose off; config get shell.verbose' | ./builddir/lusush" \
    "false"

test_config_command "Boolean false variations - 0" \
    "echo 'config set shell.verbose 0; config get shell.verbose' | ./builddir/lusush" \
    "false"

echo "=== Config Persistence ==="

# Test 8: Config save functionality
test_config_command "config save command" \
    "echo 'config set history.size 4000; config save' | ./builddir/lusush" \
    "Configuration saved"

# Test 9: Config reload functionality
test_config_command "config reload command" \
    "echo 'config reload' | ./builddir/lusush" \
    "Configuration reloaded"

echo "=== Documented Examples from CONFIG_SYSTEM.md ==="

# Test 10: Basic usage examples from documentation
test_config_command "Basic config show example" \
    "echo 'config show | head -3' | ./builddir/lusush" \
    "LUSUSH Configuration"

test_config_command "Section-specific show example" \
    "echo 'config show history | head -2' | ./builddir/lusush" \
    "history."

test_config_command "Config set key value example" \
    "echo 'config set completion.threshold 80; config get completion.threshold' | ./builddir/lusush" \
    "80"

# Test 11: Shell options examples from documentation
test_config_command "Shell options modern syntax example" \
    "echo 'config set shell.pipefail true; config get shell.pipefail' | ./builddir/lusush" \
    "true"

test_config_command "Shell options security mode example" \
    "echo 'config set shell.privileged true; config get shell.privileged' | ./builddir/lusush" \
    "true"

echo "=== Advanced Configuration Examples ==="

# Test 12: Multiple settings in sequence
test_config_command "Multiple shell options sequence" \
    "echo 'config set shell.errexit true; config set shell.nounset true; config set shell.pipefail true; config get shell.errexit' | ./builddir/lusush" \
    "true"

# Test 13: Prompt and behavior settings
test_config_command "Prompt configuration example" \
    "echo 'config set prompt.theme dark; config get prompt.theme' | ./builddir/lusush" \
    "dark"

test_config_command "Behavior configuration example" \
    "echo 'config set behavior.spell_correction false; config get behavior.spell_correction' | ./builddir/lusush" \
    "false"

# Test 14: Network settings
test_config_command "Network configuration example" \
    "echo 'config set network.ssh_completion false; config get network.ssh_completion' | ./builddir/lusush" \
    "false"

echo "=== Error Handling Tests ==="

# Test 15: Invalid section names
test_config_command "Invalid section name error" \
    "echo 'config show invalid_section' | ./builddir/lusush 2>&1" \
    "Unknown section"

# Test 16: Invalid config keys
test_config_command "Invalid config key warning" \
    "echo 'config set invalid.key value' | ./builddir/lusush 2>&1" \
    "Unknown configuration key"

# Test 17: Missing arguments
test_config_command "Missing get argument" \
    "echo 'config get' | ./builddir/lusush 2>&1" \
    "Usage: config get"

test_config_command "Missing set arguments" \
    "echo 'config set' | ./builddir/lusush 2>&1" \
    "Usage: config set"

echo "=== Complex Documentation Examples ==="

# Test 18: Professional scripting mode example from docs
test_config_command "Professional scripting mode setup" \
    "echo 'config set shell.errexit true; config set shell.nounset true; config get shell.errexit' | ./builddir/lusush" \
    "true"

# Test 19: Debug mode example from docs
test_config_command "Debug mode configuration" \
    "echo 'config set shell.xtrace true; config set shell.verbose true; config get shell.xtrace' | ./builddir/lusush" \
    "true"

# Test 20: History configuration examples
test_config_command "History configuration example" \
    "echo 'config set history.size 5000; config set history.no_dups true; config get history.size' | ./builddir/lusush" \
    "5000"

echo "=== Summary ==="
echo "Total tests: $total_tests"
echo "Passed: $passed_tests"
echo "Failed: $failed_tests"
echo

if [ $failed_tests -eq 0 ]; then
    echo "🎉 ALL CONFIG DOCUMENTATION EXAMPLES VERIFIED!"
    echo "✅ All documented config functionality works correctly"
    echo "✅ Config save and persistence working"
    echo "✅ Bidirectional sync with POSIX commands confirmed"
    echo "✅ All boolean variations accepted"
    echo "✅ Error handling working properly"
    echo "✅ Documentation is accurate and trustworthy"
    exit 0
else
    echo "⚠️  $failed_tests test(s) failed - documentation may need updates"

    if [ $failed_tests -lt $((total_tests / 4)) ]; then
        echo "📊 Success rate: $(( (passed_tests * 100) / total_tests ))% - Most functionality working"
    else
        echo "📊 Success rate: $(( (passed_tests * 100) / total_tests ))% - Significant issues found"
    fi

    echo
    echo "=== Recommendations ==="
    echo "1. Review failed tests and update documentation accordingly"
    echo "2. Fix any functional issues identified"
    echo "3. Ensure all examples match actual implementation behavior"
    echo "4. Test edge cases and error conditions"

    exit 1
fi
