# Enforced Documentation Policy

**Status**: ACTIVE - Enforced by pre-commit hooks  
**Date**: 2025-10-20  
**Enforcement**: Automatic - commits will FAIL if policy violated

---

## 🔒 POLICY OBJECTIVES

This policy is **ENFORCED** not suggested. The pre-commit hook system will **REJECT** any commit that violates these rules.

**Keywords**: SAFE | ORGANIZED | CLEAN | PRUNED

1. **SAFE** - Never lose useful information
2. **ORGANIZED** - Structured, navigable documentation
3. **CLEAN** - Root directory minimal, no clutter
4. **PRUNED** - Remove obsolete documents when safe

---

## 📋 ENFORCED RULES (Pre-commit Hook Checks)

### Rule 1: Root Directory Cleanliness ✅ ENFORCED

**Allowed in Root** (exhaustive list):
- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` (central nervous system)
- `README.md` (project introduction)
- `LICENSE` (project license)
- `meson.build`, `meson_options.txt` (build configuration)
- `.gitignore`, `.editorconfig`, `.clang-format`, `.cursorrules` (config files)
- Directories: `src/`, `include/`, `docs/`, `tests/`, `scripts/`, `examples/`, `build*/`, `audit_results/`

**PROHIBITED in Root**:
- ❌ `.o` files (build artifacts)
- ❌ `core.*` files (core dumps)
- ❌ `*.log` files (except active logs in proper directories)
- ❌ Markdown files other than AI_ASSISTANT_HANDOFF_DOCUMENT.md and README.md
- ❌ Test scripts (belong in `scripts/`)
- ❌ Temporary files

**Enforcement**: Pre-commit hook will **REJECT** commits that add prohibited files to root.

---

### Rule 2: Documentation Structure ✅ ENFORCED

**Required Structure**:
```
docs/
├── user/                          # End-user documentation
│   ├── GETTING_STARTED.md
│   ├── USER_GUIDE.md
│   └── ...
├── project/                       # Project-level documentation
│   ├── CHANGELOG.md
│   ├── DOCUMENTATION_INDEX.md
│   └── ...
├── development/                   # Lusush v1.x development docs
│   ├── BUILTIN_COMMANDS.md
│   ├── COMPLETION_SYSTEM.md
│   └── ...
├── lle_specification/             # LLE specifications (36 specs)
│   ├── 01_overview.md
│   ├── 02_terminal_abstraction_complete.md
│   └── ...
├── lle_implementation/            # LLE implementation living docs
│   ├── LLE_IMPLEMENTATION_GUIDE.md
│   ├── LLE_DEVELOPMENT_STRATEGY.md
│   ├── SPEC_IMPLEMENTATION_ORDER.md
│   ├── SPECIFICATION_IMPLEMENTATION_POLICY.md
│   └── KNOWN_ISSUES.md (if needed)
└── archived/                      # Historical/completed documents
    ├── nuclear_options/           # Nuclear option completion docs
    ├── phase_completions/         # Phase completion records
    ├── spec_audits/              # Spec compliance audits
    └── spec_extractions/         # Spec extraction audits
```

**Enforcement**: Pre-commit hook will **WARN** if new markdown files are added outside this structure.

---

### Rule 3: Living Document Maintenance ✅ ENFORCED

**Living Documents** (must stay current):
1. `AI_ASSISTANT_HANDOFF_DOCUMENT.md` (root)
2. `docs/lle_implementation/LLE_IMPLEMENTATION_GUIDE.md`
3. `docs/lle_implementation/LLE_DEVELOPMENT_STRATEGY.md`
4. `docs/lle_implementation/SPEC_IMPLEMENTATION_ORDER.md`
5. `docs/lle_implementation/KNOWN_ISSUES.md` (when issues exist)

**Current Enforcement** (already active):
- When LLE code changes, living documents MUST be updated
- Pre-commit hook verifies AI_ASSISTANT_HANDOFF_DOCUMENT.md is updated
- Commit will **FAIL** if living documents not updated

**Future Enhancement** (see Evolution section):
- Check for stale dates in living documents
- Verify cross-references between living documents
- Detect contradictions in status across documents

---

### Rule 4: Safe Archival ✅ ENFORCED (Manual Review Required)

**When to Archive**:
- ✅ Completion documents after phase is done and verified
- ✅ Audit documents after compliance is achieved
- ✅ Extraction documents after spec is implemented
- ✅ Nuclear option documents after recovery is complete
- ✅ Test result documents after issues are resolved

**NEVER Archive**:
- ❌ Active specifications
- ❌ Living documents
- ❌ Current development guides
- ❌ Unsolved issue reports

**Archive Process**:
1. Use `git mv` (preserves history)
2. Move to appropriate `docs/archived/` subdirectory
3. Update any references in active documents
4. Document the archival in commit message

**Enforcement**: Pre-commit hook will **DETECT** file moves and **REQUIRE** commit message to document archival reason.

---

### Rule 5: Pruning Policy ✅ SAFE-ONLY

**Pruning = Safe Deletion** (not archival)

**Can Be Pruned**:
- ✅ Build artifacts (.o files, core dumps)
- ✅ Duplicate documents (keep most recent, archive older)
- ✅ Temporary scratch files
- ✅ Generated documentation that can be regenerated
- ✅ Empty or near-empty placeholder files

**NEVER Prune**:
- ❌ Unique information (even if seems obsolete)
- ❌ Historical context that explains decisions
- ❌ Error reports that might recur
- ❌ Performance benchmarks (historical data valuable)

**Pruning Process**:
1. Verify content is truly duplicate/regenerable/obsolete
2. If ANY doubt, archive instead of prune
3. Document pruning in commit message with justification
4. Review with user if pruning >5 files or >1MB

**Enforcement**: Pre-commit hook will **WARN** on deletions and **REQUIRE** commit message justification.

---

## 🔧 ENFORCEMENT MECHANISMS

### Current Enforcement (Active)

**Pre-commit Hook**: `.git/hooks/pre-commit`

**Current Checks**:
1. ✅ LLE code changes require living document updates
2. ✅ LLE compilation must succeed
3. ✅ Living document existence verification
4. ⚠️  Date freshness warning (non-blocking)

**File**: `.git/hooks/pre-commit` (already in use)

---

### Enhanced Enforcement (To Be Implemented)

**Phase 1: Root Directory Enforcement**
```bash
# Check for prohibited files in root
if [[ -n $(git diff --cached --name-only | grep "^[^/]*\.o$") ]]; then
  echo "❌ POLICY VIOLATION: .o files not allowed in root"
  echo "   Build artifacts must not be committed"
  exit 1
fi

if [[ -n $(git diff --cached --name-only | grep "^core\.") ]]; then
  echo "❌ POLICY VIOLATION: core dumps not allowed in root"
  exit 1
fi

# Count markdown files in root (excluding README.md and AI_ASSISTANT_HANDOFF_DOCUMENT.md)
root_md_count=$(git diff --cached --name-only | grep "^[^/]*\.md$" | grep -v "^README\.md$" | grep -v "^AI_ASSISTANT_HANDOFF_DOCUMENT\.md$" | wc -l)
if [[ $root_md_count -gt 0 ]]; then
  echo "❌ POLICY VIOLATION: Markdown files must be in docs/ subdirectories"
  echo "   Only AI_ASSISTANT_HANDOFF_DOCUMENT.md and README.md allowed in root"
  git diff --cached --name-only | grep "^[^/]*\.md$" | grep -v "^README\.md$" | grep -v "^AI_ASSISTANT_HANDOFF_DOCUMENT\.md$"
  exit 1
fi
```

**Phase 2: Documentation Structure Enforcement**
```bash
# Warn about markdown files added outside documented structure
new_docs=$(git diff --cached --name-only | grep "\.md$")
for doc in $new_docs; do
  if [[ ! $doc =~ ^docs/(user|project|development|lle_specification|lle_implementation|archived)/ ]] && \
     [[ ! $doc =~ ^(README\.md|AI_ASSISTANT_HANDOFF_DOCUMENT\.md)$ ]]; then
    echo "⚠ WARNING: New markdown file outside documented structure: $doc"
    echo "   Consider placing in docs/project/ or appropriate subdirectory"
  fi
done
```

**Phase 3: Deletion Justification**
```bash
# Require commit message justification for deletions
deleted_files=$(git diff --cached --name-only --diff-filter=D)
if [[ -n $deleted_files ]]; then
  if ! git log -1 --pretty=%B | grep -q "PRUNED\|DELETED\|REMOVED"; then
    echo "❌ POLICY VIOLATION: File deletions require justification in commit message"
    echo "   Deleted files:"
    echo "$deleted_files"
    echo ""
    echo "   Add to commit message: 'PRUNED: <reason>' or 'DELETED: <reason>'"
    exit 1
  fi
fi
```

---

## 📈 POLICY EVOLUTION

This policy is **evolvable** but changes require:

1. **User Approval** - No AI assistant can modify policy unilaterally
2. **Documentation** - Policy changes must be documented in this file
3. **Enforcement Update** - Pre-commit hooks must be updated to match
4. **Retroactive Review** - Existing structure reviewed against new policy

**Proposed Enhancements** (for user consideration):

### Enhancement 1: Automated Archival Suggestions
- Script that detects "completion" documents >30 days old
- Suggests archival candidates for review
- Never auto-archives without approval

### Enhancement 2: Documentation Health Report
- Weekly/monthly report on documentation status
- Lists stale documents (not updated in >60 days)
- Identifies orphaned documents (no references)
- Suggests pruning candidates

### Enhancement 3: Living Document Cross-Reference Verification
- Verify dates across living documents match
- Detect status contradictions (one doc says "complete", another says "in progress")
- Enforce cross-document consistency

### Enhancement 4: AI Session Handoff Quality Check
- Verify AI_ASSISTANT_HANDOFF_DOCUMENT.md contains all required sections
- Check that current task is clearly defined
- Ensure next steps are actionable

---

## 🎯 QUICK REFERENCE

**Before Committing**:
1. ✅ Is root directory clean? (only allowed files)
2. ✅ Are new docs in correct docs/ subdirectory?
3. ✅ If LLE code changed, are living documents updated?
4. ✅ If files deleted, is justification in commit message?
5. ✅ If files moved, used `git mv` for traceability?

**Keywords to Remember**: **SAFE | ORGANIZED | CLEAN | PRUNED**

---

## 📝 REVISION HISTORY

| Date | Change | Reason |
|------|--------|--------|
| 2025-10-20 | Initial policy created | Establish enforced documentation standards |
| | Root directory rules defined | Prevent clutter, maintain cleanliness |
| | Documentation structure formalized | Organize existing 200+ markdown files |
| | Safe archival process defined | Preserve history while reducing noise |
| | Pruning policy established | Safe deletion guidelines |
| | Enhanced enforcement planned | Move from "mandatory" to "enforced" |

---

**Policy Owner**: Project Maintainer  
**Enforcement**: Automated (pre-commit hooks)  
**Review Frequency**: On-demand as structure needs evolve  
**Compliance**: MANDATORY - Non-negotiable for all commits
