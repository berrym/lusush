#!/bin/bash

# LUSUSH SUBSHELL DEMONSTRATION
# Hands-on examples showing how subshells work in practice

echo "=== LUSUSH SUBSHELL DEMONSTRATION ==="
echo "This script demonstrates subshell behavior interactively"
echo "Shell: ./builddir/lusush"
echo

# Function to run a test and show results
run_test() {
    local description="$1"
    local command="$2"

    echo "--- $description ---"
    echo "Command: $command"
    echo "Output:"
    echo "$command" | ./builddir/lusush 2>&1
    echo
}

echo "=== 1. BASIC SUBSHELL ISOLATION ==="
echo "Variables changed in subshells don't affect the parent"

run_test "Variable isolation" '
x=10
echo "Before subshell: x=$x"
( x=100; echo "Inside subshell: x=$x" )
echo "After subshell: x=$x"
'

echo "=== 2. DIRECTORY CHANGES ==="
echo "Directory changes in subshells are isolated"

run_test "Directory isolation" '
echo "Current directory: $(pwd)"
( cd /tmp; echo "Inside subshell: $(pwd)" )
echo "Back in parent: $(pwd)"
'

echo "=== 3. COMMAND SUBSTITUTION CREATES SUBSHELLS ==="
echo "Command substitution runs in a subshell"

run_test "Command substitution" '
x=20
result=$(x=200; echo "x=$x in subshell"; echo "RESULT")
echo "Captured: $result"
echo "Parent x still: $x"
'

echo "=== 4. SUBSHELLS VS BRACE GROUPS ==="
echo "Compare subshells ( ) with brace groups { }"

run_test "Subshell (isolated)" '
y=30
( y=300; echo "In subshell: y=$y" )
echo "After subshell: y=$y"
'

run_test "Brace group (shared)" '
y=30
{ y=400; echo "In brace group: y=$y"; }
echo "After brace group: y=$y"
'

echo "=== 5. EXIT STATUS PROPAGATION ==="
echo "Subshell exit status becomes command result"

run_test "Successful subshell" '
( echo "Success"; exit 0 )
echo "Exit status: $?"
'

run_test "Failed subshell" '
( echo "Failure"; exit 42 )
echo "Exit status: $?"
'

echo "=== 6. ENVIRONMENT VARIABLES ==="
echo "Environment inheritance and isolation"

run_test "Environment inheritance" '
export GLOBAL_VAR="parent_value"
LOCAL_VAR="local_value"
(
    echo "GLOBAL_VAR in subshell: $GLOBAL_VAR"
    echo "LOCAL_VAR in subshell: $LOCAL_VAR"
    export NEW_VAR="from_subshell"
    MODIFIED_LOCAL="changed"
)
echo "NEW_VAR in parent: $NEW_VAR"
echo "MODIFIED_LOCAL in parent: $MODIFIED_LOCAL"
'

echo "=== 7. PROCESS IDENTIFICATION ==="
echo "Each subshell has its own process ID"

run_test "Process IDs" '
echo "Parent shell PID: $$"
( echo "Subshell PID: $$"; echo "Parent PID: $PPID" )
echo "Back to parent PID: $$"
'

echo "=== 8. NESTED SUBSHELLS ==="
echo "Subshells can contain other subshells"

run_test "Nested subshells" '
level=1
echo "Level $level: PID $$"
(
    level=2
    echo "Level $level: PID $$"
    (
        level=3
        echo "Level $level: PID $$"
    )
    echo "Back to level $level: PID $$"
)
echo "Back to level $level: PID $$"
'

echo "=== 9. ERROR HANDLING ==="
echo "Errors in subshells can be contained"

run_test "Error containment" '
set -e  # Exit on error in parent
echo "Before risky operation"
(
    echo "In subshell, about to fail"
    false  # This will make subshell exit
    echo "This will not be reached"
) || echo "Subshell failed, but parent continues"
echo "Parent continues after subshell failure"
'

echo "=== 10. PRACTICAL PATTERNS ==="
echo "Real-world subshell usage patterns"

run_test "Safe directory operations" '
echo "Working in: $(pwd)"
(
    cd /tmp 2>/dev/null || cd /
    echo "Temporarily in: $(pwd)"
    echo "Creating test file"
    echo "test data" > subshell_test.txt 2>/dev/null || true
    ls subshell_test.txt 2>/dev/null || echo "Could not create file"
)
echo "Back in: $(pwd)"
'

run_test "Configuration testing" '
test_config() {
    (
        CONFIG_VAR="test_value"
        REQUIRED_SETTING="enabled"

        # Validate configuration
        if [ -n "$CONFIG_VAR" ] && [ "$REQUIRED_SETTING" = "enabled" ]; then
            echo "Configuration valid"
            exit 0
        else
            echo "Configuration invalid"
            exit 1
        fi
    ) && echo "Config test PASSED" || echo "Config test FAILED"
}
test_config
'

echo "=== 11. PERFORMANCE CONSIDERATIONS ==="
echo "Understanding the cost of subshells"

run_test "Multiple subshells" '
echo "Creating multiple subshells..."
for i in 1 2 3; do
    ( echo "Subshell $i: PID $$" )
done
echo "All subshells completed"
'

echo "=== 12. PIPELINE SUBSHELLS ==="
echo "Pipeline components run in subshells"

run_test "Pipeline isolation" '
counter=0
echo "data1 data2 data3" | {
    counter=100
    while read item; do
        echo "Processing: $item (counter=$counter)"
        counter=$((counter + 1))
    done
    echo "Final counter in pipeline: $counter"
}
echo "Counter in parent: $counter"
'

echo "=== DEMONSTRATION COMPLETE ==="
echo
echo "Key Takeaways:"
echo "1. Subshells provide complete isolation from parent shell"
echo "2. Variables changed in subshells don't affect parent"
echo "3. Directory changes in subshells are temporary"
echo "4. Each subshell is a separate process with its own PID"
echo "5. Command substitution automatically creates subshells"
echo "6. Subshells inherit parent environment but changes don't propagate back"
echo "7. Exit status of subshell becomes the command result"
echo "8. Use subshells for isolation, avoid for performance-critical code"
echo
echo "This demonstrates lusush's robust subshell implementation!"
