# LLE Architectural Compliance Safeguards - Setup Guide

**Date**: 2025-10-17  
**Purpose**: Installation and usage instructions for compliance tools  
**Status**: MANDATORY for all LLE development

---

## Overview

Following the discovery of fundamental architectural violations in LLE display code, comprehensive safeguards have been implemented to prevent future violations. This document explains how to install and use them.

**Core Principle**: Code that violates architecture CANNOT be committed.

---

## Quick Start (5 Minutes)

```bash
# 1. Make scripts executable
chmod +x scripts/lle_architectural_compliance_check.sh
chmod +x scripts/pre-commit-lle-compliance

# 2. Install pre-commit hook
ln -sf ../../scripts/pre-commit-lle-compliance .git/hooks/pre-commit

# 3. Test it works
./scripts/lle_architectural_compliance_check.sh

# 4. Read the reference implementation
less docs/lle_implementation/CORRECT_DISPLAY_IMPLEMENTATION.md
```

Done! Now you're protected from architectural violations.

---

## Components Installed

### 1. Architectural Compliance Check Script

**File**: `scripts/lle_architectural_compliance_check.sh`

**Purpose**: Automated detection of architectural violations

**Checks Performed** (14 checks total):
- ✅ No direct terminal writes in display code
- ✅ No ANSI escape sequences
- ✅ Display controller integration present
- ✅ No direct terminal control in editor
- ✅ Display system API usage
- ✅ No sprintf of escape sequences
- ✅ Terminal abstraction integrity
- ✅ Lusush display integration points
- ✅ Spec 08 required structures
- ✅ No prohibited function patterns
- ✅ Display flush implementation check
- ✅ No hardcoded terminal sequences
- ✅ Display error handling patterns
- ✅ Memory pool usage patterns

**Run Manually**:
```bash
./scripts/lle_architectural_compliance_check.sh
```

**Expected Output When Code is Correct**:
```
=======================================================================
LLE Architectural Compliance Check
Version: 1.0.0
=======================================================================

--- CRITICAL ARCHITECTURAL COMPLIANCE CHECKS ---

✓ PASS: No direct terminal writes in display code
✓ PASS: No ANSI escape sequences in display code
✓ PASS: Display controller integration appears present
...

COMPLIANCE CHECK PASSED

LLE code appears to follow architectural specifications.
All critical compliance checks passed.
```

**Expected Output When Violations Found**:
```
✗ VIOLATION: Direct terminal writes in display code
  Details: Display code MUST NOT write directly to terminal fd. Use Lusush display system.

COMPLIANCE CHECK FAILED

CRITICAL VIOLATIONS FOUND: 3
```

---

### 2. Pre-Commit Hook

**File**: `scripts/pre-commit-lle-compliance`  
**Installed At**: `.git/hooks/pre-commit`

**Purpose**: Automatically block commits containing architectural violations

**Installation**:
```bash
# Create symlink (recommended - auto-updates when script changes)
ln -sf ../../scripts/pre-commit-lle-compliance .git/hooks/pre-commit

# Or copy (not recommended - won't get updates)
cp scripts/pre-commit-lle-compliance .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

**Behavior**:
- Runs automatically when you `git commit`
- Only checks if LLE files are being committed
- Blocks commit if violations found
- Shows detailed error messages
- Provides bypass option (discouraged)

**Example Usage**:
```bash
$ git add src/lle/foundation/display/display.c
$ git commit -m "Fix display rendering"

LLE files detected in commit, running architectural compliance check...

✗ VIOLATION: Direct terminal writes in display code
  Details: Display code MUST NOT write directly to terminal fd. Use Lusush display system.

COMPLIANCE CHECK FAILED

==========================================
COMMIT BLOCKED: Architectural Violations
==========================================

LLE code contains architectural violations.
Fix violations before committing.

To bypass (NOT RECOMMENDED):
  git commit --no-verify
```

**Bypass** (Use ONLY if you know what you're doing):
```bash
git commit --no-verify -m "message"
```

---

### 3. Correct Implementation Reference

**File**: `docs/lle_implementation/CORRECT_DISPLAY_IMPLEMENTATION.md`

**Purpose**: Shows exactly how to implement display code correctly

**Contents**:
- Current violating code (what NOT to do)
- Correct implementation pattern (what TO do)
- Display integration structure
- Proper rendering through display system
- Syntax highlighting integration
- Step-by-step migration path
- Compliance verification checklist

**When to Read**:
- **BEFORE** implementing any display code
- When fixing BLOCKER-001 (display overflow)
- When fixing BLOCKER-003 (color rendering)
- When uncertain about proper integration
- During code review

**Key Sections**:
- Section "Current Violating Code" - Learn what to avoid
- Section "Correct Implementation Pattern" - Follow this exactly
- Section "Key Differences" - Understand the changes
- Section "Compliance Verification Checklist" - Verify your work

---

### 4. Design Violation Analysis

**File**: `docs/lle_implementation/tracking/DESIGN_VIOLATION_ANALYSIS.md`

**Purpose**: Complete analysis of what went wrong and why

**Contents**:
- Design requirements from specifications
- What was actually implemented
- Architecture violations identified
- Root cause analysis
- Why this was missed
- Lessons learned
- Prevention measures

**When to Read**:
- To understand why violations are serious
- To learn from mistakes
- To understand specification importance
- When training new developers
- For context on recovery plan

---

## Usage Workflows

### Workflow 1: Starting New Feature

```bash
# 1. Read specification
less docs/lle_specification/08_display_integration_complete.md

# 2. Read correct implementation guide
less docs/lle_implementation/CORRECT_DISPLAY_IMPLEMENTATION.md

# 3. Plan implementation following spec

# 4. Implement feature

# 5. Run compliance check
./scripts/lle_architectural_compliance_check.sh

# 6. If violations found, fix them

# 7. When passes, commit
git add src/lle/foundation/display/my_feature.c
git commit -m "Add feature following Spec 08"
# Pre-commit hook runs automatically
```

---

### Workflow 2: Fixing Existing Violations

```bash
# 1. Run compliance check to see current state
./scripts/lle_architectural_compliance_check.sh

# 2. Review violation details
# Example: "Direct terminal writes in display code"

# 3. Read correct implementation
less docs/lle_implementation/CORRECT_DISPLAY_IMPLEMENTATION.md

# 4. Fix violations following guide

# 5. Re-run compliance check
./scripts/lle_architectural_compliance_check.sh

# 6. Repeat until passes

# 7. Commit fixes
git commit -am "Fix architectural violations in display code"
```

---

### Workflow 3: Code Review

```bash
# Before approving any LLE code:

# 1. Run compliance check
./scripts/lle_architectural_compliance_check.sh

# 2. Review code against specification
# - Does it follow Spec 08 patterns?
# - Are required structures present?
# - Is display integration used correctly?

# 3. Check prohibited patterns manually
grep -r "write.*fd" src/lle/foundation/display/
grep -r "\\x1b" src/lle/foundation/display/

# 4. Verify integration
grep -r "display_controller" src/lle/foundation/display/
# Should find actual usage, not just NULL assignments

# 5. Only approve if:
# - Compliance check passes
# - Follows spec patterns
# - No prohibited patterns
# - Integration verified
```

---

## Continuous Integration Setup

### For CI/CD Pipeline

Add to your CI configuration:

```yaml
# .github/workflows/lle-compliance.yml (example)
name: LLE Architectural Compliance

on:
  pull_request:
    paths:
      - 'src/lle/foundation/**'
  push:
    paths:
      - 'src/lle/foundation/**'

jobs:
  compliance-check:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Run Architectural Compliance Check
        run: |
          chmod +x scripts/lle_architectural_compliance_check.sh
          ./scripts/lle_architectural_compliance_check.sh
      
      - name: Fail if violations found
        if: failure()
        run: |
          echo "Architectural violations detected!"
          echo "See compliance check output above."
          exit 1
```

---

## Maintenance

### Updating Compliance Checks

If new architectural requirements are added:

1. Edit `scripts/lle_architectural_compliance_check.sh`
2. Add new checks following existing pattern
3. Test on current codebase
4. Update this documentation
5. Notify all developers

### Updating Correct Implementation Guide

If implementation patterns change:

1. Edit `docs/lle_implementation/CORRECT_DISPLAY_IMPLEMENTATION.md`
2. Update examples
3. Update compliance checklist
4. Notify all developers

---

## Troubleshooting

### "Compliance check fails but I followed the spec"

**Possible causes**:
1. Spec compliance check may have false positive
2. New pattern not yet in compliance script
3. Misunderstanding of spec requirement

**Solution**:
1. Review violation details carefully
2. Check specification section referenced
3. Compare with CORRECT_DISPLAY_IMPLEMENTATION.md
4. If truly false positive, update compliance script
5. Document decision in commit message

---

### "Pre-commit hook not running"

**Check**:
```bash
# Verify hook is installed
ls -la .git/hooks/pre-commit

# Should show symlink to script
# lrwxrwxrwx ... .git/hooks/pre-commit -> ../../scripts/pre-commit-lle-compliance

# If missing, reinstall:
ln -sf ../../scripts/pre-commit-lle-compliance .git/hooks/pre-commit
```

---

### "Hook reports violations but manual check passes"

**Possible cause**: Different working directory

**Solution**:
```bash
# Hook runs from repo root, so from anywhere:
cd /path/to/lusush
./scripts/lle_architectural_compliance_check.sh
```

---

### "Need to commit urgently, hook blocking"

**Short-term bypass** (use with caution):
```bash
git commit --no-verify -m "message"
```

**But then MUST**:
1. Create issue documenting violation
2. Fix violation in next commit
3. Re-run compliance check to verify
4. Do NOT bypass repeatedly

---

## Policy Enforcement

### Required Use

**Mandatory for**:
- All LLE development work
- All commits touching `src/lle/foundation/`
- All feature implementations
- All bug fixes in LLE code

**Optional for**:
- Documentation-only changes
- Test code (but encouraged)
- Build system changes

### Zero Tolerance

**Policy**: Architectural violations are **NOT ACCEPTABLE**

**Enforcement**:
- Pre-commit hook blocks violations
- CI fails on violations
- Code review rejects violations
- Features not marked complete until compliance passes

### Exceptions

**Very rare cases** where violation might be acceptable:
1. Temporary debugging code (must not be committed)
2. Test fixtures simulating violations (in test directory only)
3. Migration code with clear TODO and deadline

**Process for exceptions**:
1. Document reason in code comments
2. Get user approval
3. Create issue for resolution
4. Set deadline for fix
5. Do NOT mark as complete

---

## Success Metrics

**Compliance system is working when**:
- ✅ No architectural violations in new code
- ✅ Compliance check passes on all commits
- ✅ Developers read specs before implementing
- ✅ Code reviews check architectural compliance
- ✅ Violations caught before manual testing
- ✅ Zero "temporary" violations in codebase

---

## Quick Reference Card

### Essential Commands

```bash
# Check compliance
./scripts/lle_architectural_compliance_check.sh

# Install pre-commit hook
ln -sf ../../scripts/pre-commit-lle-compliance .git/hooks/pre-commit

# View correct implementation
less docs/lle_implementation/CORRECT_DISPLAY_IMPLEMENTATION.md

# View violation analysis  
less docs/lle_implementation/tracking/DESIGN_VIOLATION_ANALYSIS.md

# Bypass hook (emergency only)
git commit --no-verify
```

### Critical Files

- **Compliance Check**: `scripts/lle_architectural_compliance_check.sh`
- **Pre-commit Hook**: `.git/hooks/pre-commit`
- **Correct Implementation**: `docs/lle_implementation/CORRECT_DISPLAY_IMPLEMENTATION.md`
- **Violation Analysis**: `docs/lle_implementation/tracking/DESIGN_VIOLATION_ANALYSIS.md`
- **Spec 08**: `docs/lle_specification/08_display_integration_complete.md`

### Prohibited Patterns

❌ `write(fd, ...)` - Direct terminal write  
❌ `\x1b[` - Escape sequence  
❌ `display_controller = NULL` - Not integrated  
❌ `sprintf(..., "\x1b")` - Formatting escape codes

### Required Patterns

✅ `display_integration_update()` - Through display system  
✅ `command_layer_update_content()` - Proper API  
✅ `composition_engine_compose()` - Coordinated rendering  
✅ `display_controller != NULL` - Actually integrated

---

## Support

### Questions?

1. Read `CORRECT_DISPLAY_IMPLEMENTATION.md`
2. Read specification (Spec 08)
3. Review `DESIGN_VIOLATION_ANALYSIS.md`
4. Check existing passing code for examples
5. Ask user if still unclear

### Found a Bug in Compliance Check?

1. Document the false positive/negative
2. Propose fix to compliance script
3. Test fix on current codebase
4. Submit for review
5. Update documentation

---

**Version**: 1.0.0  
**Last Updated**: 2025-10-17  
**Maintainer**: Lusush Development Team
