# LLE Specification Implementation Policy

**Version**: 1.0.0  
**Date**: 2025-10-15  
**Status**: MANDATORY CRITICAL POLICY - NO EXCEPTIONS  
**Authority**: User-mandated after CRITICAL spec deviation discovery

---

## POLICY STATEMENT

**ALL LLE specification implementations MUST include COMPLETE structure definitions with ALL fields from the audited specification, even if most functionality is stubbed for future implementation.**

**This policy is MANDATORY and CRITICAL. There are NO exceptions without explicit user approval.**

---

## Purpose and Background

### Why This Policy Exists

Following the discovery of CRITICAL specification deviation in Week 10 Input Processing (ISSUE-004), a comprehensive audit revealed:

- **Spec 02** (Terminal): ~85% compliant (minor additions acceptable)
- **Spec 03** (Buffer): **22% compliant** - Missing 7 of 9 major components
- **Spec 06** (Input): ✅ 100% compliant (after fix)
- **Spec 08** (Display): **15% compliant** - Missing 12 of 14 components  
- **Spec 09** (History): **12% compliant** - Missing 17.5 of 20 components

### Root Cause

AI assistant created "simplified" implementations without user approval:
- Invented structure names not in specs
- Omitted required fields to "start simple"
- Assumed gradual addition was acceptable
- Did not follow audited specifications exactly

### Impact

- Future integration failures (components expect spec-compliant APIs)
- Technical debt compounds (harder to add fields later)
- Architectural inconsistency across components
- Undermines value of specification audit work
- Reduces implementation success probability

---

## MANDATORY RULES

### Rule 1: Complete Structure Definitions

**REQUIREMENT**: All structures from specifications MUST be implemented with ALL fields present, even if many are NULL/unimplemented.

**Example** (Spec 06 - Input Parsing):

```c
// ✅ CORRECT - All 22 fields present
typedef struct lle_input_parser_system {
    // Core parsing components (7 fields)
    lle_input_stream_t *stream;                     // NULL if not implemented
    lle_sequence_parser_t *sequence_parser;         // NULL if not implemented
    lle_utf8_processor_t *utf8_processor;           // NULL if not implemented
    lle_key_detector_t *key_detector;               // NULL if not implemented
    lle_mouse_parser_t *mouse_parser;               // NULL if not implemented
    lle_parser_state_machine_t *state_machine;      // NULL if not implemented
    lle_input_buffer_t *input_buffer;               // NULL if not implemented
    
    // Critical system integrations (3 fields)
    lle_keybinding_integration_t *keybinding_integration;  // NULL - TODO_SPEC06
    lle_widget_hook_triggers_t *widget_hook_triggers;      // NULL - TODO_SPEC06
    lle_adaptive_terminal_parser_t *adaptive_terminal;     // NULL - TODO_SPEC06
    
    // System coordination (3 fields)
    lle_event_system_t *event_system;               // NULL - TODO_SPEC06
    lle_input_coordinator_t *coordinator;           // NULL - TODO_SPEC06
    lle_input_conflict_resolver_t *conflict_resolver; // NULL - TODO_SPEC06
    
    // Performance and optimization (4 fields)
    lle_error_context_t *error_ctx;                 // NULL - TODO_SPEC06
    lle_performance_monitor_t *perf_monitor;        // NULL - TODO_SPEC06
    lle_input_cache_t *input_cache;                 // NULL - TODO_SPEC06
    lle_memory_pool_t *memory_pool;                 // NULL - TODO_SPEC06
    
    // Synchronization and state (5 fields)
    pthread_mutex_t parser_mutex;                   // Initialized
    bool active;
    uint64_t bytes_processed;
    uint64_t keybinding_lookups;
    uint64_t widget_hooks_triggered;
} lle_input_parser_system_t;
```

```c
// ❌ WRONG - Simplified version
typedef struct lle_simple_input_processor {
    int input_fd;                    // Invented simplified version
    bool running;
    // Only ~10 fields instead of 22
} lle_simple_input_processor_t;      // Wrong name!
```

### Rule 2: Exact Naming Compliance

**REQUIREMENT**: All structure names and function names MUST match specification exactly.

**Examples**:

✅ **CORRECT**:
- `lle_input_parser_system_t` (as per Spec 06)
- `lle_input_parser_system_init()` (as per Spec 06)
- `lle_buffer_system_t` (as per Spec 03)
- `lle_buffer_system_init()` (as per Spec 03)

❌ **WRONG**:
- `lle_simple_input_processor_t` (invented name)
- `lle_simple_input_init()` (invented name)
- `lle_buffer_t` when spec calls it `lle_buffer_system_t`
- `lle_buffer_manager_t` when spec calls it `lle_buffer_system_t`

### Rule 3: TODO Marking for Stubs

**REQUIREMENT**: All unimplemented components MUST be marked with `TODO_SPEC##` comments referencing the specification.

**Format**:
```c
// TODO_SPEC06: Mouse input parsing not yet implemented (Spec 06 Section 9)
// Target: Phase 1 Month 3 (Advanced Input Features)
lle_mouse_parser_t *mouse_parser;  // NULL for now
```

### Rule 4: Supporting Type Declarations

**REQUIREMENT**: All supporting types referenced in main structures MUST be forward-declared or typedef'd, even if not yet implemented.

```c
// Forward declarations for types not yet implemented
typedef struct lle_mouse_parser lle_mouse_parser_t;
typedef struct lle_utf8_processor lle_utf8_processor_t;
typedef struct lle_keybinding_integration lle_keybinding_integration_t;
```

### Rule 5: Function Signature Compliance

**REQUIREMENT**: All public API functions MUST match specification signatures exactly (parameter count, types, order).

```c
// ✅ CORRECT - Matches Spec 06 Section 2.2 exactly
lle_result_t lle_input_parser_system_init(
    lle_input_parser_system_t **system,
    void *terminal,
    void *event_system,
    void *keybinding_engine,
    void *widget_hooks,
    void *adaptive_terminal,
    void *memory_pool);
```

```c
// ❌ WRONG - Simplified signature
int lle_simple_input_init(
    lle_simple_input_processor_t *processor,  // Wrong type
    int input_fd,
    lle_buffer_manager_t *buffer_manager,
    lle_display_buffer_renderer_t *renderer);
```

### Rule 6: No Invented Names

**REQUIREMENT**: Do NOT invent simplified names. Use specification names exactly.

❌ **FORBIDDEN**:
- `lle_simple_*` prefix (not in any spec)
- `lle_basic_*` prefix (not in any spec)  
- `lle_minimal_*` prefix (not in any spec)
- Any other invented prefixes/suffixes

✅ **REQUIRED**: Use exact names from specification documents.

### Rule 7: Deviation Requires User Approval

**REQUIREMENT**: ANY deviation from specification (naming, structure, fields, signatures) requires EXPLICIT user approval with written rationale.

**Process**:
1. Document proposed deviation with detailed justification
2. Present to user for explicit approval
3. Document approval in `KNOWN_ISSUES.md` with rationale
4. Reference approval in code comments

**No assumptions. No "it's simpler this way." No "we'll add it later."**

---

## IMPLEMENTATION PROCESS

### Pre-Implementation Checklist (MANDATORY)

Before writing ANY code for a specification component:

- [ ] Read COMPLETE specification document
- [ ] Extract ALL structure definitions (copy entire structures)
- [ ] Count ALL fields in each structure
- [ ] Extract ALL function signatures
- [ ] Verify function naming patterns
- [ ] Check for ALL supporting type definitions
- [ ] Create extraction document (like `SPEC_##_EXTRACTION_AUDIT.md`)
- [ ] Review extraction document for completeness
- [ ] Get user approval if ANY deviations needed

### Implementation Checklist (MANDATORY)

During implementation:

- [ ] Create ALL structures with ALL fields from spec
- [ ] Forward-declare ALL supporting types
- [ ] Use exact names from specification
- [ ] Mark stubs with `TODO_SPEC##` comments
- [ ] Initialize ALL pointers (even if to NULL)
- [ ] Implement working subset (e.g., Week 10 scope)
- [ ] Leave hooks for future components (NULL pointers with TODOs)
- [ ] Document what's implemented vs stubbed

### Code Review Checklist (MANDATORY)

Before committing:

- [ ] All structures have all spec fields (count them!)
- [ ] All functions match spec naming exactly
- [ ] No invented names (`lle_simple_*`, etc.)
- [ ] All stubs clearly marked with TODO_SPEC##
- [ ] Supporting types forward-declared
- [ ] Extraction document matches implementation
- [ ] Tests updated to use correct names
- [ ] No compiler errors or warnings (except unused parameter for stubs)

### Documentation Checklist (MANDATORY)

After implementation:

- [ ] Week completion document notes partial vs full implementation
- [ ] `KNOWN_ISSUES.md` updated with TODO items for stubs
- [ ] Extraction audit document saved in repo root
- [ ] Commit message documents compliance status
- [ ] Future work clearly marked in code comments

---

## ENFORCEMENT

### Pre-Commit Hooks

The following checks will be added to pre-commit hooks:

1. **Structure Field Count**: Verify structure field counts match extraction documents
2. **Naming Pattern Check**: Detect invented names (`lle_simple_*`, `lle_basic_*`)
3. **TODO Marking**: Verify stub fields have `TODO_SPEC##` comments
4. **Extraction Documents**: Verify extraction document exists for component

### Code Review Requirements

All LLE implementations require:

1. Extraction document present and reviewed
2. Field-by-field verification against spec
3. Naming compliance verification
4. Stub marking verification
5. Test update verification

### Violation Response

**Violations of this policy are CRITICAL severity.**

Response to violations:
1. **Immediate halt** of all work on component
2. **Complete reimplementation** with full spec compliance
3. **Root cause analysis** of why policy was violated
4. **Process improvement** to prevent recurrence
5. **Documentation update** with lessons learned

No shortcuts. No "good enough for now." No "we'll fix it later."

---

## RATIONALE

### Why Complete Structures Matter

1. **Future Integration**: Other components expect spec-compliant APIs
2. **Architecture Consistency**: All components follow same patterns
3. **Clear Scope**: NULL fields show what's not implemented
4. **Documentation**: Structure itself documents full capability
5. **No Surprises**: Users know exactly what's implemented vs planned
6. **Easier Testing**: Can test structure layout and field presence
7. **Refactoring Safety**: Adding functionality doesn't break structure layout

### Why Exact Naming Matters

1. **Specification Authority**: Specs are audited, names are intentional
2. **Component Discovery**: Can find components by spec name
3. **Cross-Reference**: Easy to map code to specification sections
4. **No Confusion**: Everyone uses same terminology
5. **Integration Predictability**: Components know what to expect

### Why Stubs Are Better Than Gaps

1. **Clear Intent**: `NULL // TODO_SPEC06` shows unimplemented, not forgotten
2. **Compile-Time Check**: Structure layout is fixed early
3. **Memory Layout**: Consistent across versions
4. **Easy Addition**: Just implement the stub, don't restructure
5. **Documentation**: Code shows full planned capability

---

## EXAMPLES

### ✅ COMPLIANT Implementation (Spec 06 - Input Parsing)

See `src/lle/foundation/input/input_processor.h` after commit `a94fb2b`:

- ✅ Structure name: `lle_input_parser_system_t` (exact spec name)
- ✅ Field count: 22 (matches Spec 06 Section 2.1 exactly)
- ✅ Function naming: `lle_input_parser_system_init()` (exact spec name)
- ✅ All stub fields marked: `// TODO_SPEC06: ...`
- ✅ Supporting types declared: All forward declarations present
- ✅ Week 10 functionality: Works with stubs
- ✅ Extraction document: `SPEC_06_EXTRACTION_FOR_WEEK_10.md` created

### ❌ NON-COMPLIANT Implementation (Spec 03 - Buffer - Before Fix)

See `src/lle/foundation/buffer/buffer.h` current state:

- ❌ Structure name: `lle_buffer_t` (spec calls it `lle_buffer_system_t`)
- ❌ Field count: ~12 (spec requires 9 major components in system structure)
- ❌ Missing structure: `lle_buffer_system_t` doesn't exist at all
- ❌ Function naming: `lle_buffer_init()` (spec wants `lle_buffer_system_init()`)
- ❌ Missing components: 7 of 9 components not present
- ❌ No stubs: Missing fields simply absent, not marked as TODO
- ❌ No UTF-8 processor: Critical for Week 11, not even stubbed

**This MUST be fixed before Week 11.**

---

## SPECIFICATION COMPLIANCE MATRIX

| Spec | Component | Compliance | Status |
|------|-----------|------------|--------|
| 02 | Terminal Abstraction | ~85% | ⚠️ Minor deviations acceptable |
| 03 | Buffer Management | 22% | ❌ CRITICAL - Requires fix |
| 06 | Input Parsing | 100% | ✅ COMPLIANT (after fix) |
| 08 | Display Integration | 15% | ❌ Requires fix |
| 09 | History System | 12% | ❌ Requires fix |

**Priority Order for Fixes**:
1. **Spec 03** (Buffer) - CRITICAL, blocks Week 11
2. **Spec 08** (Display) - HIGH, architecture incomplete
3. **Spec 09** (History) - MEDIUM, basic functionality works
4. **Spec 02** (Terminal) - LOW, mostly compliant

---

## POLICY UPDATES

This policy may be updated only with explicit user approval. All updates must be versioned and documented.

**Version History**:
- v1.0.0 (2025-10-15): Initial policy following ISSUE-004 discovery

---

## ACKNOWLEDGMENT

By implementing any LLE specification component, you acknowledge:

1. You have read this policy in full
2. You understand the requirements
3. You will follow the process exactly
4. You will not deviate without user approval
5. You understand violations are CRITICAL severity

**This policy exists to prevent implementation failures and ensure architectural consistency. It is not optional.**

---

## CONTACT

**Questions about this policy**: Ask user for clarification  
**Deviation requests**: Document and request explicit user approval  
**Enforcement issues**: Report immediately to user

**Remember**: The specifications were carefully audited. Follow them exactly.
