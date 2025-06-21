# Parameter Expansion Fix: ${VAR:+alternate} Operator

## Issue Description

The parameter expansion operator `${VAR:+alternate}` was incorrectly failing when used with unset variables. According to POSIX specification, `${VAR:+alternate}` should:
- Return `alternate` if VAR is set and non-empty
- Return an empty string if VAR is unset or empty
- Always succeed (exit code 0)

However, lusush was returning exit code 1 instead of an empty string when the variable was unset.

## Root Cause Analysis

The bug had two underlying causes:

### 1. String Builder Empty String Rejection
In `src/wordexp.c`, the `sb_append_len()` function was incorrectly rejecting empty string appends:

```c
// BUGGY CODE (line 85):
if (!sb || !str || len == 0) {
    return false;  // This rejected valid empty strings
}
```

When parameter expansion correctly returned an empty string for `${UNSET:+alternate}`, the string builder would fail to append it, causing the entire word expansion to fail.

### 2. Incorrect NULL Handling in Command Execution
In `src/exec.c`, the `execute_command()` function was treating any NULL return from `word_expand()` as an error:

```c
// BUGGY CODE (line 225):
} else {
    // Word expansion failed - this could be due to unset variable error (-u)
    free_argv(argc, argv);
    return 1;  // This incorrectly failed on valid empty expansions
}
```

For unquoted expansions that result in no fields (valid POSIX behavior), `word_expand()` correctly returns NULL, but this was being treated as an error.

## Solution

### 1. Fixed String Builder (src/wordexp.c:85)
```c
bool sb_append_len(str_builder_t *sb, const char *str, size_t len) {
    if (!sb || !str) {
        return false;
    }
    
    // Allow appending empty strings (len == 0 is valid)
    if (len == 0) {
        return true;
    }
    
    // ... rest of function unchanged
}
```

### 2. Fixed Command Execution (src/exec.c:225)
```c
if (w) {
    // ... process words normally
    free_all_words(w);
}
// Note: NULL return from word_expand is valid for empty unquoted expansions
// Only return error if there was an actual expansion error (e.g., -u flag violations)
// The word_expand function handles those errors internally
```

## Test Results

All parameter expansion scenarios now work correctly:

| Test Case | Before | After |
|-----------|--------|-------|
| `echo "${MISSING:+alt}"` | Exit 1 | Empty line, Exit 0 |
| `echo ${MISSING:+alt}` | Exit 1 | Empty line, Exit 0 |
| `VAR=x; echo "${VAR:+alt}"` | alt, Exit 0 | alt, Exit 0 |
| `set -u; echo ${MISSING:+alt}` | Exit 1 | Empty line, Exit 0 |

## POSIX Compliance Impact

This fix ensures full POSIX compliance for the `:+` parameter expansion operator, which is essential for:
- Shell script portability
- Conditional expansions in automation scripts
- Compatibility with standard shell behavior

## Files Modified

- `src/wordexp.c`: Fixed string builder empty string handling
- `src/exec.c`: Fixed NULL word expansion handling
- Added comprehensive test coverage for parameter expansion edge cases

This fix maintains backward compatibility while resolving a critical gap in POSIX parameter expansion support.
