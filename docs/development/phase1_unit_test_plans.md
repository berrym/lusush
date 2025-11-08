# Phase 1: Unit Test Plans
## Comprehensive Testing Strategy for Continuation Prompts

**Document Status**: Phase 1 Design - Complete Test Specifications  
**Created**: 2025-11-08  
**Purpose**: Detailed unit test plans for all components

---

## Overview

This document specifies comprehensive unit tests for all components of the hybrid continuation prompt architecture. Tests are organized by component and cover:

- **Functional correctness**: Does it work as specified?
- **Edge cases**: Does it handle boundary conditions?
- **Error handling**: Does it fail gracefully?
- **Performance**: Does it meet targets?
- **Memory safety**: No leaks or corruption?

**Testing Framework**: C unit testing (likely Check framework or custom)  
**Coverage Target**: >80% for all new code  
**Performance Monitoring**: Yes, with specific targets

---

## Test Organization

```
tests/
├── unit/
│   ├── test_screen_buffer_prefix.c        # Phase 2 tests
│   ├── test_continuation_prompt_layer.c   # Phase 3 tests
│   ├── test_composition_cursor_translation.c  # Phase 4 tests
│   └── test_composition_integration.c     # Phase 4 tests
├── integration/
│   ├── test_full_continuation_flow.c      # Phase 5 tests
│   └── test_lle_integration.c              # Phase 5 tests
└── performance/
    ├── test_composition_perf.c             # Phase 5 tests
    └── test_cursor_translation_perf.c      # Phase 5 tests
```

---

## Phase 2: Screen Buffer Prefix Tests

### Test Suite: screen_buffer_prefix

**File**: `tests/unit/test_screen_buffer_prefix.c`

---

#### Test Group: Prefix Management

```c
START_TEST(test_set_line_prefix_basic) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Set prefix on line 0
    bool result = screen_buffer_set_line_prefix(buf, 0, "> ");
    ck_assert(result == true);
    
    // Verify prefix was set
    const char *prefix = screen_buffer_get_line_prefix(buf, 0);
    ck_assert_str_eq(prefix, "> ");
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_set_line_prefix_various_lengths) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Test different prefix lengths
    screen_buffer_set_line_prefix(buf, 0, "");           // Empty
    ck_assert_str_eq(screen_buffer_get_line_prefix(buf, 0), "");
    
    screen_buffer_set_line_prefix(buf, 1, "> ");        // Short
    ck_assert_str_eq(screen_buffer_get_line_prefix(buf, 1), "> ");
    
    screen_buffer_set_line_prefix(buf, 2, "loop> ");    // Medium
    ck_assert_str_eq(screen_buffer_get_line_prefix(buf, 2), "loop> ");
    
    screen_buffer_set_line_prefix(buf, 3, "function-name> ");  // Long
    ck_assert_str_eq(screen_buffer_get_line_prefix(buf, 3), "function-name> ");
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_set_line_prefix_with_ansi) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Prefix with ANSI color codes
    screen_buffer_set_line_prefix(buf, 0, "\033[32m>\033[0m ");
    
    const char *prefix = screen_buffer_get_line_prefix(buf, 0);
    ck_assert_str_eq(prefix, "\033[32m>\033[0m ");
    
    // Visual width should be 2 (ANSI codes are invisible)
    size_t width = screen_buffer_get_line_prefix_visual_width(buf, 0);
    ck_assert_uint_eq(width, 2);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_clear_line_prefix) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Set prefix
    screen_buffer_set_line_prefix(buf, 0, "loop> ");
    ck_assert(screen_buffer_get_line_prefix(buf, 0) != NULL);
    
    // Clear prefix
    bool result = screen_buffer_clear_line_prefix(buf, 0);
    ck_assert(result == true);
    
    // Verify cleared
    const char *prefix = screen_buffer_get_line_prefix(buf, 0);
    ck_assert(prefix == NULL);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_set_line_prefix_invalid_line) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Line number out of bounds
    bool result = screen_buffer_set_line_prefix(buf, 100, "> ");
    ck_assert(result == false);
    
    // Negative line number (if using signed int)
    result = screen_buffer_set_line_prefix(buf, -1, "> ");
    ck_assert(result == false);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_set_line_prefix_null_buffer) {
    // NULL buffer pointer
    bool result = screen_buffer_set_line_prefix(NULL, 0, "> ");
    ck_assert(result == false);
}
END_TEST

START_TEST(test_set_line_prefix_null_text) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // NULL text (should clear prefix or treat as empty)
    bool result = screen_buffer_set_line_prefix(buf, 0, NULL);
    ck_assert(result == true);  // Or false, depending on design decision
    
    screen_buffer_destroy(buf);
}
END_TEST
```

**Total Tests**: 7  
**Estimated Time**: 1 hour

---

#### Test Group: Visual Width Calculation

```c
START_TEST(test_prefix_visual_width_ascii) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    screen_buffer_set_line_prefix(buf, 0, "> ");
    size_t width = screen_buffer_get_line_prefix_visual_width(buf, 0);
    ck_assert_uint_eq(width, 2);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_prefix_visual_width_ansi) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // ANSI codes: "\033[31m" (5 bytes, 0 visual width)
    //             ">" (1 byte, 1 visual width)
    //             "\033[0m" (4 bytes, 0 visual width)
    //             " " (1 byte, 1 visual width)
    // Total: 11 bytes, 2 visual columns
    screen_buffer_set_line_prefix(buf, 0, "\033[31m>\033[0m ");
    size_t width = screen_buffer_get_line_prefix_visual_width(buf, 0);
    ck_assert_uint_eq(width, 2);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_prefix_visual_width_utf8) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // "é> " - 'é' is 2 bytes but 1 visual column
    screen_buffer_set_line_prefix(buf, 0, "é> ");
    size_t width = screen_buffer_get_line_prefix_visual_width(buf, 0);
    ck_assert_uint_eq(width, 3);  // é(1) + >(1) + space(1)
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_prefix_visual_width_wide_char) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // "中> " - '中' is 3 bytes but 2 visual columns
    screen_buffer_set_line_prefix(buf, 0, "中> ");
    size_t width = screen_buffer_get_line_prefix_visual_width(buf, 0);
    ck_assert_uint_eq(width, 4);  // 中(2) + >(1) + space(1)
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_prefix_visual_width_tab) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // ">\t" - Tab expands to next multiple of 8
    // After '>' (column 1), tab expands 7 columns to reach column 8
    screen_buffer_set_line_prefix(buf, 0, ">\t");
    size_t width = screen_buffer_get_line_prefix_visual_width(buf, 0);
    ck_assert_uint_eq(width, 8);  // >(1) + tab(7)
    
    screen_buffer_destroy(buf);
}
END_TEST
```

**Total Tests**: 5  
**Estimated Time**: 1 hour

---

#### Test Group: Dirty Tracking

```c
START_TEST(test_prefix_dirty_on_set) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Initially not dirty
    ck_assert(screen_buffer_is_line_prefix_dirty(buf, 0) == false);
    
    // Set prefix marks as dirty
    screen_buffer_set_line_prefix(buf, 0, "> ");
    ck_assert(screen_buffer_is_line_prefix_dirty(buf, 0) == true);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_prefix_dirty_independent_of_content) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Set prefix (marks prefix_dirty)
    screen_buffer_set_line_prefix(buf, 0, "> ");
    screen_buffer_clear_line_prefix_dirty(buf, 0);
    
    // Set content (marks content_dirty, NOT prefix_dirty)
    screen_buffer_set_line_content(buf, 0, "hello");
    ck_assert(screen_buffer_is_line_dirty(buf, 0) == true);
    ck_assert(screen_buffer_is_line_prefix_dirty(buf, 0) == false);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_content_dirty_independent_of_prefix) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Set content (marks content_dirty)
    screen_buffer_set_line_content(buf, 0, "hello");
    screen_buffer_clear_line_dirty(buf, 0);
    
    // Set prefix (marks prefix_dirty, NOT content_dirty)
    screen_buffer_set_line_prefix(buf, 0, "> ");
    ck_assert(screen_buffer_is_line_prefix_dirty(buf, 0) == true);
    ck_assert(screen_buffer_is_line_dirty(buf, 0) == false);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_clear_prefix_dirty) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    screen_buffer_set_line_prefix(buf, 0, "> ");
    ck_assert(screen_buffer_is_line_prefix_dirty(buf, 0) == true);
    
    screen_buffer_clear_line_prefix_dirty(buf, 0);
    ck_assert(screen_buffer_is_line_prefix_dirty(buf, 0) == false);
    
    screen_buffer_destroy(buf);
}
END_TEST
```

**Total Tests**: 4  
**Estimated Time**: 45 minutes

---

#### Test Group: Cursor Translation

```c
START_TEST(test_cursor_buffer_to_display_no_prefix) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // No prefix, cursor should map directly
    int display_col = screen_buffer_translate_buffer_to_display_col(buf, 0, 5);
    ck_assert_int_eq(display_col, 5);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_cursor_buffer_to_display_with_prefix) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Prefix is "> " (2 columns)
    screen_buffer_set_line_prefix(buf, 0, "> ");
    
    // Buffer column 0 → display column 2
    int display_col = screen_buffer_translate_buffer_to_display_col(buf, 0, 0);
    ck_assert_int_eq(display_col, 2);
    
    // Buffer column 5 → display column 7
    display_col = screen_buffer_translate_buffer_to_display_col(buf, 0, 5);
    ck_assert_int_eq(display_col, 7);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_cursor_display_to_buffer_with_prefix) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Prefix is "loop> " (6 columns)
    screen_buffer_set_line_prefix(buf, 0, "loop> ");
    
    // Display column 6 → buffer column 0
    int buffer_col = screen_buffer_translate_display_to_buffer_col(buf, 0, 6);
    ck_assert_int_eq(buffer_col, 0);
    
    // Display column 10 → buffer column 4
    buffer_col = screen_buffer_translate_display_to_buffer_col(buf, 0, 10);
    ck_assert_int_eq(buffer_col, 4);
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_cursor_display_to_buffer_in_prefix_area) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Prefix is "loop> " (6 columns)
    screen_buffer_set_line_prefix(buf, 0, "loop> ");
    
    // Display column 3 (inside prefix) → buffer column 0 (clamped)
    int buffer_col = screen_buffer_translate_display_to_buffer_col(buf, 0, 3);
    ck_assert_int_eq(buffer_col, 0);
    
    screen_buffer_destroy(buf);
}
END_TEST
```

**Total Tests**: 4  
**Estimated Time**: 45 minutes

---

#### Test Group: Rendering

```c
START_TEST(test_render_line_with_prefix) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    screen_buffer_set_line_prefix(buf, 0, "> ");
    screen_buffer_set_line_content(buf, 0, "hello");
    
    char output[256];
    bool result = screen_buffer_render_line_with_prefix(buf, 0, output, sizeof(output));
    ck_assert(result == true);
    ck_assert_str_eq(output, "> hello");
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_render_line_without_prefix) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // No prefix set
    screen_buffer_set_line_content(buf, 0, "hello");
    
    char output[256];
    bool result = screen_buffer_render_line_with_prefix(buf, 0, output, sizeof(output));
    ck_assert(result == true);
    ck_assert_str_eq(output, "hello");
    
    screen_buffer_destroy(buf);
}
END_TEST

START_TEST(test_render_multiline_with_prefixes) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    screen_buffer_set_line_prefix(buf, 0, "bash$ ");
    screen_buffer_set_line_content(buf, 0, "for i in 1 2 3");
    
    screen_buffer_set_line_prefix(buf, 1, "for> ");
    screen_buffer_set_line_content(buf, 1, "do");
    
    screen_buffer_set_line_prefix(buf, 2, "for> ");
    screen_buffer_set_line_content(buf, 2, "    echo $i");
    
    char output[1024];
    bool result = screen_buffer_render_multiline_with_prefixes(buf, 0, 3, output, sizeof(output));
    ck_assert(result == true);
    
    // Verify output contains all lines with prefixes
    ck_assert(strstr(output, "bash$ for i in 1 2 3") != NULL);
    ck_assert(strstr(output, "for> do") != NULL);
    ck_assert(strstr(output, "for>     echo $i") != NULL);
    
    screen_buffer_destroy(buf);
}
END_TEST
```

**Total Tests**: 3  
**Estimated Time**: 1 hour

---

#### Test Group: Memory Management

```c
START_TEST(test_prefix_memory_allocation) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Set prefix (allocates memory)
    screen_buffer_set_line_prefix(buf, 0, "loop> ");
    
    // Verify prefix is accessible
    const char *prefix = screen_buffer_get_line_prefix(buf, 0);
    ck_assert(prefix != NULL);
    ck_assert_str_eq(prefix, "loop> ");
    
    screen_buffer_destroy(buf);
    // Should not leak (verify with Valgrind)
}
END_TEST

START_TEST(test_prefix_reallocation) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Set prefix
    screen_buffer_set_line_prefix(buf, 0, "> ");
    
    // Change prefix (should reuse structure)
    screen_buffer_set_line_prefix(buf, 0, "loop> ");
    
    const char *prefix = screen_buffer_get_line_prefix(buf, 0);
    ck_assert_str_eq(prefix, "loop> ");
    
    screen_buffer_destroy(buf);
    // Should not leak (verify with Valgrind)
}
END_TEST

START_TEST(test_prefix_clear_frees_memory) {
    screen_buffer_t *buf = screen_buffer_create();
    screen_buffer_init(buf, 80, 24);
    
    // Set prefix
    screen_buffer_set_line_prefix(buf, 0, "loop> ");
    
    // Clear prefix (should free memory)
    screen_buffer_clear_line_prefix(buf, 0);
    
    const char *prefix = screen_buffer_get_line_prefix(buf, 0);
    ck_assert(prefix == NULL);
    
    screen_buffer_destroy(buf);
    // Should not leak (verify with Valgrind)
}
END_TEST
```

**Total Tests**: 3  
**Estimated Time**: 45 minutes

---

### Phase 2 Test Summary

| Test Group | Test Count | Estimated Time |
|------------|------------|----------------|
| Prefix Management | 7 | 1 hour |
| Visual Width Calculation | 5 | 1 hour |
| Dirty Tracking | 4 | 45 minutes |
| Cursor Translation | 4 | 45 minutes |
| Rendering | 3 | 1 hour |
| Memory Management | 3 | 45 minutes |
| **Total** | **26** | **5 hours** |

---

## Phase 3: Continuation Prompt Layer Tests

### Test Suite: continuation_prompt_layer

**File**: `tests/unit/test_continuation_prompt_layer.c`

---

#### Test Group: Lifecycle

```c
START_TEST(test_create_destroy) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    ck_assert(layer != NULL);
    
    continuation_prompt_layer_destroy(layer);
    // Should not crash
}
END_TEST

START_TEST(test_init_cleanup) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    
    continuation_prompt_error_t result = continuation_prompt_layer_init(layer);
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    
    result = continuation_prompt_layer_cleanup(layer);
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_double_cleanup) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    
    // First cleanup
    continuation_prompt_error_t result = continuation_prompt_layer_cleanup(layer);
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    
    // Second cleanup (should be idempotent)
    result = continuation_prompt_layer_cleanup(layer);
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_destroy_null) {
    // Should not crash on NULL
    continuation_prompt_layer_destroy(NULL);
}
END_TEST
```

**Total Tests**: 4  
**Estimated Time**: 30 minutes

---

#### Test Group: Simple Mode

```c
START_TEST(test_simple_mode_get_prompt) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_SIMPLE);
    
    char prompt[64];
    continuation_prompt_error_t result;
    
    // All lines should return "> "
    result = continuation_prompt_layer_get_prompt_for_line(layer, 1, "for i in 1\ndo", prompt, sizeof(prompt));
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "> ");
    
    result = continuation_prompt_layer_get_prompt_for_line(layer, 2, "for i in 1\ndo\necho", prompt, sizeof(prompt));
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "> ");
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_simple_mode_performance) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_SIMPLE);
    
    char prompt[64];
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1000; i++) {
        continuation_prompt_layer_get_prompt_for_line(layer, 1, "test", prompt, sizeof(prompt));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    uint64_t avg_ns = elapsed_ns / 1000;
    
    // Should be <10 microseconds per call
    ck_assert_uint_lt(avg_ns, 10000);
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST
```

**Total Tests**: 2  
**Estimated Time**: 30 minutes

---

#### Test Group: Context-Aware Mode

```c
START_TEST(test_context_aware_if_statement) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    char prompt[64];
    const char *command = "if [ -f file ]\nthen\n    echo yes\nfi";
    
    // Line 1: "then" → inside if
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, command, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "if> ");
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_context_aware_for_loop) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    char prompt[64];
    const char *command = "for i in 1 2 3\ndo\n    echo $i\ndone";
    
    // Line 1: "do" → inside for loop
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, command, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "loop> ");
    
    // Line 2: body → still inside for loop
    result = continuation_prompt_layer_get_prompt_for_line(
        layer, 2, command, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "loop> ");
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_context_aware_function) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    char prompt[64];
    const char *command = "myfunc() {\n    echo hello\n}";
    
    // Line 1: function body
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, command, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "func> ");
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_context_aware_subshell) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    char prompt[64];
    const char *command = "(\n    echo hello\n)";
    
    // Line 1: subshell body
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, command, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "sh> ");
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_context_aware_quote) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    char prompt[64];
    const char *command = "echo \"hello\nworld\"";
    
    // Line 1: inside quote
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, command, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "> ");  // Generic for quotes
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_context_aware_nested) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    char prompt[64];
    const char *command = "if true\nthen\n    for i in 1\n    do\n        echo $i\n    done\nfi";
    
    // Line 3: inside for (nested in if) → should show loop>
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 3, command, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    ck_assert_str_eq(prompt, "loop> ");  // Innermost context
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST
```

**Total Tests**: 6  
**Estimated Time**: 2 hours

---

#### Test Group: Mode Switching

```c
START_TEST(test_set_mode) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    
    // Set to context-aware
    continuation_prompt_error_t result = continuation_prompt_layer_set_mode(
        layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    
    continuation_prompt_mode_t mode = continuation_prompt_layer_get_mode(layer);
    ck_assert_int_eq(mode, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    // Set back to simple
    result = continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_SIMPLE);
    ck_assert_int_eq(result, CONTINUATION_PROMPT_SUCCESS);
    
    mode = continuation_prompt_layer_get_mode(layer);
    ck_assert_int_eq(mode, CONTINUATION_PROMPT_MODE_SIMPLE);
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_mode_change_invalidates_cache) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    
    char prompt[64];
    const char *command = "for i in 1\ndo";
    
    // Get prompt in simple mode
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_SIMPLE);
    continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt, sizeof(prompt));
    ck_assert_str_eq(prompt, "> ");
    
    // Switch to context-aware mode
    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    
    // Get prompt again (should be different, cache invalidated)
    continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt, sizeof(prompt));
    ck_assert_str_eq(prompt, "loop> ");
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST
```

**Total Tests**: 2  
**Estimated Time**: 30 minutes

---

#### Test Group: Error Handling

```c
START_TEST(test_get_prompt_null_layer) {
    char prompt[64];
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        NULL, 1, "test", prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_ERROR_NULL_POINTER);
}
END_TEST

START_TEST(test_get_prompt_null_command) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    
    char prompt[64];
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, NULL, prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_ERROR_NULL_POINTER);
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST

START_TEST(test_get_prompt_buffer_too_small) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    continuation_prompt_layer_init(layer);
    
    char prompt[2];  // Too small for "> "
    continuation_prompt_error_t result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, "test", prompt, sizeof(prompt)
    );
    ck_assert_int_eq(result, CONTINUATION_PROMPT_ERROR_BUFFER_TOO_SMALL);
    
    continuation_prompt_layer_destroy(layer);
}
END_TEST
```

**Total Tests**: 3  
**Estimated Time**: 30 minutes

---

### Phase 3 Test Summary

| Test Group | Test Count | Estimated Time |
|------------|------------|----------------|
| Lifecycle | 4 | 30 minutes |
| Simple Mode | 2 | 30 minutes |
| Context-Aware Mode | 6 | 2 hours |
| Mode Switching | 2 | 30 minutes |
| Error Handling | 3 | 30 minutes |
| **Total** | **17** | **4 hours** |

---

## Phase 4: Composition Engine Tests

### Test Suite: composition_cursor_translation

**File**: `tests/unit/test_composition_cursor_translation.c`

(Due to length constraints, showing representative tests. Full test suite would include all edge cases from phase1_cursor_translation_algorithm.md)

---

#### Test Group: Basic Translation

```c
START_TEST(test_cursor_at_start) {
    const char *command = "hello";
    size_t row, col;
    
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 0, "bash$ ", NULL, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    ck_assert_uint_eq(row, 0);
    ck_assert_uint_eq(col, 6);  // After "bash$ "
}
END_TEST

START_TEST(test_cursor_at_end) {
    const char *command = "hello";
    size_t row, col;
    
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 5, "bash$ ", NULL, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    ck_assert_uint_eq(row, 0);
    ck_assert_uint_eq(col, 11);  // "bash$ " (6) + "hello" (5)
}
END_TEST

START_TEST(test_cursor_on_newline) {
    const char *command = "line1\nline2";
    const char *cont_prompts[] = {NULL, "> "};
    size_t row, col;
    
    // Cursor on \n (position 5)
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 5, "bash$ ", cont_prompts, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    ck_assert_uint_eq(row, 0);  // End of line 0
    ck_assert_uint_eq(col, 11); // "bash$ line1"
}
END_TEST
```

**Total Tests (Basic)**: 10  
**Estimated Time**: 2 hours

---

#### Test Group: UTF-8 and Wide Characters

```c
START_TEST(test_cursor_utf8_multi_byte) {
    const char *command = "Café";  // 'é' is 2 bytes
    size_t row, col;
    
    // Cursor after 'é' (byte position 5)
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 5, "bash$ ", NULL, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    ck_assert_uint_eq(col, 10);  // "bash$ " (6) + "Café" (4 visual)
}
END_TEST

START_TEST(test_cursor_wide_character) {
    const char *command = "中文";  // Each is 3 bytes, 2 visual columns
    size_t row, col;
    
    // Cursor after first character (byte position 3)
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 3, "bash$ ", NULL, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    ck_assert_uint_eq(col, 8);  // "bash$ " (6) + "中" (2 visual)
}
END_TEST
```

**Total Tests (UTF-8/Wide)**: 8  
**Estimated Time**: 2 hours

---

#### Test Group: ANSI and Tabs

```c
START_TEST(test_cursor_with_ansi_codes) {
    const char *command = "foo\033[31mbar\033[0m";  // ANSI codes have 0 width
    size_t row, col;
    
    // Cursor at byte 12 (after "bar", skipping ANSI codes)
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 12, "bash$ ", NULL, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    ck_assert_uint_eq(col, 12);  // "bash$ " (6) + "foobar" (6 visual)
}
END_TEST

START_TEST(test_cursor_with_tab) {
    const char *command = "a\tb";  // Tab expands
    size_t row, col;
    
    // Cursor after tab (byte position 2)
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 2, "bash$ ", NULL, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    // "bash$ " (6) + "a" (1) = 7, tab expands 1 column to 8
    ck_assert_uint_eq(col, 8);
}
END_TEST
```

**Total Tests (ANSI/Tabs)**: 6  
**Estimated Time**: 1.5 hours

---

#### Test Group: Line Wrapping

```c
START_TEST(test_cursor_with_wrapping) {
    // Create command that exceeds terminal width
    char command[200];
    memset(command, 'x', 150);
    command[150] = '\0';
    
    size_t row, col;
    
    // Cursor at position 100 (should wrap to second row)
    composition_engine_error_t result = translate_cursor_buffer_to_display(
        command, 100, "bash$ ", NULL, 80, &row, &col
    );
    
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    ck_assert_uint_gt(row, 0);  // Wrapped to subsequent row
}
END_TEST
```

**Total Tests (Wrapping)**: 5  
**Estimated Time**: 1.5 hours

---

### Phase 4 Test Summary

| Test Group | Test Count | Estimated Time |
|------------|------------|----------------|
| Basic Translation | 10 | 2 hours |
| UTF-8 and Wide Characters | 8 | 2 hours |
| ANSI and Tabs | 6 | 1.5 hours |
| Line Wrapping | 5 | 1.5 hours |
| Continuation Prompts | 8 | 2 hours |
| Reverse Translation | 6 | 1.5 hours |
| **Total** | **43** | **10.5 hours** |

---

## Phase 5: Integration Tests

(Integration tests are higher-level and test full workflows)

### Test Suite: full_continuation_flow

**File**: `tests/integration/test_full_continuation_flow.c`

```c
START_TEST(test_full_flow_simple_multiline) {
    // Setup: Create all components
    composition_engine_t *engine = composition_engine_create();
    screen_buffer_t *screen_buf = screen_buffer_create();
    continuation_prompt_layer_t *cont_layer = continuation_prompt_layer_create();
    
    // Initialize
    composition_engine_init(engine, ...);
    screen_buffer_init(screen_buf, 80, 24);
    continuation_prompt_layer_init(cont_layer);
    
    // Configure
    continuation_prompt_layer_set_mode(cont_layer, CONTINUATION_PROMPT_MODE_SIMPLE);
    composition_engine_set_continuation_layer(engine, cont_layer);
    composition_engine_enable_continuation_prompts(engine, true);
    
    // Test: Compose multiline command
    const char *command = "for i in 1 2 3\ndo\n    echo $i\ndone";
    command_layer_set_content(engine->command_layer, command);
    
    composition_engine_error_t result = composition_engine_compose(engine);
    ck_assert_int_eq(result, COMPOSITION_ENGINE_SUCCESS);
    
    // Verify: Output contains continuation prompts
    char output[2048];
    composition_engine_get_output(engine, output, sizeof(output));
    
    ck_assert(strstr(output, "> do") != NULL);
    ck_assert(strstr(output, ">     echo $i") != NULL);
    ck_assert(strstr(output, "> done") != NULL);
    
    // Cleanup
    composition_engine_destroy(engine);
    screen_buffer_destroy(screen_buf);
    continuation_prompt_layer_destroy(cont_layer);
}
END_TEST
```

**Total Integration Tests**: 12  
**Estimated Time**: 4 hours

---

## Performance Tests

### Test Suite: composition_performance

**File**: `tests/performance/test_composition_perf.c`

```c
START_TEST(test_composition_performance_target) {
    composition_engine_t *engine = setup_test_engine();
    
    const char *command = "for i in 1 2 3\ndo\n    echo $i\ndone";
    command_layer_set_content(engine->command_layer, command);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < 100; i++) {
        composition_engine_compose(engine);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    uint64_t avg_ns = elapsed_ns / 100;
    uint64_t avg_ms = avg_ns / 1000000;
    
    // Target: <5ms per composition
    ck_assert_uint_lt(avg_ms, 5);
    
    cleanup_test_engine(engine);
}
END_TEST

START_TEST(test_cursor_translation_performance_target) {
    const char *command = "for i in 1 2 3\ndo\n    echo $i\ndone";
    const char *cont_prompts[] = {NULL, "> ", "> ", "> "};
    
    size_t row, col;
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < 1000; i++) {
        translate_cursor_buffer_to_display(
            command, 15, "bash$ ", cont_prompts, 80, &row, &col
        );
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    uint64_t avg_ns = elapsed_ns / 1000;
    
    // Target: <100 microseconds per translation
    ck_assert_uint_lt(avg_ns, 100000);
}
END_TEST
```

**Total Performance Tests**: 5  
**Estimated Time**: 2 hours

---

## Test Execution Strategy

### Continuous Testing

```bash
# Run all unit tests
make test-unit

# Run specific phase tests
make test-phase2  # Screen buffer tests
make test-phase3  # Continuation prompt layer tests
make test-phase4  # Composition engine tests

# Run integration tests
make test-integration

# Run performance tests
make test-performance

# Run all tests
make test-all
```

### Coverage Analysis

```bash
# Generate coverage report
make coverage

# View coverage (target: >80%)
gcov src/display/*.c
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### Memory Leak Detection

```bash
# Run tests with Valgrind
make test-valgrind

# Check for leaks
valgrind --leak-check=full --show-leak-kinds=all ./test_runner
```

---

## Summary of All Tests

| Phase | Component | Test Count | Estimated Time |
|-------|-----------|------------|----------------|
| 2 | Screen Buffer Prefix | 26 | 5 hours |
| 3 | Continuation Prompt Layer | 17 | 4 hours |
| 4 | Cursor Translation | 43 | 10.5 hours |
| 5 | Integration Tests | 12 | 4 hours |
| 5 | Performance Tests | 5 | 2 hours |
| **Total** | | **103** | **25.5 hours** |

**Coverage Target**: >80% for all new code  
**Performance Targets**: 
- Composition: <5ms
- Cursor translation: <100μs
- Simple mode prompt: <10μs
- Context-aware prompt: <100μs

**Memory Targets**:
- Zero leaks (Valgrind clean)
- <50 bytes overhead per line with prefix

---

## Conclusion

This comprehensive test plan ensures the continuation prompt implementation is:

1. **Functionally correct**: All features work as specified
2. **Robust**: Handles all edge cases gracefully
3. **Performant**: Meets all performance targets
4. **Memory-safe**: No leaks or corruption
5. **Maintainable**: High test coverage for future changes

Tests are organized by phase for incremental validation during implementation. Each phase can be fully tested before proceeding to the next, ensuring a solid foundation.

**Phase 1 Status**: ✅ COMPLETE - All design and test planning documents created  
**Next Phase**: Phase 2 implementation can begin with complete test specifications ready
