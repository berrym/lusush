# Spec 12 Core Implementation Plan - Completion System Foundation

**Document**: SPEC12_CORE_IMPLEMENTATION_PLAN.md  
**Date**: 2025-11-22  
**Session**: 23  
**Status**: PLANNING → IMPLEMENTATION  

---

## Executive Summary

This document outlines the implementation of Spec 12 core completion system. This is **NOT** a fix to existing code - this is a **complete rewrite** of completion generation using proper architecture.

### Why Rebuild Instead of Fix?

Current completion system is fundamentally broken:
- ✗ Produces duplicates (echo appears twice)
- ✗ Wrong categorization (echo in both builtin AND external)
- ✗ Legacy code migrated from failed completion system
- ✗ No proper architecture - just ad-hoc generation

**Building interactive features (menu, cycling) on broken generation is backwards.**

### The Right Sequence

```
1. Spec 12 Core (THIS DOCUMENT)
   → Proper completion generation
   → No duplicates, correct categorization
   → Solid foundation
   
2. Screen Buffer Integration  
   → Menu through virtual layout system
   → No rendering corruption
   
3. Interactive Features
   → Single-line: Menu navigation + acceptance
   → Multi-line: Inline TAB cycling
```

### What This Plan Covers

**CORE ONLY** (no advanced features):
- ✓ Context analysis (command vs argument position)
- ✓ Source management (builtin, external, files, variables)
- ✓ Proper generation orchestration
- ✓ Deduplication
- ✓ Correct categorization

**NOT IN THIS PLAN** (future enhancements):
- ✗ Fuzzy matching
- ✗ Learning/ranking algorithms
- ✗ Plugin API
- ✗ Custom sources beyond basics

---

## Phase 1: Core Components

### Goal: Define Proper Data Structures

Replace the current ad-hoc system with Spec 12 architecture.

### 1.1: Completion System Structure

**File**: `include/lle/completion/completion_system.h`

```c
/**
 * Main completion system - orchestrates all completion functionality
 * Following Spec 12 architecture
 */
typedef struct lle_completion_system {
    /* Core engines */
    lle_context_analyzer_t *context_analyzer;    // Understand what we're completing
    lle_source_manager_t *source_manager;        // Manage completion sources
    
    /* Current state */
    lle_completion_state_t *current_state;       // Active completion session
    lle_completion_menu_state_t *menu;           // Menu state (if visible)
    
    /* Memory management */
    lle_memory_pool_t *pool;                     // Memory pool for allocations
    
    /* Configuration */
    bool enable_history_source;                  // Use history as source
    bool enable_fuzzy_matching;                  // Future: fuzzy matching
    size_t max_completions;                      // Limit results
} lle_completion_system_t;
```

### 1.2: Context Analyzer Structure

**File**: `include/lle/completion/context_analyzer.h`

```c
/**
 * Context types for completion
 */
typedef enum {
    LLE_CONTEXT_COMMAND,           // Start of command (complete command names)
    LLE_CONTEXT_ARGUMENT,          // Command argument (complete files/dirs)
    LLE_CONTEXT_VARIABLE,          // Variable expansion ($VAR)
    LLE_CONTEXT_REDIRECT,          // After redirect operator (>, <, |)
    LLE_CONTEXT_ASSIGNMENT,        // Variable assignment (VAR=)
    LLE_CONTEXT_UNKNOWN            // Can't determine context
} lle_completion_context_type_t;

/**
 * Analysis result for current completion context
 */
typedef struct lle_context_analyzer {
    /* Context type */
    lle_completion_context_type_t type;
    
    /* Position information */
    size_t word_start;                // Start of word being completed
    size_t word_end;                  // End of word being completed
    char *partial_word;               // The partial word to complete
    
    /* Command context (if applicable) */
    char *command_name;               // Current command (if in argument position)
    int argument_index;               // Which argument (0-based)
    
    /* State flags */
    bool in_quotes;                   // Inside quotes?
    bool after_redirect;              // After >, <, |, etc.
    bool in_assignment;               // In VAR=value ?
    
    /* Memory pool reference */
    lle_memory_pool_t *pool;          // For allocations
} lle_context_analyzer_t;
```

### 1.3: Source Manager Structure

**File**: `include/lle/completion/source_manager.h`

```c
/**
 * Completion source types
 */
typedef enum {
    LLE_SOURCE_BUILTINS,              // Shell builtin commands
    LLE_SOURCE_EXTERNAL_COMMANDS,     // External commands in PATH
    LLE_SOURCE_FILES,                 // File/directory paths
    LLE_SOURCE_VARIABLES,             // Environment variables
    LLE_SOURCE_HISTORY,               // Command history
    LLE_SOURCE_ALIASES,               // Shell aliases (future)
    LLE_SOURCE_FUNCTIONS,             // Shell functions (future)
} lle_source_type_t;

/**
 * Single completion source
 */
typedef struct lle_completion_source {
    lle_source_type_t type;
    const char *name;
    
    /* Source function - generates completions for given prefix */
    lle_result_t (*generate)(
        lle_memory_pool_t *pool,
        const lle_context_analyzer_t *context,
        const char *prefix,
        lle_completion_result_t *result
    );
    
    /* Optional: Check if source is applicable for context */
    bool (*is_applicable)(const lle_context_analyzer_t *context);
    
    void *user_data;                  // Source-specific data
} lle_completion_source_t;

/**
 * Source manager - registry of all completion sources
 */
typedef struct lle_source_manager {
    lle_completion_source_t *sources[MAX_COMPLETION_SOURCES];
    size_t num_sources;
    lle_memory_pool_t *pool;
} lle_source_manager_t;

#define MAX_COMPLETION_SOURCES 16
```

### 1.4: Completion State Structure

**File**: `include/lle/completion/completion_state.h`

```c
/**
 * Tracks state of current completion session
 */
typedef struct lle_completion_state {
    /* Input state */
    char *buffer_snapshot;            // Buffer at completion start
    size_t cursor_position;           // Cursor at completion start
    
    /* Context */
    lle_context_analyzer_t *context;  // Analyzed context
    
    /* Results */
    lle_completion_result_t *results; // Generated completions
    
    /* Cycling state (for inline completion) */
    int current_index;                // Current selection (for TAB cycling)
    char *original_word;              // Original partial word
    
    /* Timing */
    uint64_t generation_time_us;      // Time to generate completions
    
    /* Flags */
    bool active;                      // Completion session active?
    bool menu_mode;                   // Menu shown or inline cycling?
} lle_completion_state_t;
```

### Phase 1 Deliverables:

1. ✓ Header files with all structure definitions
2. ✓ Creation/destruction functions for each structure
3. ✓ Basic initialization/cleanup

**Estimated Lines**: ~150 lines (header + implementation)

---

## Phase 2: Context Analyzer

### Goal: Understand What We're Completing

The context analyzer examines the buffer and cursor position to determine:
- What TYPE of completion is needed
- What SOURCES should be queried
- What PREFIX to match against

### 2.1: Core Analysis Function

**File**: `src/lle/completion/context_analyzer.c`

```c
/**
 * Analyze buffer to determine completion context
 */
lle_result_t lle_context_analyze(
    const char *buffer,
    size_t cursor_pos,
    lle_memory_pool_t *pool,
    lle_context_analyzer_t **out_context)
{
    // Step 1: Extract the word being completed
    size_t word_start = find_word_start(buffer, cursor_pos);
    size_t word_end = cursor_pos;
    char *partial_word = extract_word(buffer, word_start, word_end, pool);
    
    // Step 2: Determine position type
    lle_completion_context_type_t type = determine_context_type(buffer, word_start);
    
    // Step 3: Extract command context (if in argument position)
    char *command_name = NULL;
    int arg_index = -1;
    if (type == LLE_CONTEXT_ARGUMENT) {
        extract_command_context(buffer, word_start, &command_name, &arg_index, pool);
    }
    
    // Step 4: Check special conditions
    bool in_quotes = is_inside_quotes(buffer, cursor_pos);
    bool after_redirect = is_after_redirect(buffer, word_start);
    bool in_assignment = is_in_assignment(buffer, word_start);
    
    // Step 5: Build context structure
    lle_context_analyzer_t *context = lle_pool_alloc(sizeof(*context));
    context->type = type;
    context->word_start = word_start;
    context->word_end = word_end;
    context->partial_word = partial_word;
    context->command_name = command_name;
    context->argument_index = arg_index;
    context->in_quotes = in_quotes;
    context->after_redirect = after_redirect;
    context->in_assignment = in_assignment;
    context->pool = pool;
    
    *out_context = context;
    return LLE_SUCCESS;
}
```

### 2.2: Context Type Detection

```c
/**
 * Determine what type of completion is needed
 */
static lle_completion_context_type_t determine_context_type(
    const char *buffer,
    size_t word_start)
{
    // Check for variable expansion
    if (word_start > 0 && buffer[word_start - 1] == '$') {
        return LLE_CONTEXT_VARIABLE;
    }
    
    // Check for assignment
    if (is_in_assignment(buffer, word_start)) {
        return LLE_CONTEXT_ASSIGNMENT;
    }
    
    // Check for redirect
    if (is_after_redirect(buffer, word_start)) {
        return LLE_CONTEXT_REDIRECT;
    }
    
    // Check if at command position
    if (is_command_position(buffer, word_start)) {
        return LLE_CONTEXT_COMMAND;
    }
    
    // Default: argument completion
    return LLE_CONTEXT_ARGUMENT;
}
```

### 2.3: Helper Functions

```c
// Find start of current word
static size_t find_word_start(const char *buffer, size_t cursor_pos);

// Extract word being completed
static char *extract_word(const char *buf, size_t start, size_t end, 
                          lle_memory_pool_t *pool);

// Check if position is at start of command
static bool is_command_position(const char *buffer, size_t pos);

// Check if inside quotes
static bool is_inside_quotes(const char *buffer, size_t pos);

// Check if after redirect operator
static bool is_after_redirect(const char *buffer, size_t pos);

// Check if in variable assignment
static bool is_in_assignment(const char *buffer, size_t pos);

// Extract current command and argument index
static void extract_command_context(const char *buffer, size_t pos,
                                    char **command, int *arg_index,
                                    lle_memory_pool_t *pool);
```

### Phase 2 Deliverables:

1. ✓ Context analysis function
2. ✓ Context type detection
3. ✓ All helper functions
4. ✓ Unit tests for context detection

**Estimated Lines**: ~200 lines

---

## Phase 3: Source Manager

### Goal: Manage Completion Sources

The source manager:
- Registers completion sources
- Queries applicable sources for context
- Aggregates results from multiple sources

### 3.1: Source Manager Creation

**File**: `src/lle/completion/source_manager.c`

```c
/**
 * Create source manager and register default sources
 */
lle_result_t lle_source_manager_create(
    lle_memory_pool_t *pool,
    lle_source_manager_t **out_manager)
{
    lle_source_manager_t *manager = lle_pool_alloc(sizeof(*manager));
    manager->num_sources = 0;
    manager->pool = pool;
    
    // Register default sources
    lle_source_manager_register_builtin_source(manager);
    lle_source_manager_register_external_command_source(manager);
    lle_source_manager_register_file_source(manager);
    lle_source_manager_register_variable_source(manager);
    lle_source_manager_register_history_source(manager);
    
    *out_manager = manager;
    return LLE_SUCCESS;
}
```

### 3.2: Source Registration

```c
/**
 * Register a completion source
 */
lle_result_t lle_source_manager_register(
    lle_source_manager_t *manager,
    lle_source_type_t type,
    const char *name,
    lle_completion_source_generate_fn generate_fn,
    lle_completion_source_applicable_fn applicable_fn)
{
    if (manager->num_sources >= MAX_COMPLETION_SOURCES) {
        return LLE_ERROR_LIMIT_EXCEEDED;
    }
    
    lle_completion_source_t *source = lle_pool_alloc(sizeof(*source));
    source->type = type;
    source->name = name;
    source->generate = generate_fn;
    source->is_applicable = applicable_fn;
    source->user_data = NULL;
    
    manager->sources[manager->num_sources++] = source;
    return LLE_SUCCESS;
}
```

### 3.3: Query Sources

```c
/**
 * Query all applicable sources for completions
 */
lle_result_t lle_source_manager_query(
    lle_source_manager_t *manager,
    const lle_context_analyzer_t *context,
    const char *prefix,
    lle_completion_result_t *result)
{
    for (size_t i = 0; i < manager->num_sources; i++) {
        lle_completion_source_t *source = manager->sources[i];
        
        // Check if source is applicable for this context
        if (source->is_applicable && !source->is_applicable(context)) {
            continue;  // Skip this source
        }
        
        // Query source for completions
        lle_result_t res = source->generate(manager->pool, context, prefix, result);
        if (res != LLE_SUCCESS) {
            // Log error but continue with other sources
            continue;
        }
    }
    
    return LLE_SUCCESS;
}
```

### 3.4: Default Source Implementations

Each source has applicability check and generation function:

```c
// Builtin source: Only applicable at command position
static bool builtin_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_COMMAND;
}

static lle_result_t builtin_source_generate(
    lle_memory_pool_t *pool,
    const lle_context_analyzer_t *context,
    const char *prefix,
    lle_completion_result_t *result)
{
    // Use existing lle_completion_generate_commands() but mark as BUILTIN
    // This prevents duplicates - each source queries its OWN domain
    return generate_builtin_completions(pool, prefix, result);
}

// External command source: Only applicable at command position
static bool external_command_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_COMMAND;
}

// File source: Applicable for arguments and redirects
static bool file_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_ARGUMENT ||
           context->type == LLE_CONTEXT_REDIRECT;
}

// Variable source: Only applicable for $VAR
static bool variable_source_applicable(const lle_context_analyzer_t *context) {
    return context->type == LLE_CONTEXT_VARIABLE;
}

// History source: Always applicable as fallback
static bool history_source_applicable(const lle_context_analyzer_t *context) {
    return true;  // Always available
}
```

### Phase 3 Deliverables:

1. ✓ Source manager creation/destruction
2. ✓ Source registration API
3. ✓ Source query orchestration
4. ✓ Default sources with applicability checks
5. ✓ Tests for source selection

**Estimated Lines**: ~150 lines

---

## Phase 4: Proper Completion Generation

### Goal: Orchestrate the Complete Flow

This ties everything together into the main generation function.

### 4.1: Main Generation Function

**File**: `src/lle/completion/completion_generator.c` (REWRITE)

```c
/**
 * Generate completions - PROPER Spec 12 implementation
 */
lle_result_t lle_completion_generate(
    lle_completion_system_t *system,
    const char *buffer,
    size_t cursor_pos,
    lle_completion_result_t **out_result)
{
    // Step 1: Analyze context
    lle_context_analyzer_t *context = NULL;
    lle_result_t res = lle_context_analyze(
        buffer, 
        cursor_pos, 
        system->pool, 
        &context
    );
    if (res != LLE_SUCCESS) {
        return res;
    }
    
    // Step 2: Create result structure
    lle_completion_result_t *result = NULL;
    res = lle_completion_result_create(system->pool, 64, &result);
    if (res != LLE_SUCCESS) {
        return res;
    }
    
    // Step 3: Query all applicable sources
    const char *prefix = context->partial_word ? context->partial_word : "";
    res = lle_source_manager_query(
        system->source_manager,
        context,
        prefix,
        result
    );
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        return res;
    }
    
    // Step 4: Deduplicate results
    // This is CRITICAL - fixes the duplicate "echo" bug
    res = lle_completion_deduplicate(result);
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        return res;
    }
    
    // Step 5: Sort results by relevance
    res = lle_completion_sort(result);
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        return res;
    }
    
    // Step 6: Store state in completion system
    if (system->current_state) {
        lle_completion_state_free(system->current_state);
    }
    system->current_state = create_completion_state(buffer, cursor_pos, context, result);
    
    *out_result = result;
    return LLE_SUCCESS;
}
```

### 4.2: Deduplication

```c
/**
 * Remove duplicate completions
 * CRITICAL: Fixes the "echo appears twice" bug
 */
lle_result_t lle_completion_deduplicate(lle_completion_result_t *result) {
    if (!result || result->count <= 1) {
        return LLE_SUCCESS;
    }
    
    // Use hash set to track seen completions
    // Remove any item whose text we've already seen
    
    size_t write_pos = 0;
    for (size_t read_pos = 0; read_pos < result->count; read_pos++) {
        const char *text = result->items[read_pos].text;
        
        // Check if we've seen this text before
        bool duplicate = false;
        for (size_t check = 0; check < write_pos; check++) {
            if (strcmp(result->items[check].text, text) == 0) {
                duplicate = true;
                break;
            }
        }
        
        // Keep only unique items
        if (!duplicate) {
            if (write_pos != read_pos) {
                result->items[write_pos] = result->items[read_pos];
            }
            write_pos++;
        }
    }
    
    result->count = write_pos;
    return LLE_SUCCESS;
}
```

### 4.3: Sorting

```c
/**
 * Sort completions by relevance
 */
lle_result_t lle_completion_sort(lle_completion_result_t *result) {
    if (!result || result->count <= 1) {
        return LLE_SUCCESS;
    }
    
    // Sort by:
    // 1. Exact prefix matches first
    // 2. Type (builtins, then commands, then files)
    // 3. Alphabetically
    
    qsort(result->items, result->count, 
          sizeof(lle_completion_item_t),
          completion_compare_fn);
    
    return LLE_SUCCESS;
}
```

### Phase 4 Deliverables:

1. ✓ Rewritten generation function using Spec 12 architecture
2. ✓ Deduplication implementation
3. ✓ Sorting implementation
4. ✓ Integration tests for complete flow

**Estimated Lines**: ~100 lines

---

## Testing Strategy

### Unit Tests (Per Phase)

**Phase 1**: Structure creation/destruction
```c
test_completion_system_create_destroy()
test_context_analyzer_create_destroy()
test_source_manager_create_destroy()
```

**Phase 2**: Context analysis
```c
test_context_command_position()      // "ec" → COMMAND context
test_context_argument_position()     // "echo foo" → ARGUMENT context
test_context_variable()              // "$PA" → VARIABLE context
test_context_redirect()              // "cat > fi" → REDIRECT context
```

**Phase 3**: Source management
```c
test_source_registration()
test_source_applicability()          // Right sources for context
test_source_query_multiple()         // Query multiple sources
```

**Phase 4**: Complete flow
```c
test_generate_no_duplicates()        // Fix: "echo" only once
test_generate_correct_categories()   // Fix: builtin vs external
test_generate_sorted()               // Results in order
```

### Integration Tests

```c
test_complete_builtin_command()      // "ec[TAB]" → echo
test_complete_external_command()     // "ca[TAB]" → cat, cal, case
test_complete_file_argument()        // "cat fi[TAB]" → file.txt
test_complete_variable()             // "$PA[TAB]" → $PATH
test_complete_after_redirect()       // "cat > fi[TAB]" → files
```

### Manual Testing Script

```bash
#!/bin/bash
# Test Spec 12 core completion generation

echo "Test 1: Builtin command completion"
# Type: ec[TAB]
# Expected: echo (appears ONCE, marked as builtin)

echo "Test 2: External command completion"  
# Type: ca[TAB]
# Expected: cat, cal, case (NO DUPLICATES)

echo "Test 3: File completion"
# Type: cat fi[TAB]
# Expected: file1.txt, file2.txt (in current dir)

echo "Test 4: Variable completion"
# Type: echo $PA[TAB]
# Expected: $PATH

echo "Test 5: No duplicates"
# Type: ec[TAB]
# Expected: echo appears ONCE (not in both builtin and external)
```

---

## Migration Plan

### Old Code to Remove

After Phase 4 complete:

1. **src/lle/completion/completion_generator.c**: 
   - Delete old `lle_completion_generate()` 
   - Replace with Spec 12 version

2. **Remove ad-hoc context detection**:
   - Old `lle_completion_is_command_position()`
   - Replace with context analyzer

3. **Remove manual source calls**:
   - Old direct calls to source functions
   - Replace with source manager query

### Compatibility Shims

Keep existing API for now:
```c
// Old API - redirects to new implementation
lle_result_t lle_completion_generate(
    lle_memory_pool_t *pool,
    const char *buffer,
    size_t cursor_pos,
    lle_completion_result_t **result)
{
    // Create temporary completion system
    // Call new Spec 12 generation
    // Return result
    // (Bridge until all callers updated)
}
```

---

## Success Criteria

### Phase 1 Complete When:
- ✓ All structures compile
- ✓ Create/destroy functions work
- ✓ Memory doesn't leak

### Phase 2 Complete When:
- ✓ Context analyzer detects all context types correctly
- ✓ Handles edge cases (quotes, redirects, assignments)
- ✓ Unit tests pass

### Phase 3 Complete When:
- ✓ Sources registered successfully
- ✓ Applicability checks work correctly
- ✓ Multiple sources queried properly

### Phase 4 Complete When:
- ✓ **NO DUPLICATES** in completion results
- ✓ **CORRECT CATEGORIZATION** (builtin vs external)
- ✓ Completions sorted properly
- ✓ All integration tests pass

### Final Success:
```
Type: ec[TAB]
Result: ["echo"] (appears ONCE, marked as builtin)

Type: ca[TAB]  
Result: ["cal", "case", "cat"] (NO DUPLICATES, sorted)

Type: cat $PA[TAB]
Result: ["$PATH", "$PAM_*", ...] (variables only)
```

**This is working completion generation.**

---

## Timeline Estimate

- Phase 1 (Core components): ~2 hours
- Phase 2 (Context analyzer): ~3 hours  
- Phase 3 (Source manager): ~2 hours
- Phase 4 (Generation + dedup): ~2 hours
- Testing: ~2 hours
- **Total**: ~11 hours of focused work

---

## Next Steps After Spec 12 Core

Once completion generation works correctly:

1. **Screen Buffer Integration**
   - Implement `screen_buffer_render_with_menu()`
   - Fix menu rendering corruption
   - Menu through virtual layout system

2. **Interactive Features - Single Line**
   - Menu navigation (arrow keys)
   - Completion acceptance (Enter)
   - Menu dismissal (ESC, character input)

3. **Interactive Features - Multi Line**  
   - Inline TAB cycling
   - Replace word in buffer with completion

4. **Polish**
   - Configuration options
   - Performance optimization
   - Documentation

---

## Conclusion

This plan implements Spec 12 core completion system properly. The result will be:

- ✓ No duplicates
- ✓ Correct categorization  
- ✓ Context-aware completion
- ✓ Extensible architecture
- ✓ Solid foundation for interactive features

**Build the foundation right, THEN add the features.**
