# POSIX-Compliant Symbol Table and Variable Expansion System Design

## Current System Analysis

### Problems Identified

1. **Symbol Table Architecture Issues:**
   - Simple linked list implementation - O(n) lookup time
   - No proper scoping model for POSIX shell environments
   - Limited support for shell variable attributes (readonly, export, etc.)
   - No distinction between shell variables and environment variables
   - Inefficient memory management for large variable sets

2. **Variable Expansion Issues:**
   - Incomplete POSIX parameter expansion support
   - No proper handling of array variables (required for $@ and $*)
   - Limited arithmetic expansion support
   - Inconsistent variable scoping in different contexts
   - Poor handling of special variables ($?, $$, $#, etc.)

3. **POSIX Compliance Gaps:**
   - Missing variable attributes (local, readonly, export)
   - No proper handling of subshell environments
   - Incomplete support for parameter expansion patterns
   - No support for array variables ($@, $*, indexed arrays)
   - Limited special variable support

## Proposed POSIX-Compliant Design

### 1. Enhanced Symbol Table Architecture

```c
// Enhanced variable attributes
typedef enum {
    VAR_ATTR_NONE       = 0,
    VAR_ATTR_READONLY   = (1 << 0),  // readonly variable
    VAR_ATTR_EXPORT     = (1 << 1),  // exported to environment
    VAR_ATTR_LOCAL      = (1 << 2),  // local to function/subshell
    VAR_ATTR_ARRAY      = (1 << 3),  // array variable
    VAR_ATTR_INTEGER    = (1 << 4),  // integer variable (arithmetic context)
    VAR_ATTR_SPECIAL    = (1 << 5),  // special shell variable
    VAR_ATTR_FUNCTION   = (1 << 6),  // function definition
    VAR_ATTR_NAMEREF    = (1 << 7),  // nameref variable (bash extension)
} var_attr_t;

// Variable scope types
typedef enum {
    SCOPE_GLOBAL,       // Global shell scope
    SCOPE_FUNCTION,     // Function scope
    SCOPE_SUBSHELL,     // Subshell scope
    SCOPE_LOCAL,        // Local declaration
} scope_type_t;

// Enhanced variable entry
typedef struct shell_var {
    char *name;                    // Variable name
    char **values;                 // Array of values (single value = values[0])
    size_t value_count;           // Number of values (1 for scalar)
    size_t value_capacity;        // Allocated capacity
    var_attr_t attributes;        // Variable attributes
    scope_type_t scope;           // Variable scope
    struct shell_var *next;       // Next in hash bucket
    node_t *function_body;        // For function variables
} shell_var_t;

// Hash table for efficient lookups
#define SYMTAB_HASH_SIZE 256

typedef struct symbol_table {
    shell_var_t *buckets[SYMTAB_HASH_SIZE];  // Hash buckets
    struct symbol_table *parent;             // Parent scope
    scope_type_t scope_type;                 // Type of this scope
    size_t variable_count;                   // Number of variables
} symbol_table_t;

// Global symbol table stack
typedef struct {
    symbol_table_t *current;     // Current scope
    symbol_table_t *global;      // Global scope
    size_t depth;                // Current nesting depth
} symbol_stack_t;
```

### 2. POSIX Parameter Expansion Engine

```c
// Parameter expansion types
typedef enum {
    PARAM_EXP_SIMPLE,           // $var, ${var}
    PARAM_EXP_LENGTH,           // ${#var}
    PARAM_EXP_DEFAULT,          // ${var-default}, ${var:-default}
    PARAM_EXP_ASSIGN,           // ${var=value}, ${var:=value}
    PARAM_EXP_ERROR,            // ${var?error}, ${var:?error}
    PARAM_EXP_ALTERNATE,        // ${var+alt}, ${var:+alt}
    PARAM_EXP_SUBSTRING,        // ${var:offset:length}
    PARAM_EXP_PREFIX_REMOVE,    // ${var#pattern}, ${var##pattern}
    PARAM_EXP_SUFFIX_REMOVE,    // ${var%pattern}, ${var%%pattern}
    PARAM_EXP_PATTERN_REPLACE,  // ${var/pattern/replacement}
    PARAM_EXP_CASE_MODIFY,      // ${var^pattern}, ${var,pattern}
} param_exp_type_t;

// Parameter expansion context
typedef struct {
    param_exp_type_t type;
    char *var_name;
    char *pattern;
    char *replacement;
    int offset;
    int length;
    bool colon_modifier;        // : modifier (test for empty)
    bool global_replace;        // // for global replacement
} param_expansion_t;

// Expansion result
typedef struct {
    char **words;               // Resulting words (after field splitting)
    size_t word_count;          // Number of words
    bool should_split;          // Whether to perform field splitting
    bool should_glob;           // Whether to perform pathname expansion
} expansion_result_t;
```

### 3. Special Variables Handler

```c
// Special variable types
typedef enum {
    SPECIAL_VAR_PID,            // $$
    SPECIAL_VAR_EXIT_STATUS,    // $?
    SPECIAL_VAR_ARG_COUNT,      // $#
    SPECIAL_VAR_POSITIONAL,     // $0, $1, $2, ...
    SPECIAL_VAR_ALL_ARGS,       // $* 
    SPECIAL_VAR_ALL_ARGS_Q,     // $@
    SPECIAL_VAR_FLAGS,          // $-
    SPECIAL_VAR_RANDOM,         // $RANDOM
    SPECIAL_VAR_LINENO,         // $LINENO
} special_var_type_t;

typedef struct {
    char *name;
    special_var_type_t type;
    char *(*getter)(void);      // Function to get current value
    int (*setter)(const char*); // Function to set value (NULL if readonly)
} special_var_handler_t;
```

### 4. Core API Functions

```c
// Symbol table management
symbol_table_t *symtab_create(scope_type_t scope, symbol_table_t *parent);
void symtab_destroy(symbol_table_t *table);
symbol_table_t *symtab_push_scope(scope_type_t scope);
symbol_table_t *symtab_pop_scope(void);

// Variable operations
shell_var_t *var_create(const char *name, const char *value, var_attr_t attrs);
shell_var_t *var_lookup(const char *name);
shell_var_t *var_lookup_scope(const char *name, symbol_table_t *scope);
int var_set(const char *name, const char *value, var_attr_t attrs);
int var_set_array(const char *name, char **values, size_t count, var_attr_t attrs);
int var_unset(const char *name);
int var_export(const char *name);
int var_readonly(const char *name);

// Variable expansion
expansion_result_t *expand_parameter(const char *param_expr);
expansion_result_t *expand_word(const char *word);
char *expand_to_string(const char *word);
void free_expansion_result(expansion_result_t *result);

// Special variables
int register_special_var(const char *name, special_var_handler_t *handler);
char *get_special_var_value(const char *name);
int set_special_var_value(const char *name, const char *value);

// Array operations (for $@, $*, etc.)
int var_array_append(const char *name, const char *value);
char **var_get_array(const char *name, size_t *count);
int var_array_set_element(const char *name, int index, const char *value);

// Environment integration
int sync_environment(void);                    // Sync exported vars to environment
int import_environment(void);                  // Import environment variables
char **create_environment_array(void);        // Create environ array for exec
```

### 5. POSIX Compliance Features

#### A. Proper Variable Scoping
- Function-local variables with `local` builtin
- Subshell variable isolation
- Proper inheritance rules

#### B. Complete Parameter Expansion
- All POSIX parameter expansion forms
- Proper field splitting (IFS handling)
- Quote removal and pathname expansion

#### C. Special Variables
- Complete set of POSIX special variables
- Proper positional parameter handling
- Array-like behavior for $@ and $*

#### D. Variable Attributes
- Readonly variables (`readonly` builtin)
- Exported variables (`export` builtin)
- Integer variables for arithmetic contexts

### 6. Implementation Strategy

#### Phase 1: Core Symbol Table
1. Implement hash table-based symbol table
2. Add variable attribute support
3. Implement proper scoping

#### Phase 2: Enhanced Expansion
1. Implement complete parameter expansion parser
2. Add proper field splitting and quote removal
3. Integrate with pathname expansion

#### Phase 3: Special Variables
1. Implement special variable handlers
2. Add positional parameter support
3. Implement $@ and $* properly

#### Phase 4: Integration
1. Update parser to use new variable system
2. Update execution engine
3. Add builtin commands (export, readonly, local, etc.)

### 7. Benefits

1. **Performance**: O(1) average variable lookup time
2. **POSIX Compliance**: Complete parameter expansion support
3. **Maintainability**: Clean separation of concerns
4. **Extensibility**: Easy to add new variable types and expansions
5. **Memory Efficiency**: Better memory management for large variable sets

### 8. Migration Plan

1. Implement new system alongside old system
2. Add compatibility layer for existing code
3. Gradually migrate components to new system
4. Remove old system once migration is complete

This design addresses all the major POSIX compliance issues while providing a solid foundation for future shell features.
