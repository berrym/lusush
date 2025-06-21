# Assignment Parsing Bug Fix Documentation

## Issue Description
**Bug**: Assignment parsing in the modern parser was losing the variable name when processing assignment statements.

**Symptom**: Commands like `a=test` would fail to properly create the assignment because the variable name (`a`) was lost after advancing the tokenizer to consume the `=` token.

**Root Cause**: The parser was advancing the tokenizer to consume tokens before saving the variable name, causing the current token pointer to move past the variable name.

## Code Location
**File**: `src/parser_modern.c`  
**Function**: `parse_simple_command()` - assignment detection logic

## Original Problematic Code Pattern
```c
// Check for assignment (word followed by =)
if (modern_token_is_word_like(current->type)) {
    modern_token_t *next = modern_tokenizer_peek(parser->tokenizer);
    if (next && next->type == MODERN_TOK_ASSIGN) {
        // BUG: Advancing tokenizer before saving variable name
        modern_tokenizer_advance(parser->tokenizer); // consume variable name
        modern_tokenizer_advance(parser->tokenizer); // consume '='
        
        // Variable name is now lost!
        char *var_name = current->text; // current is now pointing to '='
    }
}
```

## Fixed Code
```c
// Check for assignment (word followed by =)
if (modern_token_is_word_like(current->type)) {
    modern_token_t *next = modern_tokenizer_peek(parser->tokenizer);
    if (next && next->type == MODERN_TOK_ASSIGN) {
        // FIX: Save variable name BEFORE advancing tokenizer
        char *var_name = strdup(current->text);
        if (!var_name) {
            free_node_tree(command);
            return NULL;
        }
        
        // Now safe to advance tokenizer
        modern_tokenizer_advance(parser->tokenizer); // consume variable name
        modern_tokenizer_advance(parser->tokenizer); // consume '='
        
        // Use saved variable name
        strcpy(assignment, var_name);  // Use saved variable name
        free(var_name);
    }
}
```

## Impact
- **Before Fix**: Assignment statements like `a=test` would fail to execute
- **After Fix**: All assignment statements work correctly
- **Variable Scoping**: Proper variable creation and scoping maintained
- **Memory Management**: Added proper cleanup for saved variable name

## Testing
Assignment parsing now works correctly for:
- Simple assignments: `var=value`
- Empty assignments: `var=`
- Assignments in command sequences: `a=test; echo $a`
- Assignments followed by control structures: `x=5; for i in $x; do echo $i; done`

## Related Issues
This fix was critical for command sequence execution, as assignments are commonly used before control structures in shell scripts.
