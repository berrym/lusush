#!/bin/bash

# Path to lusush binary
LUSUSH_BINARY="~/Lab/c/lusush/builddir/lusush"

echo "=== LUSUSH POSIX OPTIONS TEST SUITE ==="
echo

test_count=0
pass_count=0

# Helper function to run test
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_exit="$3"
    
    test_count=$((test_count + 1))
    echo -n "Test $test_count: $test_name ... "
    
    # Run command and capture exit code
    eval "$command" >/dev/null 2>&1
    actual_exit=$?
    
    if [ "$actual_exit" = "$expected_exit" ]; then
        echo "PASS"
        pass_count=$((pass_count + 1))
    else
        echo "FAIL (expected $expected_exit, got $actual_exit)"
    fi
}

echo "Testing POSIX command-line options:"
echo

# Test -c option (command string)
run_test "-c option basic" "${LUSUSH_BINARY} -c 'echo test'" 0
run_test "-c option with exit" "${LUSUSH_BINARY} -c 'exit 42'" 42

# Test -e option (errexit)
run_test "-e option with success" "${LUSUSH_BINARY} -e -c 'true; echo success'" 0
run_test "-e option with failure" "${LUSUSH_BINARY} -e -c 'false; echo should_not_print'" 1

# Test -x option (xtrace)
run_test "-x option basic" "${LUSUSH_BINARY} -x -c 'echo trace_test'" 0

# Test -n option (syntax check)
run_test "-n option valid syntax" "${LUSUSH_BINARY} -n -c 'echo valid'" 0
run_test "-n option invalid syntax" "${LUSUSH_BINARY} -n -c 'if then'" 2

# Test -v option (verbose)
run_test "-v option" "${LUSUSH_BINARY} -v -c 'echo verbose'" 0

# Test -u option (nounset)
run_test "-u option with set var" "${LUSUSH_BINARY} -u -c 'var=test; echo \$var'" 0
run_test "-u option with unset var" "${LUSUSH_BINARY} -u -c 'echo \$unset_var'" 1

# Test -f option (noglob)
run_test "-f option" "${LUSUSH_BINARY} -f -c 'echo *.md'" 0

# Test -i option (interactive)
run_test "-i option" "${LUSUSH_BINARY} -i -c 'echo interactive'" 0

# Test -l option (login shell)
run_test "-l option" "${LUSUSH_BINARY} -l -c 'echo login'" 0

# Test -s option (read from stdin)
run_test "-s option" "echo 'echo stdin_test' | ${LUSUSH_BINARY} -s" 0

# Test -h option (help/hash)
run_test "-h option" "${LUSUSH_BINARY} -h -c 'echo help'" 0

# Test -m option (monitor/job control)
run_test "-m option" "${LUSUSH_BINARY} -m -c 'echo monitor'" 0

# Test set builtin
run_test "set -e builtin" "${LUSUSH_BINARY} -c 'set -e; true; echo success'" 0
run_test "set +e builtin" "${LUSUSH_BINARY} -c 'set +e; false; echo continued'" 0
run_test "set -x builtin" "${LUSUSH_BINARY} -c 'set -x; echo traced'" 0

# Test combined options
run_test "combined options -ex" "${LUSUSH_BINARY} -ex -c 'echo combined'" 0
run_test "combined options -ve" "${LUSUSH_BINARY} -ve -c 'echo verbose_errexit'" 0

echo
echo "=== TEST RESULTS ==="
echo "Passed: $pass_count/$test_count tests"
if [ "$test_count" -gt 0 ]; then
    echo "Success rate: $((pass_count * 100 / test_count))%"
else
    echo "Success rate: 0% (no tests run)"
fi

if [ "$pass_count" = "$test_count" ]; then
    echo "STATUS: ALL TESTS PASSED"
    exit 0
else
    echo "STATUS: SOME TESTS FAILED" 
    exit 1
fi
