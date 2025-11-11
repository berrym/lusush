# Lusush Known Bugs

This directory contains permanent documentation of known bugs in lusush. These documents should NOT be deleted until the bugs are completely fixed and verified.

---

## 🔴 CRITICAL Bugs

### [CRITICAL_PARSER_UTF8_BUG.md](./CRITICAL_PARSER_UTF8_BUG.md)
**Status**: Open  
**Discovered**: 2025-11-11 (Session 12)  
**Component**: Shell parser/tokenizer  
**Impact**: Cannot execute commands containing UTF-8 characters  

**Summary**: The shell parser/tokenizer is not UTF-8 aware. When a command contains multi-byte UTF-8 characters, the tokenizer misinterprets the bytes and fails with "syntax error: unterminated quoted string".

**Examples**:
```bash
$ echo café
lusush: syntax error: unterminated quoted string

$ echo 日本  
lusush: syntax error: unterminated quoted string
```

**Next Steps**: Complete LLE UTF-8 tests (3-7), then implement UTF-8 aware tokenizer.

---

## Bug Document Guidelines

### When to Create a Bug Document

Create a permanent bug document when:
- Bug is CRITICAL or HIGH severity
- Bug affects core functionality
- Bug requires significant investigation/fix
- Bug might be rediscovered later if not documented
- Bug is discovered but cannot be fixed immediately

### Bug Document Template

```markdown
# SEVERITY: Bug Title

**Status**: Open/In Progress/Fixed  
**Discovered**: Date  
**Component**: Affected components  
**Impact**: User/functional impact  
**Severity**: CRITICAL/HIGH/MEDIUM/LOW  
**Priority**: HIGH/MEDIUM/LOW  

## Reproduction
[Steps to reproduce]

## Technical Analysis
[Root cause, affected code]

## Impact Assessment
[Who is affected, how severe]

## Potential Solutions
[Possible fixes]

## Testing Strategy
[How to verify fix]

## References
[Related docs, code]
```

### Bug Document Lifecycle

1. **Created**: Bug discovered and documented
2. **In Progress**: Fix implementation started
3. **Fixed**: Code changed to fix bug
4. **Verified**: Tests confirm fix works
5. **Closed**: Document archived (moved to bugs/closed/ directory)

---

## Finding Bug Documents

All bug documents are in this directory (`docs/bugs/`).

**By Severity**:
- CRITICAL: Bugs that block core functionality
- HIGH: Bugs that significantly impair functionality
- MEDIUM: Bugs that cause inconvenience
- LOW: Minor issues or edge cases

**By Status**:
- Open: Not yet fixed
- In Progress: Fix implementation started
- Fixed: Code changed but not fully verified
- Closed: Fixed and verified (moved to bugs/closed/)

---

## Related Documentation

- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - Current work status (living document, changes frequently)
- `docs/development/` - Implementation documentation
- `docs/bugs/` - Permanent bug documentation (this directory)

The key difference: AI handoff document is a **living document** that changes with each session. Bug documents are **permanent records** that persist until bugs are fixed.
