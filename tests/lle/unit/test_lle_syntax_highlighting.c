/**
 * @file test_lle_syntax_highlighting.c
 * @brief Unit tests for spec-compliant LLE Syntax Highlighting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lle/syntax_highlighting.h"

/* Test framework macros */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) do { printf("Test: %s... ", name); tests_run++; } while(0)
#define TEST_PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define TEST_FAIL(msg) do { tests_failed++; printf("FAIL: %s\n", msg); } while(0)

/* Stubs for lusush core functions */
bool is_builtin(const char *name) {
    static const char *builtins[] = {
        "cd", "echo", "exit", "export", "alias", "unalias", "history",
        "jobs", "fg", "bg", "kill", "pwd", "source", "set", "unset",
        "config", "display", "theme", "ehistory", "type", "help",
        "read", "test", "true", "false", "exec", "eval", "shift",
        "return", "break", "continue", "wait", "trap", "umask",
        NULL
    };
    for (int i = 0; builtins[i]; i++) {
        if (strcmp(name, builtins[i]) == 0) return true;
    }
    return false;
}

char *lookup_alias(const char *name) {
    (void)name;
    return NULL;  /* No aliases in test */
}

/* Token type to string for debugging */
static const char *token_type_str(lle_syntax_token_type_t type) {
    switch (type) {
        case LLE_TOKEN_COMMAND_VALID: return "COMMAND_VALID";
        case LLE_TOKEN_COMMAND_INVALID: return "COMMAND_INVALID";
        case LLE_TOKEN_COMMAND_BUILTIN: return "COMMAND_BUILTIN";
        case LLE_TOKEN_COMMAND_ALIAS: return "COMMAND_ALIAS";
        case LLE_TOKEN_KEYWORD: return "KEYWORD";
        case LLE_TOKEN_STRING_SINGLE: return "STRING_SINGLE";
        case LLE_TOKEN_STRING_DOUBLE: return "STRING_DOUBLE";
        case LLE_TOKEN_VARIABLE: return "VARIABLE";
        case LLE_TOKEN_VARIABLE_SPECIAL: return "VARIABLE_SPECIAL";
        case LLE_TOKEN_PIPE: return "PIPE";
        case LLE_TOKEN_REDIRECT: return "REDIRECT";
        case LLE_TOKEN_COMMENT: return "COMMENT";
        case LLE_TOKEN_OPTION: return "OPTION";
        case LLE_TOKEN_ARGUMENT: return "ARGUMENT";
        case LLE_TOKEN_WHITESPACE: return "WHITESPACE";
        default: return "OTHER";
    }
}

/* Test helper: get first non-whitespace token type */
static lle_syntax_token_type_t get_first_command_type(lle_syntax_highlighter_t *h, const char *input) {
    lle_syntax_highlight(h, input, strlen(input));
    size_t count;
    const lle_syntax_token_t *tokens = lle_syntax_get_tokens(h, &count);
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type != LLE_TOKEN_WHITESPACE) {
            return tokens[i].type;
        }
    }
    return LLE_TOKEN_UNKNOWN;
}

/* Test: Highlighter creation */
static void test_highlighter_create(void) {
    TEST_START("highlighter_create");
    lle_syntax_highlighter_t *h = NULL;
    int rc = lle_syntax_highlighter_create(&h);
    if (rc == 0 && h != NULL) {
        lle_syntax_highlighter_destroy(h);
        TEST_PASS();
    } else {
        TEST_FAIL("failed to create highlighter");
    }
}

/* Test: Builtin detection */
static void test_builtins(void) {
    lle_syntax_highlighter_t *h = NULL;
    lle_syntax_highlighter_create(&h);
    
    /* Classic builtins */
    TEST_START("builtin: cd");
    if (get_first_command_type(h, "cd") == LLE_TOKEN_COMMAND_BUILTIN) TEST_PASS();
    else TEST_FAIL("cd not detected as builtin");
    
    TEST_START("builtin: echo");
    if (get_first_command_type(h, "echo hello") == LLE_TOKEN_COMMAND_BUILTIN) TEST_PASS();
    else TEST_FAIL("echo not detected as builtin");
    
    /* Lusush-specific builtins */
    TEST_START("builtin: config");
    if (get_first_command_type(h, "config") == LLE_TOKEN_COMMAND_BUILTIN) TEST_PASS();
    else TEST_FAIL("config not detected as builtin");
    
    TEST_START("builtin: display");
    if (get_first_command_type(h, "display") == LLE_TOKEN_COMMAND_BUILTIN) TEST_PASS();
    else TEST_FAIL("display not detected as builtin");
    
    TEST_START("builtin: theme");
    if (get_first_command_type(h, "theme") == LLE_TOKEN_COMMAND_BUILTIN) TEST_PASS();
    else TEST_FAIL("theme not detected as builtin");
    
    TEST_START("builtin: ehistory");
    if (get_first_command_type(h, "ehistory") == LLE_TOKEN_COMMAND_BUILTIN) TEST_PASS();
    else TEST_FAIL("ehistory not detected as builtin");
    
    lle_syntax_highlighter_destroy(h);
}

/* Test: External commands */
static void test_external_commands(void) {
    lle_syntax_highlighter_t *h = NULL;
    lle_syntax_highlighter_create(&h);
    
    TEST_START("external: ls");
    lle_syntax_token_type_t type = get_first_command_type(h, "ls");
    if (type == LLE_TOKEN_COMMAND_VALID) TEST_PASS();
    else { printf("(got %s) ", token_type_str(type)); TEST_FAIL("ls not detected as valid command"); }
    
    TEST_START("external: grep");
    type = get_first_command_type(h, "grep foo");
    if (type == LLE_TOKEN_COMMAND_VALID) TEST_PASS();
    else { printf("(got %s) ", token_type_str(type)); TEST_FAIL("grep not detected as valid command"); }
    
    lle_syntax_highlighter_destroy(h);
}

/* Test: Invalid commands */
static void test_invalid_commands(void) {
    lle_syntax_highlighter_t *h = NULL;
    lle_syntax_highlighter_create(&h);
    
    TEST_START("invalid: notarealcmd123");
    lle_syntax_token_type_t type = get_first_command_type(h, "notarealcmd123");
    if (type == LLE_TOKEN_COMMAND_INVALID) TEST_PASS();
    else { printf("(got %s) ", token_type_str(type)); TEST_FAIL("fake command not detected as invalid"); }
    
    lle_syntax_highlighter_destroy(h);
}

/* Test: Keywords */
static void test_keywords(void) {
    lle_syntax_highlighter_t *h = NULL;
    lle_syntax_highlighter_create(&h);
    
    TEST_START("keyword: if");
    if (get_first_command_type(h, "if") == LLE_TOKEN_KEYWORD) TEST_PASS();
    else TEST_FAIL("if not detected as keyword");
    
    TEST_START("keyword: for");
    if (get_first_command_type(h, "for") == LLE_TOKEN_KEYWORD) TEST_PASS();
    else TEST_FAIL("for not detected as keyword");
    
    TEST_START("keyword: while");
    if (get_first_command_type(h, "while") == LLE_TOKEN_KEYWORD) TEST_PASS();
    else TEST_FAIL("while not detected as keyword");
    
    lle_syntax_highlighter_destroy(h);
}

/* Test: Pipes and operators */
static void test_operators(void) {
    lle_syntax_highlighter_t *h = NULL;
    lle_syntax_highlighter_create(&h);
    
    TEST_START("pipe in 'ls | grep'");
    lle_syntax_highlight(h, "ls | grep foo", 13);
    size_t count;
    const lle_syntax_token_t *tokens = lle_syntax_get_tokens(h, &count);
    bool found_pipe = false;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == LLE_TOKEN_PIPE) found_pipe = true;
    }
    if (found_pipe) TEST_PASS();
    else TEST_FAIL("pipe not detected");
    
    lle_syntax_highlighter_destroy(h);
}

/* Test: Variables */
static void test_variables(void) {
    lle_syntax_highlighter_t *h = NULL;
    lle_syntax_highlighter_create(&h);
    
    TEST_START("variable: $HOME");
    lle_syntax_highlight(h, "echo $HOME", 10);
    size_t count;
    const lle_syntax_token_t *tokens = lle_syntax_get_tokens(h, &count);
    bool found_var = false;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == LLE_TOKEN_VARIABLE) found_var = true;
    }
    if (found_var) TEST_PASS();
    else TEST_FAIL("variable not detected");
    
    TEST_START("special variable: $?");
    lle_syntax_highlight(h, "echo $?", 7);
    tokens = lle_syntax_get_tokens(h, &count);
    bool found_special = false;
    for (size_t i = 0; i < count; i++) {
        if (tokens[i].type == LLE_TOKEN_VARIABLE_SPECIAL) found_special = true;
    }
    if (found_special) TEST_PASS();
    else TEST_FAIL("special variable not detected");
    
    lle_syntax_highlighter_destroy(h);
}

/* Test: ANSI rendering */
static void test_ansi_render(void) {
    lle_syntax_highlighter_t *h = NULL;
    lle_syntax_highlighter_create(&h);
    
    TEST_START("ANSI render output");
    lle_syntax_highlight(h, "echo hello", 10);
    char output[1024];
    int len = lle_syntax_render_ansi(h, "echo hello", output, sizeof(output));
    if (len > 0 && strlen(output) > 0) {
        printf("(rendered %d bytes) ", len);
        TEST_PASS();
    } else {
        TEST_FAIL("render returned no output");
    }
    
    lle_syntax_highlighter_destroy(h);
}

int main(void) {
    printf("=== LLE Syntax Highlighting Unit Tests ===\n\n");
    
    test_highlighter_create();
    test_builtins();
    test_external_commands();
    test_invalid_commands();
    test_keywords();
    test_operators();
    test_variables();
    test_ansi_render();
    
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed (of %d)\n", tests_passed, tests_failed, tests_run);
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
