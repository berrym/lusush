# LUSUSH TECHNICAL TROUBLESHOOTING GUIDE

## Debugging Word Expansion Issues

### Common Symptoms and Fixes

#### 1. Characters Disappearing After Variable Expansion

**Symptom**: Characters immediately following variable expansions are lost.
```bash
echo "$VAR)"  # ')' disappears
echo "${X}abc"  # 'abc' disappears
```

**Root Cause**: Incorrect `consumed` calculation in `src/wordexp.c`
```c
// WRONG:
end = find_var_name_end(p, 1) + 1;  // Over-consumes characters

// CORRECT:
end = find_var_name_end(p, 1);      // Consumes exactly the variable name
```

**Location**: `src/wordexp.c`, function `word_expand()`, variable expansion case

#### 2. Memory Corruption in Variable Length Expansion

**Symptom**: `free(): invalid pointer` crashes when using `${#VAR}`

**Root Cause**: Freeing non-malloc'd pointers
```c
// PROBLEM: actual_var_name points to var_name+1, not malloc'd memory
if (*var_name == '#' && var_name[1] != '\0') {
    actual_var_name = var_name + 1;  // NOT malloc'd!
}

// Later:
if (actual_var_name != var_name) {
    free(actual_var_name);  // CRASH: freeing var_name+1
}
```

**Solution**: Track allocation status
```c
bool allocated_actual_var_name = false;
// ... set flag when malloc'ing actual_var_name
if (allocated_actual_var_name) {
    free(actual_var_name);
}
```

#### 3. Parameter Expansion Operators Not Working

**Symptom**: `${VAR:+alternate}` fails with unset variables

**Root Cause**: Unset variable check happens before expansion processing
```c
// WRONG ORDER:
if (!var_exists && is_posix_option_set('u')) {
    return error;  // Triggered before checking expansion operators
}
// ... parameter expansion processing

// CORRECT ORDER:
if (param_expansion_op) {
    // Process expansion operators first
} else {
    // Only check unset variables if no expansion operator
    if (!var_exists && is_posix_option_set('u')) {
        return error;
    }
}
```

---

## Parser Extension Guide

### Adding Control Structures

#### 1. Parser Tokens
Add new token types in scanner:
```c
// src/scanner.c
typedef enum {
    // ... existing tokens
    TOKEN_FOR,
    TOKEN_DO,
    TOKEN_DONE,
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELSE,
    TOKEN_FI,
} token_type_t;
```

#### 2. AST Node Types
Extend node types:
```c
// include/lusush.h
typedef enum {
    // ... existing nodes
    NODE_FOR_LOOP,
    NODE_IF_STMT,
    NODE_WHILE_LOOP,
} node_type_t;

typedef struct {
    char *variable;        // for loop variable
    node_t *word_list;     // items to iterate
    node_t *body;          // loop body
} for_loop_t;
```

#### 3. Parsing Functions
Add parsing functions:
```c
// src/parser.c
node_t *parse_for_statement(source_t *src);
node_t *parse_if_statement(source_t *src);

// Example for loop parser:
node_t *parse_for_statement(source_t *src) {
    // Expect: for VAR in WORDS; do COMMANDS; done
    expect_token(src, TOKEN_FOR);
    token_t *var = expect_token(src, TOKEN_WORD);
    expect_token(src, TOKEN_IN);
    // ... parse word list
    expect_token(src, TOKEN_DO);
    // ... parse commands
    expect_token(src, TOKEN_DONE);
}
```

#### 4. Execution Engine
Add execution support:
```c
// src/exec.c
int execute_for_loop(node_t *node) {
    for_loop_t *loop = &node->for_loop;
    
    // Expand word list
    word_t *words = expand_word_list(loop->word_list);
    
    // Execute loop body for each word
    for (word_t *w = words; w; w = w->next) {
        set_shell_var(loop->variable, w->data);
        int exit_code = execute_node(loop->body);
        if (should_break_loop(exit_code)) break;
    }
    
    free_word_list(words);
    return 0;
}
```

---

## Memory Management Patterns

### Safe Variable Name Handling
```c
char *actual_var_name = var_name;
bool allocated_actual_var_name = false;

if (needs_allocation) {
    actual_var_name = malloc(size);
    allocated_actual_var_name = true;
} else {
    actual_var_name = var_name + offset;  // Pointer arithmetic
    allocated_actual_var_name = false;
}

// Cleanup
if (allocated_actual_var_name) {
    free(actual_var_name);
}
free(var_name);  // Always free the original
```

### Word Expansion Result Handling
```c
expansion_t result = {EXP_NO_EXPANSION, NULL, 0};

result.expanded = malloc(len + 1);
if (!result.expanded) {
    result.result = EXP_ERROR;
    goto cleanup;
}

strcpy(result.expanded, value);
result.len = len;
result.result = EXP_OK;

cleanup:
    // Always clean up temporary allocations
    if (temp_buffer) free(temp_buffer);
    return result;
```

---

## Testing Framework Usage

### Unit Testing Word Expansion
```bash
# Test variable expansion
./builddir/lusush -c 'VAR=test; echo "${VAR}"'

# Test edge cases
./builddir/lusush -c 'echo "$VAR)"'  # Character after expansion
./builddir/lusush -c 'echo "${#VAR}"'  # Length expansion
./builddir/lusush -c 'echo "${UNSET:-default}"'  # Parameter expansion
```

### Comprehensive Testing
```bash
# Run full test suite
./test_posix_options.sh

# Run comprehensive feature test
./builddir/lusush test-comprehensive.sh

# Test specific features
./builddir/lusush -c 'for i in 1 2 3; do echo $i; done'  # Control structures
```

### Memory Debugging
```bash
# Run with Valgrind to detect memory issues
valgrind --leak-check=full ./builddir/lusush test-comprehensive.sh

# Check for specific memory corruption
valgrind --tool=memcheck --track-origins=yes ./builddir/lusush -c 'echo "${#VAR}"'
```

---

## Performance Considerations

### Word Expansion Optimization
- Avoid redundant string allocations
- Cache commonly used variable values
- Optimize string building operations

### Parser Optimization
- Implement lookahead to reduce backtracking
- Use efficient token streaming
- Minimize AST node allocations for simple commands

### Execution Optimization
- Cache builtin function lookups
- Optimize pipeline creation/destruction
- Reuse command argument arrays

---

## Common Development Pitfalls

### 1. Token Parsing Order
```c
// WRONG: Consuming tokens without proper lookahead
token_t *tok = get_next_token(src);
if (tok->type == TOKEN_FOR) {
    // Already consumed the token!
}

// CORRECT: Peek before consuming
token_t *tok = peek_token(src);
if (tok->type == TOKEN_FOR) {
    consume_token(src);  // Now consume it
}
```

### 2. Memory Leaks in Error Paths
```c
// WRONG: Memory leak on error
char *buffer = malloc(size);
if (error_condition) {
    return -1;  // Leaked buffer!
}

// CORRECT: Cleanup on all paths
char *buffer = malloc(size);
if (error_condition) {
    free(buffer);
    return -1;
}
```

### 3. Infinite Recursion in Expansion
```c
// WRONG: No recursion depth limit
char *expand_aliases(char *cmd) {
    char *expanded = get_alias(cmd);
    if (expanded) {
        return expand_aliases(expanded);  // Infinite recursion!
    }
    return cmd;
}

// CORRECT: Limit recursion depth
char *expand_aliases(char *cmd, int depth) {
    if (depth > MAX_ALIAS_DEPTH) return cmd;
    char *expanded = get_alias(cmd);
    if (expanded) {
        return expand_aliases(expanded, depth + 1);
    }
    return cmd;
}
```

---

## Debugging Tools and Techniques

### Trace Execution
```bash
# Enable trace mode for debugging
./builddir/lusush -x -c 'command_to_debug'

# Verbose mode for input processing
./builddir/lusush -v -c 'command_to_debug'
```

### Debug Output in Code
```c
#ifdef DEBUG_WORDEXP
    fprintf(stderr, "DEBUG: Expanding '%s'\n", word);
    fprintf(stderr, "DEBUG: Variable '%s' = '%s'\n", var_name, var_value);
#endif
```

### GDB Debugging
```bash
# Debug with GDB
gdb ./builddir/lusush
(gdb) set args -c 'echo "${#VAR}"'
(gdb) break word_expand
(gdb) run
```

---

This guide should help developers understand the codebase structure, common issues, and how to implement new features safely.
