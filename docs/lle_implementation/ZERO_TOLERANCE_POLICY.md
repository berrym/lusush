# LLE Zero-Tolerance Implementation Policy

**Document**: ZERO_TOLERANCE_POLICY.md  
**Date**: 2025-10-23  
**Status**: AUTHORITATIVE - Supersedes all previous policies  
**Authority**: User mandate  
**Enforcement**: Pre-commit hooks (BLOCKING)

---

## Policy Statement

**ALL LLE code committed to the repository MUST be 100% specification-compliant with ZERO tolerance for incomplete implementations.**

This is a **MANDATORY, ABSOLUTE, NON-NEGOTIABLE** policy with **NO EXCEPTIONS**.

---

## What Zero-Tolerance Means

### FORBIDDEN (Will cause commit rejection or nuclear revert)

1. **Stubs** - Functions that return error codes without implementation
2. **TODOs** - Any TODO/STUB/FIXME markers in code or comments
3. **Simplified Implementations** - Algorithms that don't match spec requirements
4. **Deferred Work** - Comments indicating "implement later" or "phase X"
5. **Partial Implementations** - Structures missing required fields
6. **Emoji in Commits** - Unprofessional, violates git log standards
7. **False Compliance Claims** - Claiming "100% compliant" when code has simplifications

### REQUIRED (Enforcement is absolute)

1. **100% Spec Compliance** - All implemented code must exactly match specifications
2. **Complete Structures** - All structure fields from spec must be present and functional
3. **Complete Functions** - All implemented functions must be fully functional
4. **Exact Names** - Structure/function names must match spec exactly
5. **Professional Commits** - Plain text only, no emoji, accurate descriptions

---

## Phased Implementation - ONLY Acceptable Approach

**Question**: "How can I implement large specs without creating simplified code?"

**Answer**: Implement complete functionality in layers that may not compile until dependencies are ready.

### Acceptable Phasing Strategy

**Implement FEWER functions completely, not ALL functions incompletely**

#### Example: Spec 03 Buffer Management (Correct Approach)

**Phase 1: Foundation Layer**
```c
// ✓ CORRECT: Complete structure with all fields
typedef struct lle_buffer_system {
    lle_buffer_t *current_buffer;           // Implemented
    lle_buffer_pool_t *buffer_pool;         // Implemented  
    lle_cursor_manager_t *cursor_mgr;       // NULL - not in Phase 1
    lle_change_tracker_t *change_tracker;   // NULL - not in Phase 1
    lle_buffer_validator_t *validator;      // NULL - not in Phase 1
    // ... ALL fields present, some NULL
} lle_buffer_system_t;

// ✓ CORRECT: Fully functional init (initializes what's in Phase 1)
lle_result_t lle_buffer_system_init(lle_buffer_system_t **system) {
    // Complete implementation for Phase 1 components
    // Other components remain NULL until their phase
}
```

**Status**: May not compile if `lle_cursor_manager_t` not defined yet - **THIS IS ACCEPTABLE**

**Phase 2: Cursor Layer**
```c
// ✓ CORRECT: Now implement cursor manager completely
typedef struct lle_cursor_manager {
    // ALL fields from spec, fully implemented
} lle_cursor_manager_t;

lle_result_t lle_cursor_manager_init(...) {
    // 100% complete implementation
}
```

**Status**: Now buffer_system compiles because cursor_manager is defined

#### What's NOT Acceptable

```c
// ❌ WRONG: Simplified structure
typedef struct lle_simple_buffer {
    char *data;      // Only 2 fields instead of 10
    size_t length;   // Missing required fields
} lle_simple_buffer_t;   // Wrong name!

// ❌ WRONG: "Phase 3 simplification" comment
size_t lle_utf8_count_graphemes(const char *text, size_t length) {
    // Phase 3 simplification: grapheme count = codepoint count
    // Full grapheme cluster rules deferred
    return lle_utf8_count_codepoints(text, length);  // WRONG ALGORITHM
}

// ❌ WRONG: Stub function
lle_result_t lle_some_function(...) {
    // TODO: Implement in Phase 5
    return LLE_ERROR_NOT_IMPLEMENTED;
}
```

---

## Enforcement Mechanisms

### Pre-Commit Hooks (Automatic BLOCKING)

The `.git/hooks/pre-commit` script enforces:

1. **Emoji Detection**: Blocks any commit with emoji characters
2. **TODO/STUB/FIXME**: Blocks code with incomplete markers  
3. **Simplified Language**: Blocks comments with "simplified" or "simplification"
4. **Deferred Language**: Blocks comments with "deferred" or "defer to later"
5. **False Claims**: Warns on "100% compliant" claims (3-second delay to reconsider)
6. **Stub Returns**: Blocks `LLE_ERROR_NOT_IMPLEMENTED` or `LLE_ERROR_FEATURE_NOT_AVAILABLE`

### Nuclear Option (Manual Response to Violations)

**If violations slip through hooks or are discovered later:**

1. **Immediate identification** of violating commits
2. **Full git reset** to state before violations (nuclear revert)
3. **Deletion** of any violating documentation
4. **Strengthening** of enforcement to prevent recurrence

**Recent Example (2025-10-23)**:
- 21 commits reverted (Spec 03, 08, 22 work)
- Reason: Emoji in commits, simplified implementations, false compliance claims
- Reset to commit `59ad8a9` (clean state)

---

## Why This Policy Exists

### Past Failures

**Nuclear Option #1**: Custom APIs instead of spec APIs  
**Nuclear Option #2**: Simplified implementations claimed as "complete"  
**Nuclear Option #3**: Emoji violations, false claims, zero-tolerance violations

### The Mandate

> "we will have to completely implement the specs with no incomplete code"
> 
> "there should be no such thing as a minimal anything that is in direct violation of the mandates"
>
> "phased approach was only meant for implementing subsets of complete types and functions at a time, then continuing with more complete and spec compliant implementations"
>
> "no matter what all specs must be 100% completely implemented and 100% spec compliant"

### Success Criteria

**How we raise the probability of LLE success:**
1. Complete API designs validated before implementation
2. Zero tolerance for simplified code
3. 100% spec compliance maintained throughout
4. Professional development standards enforced
5. Clean git history enables future integration

---

## Frequently Asked Questions

### Q: "Can I use phased implementation?"

**A**: Yes, but only to implement **fewer functions completely**, not **all functions incompletely**.

Implement foundation layer with complete structures (some fields NULL). Then implement next layer completely. Code may not compile between phases - this is acceptable as long as each commit contains 100% compliant code for what IS implemented.

### Q: "What if the spec is too large to implement at once?"

**A**: Break it into layers (foundation, core operations, advanced features). Each layer implements complete functionality. Later layers build on earlier ones.

**NOT acceptable**: Implement all layers with simplified algorithms, plan to "fix later".

### Q: "Can I add TODO comments for unimplemented features?"

**A**: No. If a feature isn't implemented yet, don't mention it in code. Document in living documents what's not yet done. Code must reflect only what IS implemented, completely.

### Q: "What about circular dependencies between specs?"

**A**: Implement in layers where each spec is partially complete but what IS complete is 100% spec-compliant. Code may not compile until dependencies are resolved - acceptable.

**Example**: Spec 03 Phase 1 implements buffer without cursor (cursor is NULL). Spec 04 implements cursor. Spec 03 Phase 2 adds cursor integration. All code at each step is 100% compliant for what exists.

### Q: "My commit was rejected for emoji. Can I just remove the emoji and recommit?"

**A**: Yes. Amend the commit to remove emoji and recommit. Use plain text: "FIXED", "COMPLETE", "WARNING" instead of ✅/❌/⚠️.

### Q: "I claimed '100% compliant' but it's actually simplified. What happens?"

**A**: Nuclear option will be invoked. All violating commits reverted. This is why the pre-commit hook warns you with a 3-second delay on compliance claims.

---

## Relationship to Other Documents

**This document SUPERSEDES:**
- ~~SPECIFICATION_IMPLEMENTATION_POLICY.md~~ (deleted - allowed stubs)
- ~~SPEC_03_PHASED_IMPLEMENTATION_PLAN.md~~ (deleted - mandated simplified implementations)
- Any other documents suggesting incomplete implementations are acceptable

**This document COMPLEMENTS:**
- Specification files (define WHAT to implement)
- Implementation guides (define HOW to structure work)
- Living documents (track WHAT is done and what remains)

---

## Conclusion

**Zero tolerance means ZERO tolerance.**

No stubs. No TODOs. No simplified implementations. No deferrals. No emoji. No false claims.

100% spec compliance when committed, or don't commit.

**This is the only acceptable standard for LLE development.**
