/**
 * @file test_executor.c
 * @brief Integration tests for the shell executor
 *
 * These tests exercise the executor through actual command execution,
 * covering builtins, pipelines, control structures, and variable expansion.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#include "executor.h"
#include "symtable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Test framework macros */
#define TEST(name) static void test_##name(void)
#define RUN_TEST(name)                                                         \
    do {                                                                       \
        printf("  Running: %s...\n", #name);                                   \
        test_##name();                                                         \
        printf("    PASSED\n");                                                \
    } while (0)

#define ASSERT(condition, message)                                             \
    do {                                                                       \
        if (!(condition)) {                                                    \
            printf("    FAILED: %s\n", message);                               \
            printf("      at %s:%d\n", __FILE__, __LINE__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define ASSERT_EQ(actual, expected, message)                                   \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            printf("    FAILED: %s\n", message);                               \
            printf("      Expected: %d, Got: %d\n", (int)(expected),           \
                   (int)(actual));                                             \
            printf("      at %s:%d\n", __FILE__, __LINE__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define ASSERT_STR_EQ(actual, expected, message)                               \
    do {                                                                       \
        const char *_actual = (actual);                                        \
        const char *_expected = (expected);                                    \
        if (_actual == NULL || _expected == NULL) {                            \
            if (_actual != _expected) {                                        \
                printf("    FAILED: %s\n", message);                           \
                printf("      Expected: %s, Got: %s\n",                        \
                       _expected ? _expected : "NULL",                         \
                       _actual ? _actual : "NULL");                            \
                printf("      at %s:%d\n", __FILE__, __LINE__);                \
                exit(1);                                                       \
            }                                                                  \
        } else if (strcmp(_actual, _expected) != 0) {                          \
            printf("    FAILED: %s\n", message);                               \
            printf("      Expected: \"%s\", Got: \"%s\"\n", _expected,         \
                   _actual);                                                   \
            printf("      at %s:%d\n", __FILE__, __LINE__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define ASSERT_NOT_NULL(ptr, message)                                          \
    do {                                                                       \
        if ((ptr) == NULL) {                                                   \
            printf("    FAILED: %s\n", message);                               \
            printf("      at %s:%d\n", __FILE__, __LINE__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

/* ============================================================================
 * LIFECYCLE TESTS
 * ============================================================================ */

TEST(executor_new_free) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new should return non-NULL");
    ASSERT_EQ(exec->exit_status, 0, "Initial exit status should be 0");
    ASSERT(!exec->has_error, "Should not have error initially");
    executor_free(exec);
}

TEST(executor_with_symtable) {
    symtable_manager_t *mgr = symtable_manager_new();
    ASSERT_NOT_NULL(mgr, "symtable_manager_new failed");
    
    executor_t *exec = executor_new_with_symtable(mgr);
    ASSERT_NOT_NULL(exec, "executor_new_with_symtable failed");
    ASSERT(exec->symtable == mgr, "Symtable should be set");
    
    executor_free(exec);
    symtable_manager_free(mgr);
}

/* ============================================================================
 * SIMPLE COMMAND TESTS
 * ============================================================================ */

TEST(execute_true) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "true");
    ASSERT_EQ(status, 0, "true should return 0");
    ASSERT_EQ(exec->exit_status, 0, "Exit status should be 0");
    
    executor_free(exec);
}

TEST(execute_false) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "false");
    ASSERT_EQ(status, 1, "false should return 1");
    ASSERT_EQ(exec->exit_status, 1, "Exit status should be 1");
    
    executor_free(exec);
}

TEST(execute_colon) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, ":");
    ASSERT_EQ(status, 0, ": (colon) should return 0");
    
    executor_free(exec);
}

TEST(execute_exit_status) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "true");
    (void)executor_execute_command_line(exec, "echo $?");
    ASSERT_EQ(exec->exit_status, 0, "echo should succeed");
    
    executor_free(exec);
}

/* ============================================================================
 * VARIABLE TESTS
 * ============================================================================ */

TEST(variable_assignment) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "FOO=bar");
    ASSERT_EQ(status, 0, "Assignment should succeed");
    
    /* Verify variable was set */
    char *value = symtable_get_var(exec->symtable, "FOO");
    ASSERT_NOT_NULL(value, "Variable should be set");
    ASSERT_STR_EQ(value, "bar", "Variable value mismatch");
    free(value);
    
    executor_free(exec);
}

TEST(variable_expansion) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "MYVAR=hello");
    
    /* Test that variable exists */
    char *value = symtable_get_var(exec->symtable, "MYVAR");
    ASSERT_NOT_NULL(value, "Variable should be set");
    ASSERT_STR_EQ(value, "hello", "Variable value mismatch");
    free(value);
    
    executor_free(exec);
}

TEST(multiple_assignments) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "A=1");
    executor_execute_command_line(exec, "B=2");
    executor_execute_command_line(exec, "C=3");
    
    char *a = symtable_get_var(exec->symtable, "A");
    char *b = symtable_get_var(exec->symtable, "B");
    char *c = symtable_get_var(exec->symtable, "C");
    
    ASSERT_STR_EQ(a, "1", "A should be 1");
    ASSERT_STR_EQ(b, "2", "B should be 2");
    ASSERT_STR_EQ(c, "3", "C should be 3");
    
    free(a);
    free(b);
    free(c);
    executor_free(exec);
}

/* ============================================================================
 * CONTROL STRUCTURE TESTS
 * ============================================================================ */

TEST(if_true_branch) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, 
        "if true; then RESULT=yes; else RESULT=no; fi");
    ASSERT_EQ(status, 0, "if statement should succeed");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "yes", "Should take true branch");
    free(result);
    
    executor_free(exec);
}

TEST(if_false_branch) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, 
        "if false; then RESULT=yes; else RESULT=no; fi");
    ASSERT_EQ(status, 0, "if statement should succeed");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "no", "Should take false branch");
    free(result);
    
    executor_free(exec);
}

TEST(for_loop_basic) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, 
        "COUNT=0; for i in 1 2 3; do COUNT=$((COUNT+1)); done");
    ASSERT_EQ(status, 0, "for loop should succeed");
    
    char *count = symtable_get_var(exec->symtable, "COUNT");
    ASSERT_NOT_NULL(count, "COUNT should be set");
    ASSERT_STR_EQ(count, "3", "Should iterate 3 times");
    free(count);
    
    executor_free(exec);
}

TEST(for_loop_no_in) {
    /* Tests Issue #55 fix - for without 'in' iterates over $@ */
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    /* Set positional parameters and iterate */
    int status = executor_execute_command_line(exec,
        "set -- a b c; COUNT=0; for arg; do COUNT=$((COUNT+1)); done");
    ASSERT_EQ(status, 0, "for loop without 'in' should succeed");
    
    char *count = symtable_get_var(exec->symtable, "COUNT");
    ASSERT_NOT_NULL(count, "COUNT should be set");
    ASSERT_STR_EQ(count, "3", "Should iterate over 3 positional params");
    free(count);
    
    executor_free(exec);
}

TEST(while_loop) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec,
        "N=0; while [ $N -lt 5 ]; do N=$((N+1)); done");
    ASSERT_EQ(status, 0, "while loop should succeed");
    
    char *n = symtable_get_var(exec->symtable, "N");
    ASSERT_NOT_NULL(n, "N should be set");
    ASSERT_STR_EQ(n, "5", "Should count to 5");
    free(n);
    
    executor_free(exec);
}

TEST(until_loop) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec,
        "N=0; until [ $N -ge 3 ]; do N=$((N+1)); done");
    ASSERT_EQ(status, 0, "until loop should succeed");
    
    char *n = symtable_get_var(exec->symtable, "N");
    ASSERT_NOT_NULL(n, "N should be set");
    ASSERT_STR_EQ(n, "3", "Should count to 3");
    free(n);
    
    executor_free(exec);
}

TEST(case_statement) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec,
        "X=foo; case $X in foo) RESULT=matched;; bar) RESULT=bar;; esac");
    ASSERT_EQ(status, 0, "case statement should succeed");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "matched", "Should match foo pattern");
    free(result);
    
    executor_free(exec);
}

TEST(case_wildcard) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec,
        "X=unknown; case $X in foo) RESULT=foo;; *) RESULT=default;; esac");
    ASSERT_EQ(status, 0, "case statement should succeed");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "default", "Should match wildcard");
    free(result);
    
    executor_free(exec);
}

/* ============================================================================
 * LOGICAL OPERATOR TESTS
 * ============================================================================ */

TEST(and_operator_success) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "true && RESULT=yes");
    ASSERT_EQ(status, 0, "&& with true should succeed");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "yes", "Second command should run");
    free(result);
    
    executor_free(exec);
}

TEST(and_operator_fail) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    /* Set RESULT first, then verify it's NOT changed */
    executor_execute_command_line(exec, "RESULT=initial");
    int status = executor_execute_command_line(exec, "false && RESULT=changed");
    ASSERT_EQ(status, 1, "&& with false should fail");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should still be set");
    ASSERT_STR_EQ(result, "initial", "Second command should NOT run");
    free(result);
    
    executor_free(exec);
}

TEST(or_operator_success) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    /* First succeeds, second should not run */
    executor_execute_command_line(exec, "RESULT=initial");
    int status = executor_execute_command_line(exec, "true || RESULT=changed");
    ASSERT_EQ(status, 0, "|| with true should succeed");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "initial", "Second command should NOT run");
    free(result);
    
    executor_free(exec);
}

TEST(or_operator_fail) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "false || RESULT=yes");
    ASSERT_EQ(status, 0, "|| should run second after first fails");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "yes", "Second command should run");
    free(result);
    
    executor_free(exec);
}

/* ============================================================================
 * FUNCTION TESTS
 * ============================================================================ */

TEST(function_definition_posix) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "myfunc() { CALLED=yes; }");
    ASSERT_EQ(status, 0, "Function definition should succeed");
    
    status = executor_execute_command_line(exec, "myfunc");
    ASSERT_EQ(status, 0, "Function call should succeed");
    
    char *called = symtable_get_var(exec->symtable, "CALLED");
    ASSERT_NOT_NULL(called, "CALLED should be set");
    ASSERT_STR_EQ(called, "yes", "Function should have been called");
    free(called);
    
    executor_free(exec);
}

TEST(function_definition_ksh) {
    /* Tests Issue #56 fix - function without parentheses */
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "function myfunc { CALLED=yes; }");
    ASSERT_EQ(status, 0, "ksh-style function definition should succeed");
    
    status = executor_execute_command_line(exec, "myfunc");
    ASSERT_EQ(status, 0, "Function call should succeed");
    
    char *called = symtable_get_var(exec->symtable, "CALLED");
    ASSERT_NOT_NULL(called, "CALLED should be set");
    ASSERT_STR_EQ(called, "yes", "Function should have been called");
    free(called);
    
    executor_free(exec);
}

TEST(function_with_args) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "setarg() { ARG1=$1; ARG2=$2; }");
    int status = executor_execute_command_line(exec, "setarg hello world");
    ASSERT_EQ(status, 0, "Function call should succeed");
    
    char *arg1 = symtable_get_var(exec->symtable, "ARG1");
    char *arg2 = symtable_get_var(exec->symtable, "ARG2");
    ASSERT_STR_EQ(arg1, "hello", "ARG1 should be hello");
    ASSERT_STR_EQ(arg2, "world", "ARG2 should be world");
    free(arg1);
    free(arg2);
    
    executor_free(exec);
}

TEST(function_return) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "retfunc() { return 42; }");
    int status = executor_execute_command_line(exec, "retfunc");
    ASSERT_EQ(status, 42, "Function should return 42");
    
    executor_free(exec);
}

/* ============================================================================
 * ARITHMETIC TESTS
 * ============================================================================ */

TEST(arithmetic_basic) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "RESULT=$((2 + 3))");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "5", "2 + 3 should equal 5");
    free(result);
    
    executor_free(exec);
}

TEST(arithmetic_multiply) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "RESULT=$((4 * 5))");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "20", "4 * 5 should equal 20");
    free(result);
    
    executor_free(exec);
}

TEST(arithmetic_variable) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "X=10");
    executor_execute_command_line(exec, "Y=20");
    executor_execute_command_line(exec, "RESULT=$((X + Y))");
    
    char *result = symtable_get_var(exec->symtable, "RESULT");
    ASSERT_NOT_NULL(result, "RESULT should be set");
    ASSERT_STR_EQ(result, "30", "10 + 20 should equal 30");
    free(result);
    
    executor_free(exec);
}

TEST(arithmetic_increment) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "N=5");
    executor_execute_command_line(exec, "N=$((N + 1))");
    
    char *n = symtable_get_var(exec->symtable, "N");
    ASSERT_NOT_NULL(n, "N should be set");
    ASSERT_STR_EQ(n, "6", "5 + 1 should equal 6");
    free(n);
    
    executor_free(exec);
}

/* ============================================================================
 * SUBSHELL AND GROUPING TESTS
 * ============================================================================ */

TEST(subshell_isolation) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "OUTER=yes");
    /* Variable set in subshell should not affect parent */
    executor_execute_command_line(exec, "(INNER=subshell)");
    
    char *outer = symtable_get_var(exec->symtable, "OUTER");
    ASSERT_NOT_NULL(outer, "OUTER should be set");
    ASSERT_STR_EQ(outer, "yes", "OUTER should be yes");
    free(outer);
    
    /* INNER should not exist in parent */
    char *inner = symtable_get_var(exec->symtable, "INNER");
    ASSERT(inner == NULL, "INNER should NOT be set in parent");
    
    executor_free(exec);
}

TEST(brace_group) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    /* Brace group runs in current shell */
    int status = executor_execute_command_line(exec, "{ A=1; B=2; }");
    ASSERT_EQ(status, 0, "Brace group should succeed");
    
    char *a = symtable_get_var(exec->symtable, "A");
    char *b = symtable_get_var(exec->symtable, "B");
    ASSERT_STR_EQ(a, "1", "A should be 1");
    ASSERT_STR_EQ(b, "2", "B should be 2");
    free(a);
    free(b);
    
    executor_free(exec);
}

/* ============================================================================
 * TEST COMMAND ([) TESTS
 * ============================================================================ */

TEST(test_string_equal) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "[ foo = foo ]");
    ASSERT_EQ(status, 0, "[ foo = foo ] should be true");
    
    status = executor_execute_command_line(exec, "[ foo = bar ]");
    ASSERT_EQ(status, 1, "[ foo = bar ] should be false");
    
    executor_free(exec);
}

TEST(test_string_not_equal) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "[ foo != bar ]");
    ASSERT_EQ(status, 0, "[ foo != bar ] should be true");
    
    status = executor_execute_command_line(exec, "[ foo != foo ]");
    ASSERT_EQ(status, 1, "[ foo != foo ] should be false");
    
    executor_free(exec);
}

TEST(test_numeric_compare) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    ASSERT_EQ(executor_execute_command_line(exec, "[ 5 -eq 5 ]"), 0, "-eq should work");
    ASSERT_EQ(executor_execute_command_line(exec, "[ 5 -ne 3 ]"), 0, "-ne should work");
    ASSERT_EQ(executor_execute_command_line(exec, "[ 5 -gt 3 ]"), 0, "-gt should work");
    ASSERT_EQ(executor_execute_command_line(exec, "[ 5 -ge 5 ]"), 0, "-ge should work");
    ASSERT_EQ(executor_execute_command_line(exec, "[ 3 -lt 5 ]"), 0, "-lt should work");
    ASSERT_EQ(executor_execute_command_line(exec, "[ 3 -le 3 ]"), 0, "-le should work");
    
    executor_free(exec);
}

TEST(test_string_empty) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "[ -z '' ]");
    ASSERT_EQ(status, 0, "[ -z '' ] should be true");
    
    status = executor_execute_command_line(exec, "[ -z 'notempty' ]");
    ASSERT_EQ(status, 1, "[ -z 'notempty' ] should be false");
    
    status = executor_execute_command_line(exec, "[ -n 'notempty' ]");
    ASSERT_EQ(status, 0, "[ -n 'notempty' ] should be true");
    
    executor_free(exec);
}

/* ============================================================================
 * BUILTIN TESTS
 * ============================================================================ */

TEST(builtin_export) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "export MYEXPORT=value");
    ASSERT_EQ(status, 0, "export should succeed");
    
    char *value = symtable_get_var(exec->symtable, "MYEXPORT");
    ASSERT_NOT_NULL(value, "MYEXPORT should be set");
    ASSERT_STR_EQ(value, "value", "Value should be 'value'");
    free(value);
    
    executor_free(exec);
}

TEST(builtin_unset) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "TOUNSET=exists");
    char *before = symtable_get_var(exec->symtable, "TOUNSET");
    ASSERT_NOT_NULL(before, "Variable should exist before unset");
    free(before);
    
    executor_execute_command_line(exec, "unset TOUNSET");
    char *after = symtable_get_var(exec->symtable, "TOUNSET");
    ASSERT(after == NULL, "Variable should not exist after unset");
    
    executor_free(exec);
}

TEST(builtin_readonly) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "readonly MYCONST=constant");
    ASSERT_EQ(status, 0, "readonly should succeed");
    
    char *value = symtable_get_var(exec->symtable, "MYCONST");
    ASSERT_NOT_NULL(value, "MYCONST should be set");
    ASSERT_STR_EQ(value, "constant", "Value should be 'constant'");
    free(value);
    
    executor_free(exec);
}

TEST(builtin_eval) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "eval 'EVALED=yes'");
    ASSERT_EQ(status, 0, "eval should succeed");
    
    char *value = symtable_get_var(exec->symtable, "EVALED");
    ASSERT_NOT_NULL(value, "EVALED should be set");
    ASSERT_STR_EQ(value, "yes", "Value should be 'yes'");
    free(value);
    
    executor_free(exec);
}

TEST(builtin_shift) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    /* Test shift with positional parameters */
    executor_execute_command_line(exec, "set -- a b c d e");
    executor_execute_command_line(exec, "FIRST=$1");
    executor_execute_command_line(exec, "shift");
    executor_execute_command_line(exec, "AFTER=$1");
    
    char *first = symtable_get_var(exec->symtable, "FIRST");
    char *after = symtable_get_var(exec->symtable, "AFTER");
    
    ASSERT_STR_EQ(first, "a", "First should be 'a'");
    ASSERT_STR_EQ(after, "b", "After shift, $1 should be 'b'");
    
    free(first);
    free(after);
    executor_free(exec);
}

TEST(builtin_set_positional) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    executor_execute_command_line(exec, "set -- one two three");
    executor_execute_command_line(exec, "P1=$1; P2=$2; P3=$3");
    
    char *p1 = symtable_get_var(exec->symtable, "P1");
    char *p2 = symtable_get_var(exec->symtable, "P2");
    char *p3 = symtable_get_var(exec->symtable, "P3");
    
    ASSERT_STR_EQ(p1, "one", "$1 should be 'one'");
    ASSERT_STR_EQ(p2, "two", "$2 should be 'two'");
    ASSERT_STR_EQ(p3, "three", "$3 should be 'three'");
    
    free(p1);
    free(p2);
    free(p3);
    executor_free(exec);
}

/* ============================================================================
 * COMMAND LIST TESTS
 * ============================================================================ */

TEST(command_list_semicolon) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec, "A=1; B=2; C=3");
    ASSERT_EQ(status, 0, "Command list should succeed");
    
    char *a = symtable_get_var(exec->symtable, "A");
    char *b = symtable_get_var(exec->symtable, "B");
    char *c = symtable_get_var(exec->symtable, "C");
    
    ASSERT_STR_EQ(a, "1", "A should be 1");
    ASSERT_STR_EQ(b, "2", "B should be 2");
    ASSERT_STR_EQ(c, "3", "C should be 3");
    
    free(a);
    free(b);
    free(c);
    executor_free(exec);
}

/* ============================================================================
 * BREAK/CONTINUE TESTS
 * ============================================================================ */

TEST(loop_break) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    int status = executor_execute_command_line(exec,
        "N=0; for i in 1 2 3 4 5; do N=$((N+1)); if [ $N -eq 3 ]; then break; fi; done");
    ASSERT_EQ(status, 0, "Loop with break should succeed");
    
    char *n = symtable_get_var(exec->symtable, "N");
    ASSERT_NOT_NULL(n, "N should be set");
    ASSERT_STR_EQ(n, "3", "Loop should break at 3");
    free(n);
    
    executor_free(exec);
}

TEST(loop_continue) {
    executor_t *exec = executor_new();
    ASSERT_NOT_NULL(exec, "executor_new failed");
    
    /* Count only odd numbers: skip even iterations */
    int status = executor_execute_command_line(exec,
        "SUM=0; for i in 1 2 3 4 5; do "
        "if [ $((i % 2)) -eq 0 ]; then continue; fi; "
        "SUM=$((SUM + i)); done");
    ASSERT_EQ(status, 0, "Loop with continue should succeed");
    
    char *sum = symtable_get_var(exec->symtable, "SUM");
    ASSERT_NOT_NULL(sum, "SUM should be set");
    ASSERT_STR_EQ(sum, "9", "Sum of 1+3+5 should be 9");
    free(sum);
    
    executor_free(exec);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

/* Forward declarations for initialization */
extern void init_symtable(void);
extern void free_global_symtable(void);

int main(void) {
    printf("Running executor integration tests...\n\n");
    
    /* Initialize global symbol table - required for executor_new() */
    init_symtable();
    
    printf("Lifecycle tests:\n");
    RUN_TEST(executor_new_free);
    RUN_TEST(executor_with_symtable);
    
    printf("\nSimple command tests:\n");
    RUN_TEST(execute_true);
    RUN_TEST(execute_false);
    RUN_TEST(execute_colon);
    RUN_TEST(execute_exit_status);
    
    printf("\nVariable tests:\n");
    RUN_TEST(variable_assignment);
    RUN_TEST(variable_expansion);
    RUN_TEST(multiple_assignments);
    
    printf("\nControl structure tests:\n");
    RUN_TEST(if_true_branch);
    RUN_TEST(if_false_branch);
    RUN_TEST(for_loop_basic);
    RUN_TEST(for_loop_no_in);
    RUN_TEST(while_loop);
    RUN_TEST(until_loop);
    RUN_TEST(case_statement);
    RUN_TEST(case_wildcard);
    
    printf("\nLogical operator tests:\n");
    RUN_TEST(and_operator_success);
    RUN_TEST(and_operator_fail);
    RUN_TEST(or_operator_success);
    RUN_TEST(or_operator_fail);
    
    printf("\nFunction tests:\n");
    RUN_TEST(function_definition_posix);
    RUN_TEST(function_definition_ksh);
    RUN_TEST(function_with_args);
    RUN_TEST(function_return);
    
    printf("\nArithmetic tests:\n");
    RUN_TEST(arithmetic_basic);
    RUN_TEST(arithmetic_multiply);
    RUN_TEST(arithmetic_variable);
    RUN_TEST(arithmetic_increment);
    
    printf("\nSubshell and grouping tests:\n");
    RUN_TEST(subshell_isolation);
    RUN_TEST(brace_group);
    
    printf("\nTest command tests:\n");
    RUN_TEST(test_string_equal);
    RUN_TEST(test_string_not_equal);
    RUN_TEST(test_numeric_compare);
    RUN_TEST(test_string_empty);
    
    printf("\nBuiltin tests:\n");
    RUN_TEST(builtin_export);
    RUN_TEST(builtin_unset);
    RUN_TEST(builtin_readonly);
    RUN_TEST(builtin_eval);
    RUN_TEST(builtin_shift);
    RUN_TEST(builtin_set_positional);
    
    printf("\nCommand list tests:\n");
    RUN_TEST(command_list_semicolon);
    
    printf("\nBreak/continue tests:\n");
    RUN_TEST(loop_break);
    RUN_TEST(loop_continue);
    
    printf("\n========================================\n");
    printf("All executor integration tests PASSED!\n");
    printf("========================================\n");
    
    /* Cleanup global symbol table */
    free_global_symtable();
    
    return 0;
}
