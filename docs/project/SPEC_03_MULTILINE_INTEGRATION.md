# Spec 03 Phase 7: Multiline Manager Integration Design

**Date**: 2025-10-24  
**Status**: Design Document  
**Decision**: Integrate with existing `input_continuation.c` rather than reimplementing

## Background

Spec 03 Phase 7 requires a multiline manager for tracking shell constructs (quotes, brackets, control structures) within the LLE buffer system.

Lusush already has a working multiline parser in `src/input_continuation.c` that handles:
- Quote tracking (single, double, backtick)
- Bracket/brace/parenthesis counting
- Control structure detection (if/while/for/case)
- Here-document parsing
- Continuation line detection

## Decision

**Use integration wrapper approach** rather than reimplementing the parser.

### Rationale

1. **DRY Principle** - Avoid duplicating 400 lines of working, tested parsing logic
2. **Consistency** - LLE and main input system use same parser (same behavior)
3. **Maintainability** - One parser to maintain and fix bugs in
4. **Risk Reduction** - Reuse proven code rather than introducing new bugs
5. **Spec Compliance** - Wrapper can expose exact API required by Spec 03

## Architecture

### Component Relationship

```
┌─────────────────────────────────────┐
│   LLE Buffer Management (Spec 03)   │
│                                     │
│  ┌───────────────────────────────┐ │
│  │  Multiline Manager Wrapper    │ │  ← New component
│  │  (src/lle/multiline_manager.c)│ │
│  └──────────┬────────────────────┘ │
│             │ uses                 │
└─────────────┼──────────────────────┘
              │
              ▼
   ┌────────────────────────────┐
   │  Shared Multiline Parser   │  ← Existing component
   │  (src/input_continuation.c)│
   └────────────────────────────┘
```

### Data Structure Mapping

**Spec 03 requires:**
```c
typedef struct lle_multiline_context {
    lle_shell_parser_state_t parser_state;
    lle_bracket_stack_t *bracket_stack;
    lle_quote_state_t quote_state;
    char *current_construct;
    size_t construct_start_line;
    bool construct_complete;
    bool needs_continuation;
    // ... other fields
} lle_multiline_context_t;
```

**We have:**
```c
typedef struct continuation_state_t {
    int quote_count;
    bool in_single_quote;
    bool in_double_quote;
    int paren_count;
    int brace_count;
    int bracket_count;
    bool in_if_statement;
    bool in_case_statement;
    // ... other fields
} continuation_state_t;
```

**Wrapper approach:**
```c
struct lle_multiline_context {
    continuation_state_t *core_state;  // Delegate to existing parser
    
    // LLE-specific additions
    size_t construct_start_line;
    size_t construct_start_offset;
    char *current_construct;
    uint32_t cache_key;
    bool cache_valid;
};
```

## Implementation Plan

### Phase 7.1: Wrapper Infrastructure (2-3 hours)

**Create**: `src/lle/multiline_manager.c`

**Functions**:
```c
// Lifecycle
lle_result_t lle_multiline_context_init(lle_multiline_context_t **ctx);
lle_result_t lle_multiline_context_destroy(lle_multiline_context_t *ctx);
lle_result_t lle_multiline_context_reset(lle_multiline_context_t *ctx);

// Core delegation
lle_result_t lle_multiline_analyze_line(
    lle_multiline_context_t *ctx,
    const char *line,
    size_t length
);

bool lle_multiline_is_complete(const lle_multiline_context_t *ctx);
bool lle_multiline_needs_continuation(const lle_multiline_context_t *ctx);

// State queries
const char *lle_multiline_get_prompt(const lle_multiline_context_t *ctx);
const char *lle_multiline_get_construct(const lle_multiline_context_t *ctx);
```

**Implementation strategy**:
- Wrap `continuation_state_t` inside `lle_multiline_context_t`
- Delegate parsing to `continuation_analyze_line()`
- Add LLE-specific tracking (line numbers, construct names)
- Map completion status from `continuation_is_complete()`

### Phase 7.2: Buffer Integration (2-3 hours)

**Create**: `src/lle/multiline_buffer_integration.c` (or add to multiline_manager.c)

**Functions**:
```c
lle_result_t lle_multiline_manager_analyze_buffer(
    lle_multiline_manager_t *manager,
    lle_buffer_t *buffer
);

lle_result_t lle_multiline_manager_update_line_state(
    lle_multiline_manager_t *manager,
    lle_buffer_t *buffer,
    size_t line_index
);
```

**Features**:
- Analyze entire buffer line-by-line
- Update `lle_line_info_t.ml_state` for each line
- Set `LLE_LINE_FLAG_NEEDS_CONTINUATION` flags
- Track `buffer->multiline_active` status

### Phase 7.3: Testing (2-4 hours)

**Compliance tests**:
- Structure verification (multiline context exists)
- API coverage (all required functions present)

**Functional tests**:
- Single-line complete input
- Multiline quote handling
- Multiline if/then/fi structures
- Nested constructs
- Here-document parsing

**Integration tests**:
- Buffer operations with multiline state
- Multiline state preservation across edits
- Line state updates

## Testing Strategy

### Unit Tests
- Test wrapper functions delegate correctly
- Test state mapping between continuation_state_t and lle_multiline_context_t
- Test prompt generation

### Integration Tests
- Test buffer-wide analysis
- Test line state updates
- Test multiline flag management

### Compatibility Tests
- Verify LLE and main input system parse identically
- Test same inputs through both parsers

## Performance Considerations

**Existing parser performance** (from input_continuation.c):
- Linear scan per line: O(n) where n = line length
- Minimal allocations (only here-doc delimiter)
- No complex data structures

**Wrapper overhead**:
- Delegation: negligible (function call)
- State mapping: O(1) pointer dereference
- LLE tracking: O(1) field updates

**Buffer analysis**: O(m × n) where m = line count, n = avg line length
- Acceptable for typical shell inputs (<100 lines)
- Can add caching if needed (cache_key, cache_valid)

## Compliance with Spec 03

### Required Features (from spec)
- ✅ Multiline context structure
- ✅ Buffer analysis function
- ✅ Line state tracking
- ✅ Completion detection
- ✅ Continuation detection
- ✅ Construct identification

### Spec Deviations
- **Internal implementation**: Uses delegation rather than reimplementation
- **API**: Exposes exact API required by spec
- **Behavior**: Identical to spec requirements (delegates to proven parser)

### Justification
The spec defines **what** the multiline manager must do, not **how** it must be implemented. Using a proven parser is an implementation detail that meets all functional requirements.

## Risks and Mitigations

### Risk: API Mismatch
**Mitigation**: Wrapper provides exact API from spec, internal delegation is transparent

### Risk: Behavioral Differences
**Mitigation**: Write compatibility tests comparing LLE and main input parsing

### Risk: Performance Overhead
**Mitigation**: Measure performance, add caching if needed

### Risk: Future Divergence
**Mitigation**: Document that both systems use same parser, enforce with tests

## Success Criteria

✅ All Spec 03 Phase 7 APIs implemented  
✅ Compliance tests pass  
✅ Functional tests pass (multiline parsing works)  
✅ Integration tests pass (buffer integration works)  
✅ Zero code duplication with input_continuation.c  
✅ Compatibility tests pass (same parsing behavior)  
✅ Performance meets spec requirements (<1ms per line)  
✅ Zero memory leaks (valgrind verified)  

## Estimated Effort

- Phase 7.1: Wrapper infrastructure - 2-3 hours
- Phase 7.2: Buffer integration - 2-3 hours  
- Phase 7.3: Testing - 2-4 hours
- **Total: 6-10 hours (approximately 1 day)**

## Next Steps

1. Review this design with user
2. Implement Phase 7.1 (wrapper)
3. Implement Phase 7.2 (buffer integration)
4. Implement Phase 7.3 (testing)
5. Update Spec 03 completion status
6. Move to performance benchmarks and memory audit
