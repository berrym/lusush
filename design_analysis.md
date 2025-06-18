# Token Pushback Design Analysis

## Current Limitations
- Single token pushback only
- No token context preservation
- Limited parser lookahead capability

## Proposed Improvements Using Enhanced Symbol Table

### Option 1: Token-Aware Symbol Table Extension

```c
// Enhanced symtable entry to handle different data types
typedef enum {
    SYM_STR,
    SYM_FUNC, 
    SYM_TOKEN,      // New: for token storage
    SYM_TOKEN_STACK // New: for token stacks
} symbol_type_t;

typedef struct {
    token_t **tokens;
    size_t capacity;
    size_t count;
    size_t head;  // for circular buffer
} token_stack_t;

// Enhanced symtable entry
typedef struct symtable_entry {
    char *name;
    symbol_type_t val_type;
    union {
        char *str_val;
        node_t *func_body;
        token_t *token_val;      // Single token
        token_stack_t *token_stack; // Token stack/queue
    } value;
    unsigned int flags;
    struct symtable_entry *next;
} symtable_entry_t;
```

### Option 2: Dedicated Token Management System

Rather than overloading the symbol table, create a specialized token management system:

```c
// Dedicated token pushback manager
typedef struct {
    token_t **token_stack;
    size_t capacity;
    size_t count;
    size_t max_lookahead;
} token_pushback_manager_t;

// Integration with existing scanner
typedef struct {
    source_t *source;
    token_pushback_manager_t *pushback_mgr;
    token_t *current_token;
    token_t *previous_token;
} enhanced_scanner_t;
```

## Benefits of Each Approach

### Symbol Table Extension Benefits:
- Unified storage mechanism
- Leverages existing scoping/cleanup
- Could support named token contexts
- Good memory management patterns

### Dedicated System Benefits:
- Cleaner separation of concerns
- Optimized for token-specific needs
- Simpler implementation
- Better performance for token operations

## Implementation Requirements

### For Symbol Table Approach:
1. Add token-specific entry types
2. Extend memory management for token cleanup
3. Add token stack operations to symtable API
4. Modify scanner to use symtable for pushback

### For Dedicated System:
1. Create token_pushback_manager module
2. Implement circular buffer for tokens
3. Add lookahead/pushback operations
4. Integrate with existing scanner

## Recommendation

**Use Option 2 (Dedicated System)** because:
- Symbol tables are conceptually for variables/functions
- Token pushback has different lifecycle/scope needs
- Cleaner, more maintainable code
- Better performance characteristics
- Easier to implement and test

## Implementation Outline

```c
// token_pushback.h
typedef struct token_pushback_manager {
    token_t **tokens;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
} token_pushback_manager_t;

token_pushback_manager_t *create_pushback_manager(size_t capacity);
void destroy_pushback_manager(token_pushback_manager_t *mgr);
int pushback_token(token_pushback_manager_t *mgr, token_t *tok);
token_t *pop_token(token_pushback_manager_t *mgr);
token_t *peek_token(token_pushback_manager_t *mgr, size_t offset);
size_t pushback_count(token_pushback_manager_t *mgr);
void clear_pushback(token_pushback_manager_t *mgr);
```

This approach would provide:
- Multiple token lookahead
- Efficient token storage
- Clean integration with existing code
- Better error recovery in parser
- Support for complex parsing scenarios
