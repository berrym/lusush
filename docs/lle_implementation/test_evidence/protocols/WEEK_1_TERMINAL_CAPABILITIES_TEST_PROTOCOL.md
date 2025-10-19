# Manual Test Protocol: Week 1 Terminal Capability Detection

**Feature**: Terminal Capability Detection  
**Week**: 1  
**Spec Reference**: 02_terminal_abstraction_complete.md, audit_26_into_02.md  
**Date**: 2025-10-19  
**Tester**: [To be filled during testing]

---

## Test Information

- **Feature**: Fast terminal capability detection (<50ms)
- **Version**: Week 1 Fresh Start
- **Implementation**: src/lle/terminal/capabilities.c
- **Test Binary**: tests/lle/behavioral/test_terminal_capabilities_behavioral

---

## Test Environment

**Required Testing Environments** (minimum 5 terminal types):

1. ✓ **Konsole** (KDE terminal)
2. ✓ **GNOME Terminal** (GNOME terminal)
3. ✓ **xterm** (Classic X terminal)
4. ✓ **Alacritty** (GPU-accelerated)
5. ✓ **tmux** (Terminal multiplexer)

**Optional Additional Testing**:
- kitty (GPU-accelerated)
- iTerm2 (macOS - if available)
- VS Code terminal
- screen (Terminal multiplexer)

---

## Test Prerequisites

### Build Test Binary

```bash
cd /home/mberry/Lab/c/lusush

# Compile the capability detection
gcc -c src/lle/terminal/capabilities.c -o build/capabilities.o \
    -I include -Wall -Wextra -O2

# Compile and link the test
gcc tests/lle/behavioral/test_terminal_capabilities_behavioral.c \
    build/capabilities.o -o build/test_terminal_capabilities \
    -I include -Wall -Wextra -O2

# Verify binary exists
ls -lh build/test_terminal_capabilities
```

---

## Test Procedure

### Test 1: Automated Test Suite (All Terminals)

**Run in each terminal type:**

```bash
./build/test_terminal_capabilities
```

**Expected Behavior**:
- All 12 tests pass
- Performance test shows <50ms (typically <5ms)
- Terminal type correctly identified
- Color capabilities detected appropriately
- Geometry matches terminal size

**Pass Criteria**:
- [ ] All 12 automated tests PASS
- [ ] Detection time <50ms
- [ ] No crashes or errors
- [ ] Terminal type identified correctly

**Evidence Required**: Screenshot of test output

---

### Test 2: Performance Validation (Each Terminal)

**Run multiple times to verify consistency:**

```bash
for i in {1..10}; do 
    ./build/test_terminal_capabilities | grep "Performance requirement"
done
```

**Expected Behavior**:
- All runs complete in <50ms
- Timing is consistent (variance <10ms)
- No timeouts or failures

**Pass Criteria**:
- [ ] All 10 runs <50ms
- [ ] Average time <20ms
- [ ] No performance degradation

**Evidence Required**: Screenshot showing all 10 runs

---

### Test 3: Terminal Geometry Detection

**Resize terminal and re-run:**

```bash
# Test at different sizes:
# 1. Small: 80x24 (standard)
# 2. Medium: 120x40
# 3. Large: 200x60
# 4. Wide: 300x24

./build/test_terminal_capabilities | grep "Terminal Geometry"
```

**Expected Behavior**:
- Detected size matches actual terminal size
- Reasonable bounds (20-500 cols, 10-200 rows)
- Updates correctly on resize

**Pass Criteria**:
- [ ] Size matches actual terminal (verified with `tput cols` and `tput lines`)
- [ ] All size variations detected correctly
- [ ] No crashes with extreme sizes

**Evidence Required**: Screenshots at 3+ different sizes

---

### Test 4: Color Capability Detection

**Test different color environments:**

```bash
# Test 1: Truecolor support
COLORTERM=truecolor ./build/test_terminal_capabilities | grep "Color"

# Test 2: 256-color support
TERM=xterm-256color unset COLORTERM; ./build/test_terminal_capabilities | grep "Color"

# Test 3: Basic color only
TERM=xterm unset COLORTERM; ./build/test_terminal_capabilities | grep "Color"

# Test 4: No color (conservative)
TERM=dumb ./build/test_terminal_capabilities | grep "Color"
```

**Expected Behavior**:
- COLORTERM=truecolor → Detects truecolor
- TERM=xterm-256color → Detects 256 colors
- TERM=xterm → Detects 16 colors or conservative
- TERM=dumb → No color support

**Pass Criteria**:
- [ ] Truecolor detected correctly
- [ ] 256-color detected correctly
- [ ] Conservative fallback works
- [ ] Flags are consistent (truecolor → 256 → ANSI)

**Evidence Required**: Screenshot of all 4 color tests

---

### Test 5: Adaptive Terminal Detection (Spec 26)

**Test enhanced terminal detection:**

```bash
# Test 1: VS Code terminal
TERM_PROGRAM=vscode ./build/test_terminal_capabilities | grep -E "(Enhanced|AI)"

# Test 2: AI environment
CLAUDE_CODE=1 ./build/test_terminal_capabilities | grep -E "(Enhanced|AI)"

# Test 3: Regular terminal
unset TERM_PROGRAM CLAUDE_CODE; ./build/test_terminal_capabilities | grep -E "(Enhanced|AI)"
```

**Expected Behavior**:
- TERM_PROGRAM=vscode → Enhanced terminal detected
- CLAUDE_CODE=1 → AI environment detected
- Regular terminal → Neither flag set

**Pass Criteria**:
- [ ] Enhanced terminal detected correctly
- [ ] AI environment detected correctly
- [ ] Regular terminals not misidentified

**Evidence Required**: Screenshot of adaptive detection tests

---

### Test 6: Non-TTY Behavior

**Test with piped input/output:**

```bash
# Pipe stdout (not a TTY)
./build/test_terminal_capabilities | cat | head -20

# Redirect to file
./build/test_terminal_capabilities > /tmp/test_output.txt
cat /tmp/test_output.txt
```

**Expected Behavior**:
- Detection succeeds (no crash)
- is_tty = false
- Conservative defaults (80x24, no color)
- All tests handle non-TTY gracefully

**Pass Criteria**:
- [ ] No crashes in non-TTY mode
- [ ] Reasonable fallback behavior
- [ ] Tests pass or skip appropriately

**Evidence Required**: Screenshot of non-TTY test output

---

### Test 7: Cross-Terminal Compatibility Matrix

**Create compatibility matrix by running in each terminal:**

| Terminal | Version | Detection Time | Color Depth | Type Detected | Pass/Fail |
|----------|---------|----------------|-------------|---------------|-----------|
| konsole  | X.Y.Z   | ? μs           | ?           | ?             | ?         |
| gnome-terminal | X.Y | ? μs           | ?           | ?             | ?         |
| xterm    | X.Y     | ? μs           | ?           | ?             | ?         |
| alacritty| X.Y     | ? μs           | ?           | ?             | ?         |
| tmux     | X.Y     | ? μs           | ?           | ?             | ?         |

**Fill in table by running:**

```bash
echo "Terminal: $(echo $TERM_PROGRAM) / $TERM"
./build/test_terminal_capabilities | grep -E "(Detection time|Color Depth|Terminal Type)"
```

**Pass Criteria**:
- [ ] All 5+ terminals tested
- [ ] All terminals PASS
- [ ] No terminal-specific failures
- [ ] Capabilities appropriate for each terminal

**Evidence Required**: Completed compatibility matrix + screenshots

---

## Quality Gate Checklist

### Automated Testing
- [ ] All 12 automated tests pass in all terminals
- [ ] Performance requirement (<50ms) met in all terminals
- [ ] Edge cases covered (non-TTY, unknown terminal, etc.)
- [ ] No memory leaks (run with valgrind if needed)

### Manual Testing
- [ ] Tested on minimum 5 terminal types
- [ ] Terminal geometry detection accurate
- [ ] Color capability detection appropriate
- [ ] Adaptive detection (Spec 26) working
- [ ] Non-TTY behavior safe and reasonable

### Documentation
- [ ] All test evidence captured (screenshots)
- [ ] Compatibility matrix completed
- [ ] Any issues documented in KNOWN_ISSUES.md
- [ ] Test protocol followed completely

### Architectural Compliance
- [ ] NO terminal state queries during operation
- [ ] NO escape sequences sent to terminal
- [ ] Detection uses only: environment, ioctl, terminfo
- [ ] Single detection at initialization (not repeated)
- [ ] Compliance check passes

---

## Evidence Storage

**Location**: `docs/lle_implementation/test_evidence/screenshots/week_1/`

**Required Files**:
- `20251019_automated_tests_konsole.png`
- `20251019_automated_tests_gnome_terminal.png`
- `20251019_automated_tests_xterm.png`
- `20251019_automated_tests_alacritty.png`
- `20251019_automated_tests_tmux.png`
- `20251019_performance_validation.png`
- `20251019_geometry_detection_sizes.png`
- `20251019_color_detection_all_modes.png`
- `20251019_adaptive_detection_spec26.png`
- `20251019_non_tty_behavior.png`
- `20251019_compatibility_matrix.png`

---

## Pass/Fail Criteria

### PASS Requirements (ALL must be met):
- ✓ All 12 automated tests pass in all 5+ terminals
- ✓ Performance <50ms in all terminals (MANDATORY)
- ✓ Terminal geometry accurate (±1 col/row acceptable)
- ✓ Color capabilities appropriate for each terminal
- ✓ Adaptive detection (Spec 26) working correctly
- ✓ No crashes, segfaults, or errors
- ✓ Non-TTY behavior safe
- ✓ Architectural compliance (no terminal queries)

### FAIL Conditions (ANY causes FAIL):
- ✗ Any automated test fails
- ✗ Performance >50ms in any terminal
- ✗ Crashes or segfaults
- ✗ Terminal queries detected (architectural violation)
- ✗ Incorrect terminal type classification
- ✗ Unsafe non-TTY behavior

---

## Known Limitations

Document any known limitations discovered during testing:

- [ ] None identified yet

---

## Sign-Off

**Tester**: ___________________________  
**Date**: ___________________________  
**Terminal Environment**: ___________________________

**Result**: 
- [ ] PASS - All criteria met, Week 1 complete
- [ ] PARTIAL - Some issues (document below)
- [ ] FAIL - Critical issues (document below)

**Issues Found**: 

(None expected for fresh implementation)

**Notes**:

(Add any observations)

---

**Next Steps After Pass**:
- Update AI_ASSISTANT_HANDOFF_DOCUMENT.md with Week 1 completion
- Update living documents
- Commit Week 1 implementation
- Proceed to Week 2: Display Integration

---

**Test Protocol Version**: 1.0  
**Created**: 2025-10-19  
**Status**: Ready for testing
