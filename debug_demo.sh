#!/usr/bin/env lusush

# Comprehensive Lusush Debugger Demo Script
# This script demonstrates all debugger features

echo "=== Lusush Interactive Debugger Demo ==="
echo "This script demonstrates breakpoints, variable inspection, and debugging features"
echo

# Set some variables for debugging
USER_NAME="Alice"
PROJECT_NAME="Lusush Shell Debugger"
VERSION=1.0
DEBUG_LEVEL=3

echo "Initializing variables:"
echo "  USER_NAME = $USER_NAME"
echo "  PROJECT_NAME = $PROJECT_NAME"
echo "  VERSION = $VERSION"
echo "  DEBUG_LEVEL = $DEBUG_LEVEL"
echo

# Simple function to demonstrate function debugging
calculate_result() {
    local base=$1
    local multiplier=$2
    local result=$((base * multiplier + 10))
    echo "Function calculation: $base * $multiplier + 10 = $result"
    return 0
}

echo "=== Function Call Test ==="
calculate_result 5 3
calculate_result 7 2
echo

# Loop demonstration for step debugging
echo "=== Loop Debugging Test ==="
for count in 1 2 3 4 5; do
    echo "Loop iteration: $count"
    status="processing"
    if [ $count -eq 3 ]; then
        status="critical_point"
        echo "  -> Critical point reached at iteration $count"
    fi
    echo "  -> Status: $status"
done
echo

# Conditional logic for breakpoint testing
echo "=== Conditional Logic Test ==="
test_value=42
if [ $test_value -gt 40 ]; then
    echo "Test value $test_value is greater than 40"
    category="high"
elif [ $test_value -gt 20 ]; then
    echo "Test value $test_value is medium (20-40)"
    category="medium"
else
    echo "Test value $test_value is low (under 20)"
    category="low"
fi
echo "Final category: $category"
echo

# Array-like variable handling
echo "=== Variable Inspection Test ==="
FILES="file1.txt file2.txt file3.txt"
echo "Processing files: $FILES"

for file in $FILES; do
    echo "Processing: $file"
    size=$((RANDOM % 1000 + 100))
    echo "  File size: ${size} bytes"
done
echo

# Error handling scenario
echo "=== Error Handling Test ==="
error_count=0
for test_case in "pass" "fail" "pass" "error" "pass"; do
    echo "Running test case: $test_case"
    case $test_case in
        "pass")
            echo "  ✓ Test passed"
            ;;
        "fail")
            echo "  ✗ Test failed"
            error_count=$((error_count + 1))
            ;;
        "error")
            echo "  ⚠ Test error encountered"
            error_count=$((error_count + 1))
            ;;
    esac
done
echo "Total errors encountered: $error_count"
echo

echo "=== Demo Complete ==="
echo "Script finished successfully!"
echo "Total variables created: USER_NAME, PROJECT_NAME, VERSION, DEBUG_LEVEL, status, category, FILES, error_count"
echo "Functions defined: calculate_result"
echo
echo "To debug this script, try:"
echo "  debug on"
echo "  debug break add debug_demo.sh 25    # Break at function definition"
echo "  debug break add debug_demo.sh 35    # Break in loop"
echo "  debug break add debug_demo.sh 45    # Break at conditional"
echo "  source debug_demo.sh"
echo
echo "During debugging, use:"
echo "  vars              # Show all variables"
echo "  print USER_NAME   # Inspect specific variable"
echo "  stack             # Show call stack"
echo "  step              # Step through execution"
echo "  continue          # Continue to next breakpoint"
