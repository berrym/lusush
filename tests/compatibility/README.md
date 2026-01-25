# Lush Compatibility Validation Suite

This directory contains the comprehensive compatibility validation framework
for testing lush against bash and zsh.

## Design Philosophy

Per `docs/VISION.md`, lush is a **superset shell** that:
- Accepts both bash AND zsh syntax via syntax bridging
- Uses profiles (posix, bash, zsh, lush) as **presets, not restrictions**
- Has a unified feature matrix that can be queried and overridden

This test suite validates that:
1. Lush correctly parses syntax from both shells
2. Features behave correctly in each profile mode
3. Gaps in implementation are identified and tracked
4. Semantic differences are documented

## Directory Structure

```
compatibility/
  validate.sh          # Main validation runner
  lib/
    test_framework.sh  # Testing utilities
    report.sh          # Report generation
  corpus/
    syntax/            # Pure syntax tests (parsing)
      bash/            # Bash-specific syntax
      zsh/             # Zsh-specific syntax  
      common/          # Syntax common to both
    features/          # Feature-specific tests
      arrays/          # Array behavior tests
      expansion/       # Parameter expansion tests
      globbing/        # Glob pattern tests
      ...
    semantic/          # Semantic behavior tests
      word_splitting/  # Word split differences
      array_indexing/  # 0 vs 1 indexed
      ...
  reports/             # Generated test reports
```

## Test Categories

### 1. Syntax Tests
Tests whether input parses successfully (using `-n` flag).
Each test specifies expected behavior per shell.

### 2. Feature Tests  
Tests specific features from the feature matrix.
Run with different profile modes to validate behavior.

### 3. Semantic Tests
Tests runtime behavior differences (output, exit codes).
Documents where lush intentionally differs.

## Test File Format

Each `.test` file uses a simple format:

```
# Test name and description
@test "array element access"
@feature FEATURE_INDEXED_ARRAYS
@profiles bash zsh lush

# Input to test
@input
arr=(a b c)
echo ${arr[0]}
@end

# Expected results per shell/profile
@expect bash:output "a"
@expect zsh:output ""     # zsh is 1-indexed
@expect lush:output "a"   # lush uses 0-indexed by default
@expect lush:zsh:output "" # lush in zsh profile

# Or for syntax-only tests
@expect bash:parse ok
@expect zsh:parse ok
@expect lush:parse ok
```

## Running Tests

```bash
# Run all tests
./validate.sh

# Run specific category
./validate.sh --category syntax

# Run with specific profile
./validate.sh --profile bash

# Run specific feature tests
./validate.sh --feature arrays

# Generate gap report
./validate.sh --report gaps
```

## Gap Tracking

The framework maintains a gap database tracking:
- Features that parse but don't execute correctly
- Semantic differences from reference shells
- Unimplemented syntax that should work

See `reports/gaps.md` for current status.
