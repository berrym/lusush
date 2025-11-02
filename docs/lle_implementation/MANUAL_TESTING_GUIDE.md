# LLE Phase 4 Manual Testing Guide

**Document Version:** 1.0.0  
**Date:** 2025-11-02  
**Phase:** Spec 09 Phase 4 (Forensics, Deduplication, Multiline)  
**Purpose:** Comprehensive manual testing procedures for Phase 4 features

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Test Environment Setup](#test-environment-setup)
3. [Forensic Tracking Tests](#forensic-tracking-tests)
4. [Deduplication Tests](#deduplication-tests)
5. [Multiline Command Tests](#multiline-command-tests)
6. [Integration Tests](#integration-tests)
7. [Performance Tests](#performance-tests)
8. [Debug Procedures](#debug-procedures)
9. [Expected Outputs Reference](#expected-outputs-reference)

---

## Prerequisites

### Required Build
```bash
cd /home/mberry/Lab/c/lusush
meson setup builddir
meson compile -C builddir
```

### Verify Phase 4 Modules
```bash
ls -lh builddir/src/lle/history_*.o | grep -E "(forensics|dedup|multiline)"
```

**Expected Output:**
```
-rw-r--r-- 1 user user  23K Nov  2 XX:XX history_forensics.c.o
-rw-r--r-- 1 user user  38K Nov  2 XX:XX history_dedup.c.o
-rw-r--r-- 1 user user 6.6K Nov  2 XX:XX history_multiline.c.o
```

### Create Test Working Directory
```bash
mkdir -p /tmp/lle_manual_tests
cd /tmp/lle_manual_tests
export LLE_TEST_DIR=$(pwd)
```

---

## Test Environment Setup

### 1. Create Test History File
```bash
cat > test_history_config.sh << 'EOF'
#!/bin/bash
export HISTFILE="${LLE_TEST_DIR}/test_history.txt"
export HISTSIZE=1000
export HISTFILESIZE=2000
export LLE_ENABLE_FORENSICS=1
export LLE_ENABLE_DEDUP=1
export LLE_ENABLE_MULTILINE=1
echo "Test environment configured:"
echo "  HISTFILE: $HISTFILE"
echo "  Forensics: Enabled"
echo "  Deduplication: Enabled"
echo "  Multiline: Enabled"
EOF
chmod +x test_history_config.sh
source test_history_config.sh
```

### 2. Create Test Helper Scripts

**inspect_history.sh** - View history with metadata:
```bash
cat > inspect_history.sh << 'EOF'
#!/bin/bash
if [ ! -f "${HISTFILE}" ]; then
    echo "Error: History file not found: ${HISTFILE}"
    exit 1
fi

echo "=== History File Contents ==="
echo "File: ${HISTFILE}"
echo "Size: $(wc -l < ${HISTFILE}) entries"
echo ""
cat -n "${HISTFILE}"
EOF
chmod +x inspect_history.sh
```

**check_forensics.sh** - Verify forensic metadata:
```bash
cat > check_forensics.sh << 'EOF'
#!/bin/bash
echo "=== Current Process Forensics ==="
echo "Process ID: $$"
echo "Session ID: $(ps -p $$ -o sid=)"
echo "User ID: $(id -u)"
echo "Group ID: $(id -g)"
echo "Terminal: $(tty)"
echo "Timestamp: $(date +%s%N)"
EOF
chmod +x check_forensics.sh
```

---

## Forensic Tracking Tests

### Test F1: Basic Forensic Context Capture

**Objective:** Verify that forensic metadata is captured for commands

**Procedure:**
1. Clear history and start fresh:
   ```bash
   > "${HISTFILE}"
   history -c
   ```

2. Execute test commands:
   ```bash
   echo "Test command 1"
   date
   whoami
   ```

3. Check forensic capture:
   ```bash
   ./check_forensics.sh
   ```

4. Inspect history:
   ```bash
   ./inspect_history.sh
   ```

**Expected Results:**
- Each command should have captured:
  - Process ID (PID)
  - Session ID (SID)
  - User ID (UID)
  - Group ID (GID)
  - Terminal name (e.g., /dev/pts/0)
  - Timestamp (nanosecond precision)

**Verification:**
- PID should match current shell: `echo $$`
- UID should match: `id -u`
- GID should match: `id -g`
- Terminal should match: `tty`

**Debug If Failed:**
```bash
# Check if forensics module loaded
nm builddir/src/lle/history_forensics.c.o | grep lle_forensic_capture_context

# Check environment variable
echo "LLE_ENABLE_FORENSICS: ${LLE_ENABLE_FORENSICS}"
```

---

### Test F2: Forensic Timing Accuracy

**Objective:** Verify high-precision timing capture

**Procedure:**
1. Execute long-running command:
   ```bash
   time sleep 2
   ```

2. Execute quick command:
   ```bash
   echo "Quick"
   ```

3. Check timing data in history

**Expected Results:**
- `sleep 2` should show ~2000000000 nanoseconds duration
- `echo` should show < 100000000 nanoseconds duration
- Start time < End time for both commands

**Verification:**
```bash
# Check nanosecond precision available
date +%s%N | tail -c 10
# Should show 9 digits (nanoseconds)
```

---

### Test F3: Usage Count Tracking

**Objective:** Verify usage count increments on repeated access

**Procedure:**
1. Execute same command multiple times:
   ```bash
   echo "Test"
   echo "Test"
   echo "Test"
   ```

2. Use history search:
   ```bash
   history | grep "Test"
   ```

3. Re-execute from history:
   ```bash
   !-1  # Execute last command
   !-2  # Execute second-to-last
   ```

**Expected Results:**
- Usage count should increment with each access
- Last access time should update each time

**Debug If Failed:**
```bash
# Enable verbose forensics logging (if implemented)
export LLE_FORENSIC_DEBUG=1
```

---

### Test F4: Multi-User Forensics

**Objective:** Verify forensics work across user contexts (if applicable)

**Procedure:**
1. Run command as current user:
   ```bash
   echo "User: $(whoami)"
   ./check_forensics.sh
   ```

2. If possible, run as different user (requires sudo):
   ```bash
   sudo -u nobody bash -c "cd ${LLE_TEST_DIR} && source test_history_config.sh && echo 'Different user'"
   ```

**Expected Results:**
- Different UID/GID captured for different users
- Each user context properly isolated

**Skip If:** No sudo access or single-user system

---

## Deduplication Tests

### Test D1: Duplicate Detection (KEEP_RECENT Strategy)

**Objective:** Verify recent duplicates replace old duplicates

**Procedure:**
1. Configure deduplication:
   ```bash
   export LLE_DEDUP_STRATEGY="KEEP_RECENT"
   ```

2. Add duplicate commands:
   ```bash
   echo "Duplicate test"
   ls -la
   pwd
   echo "Duplicate test"  # Duplicate
   whoami
   echo "Duplicate test"  # Duplicate again
   ```

3. Inspect history:
   ```bash
   ./inspect_history.sh
   history | grep "Duplicate test"
   ```

**Expected Results:**
- Only ONE "Duplicate test" entry in history
- It should be the MOST RECENT occurrence
- Other unique commands preserved

**Verification:**
```bash
# Count occurrences
grep -c "Duplicate test" "${HISTFILE}"
# Should output: 1
```

---

### Test D2: Metadata Merging

**Objective:** Verify forensic metadata merges correctly on deduplication

**Procedure:**
1. Execute command with delay:
   ```bash
   echo "Merge test"
   sleep 1
   echo "Merge test"  # Duplicate
   sleep 1
   echo "Merge test"  # Duplicate again
   ```

2. Check merged metadata

**Expected Results:**
- Usage count: 3 (accumulated)
- Start time: From FIRST execution
- Last access: From LAST execution
- Total duration: Sum of all executions

---

### Test D3: Deduplication Strategies Comparison

**Objective:** Test all deduplication strategies

**Test Matrix:**

| Strategy | Command Sequence | Expected Result |
|----------|-----------------|-----------------|
| IGNORE | `cmd`, `cmd`, `cmd` | 3 entries (no dedup) |
| KEEP_RECENT | `cmd`, `cmd`, `cmd` | 1 entry (most recent) |
| KEEP_FREQUENT | `cmd1`, `cmd1`, `cmd2` | `cmd1` prioritized |
| MERGE_METADATA | `cmd`, `cmd`, `cmd` | 1 entry, usage_count=3 |

**Procedure for each strategy:**
```bash
# Test IGNORE
export LLE_DEDUP_STRATEGY="IGNORE"
> "${HISTFILE}"
echo "test"; echo "test"; echo "test"
echo "IGNORE result: $(grep -c 'test' ${HISTFILE})"

# Test KEEP_RECENT  
export LLE_DEDUP_STRATEGY="KEEP_RECENT"
> "${HISTFILE}"
echo "test"; echo "test"; echo "test"
echo "KEEP_RECENT result: $(grep -c 'test' ${HISTFILE})"

# Test KEEP_FREQUENT
export LLE_DEDUP_STRATEGY="KEEP_FREQUENT"
> "${HISTFILE}"
for i in {1..5}; do echo "frequent"; done
echo "other"
./inspect_history.sh

# Test MERGE_METADATA
export LLE_DEDUP_STRATEGY="MERGE_METADATA"
> "${HISTFILE}"
echo "merge"; sleep 1; echo "merge"; sleep 1; echo "merge"
# Check usage_count metadata
```

**Expected Results:** Match table above

---

### Test D4: Deduplication with Similar Commands

**Objective:** Verify exact match requirements

**Procedure:**
```bash
export LLE_DEDUP_STRATEGY="KEEP_RECENT"
> "${HISTFILE}"

echo "test"
echo "test "   # Trailing space
echo "test  "  # Two trailing spaces
echo " test"   # Leading space
echo "Test"    # Different case

./inspect_history.sh
```

**Expected Results:**
- All 5 commands preserved (NOT duplicates)
- Exact string matching only
- Whitespace matters
- Case-sensitive

**Verification:**
```bash
wc -l "${HISTFILE}"
# Should output: 5
```

---

## Multiline Command Tests

### Test M1: Basic Multiline Detection

**Objective:** Verify multiline command detection

**Procedure:**
1. Execute multiline commands:
   ```bash
   for i in 1 2 3; do
       echo "Line $i"
   done
   ```

   ```bash
   if [ -f /etc/passwd ]; then
       echo "File exists"
   fi
   ```

   ```bash
   cat << 'EOF'
   Heredoc line 1
   Heredoc line 2
   EOF
   ```

2. Inspect history:
   ```bash
   ./inspect_history.sh
   ```

**Expected Results:**
- Each multiline command stored as complete unit
- Original formatting preserved
- Continuation characters visible

---

### Test M2: Multiline Reconstruction

**Objective:** Verify multiline can be reconstructed in different formats

**Test Formats:**

1. **ORIGINAL** - Preserve exact formatting:
   ```bash
   for i in 1 2 3; do
       echo "Number: $i"
   done
   ```

2. **FLATTENED** - Single line:
   ```bash
   for i in 1 2 3; do echo "Number: $i"; done
   ```

3. **COMPACT** - Minimal whitespace:
   ```bash
   for i in 1 2 3;do echo "Number: $i";done
   ```

**Procedure:**
```bash
# Store multiline command
cat > test_multiline.sh << 'EOF'
#!/bin/bash
for file in *.txt; do
    if [ -f "$file" ]; then
        echo "Processing: $file"
    fi
done
EOF

# Source it to add to history
source test_multiline.sh

# Retrieve and test reconstruction
# (Requires LLE API access or shell function)
```

**Expected Results:**
- Each format should be syntactically valid
- ORIGINAL maintains readability
- FLATTENED works for search
- COMPACT minimizes storage

---

### Test M3: Complex Multiline Structures

**Objective:** Test nested and complex structures

**Procedure:**
```bash
# Test 1: Nested loops
for i in {1..3}; do
    for j in {a..c}; do
        echo "${i}${j}"
    done
done

# Test 2: Pipeline multiline
cat /etc/passwd | \
    grep "^root" | \
    cut -d: -f1,3,6 | \
    sort

# Test 3: Subshell multiline
(
    cd /tmp
    ls -la
    pwd
)

# Test 4: Function definition
function test_func() {
    local var="test"
    echo "Function: $var"
    return 0
}

./inspect_history.sh
```

**Expected Results:**
- All structures detected as multiline
- Proper nesting preserved
- Continuation symbols handled
- Syntactic integrity maintained

---

### Test M4: Multiline Edge Cases

**Objective:** Test boundary conditions

**Procedure:**
```bash
# Test 1: Empty lines in multiline
cat << 'EOF'
Line 1

Line 3 (empty line above)
EOF

# Test 2: Comments in multiline
for i in 1 2 3; do
    # This is a comment
    echo "$i"  # Inline comment
done

# Test 3: Very long multiline (stress test)
for i in {1..100}; do
    echo "Line $i with some extra text to make it longer"
done

# Test 4: Mixed quotes
echo "Double quote" && \
echo 'Single quote' && \
echo `Backtick command`

./inspect_history.sh
```

**Expected Results:**
- Empty lines preserved or handled gracefully
- Comments included in original format
- Long multiline within limits (check MAX_MULTILINE_LENGTH)
- Quote types properly distinguished

---

### Test M5: Multiline with Heredocs

**Objective:** Verify heredoc handling

**Procedure:**
```bash
# Test 1: Basic heredoc
cat << 'EOF'
This is a heredoc
Multiple lines
EOF

# Test 2: Heredoc with variable expansion
VAR="expanded"
cat << EOF
This is $VAR
Current dir: $(pwd)
EOF

# Test 3: Heredoc with indentation (<<-)
cat <<- 'INDENT'
    Indented line 1
    Indented line 2
INDENT

# Test 4: Appending heredoc
cat >> /tmp/test_heredoc.txt << 'APPEND'
Appended line 1
Appended line 2
APPEND

./inspect_history.sh
```

**Expected Results:**
- Heredoc delimiters preserved
- Content between delimiters stored
- Quote handling (quoted vs unquoted delimiter)
- Indentation handling (<<- vs <<)

---

## Integration Tests

### Test I1: All Features Together

**Objective:** Verify forensics + dedup + multiline work together

**Procedure:**
```bash
# Configure all features
export LLE_ENABLE_FORENSICS=1
export LLE_ENABLE_DEDUP=1
export LLE_DEDUP_STRATEGY="MERGE_METADATA"
export LLE_ENABLE_MULTILINE=1

# Clear history
> "${HISTFILE}"
history -c

# Test scenario: Repeated multiline with forensics
for i in 1 2 3; do
    for i in {1..3}; do
        echo "Integration test"
    done
    sleep 1
done

# Check results
./inspect_history.sh
./check_forensics.sh
```

**Expected Results:**
- Multiline command detected and preserved
- Forensic metadata captured (PID, timing, etc.)
- Deduplication applied (usage count increments)
- All metadata correctly associated

**Verification Checklist:**
- [ ] Multiline structure intact
- [ ] Forensic PID matches current shell
- [ ] Usage count > 1 (due to loop)
- [ ] Timestamps in nanoseconds
- [ ] No duplicate entries

---

### Test I2: Performance Under Load

**Objective:** Test performance with many commands

**Procedure:**
```bash
# Generate load
time for i in {1..1000}; do
    echo "Command $i"
done

# Check history size
wc -l "${HISTFILE}"

# Check performance
./inspect_history.sh | tail -20
```

**Expected Results:**
- All 1000 commands processed
- No crashes or memory errors
- Reasonable performance (< 10 seconds for 1000 commands)
- History file integrity maintained

**Performance Targets:**
- Command throughput: > 100 commands/second
- Memory usage: Stable (no leaks)
- History file size: Proportional to command count

---

### Test I3: Persistence and Recovery

**Objective:** Verify history persists across sessions

**Procedure:**
```bash
# Session 1: Add commands with all features
export LLE_ENABLE_FORENSICS=1
export LLE_ENABLE_DEDUP=1
export LLE_ENABLE_MULTILINE=1

echo "Session 1 command"
for i in 1 2; do
    echo "Multiline $i"
done

# Note current state
cp "${HISTFILE}" "${HISTFILE}.session1"
./inspect_history.sh > session1_output.txt

# Exit and restart
exit

# Session 2: Verify persistence
source test_history_config.sh
./inspect_history.sh > session2_output.txt

# Compare
diff session1_output.txt session2_output.txt
```

**Expected Results:**
- History from session 1 available in session 2
- Forensic metadata preserved
- Multiline commands intact
- No data loss or corruption

---

## Performance Tests

### Test P1: Memory Usage

**Objective:** Measure memory footprint

**Procedure:**
```bash
# Baseline memory
ps -o pid,vsz,rss,cmd -p $$ > memory_baseline.txt

# Add many commands
for i in {1..10000}; do
    echo "Memory test command $i"
done

# Check memory after
ps -o pid,vsz,rss,cmd -p $$ > memory_after.txt

# Compare
echo "=== Memory Usage Comparison ==="
paste memory_baseline.txt memory_after.txt
```

**Expected Results:**
- VSZ (Virtual Size) increase < 50MB for 10K commands
- RSS (Resident Set) increase < 20MB for 10K commands
- No memory leaks (stable after initial allocation)

**Debug If Failed:**
```bash
# Check for memory leaks with valgrind (if available)
valgrind --leak-check=full --log-file=valgrind_output.txt ./builddir/tests/lle/test_history_phase4_complete
```

---

### Test P2: Deduplication Performance

**Objective:** Measure dedup overhead

**Procedure:**
```bash
# Test 1: Without dedup
export LLE_ENABLE_DEDUP=0
> "${HISTFILE}"
time for i in {1..1000}; do echo "test"; done
NO_DEDUP_TIME=$?

# Test 2: With dedup
export LLE_ENABLE_DEDUP=1
export LLE_DEDUP_STRATEGY="KEEP_RECENT"
> "${HISTFILE}"
time for i in {1..1000}; do echo "test"; done
DEDUP_TIME=$?

echo "Without dedup: ${NO_DEDUP_TIME}s"
echo "With dedup: ${DEDUP_TIME}s"
echo "Overhead: $((DEDUP_TIME - NO_DEDUP_TIME))s"
```

**Expected Results:**
- Dedup overhead < 20% for repeated commands
- Hash computation efficient
- No performance degradation over time

---

### Test P3: Multiline Parsing Performance

**Objective:** Measure multiline detection overhead

**Procedure:**
```bash
# Generate test file with mix of single/multiline
cat > perf_multiline_test.sh << 'EOF'
#!/bin/bash
for i in {1..500}; do
    echo "Single line $i"
    for j in {1..3}; do
        echo "Nested $i.$j"
    done
done
EOF

time bash perf_multiline_test.sh
```

**Expected Results:**
- Multiline detection overhead < 10%
- No exponential slowdown with nesting
- Efficient parsing of complex structures

---

## Debug Procedures

### Debug Procedure 1: History Not Saving

**Symptoms:**
- Commands not appearing in history file
- Empty or missing HISTFILE

**Debug Steps:**
1. Check environment:
   ```bash
   echo "HISTFILE: ${HISTFILE}"
   echo "HISTSIZE: ${HISTSIZE}"
   echo "HISTFILESIZE: ${HISTFILESIZE}"
   ```

2. Check file permissions:
   ```bash
   ls -la "${HISTFILE}"
   touch "${HISTFILE}" && echo "Write OK" || echo "Write FAILED"
   ```

3. Check LLE initialization:
   ```bash
   ldd builddir/src/lle/history_core.c.o
   nm builddir/src/lle/history_core.c.o | grep lle_history_core_create
   ```

4. Enable debug logging (if available):
   ```bash
   export LLE_DEBUG=1
   export LLE_VERBOSE=1
   ```

---

### Debug Procedure 2: Forensics Not Capturing

**Symptoms:**
- Missing forensic metadata
- Null or zero values in forensic fields

**Debug Steps:**
1. Verify module loaded:
   ```bash
   nm builddir/src/lle/history_forensics.c.o | grep -E "(capture_context|apply_to_entry)"
   ```

2. Check system capabilities:
   ```bash
   # Test process info access
   cat /proc/$$/status
   cat /proc/$$/stat
   ```

3. Test forensic capture directly:
   ```bash
   # If test binary available
   ./builddir/tests/lle/test_history_phase4_complete --filter=forensic
   ```

4. Check for permission issues:
   ```bash
   # Some forensic info requires privileges
   id -u  # UID
   id -g  # GID
   ps -o sid= -p $$  # Session ID
   ```

---

### Debug Procedure 3: Deduplication Not Working

**Symptoms:**
- Duplicate commands appearing in history
- Dedup strategy ignored

**Debug Steps:**
1. Verify configuration:
   ```bash
   echo "LLE_ENABLE_DEDUP: ${LLE_ENABLE_DEDUP}"
   echo "LLE_DEDUP_STRATEGY: ${LLE_DEDUP_STRATEGY}"
   ```

2. Check dedup engine initialization:
   ```bash
   nm builddir/src/lle/history_dedup.c.o | grep lle_history_dedup_create
   ```

3. Test exact string matching:
   ```bash
   # Must be EXACT match
   echo "test"
   echo "test"   # Different (trailing space)
   ```

4. Verify hash function:
   ```bash
   # Check dedup statistics (if exposed)
   # Expected: hash collisions = 0
   ```

---

### Debug Procedure 4: Multiline Corruption

**Symptoms:**
- Multiline commands broken or truncated
- Syntax errors when reconstructing

**Debug Steps:**
1. Check buffer sizes:
   ```bash
   # In history.h, verify:
   # LLE_MAX_COMMAND_LENGTH >= 8192
   # LLE_MAX_MULTILINE_LENGTH >= 32768
   ```

2. Test incremental building:
   ```bash
   # Single line
   echo "test"
   
   # Two lines
   echo "test" && \
   echo "test2"
   
   # Three lines
   for i in 1 2 3; do
       echo "$i"
   done
   ```

3. Check continuation detection:
   ```bash
   # Should detect continuation
   echo "line1" \
   "line2"
   
   # Should NOT detect (inside quotes)
   echo "line1 \
   line2"
   ```

4. Verify storage integrity:
   ```bash
   # Check both original and flattened stored
   ./inspect_history.sh | grep -A 5 "multiline"
   ```

---

## Expected Outputs Reference

### Sample History File Format

```
# Basic command
echo "Hello World"

# Multiline command (original format)
for i in {1..3}; do
    echo "Number: $i"
done

# With forensic metadata (if enabled)
# PID:12345 SID:12340 UID:1000 GID:1000 TTY:/dev/pts/0 START:1698800000000000000 END:1698800000123456789 USAGE:1
echo "With forensics"
```

### Sample check_forensics.sh Output

```
=== Current Process Forensics ===
Process ID: 12345
Session ID: 12340
User ID: 1000
Group ID: 1000
Terminal: /dev/pts/0
Timestamp: 1698800000000000000
```

### Sample Automated Test Output

```
Running LLE Phase 4 Comprehensive Tests...

[Forensic Tracking Tests]
  ✓ test_forensic_context_capture
  ✓ test_forensic_apply_to_entry
  ✓ test_forensic_timing
  ✓ test_forensic_usage_tracking

[Deduplication Tests]
  ✓ test_dedup_keep_recent_strategy
  ✓ test_dedup_merge_metadata
  ✓ test_dedup_ignore_strategy
  ✓ test_dedup_with_forensics

[Multiline Tests]
  ✓ test_multiline_detection
  ✓ test_multiline_preservation
  ✓ test_multiline_reconstruction_original
  ✓ test_multiline_reconstruction_flattened
  ✓ test_multiline_complex_structure

[Integration Tests]
  ✓ test_all_phase4_features_together
  ✓ test_dedup_multiline_interaction
  ✓ test_forensics_multiline_interaction

==================================================
Results: 16 passed, 0 failed, 0 skipped
Status: ALL TESTS PASSED
==================================================
```

---

## Quick Test Checklist

Use this checklist for rapid validation:

```
[ ] Build completes without errors
[ ] Phase 4 modules present (forensics, dedup, multiline)
[ ] Basic command adds to history
[ ] Forensic PID captured and matches shell
[ ] Duplicate commands deduplicated
[ ] Multiline command preserved intact
[ ] Automated tests pass (16/16)
[ ] No memory leaks detected
[ ] Performance acceptable (>100 cmd/s)
[ ] History persists across sessions
```

---

## Test Summary Report Template

After completing manual tests, fill out this summary:

```
=== LLE Phase 4 Manual Testing Report ===
Date: _____________
Tester: _____________
Build: _____________

Forensic Tests:
  F1: [ PASS / FAIL ] - Basic context capture
  F2: [ PASS / FAIL ] - Timing accuracy
  F3: [ PASS / FAIL ] - Usage tracking
  F4: [ PASS / FAIL ] - Multi-user

Deduplication Tests:
  D1: [ PASS / FAIL ] - KEEP_RECENT strategy
  D2: [ PASS / FAIL ] - Metadata merging
  D3: [ PASS / FAIL ] - All strategies
  D4: [ PASS / FAIL ] - Similar commands

Multiline Tests:
  M1: [ PASS / FAIL ] - Basic detection
  M2: [ PASS / FAIL ] - Reconstruction
  M3: [ PASS / FAIL ] - Complex structures
  M4: [ PASS / FAIL ] - Edge cases
  M5: [ PASS / FAIL ] - Heredocs

Integration Tests:
  I1: [ PASS / FAIL ] - All features together
  I2: [ PASS / FAIL ] - Performance under load
  I3: [ PASS / FAIL ] - Persistence

Performance Tests:
  P1: [ PASS / FAIL ] - Memory usage
  P2: [ PASS / FAIL ] - Dedup overhead
  P3: [ PASS / FAIL ] - Multiline parsing

Issues Found:
_________________________________________________
_________________________________________________
_________________________________________________

Overall Status: [ PASS / FAIL ]
Ready for Production: [ YES / NO ]
```

---

## End of Manual Testing Guide

**Next Steps After Testing:**
1. Run automated test suite: `./builddir/tests/lle/test_history_phase4_complete`
2. Execute manual tests from this guide
3. Document any issues found
4. File bug reports if necessary
5. Proceed to production deployment once all tests pass

**Support:**
- Review automated tests: `tests/lle/functional/test_history_phase4_complete.c`
- Check implementation docs: `docs/lle_implementation/`
- Consult living documents: AI_ASSISTANT_HANDOFF_DOCUMENT.md

---

**End of Document**
