# Phase 2: Safe Display Management Implementation Guide
## Real-Time Syntax Highlighting - Safe Visual Implementation

**Phase**: 2 of 3  
**Estimated Time**: 8 hours  
**Priority**: Core visual highlighting without display corruption  
**Prerequisites**: Phase 1 complete (Smart Character Triggering working)

---

## 🎯 PHASE 2 OBJECTIVES

### Primary Goals
- ✅ Replace `lusush_mark_highlight_needed()` with actual safe highlighting
- ✅ Implement readline-compatible display management
- ✅ Eliminate all display corruption and terminal state issues
- ✅ Maintain perfect integration with existing readline features

### Success Criteria
- [ ] Real-time syntax highlighting visually working
- [ ] Zero display corruption in any mode (search, completion, etc.)
- [ ] All special readline modes work perfectly
- [ ] Cursor positioning remains accurate
- [ ] Performance maintained from Phase 1

---

## 📋 PRE-IMPLEMENTATION CHECKLIST

### Verify Phase 1 Completion
```bash
# 1. Confirm Phase 1 is working
cd lusush && ninja -C builddir

# 2. Test smart triggering is functional
echo 'echo hello | grep world' | ./builddir/lusush -i 2>&1 | grep "DEBUG.*trigger"
# Should show: Intelligent triggers on operators and word boundaries

# 3. Verify no regressions
./test_interactive.sh && echo "Phase 1 regression test: PASS"

# 4. Confirm framework functions exist
grep -n "lusush_mark_highlight_needed" src/readline_integration.c
# Should show: The placeholder function from Phase 1
```

### Required Understanding
- Phase 1 provides intelligent character triggering
- `lusush_syntax_update_hook()` is called when highlighting needed
- All syntax analysis functions already exist and work
- Safety framework is implemented and tested

---

## 🔧 STEP-BY-STEP IMPLEMENTATION

### Step 1: Create Safe Buffer Management (90 minutes)

#### Location: `src/readline_integration.c`
#### Find: `static void lusush_mark_highlight_needed(void)` (added in Phase 1)

**Replace the Phase 1 placeholder with real implementation:**

```c
// Remove the old placeholder and add:

// Buffer management for safe highlighting
typedef struct {
    char *colored_buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    bool needs_realloc;
} highlight_buffer_t;

static highlight_buffer_t highlight_buf = {NULL, 0, 0, false};

// Initialize highlighting buffer
static bool init_highlight_buffer(size_t initial_size) {
    if (highlight_buf.colored_buffer) {
        free(highlight_buf.colored_buffer);
    }
    
    highlight_buf.buffer_capacity = initial_size * 10; // Extra space for color codes
    highlight_buf.colored_buffer = malloc(highlight_buf.buffer_capacity);
    
    if (!highlight_buf.colored_buffer) {
        highlight_buf.buffer_capacity = 0;
        return false;
    }
    
    highlight_buf.buffer_size = 0;
    highlight_buf.needs_realloc = false;
    return true;
}

// Clean up highlighting buffer
static void cleanup_highlight_buffer(void) {
    if (highlight_buf.colored_buffer) {
        free(highlight_buf.colored_buffer);
        highlight_buf.colored_buffer = NULL;
        highlight_buf.buffer_size = 0;
        highlight_buf.buffer_capacity = 0;
    }
}

// Ensure buffer has enough space
static bool ensure_buffer_capacity(size_t needed_size) {
    if (needed_size <= highlight_buf.buffer_capacity) {
        return true;
    }
    
    size_t new_capacity = needed_size * 2;
    char *new_buffer = realloc(highlight_buf.colored_buffer, new_capacity);
    
    if (!new_buffer) {
        return false;
    }
    
    highlight_buf.colored_buffer = new_buffer;
    highlight_buf.buffer_capacity = new_capacity;
    return true;
}
```

### Step 2: Implement Safe Highlighting Core (120 minutes)

**Replace `lusush_mark_highlight_needed()` with actual implementation:**

```c
// Phase 2: Real highlighting implementation
static void lusush_apply_safe_highlighting(void) {
    // Critical safety check
    if (!is_safe_for_highlighting()) {
        return;
    }
    
    // Validate readline state
    if (!rl_line_buffer || rl_end <= 0) {
        return;
    }
    
    size_t line_length = strlen(rl_line_buffer);
    if (line_length == 0) {
        return;
    }
    
    // Initialize buffer if needed
    if (!highlight_buf.colored_buffer || 
        !ensure_buffer_capacity(line_length * 10)) {
        // Fallback to normal display on memory issues
        rl_redisplay();
        return;
    }
    
    // Generate colored version
    if (!generate_colored_line(rl_line_buffer, highlight_buf.colored_buffer, 
                              highlight_buf.buffer_capacity)) {
        // Fallback on generation failure
        rl_redisplay();
        return;
    }
    
    // Apply the highlighting using safe method
    apply_highlighting_safely();
}

static bool generate_colored_line(const char *line, char *colored_output, size_t max_size) {
    if (!line || !colored_output || max_size == 0) {
        return false;
    }
    
    size_t output_pos = 0;
    size_t line_len = strlen(line);
    size_t i = 0;
    
    // State tracking
    bool in_string = false;
    bool in_single_quote = false;
    char string_char = '\0';
    
    while (i < line_len && output_pos < max_size - 50) { // Reserve space for color codes
        char c = line[i];
        
        // Handle string literals
        if (!in_string && (c == '"' || c == '\'')) {
            in_string = true;
            string_char = c;
            in_single_quote = (c == '\'');
            
            // Add string color
            output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                 "%s%c", string_color, c);
            i++;
            continue;
        }
        
        if (in_string && c == string_char) {
            // End of string
            output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                 "%c%s", c, reset_color);
            in_string = false;
            in_single_quote = false;
            string_char = '\0';
            i++;
            continue;
        }
        
        if (in_string) {
            // Inside string - just copy character
            if (output_pos < max_size - 1) {
                colored_output[output_pos++] = c;
            }
            i++;
            continue;
        }
        
        // Handle comments
        if (c == '#') {
            output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                 "%s", comment_color);
            while (i < line_len && output_pos < max_size - 10) {
                colored_output[output_pos++] = line[i++];
            }
            output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                 "%s", reset_color);
            break;
        }
        
        // Handle variables
        if (c == '$') {
            output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                 "%s%c", variable_color, c);
            i++;
            
            // Handle variable name
            while (i < line_len && output_pos < max_size - 10 &&
                   (isalnum(line[i]) || line[i] == '_' || line[i] == '{' || line[i] == '}')) {
                colored_output[output_pos++] = line[i++];
            }
            
            output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                 "%s", reset_color);
            continue;
        }
        
        // Handle operators
        if (strchr("|&;<>()", c)) {
            output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                 "%s%c%s", operator_color, c, reset_color);
            i++;
            continue;
        }
        
        // Handle word separators
        if (lusush_is_word_separator(c)) {
            colored_output[output_pos++] = c;
            i++;
            continue;
        }
        
        // Handle words (commands, keywords, numbers)
        if (isalnum(c) || c == '_' || c == '-' || c == '.') {
            size_t word_start = i;
            while (i < line_len && (isalnum(line[i]) || line[i] == '_' || 
                                   line[i] == '-' || line[i] == '.')) {
                i++;
            }
            
            size_t word_len = i - word_start;
            const char *word = line + word_start;
            const char *color = NULL;
            
            // Determine word type and color
            if (lusush_is_shell_keyword(word, word_len)) {
                color = keyword_color;
            } else if (lusush_is_shell_builtin(word, word_len)) {
                color = command_color;
            } else if (word_len > 0 && isdigit(word[0])) {
                // Check if it's a number
                bool is_number = true;
                for (size_t j = 0; j < word_len; j++) {
                    if (!isdigit(word[j]) && word[j] != '.') {
                        is_number = false;
                        break;
                    }
                }
                if (is_number) {
                    color = number_color;
                }
            }
            
            // Output the word with color
            if (color) {
                output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                     "%s", color);
            }
            
            // Copy the word
            for (size_t j = 0; j < word_len && output_pos < max_size - 10; j++) {
                colored_output[output_pos++] = word[j];
            }
            
            if (color) {
                output_pos += snprintf(colored_output + output_pos, max_size - output_pos,
                                     "%s", reset_color);
            }
            continue;
        }
        
        // Default: copy character as-is
        if (output_pos < max_size - 1) {
            colored_output[output_pos++] = c;
        }
        i++;
    }
    
    // Null terminate
    colored_output[output_pos] = '\0';
    return true;
}
```

### Step 3: Implement Safe Display Method (90 minutes)

**Add the safe display application:**

```c
static void apply_highlighting_safely(void) {
    // Method 1: Readline Buffer Swap (Safest)
    if (try_buffer_swap_method()) {
        return;
    }
    
    // Method 2: Direct Terminal Control (Backup)
    if (try_direct_terminal_method()) {
        return;
    }
    
    // Method 3: Fallback to standard display
    rl_redisplay();
}

static bool try_buffer_swap_method(void) {
    // This method temporarily swaps the readline buffer
    // Uses readline's own display mechanisms for safety
    
    if (!rl_line_buffer || !highlight_buf.colored_buffer) {
        return false;
    }
    
    // Save current state
    char *original_buffer = rl_line_buffer;
    int original_point = rl_point;
    int original_end = rl_end;
    
    // Calculate new cursor position accounting for color codes
    int adjusted_point = calculate_adjusted_cursor_position(original_point);
    
    // Temporarily replace buffer
    rl_line_buffer = highlight_buf.colored_buffer;
    rl_point = adjusted_point;
    rl_end = strlen(highlight_buf.colored_buffer);
    
    // Use readline's safe redisplay
    rl_redisplay();
    
    // Restore original state
    rl_line_buffer = original_buffer;
    rl_point = original_point;
    rl_end = original_end;
    
    return true;
}

static int calculate_adjusted_cursor_position(int original_pos) {
    // Calculate where the cursor should be in the colored buffer
    // Account for color escape sequences that don't affect cursor position
    
    if (!rl_line_buffer || !highlight_buf.colored_buffer || original_pos <= 0) {
        return 0;
    }
    
    int visible_chars = 0;
    int colored_pos = 0;
    const char *colored = highlight_buf.colored_buffer;
    
    while (colored[colored_pos] && visible_chars < original_pos) {
        if (colored[colored_pos] == '\033') {
            // Skip escape sequence
            while (colored[colored_pos] && colored[colored_pos] != 'm') {
                colored_pos++;
            }
            if (colored[colored_pos] == 'm') {
                colored_pos++;
            }
        } else {
            // Regular character
            visible_chars++;
            colored_pos++;
        }
    }
    
    return colored_pos;
}

static bool try_direct_terminal_method(void) {
    // Backup method: Direct terminal control
    // Only use if buffer swap fails
    
    if (!isatty(STDOUT_FILENO) || !rl_line_buffer) {
        return false;
    }
    
    // Save cursor position
    printf("\033[s");
    
    // Clear line and reposition
    printf("\r\033[K");
    
    // Print prompt
    if (rl_prompt) {
        printf("%s", rl_prompt);
    }
    
    // Print colored line
    if (highlight_buf.colored_buffer) {
        printf("%s", highlight_buf.colored_buffer);
    }
    
    // Restore cursor position relative to prompt
    printf("\033[u");
    
    // Move to correct position
    if (rl_point > 0) {
        printf("\r");
        if (rl_prompt) {
            printf("%s", rl_prompt);
        }
        
        // Move cursor to correct position (counting visible characters only)
        int visible_pos = 0;
        for (int i = 0; i < rl_point && rl_line_buffer[i]; i++) {
            printf("\033[C");
            visible_pos++;
        }
    }
    
    fflush(stdout);
    return true;
}
```

### Step 4: Enhanced Safety and Error Handling (60 minutes)

**Update the safety framework:**

```c
// Enhanced safety checking for Phase 2
static bool is_safe_for_highlighting(void) {
    // All Phase 1 checks plus Phase 2 specific ones
    
    // 1. Basic readline state validation (from Phase 1)
    if (rl_readline_state & (RL_STATE_ISEARCH | RL_STATE_NSEARCH | 
                            RL_STATE_SEARCH | RL_STATE_COMPLETING |
                            RL_STATE_VICMDONCE | RL_STATE_VIMOTION |
                            RL_STATE_MOREINPUT | RL_STATE_MULTIKEY |
                            RL_STATE_CALLBACK)) {
        return false;
    }
    
    // 2. Buffer validity (enhanced)
    if (!rl_line_buffer || rl_end < 0 || rl_point < 0 || rl_point > rl_end) {
        return false;
    }
    
    // 3. Terminal state validation
    if (!isatty(STDOUT_FILENO) || !isatty(STDIN_FILENO)) {
        return false;
    }
    
    // 4. Recursion protection (enhanced)
    static int recursion_depth = 0;
    if (recursion_depth > 0) {
        return false;
    }
    
    // 5. Memory state validation
    if (!highlight_buf.colored_buffer || highlight_buf.buffer_capacity == 0) {
        return false;
    }
    
    // 6. Terminal capability check
    const char *term = getenv("TERM");
    if (!term || strstr(term, "dumb")) {
        return false;
    }
    
    return true;
}

// Emergency recovery system
static void lusush_emergency_recovery(const char *error_context) {
    static int recovery_count = 0;
    recovery_count++;
    
    // Log the error
    fprintf(stderr, "[ERROR] Syntax highlighting emergency recovery #%d: %s\n", 
            recovery_count, error_context ? error_context : "unknown");
    
    // Disable highlighting temporarily
    syntax_highlighting_enabled = false;
    
    // Clean up any corrupted state
    cleanup_highlight_buffer();
    
    // Reset readline to safe state
    rl_redisplay_function = rl_redisplay;
    rl_pre_input_hook = NULL;
    
    // Clear any terminal corruption
    printf("\r\033[K");
    rl_forced_update_display();
    
    // Re-enable after a brief pause (if not too many failures)
    if (recovery_count < 3) {
        // Schedule re-enable
        rl_pre_input_hook = lusush_delayed_renable_hook;
    }
}

static int lusush_delayed_renable_hook(void) {
    // Re-enable highlighting after emergency recovery
    syntax_highlighting_enabled = true;
    rl_getc_function = lusush_getc;
    
    // Reinitialize buffer
    if (rl_line_buffer) {
        init_highlight_buffer(strlen(rl_line_buffer) + 100);
    }
    
    rl_pre_input_hook = NULL;
    fprintf(stderr, "[INFO] Syntax highlighting re-enabled after recovery\n");
    return 0;
}
```

### Step 5: Integration and Initialization Updates (45 minutes)

**Update the main enabling function:**

```c
// Update lusush_syntax_highlighting_set_enabled from Phase 1
void lusush_syntax_highlighting_set_enabled(bool enabled) {
    syntax_highlighting_enabled = enabled;
    
    if (enabled) {
        // Phase 2: Enable both smart triggering AND safe display
        rl_getc_function = lusush_getc;
        reset_typing_state();
        
        // Initialize highlighting buffer
        size_t initial_size = rl_line_buffer ? strlen(rl_line_buffer) + 100 : 1000;
        if (!init_highlight_buffer(initial_size)) {
            fprintf(stderr, "[ERROR] Failed to initialize highlighting buffer\n");
            syntax_highlighting_enabled = false;
            rl_getc_function = rl_getc;
            return;
        }
        
        // Keep standard redisplay as primary, but we'll override when safe
        rl_redisplay_function = rl_redisplay;
        
        fprintf(stderr, "[INFO] Real-time syntax highlighting enabled (Phase 2)\n");
    } else {
        // Disable: clean up and restore standard functions
        cleanup_highlight_buffer();
        rl_redisplay_function = rl_redisplay;
        rl_getc_function = rl_getc;
        rl_pre_input_hook = NULL;
        
        fprintf(stderr, "[INFO] Syntax highlighting disabled\n");
    }
}

// Update the hook function to call real highlighting
static int lusush_syntax_update_hook(void) {
    // Only update if it's safe to do so
    if (is_safe_for_highlighting()) {
        // Phase 2: Apply actual highlighting instead of just marking
        lusush_apply_safe_highlighting();
    }
    
    // Remove the hook after use
    rl_pre_input_hook = NULL;
    return 0;
}
```

---

## 🧪 TESTING PROCEDURES

### Test 1: Basic Visual Highlighting (30 minutes)

```bash
# Enable syntax highlighting and test visual output
cd lusush
ninja -C builddir

# Test 1: Basic command highlighting
echo 'echo "hello world"' | ./builddir/lusush -i
# Expected: 'echo' in green, "hello world" in yellow

# Test 2: Operator highlighting
echo 'ls | grep test' | ./builddir/lusush -i
# Expected: '|' in red

# Test 3: Variable highlighting
echo 'echo $HOME' | ./builddir/lusush -i
# Expected: '$HOME' in magenta

# Test 4: Keyword highlighting
echo 'if test; then echo ok; fi' | ./builddir/lusush -i
# Expected: 'if', 'then', 'fi' in blue
```

### Test 2: Special Mode Safety (45 minutes)

```bash
# Test that highlighting doesn't break special modes

# Test 1: Reverse search (Ctrl+R)
./builddir/lusush -i
# Type some commands, then press Ctrl+R and search
# Expected: No display corruption, search works normally

# Test 2: Tab completion
./builddir/lusush -i
# Type "gi" and press TAB for git completion
# Expected: Completion works, highlighting resumes after

# Test 3: History navigation
./builddir/lusush -i
# Use arrow keys to navigate history
# Expected: No artifacts, highlighting updates correctly

# Test 4: Multi-line editing
./builddir/lusush -i
# Type: for i in 1 2 3\ndo\necho $i\ndone
# Expected: Highlighting works across all lines
```

### Test 3: Performance and Stability (30 minutes)

```bash
# Test performance with highlighting enabled
time for i in {1..50}; do
    echo "echo test$i" | ./builddir/lusush -i > /dev/null
done
# Expected: < 2 seconds total

# Test memory stability
valgrind --leak-check=full ./builddir/lusush -c 'echo "memory test with highlighting"'
# Expected: No memory leaks

# Test long commands
echo 'echo "very long command with many words and operators | grep something && echo success || echo failure"' | ./builddir/lusush -i
# Expected: Highlights correctly, no buffer overflow
```

### Test 4: Error Recovery (15 minutes)

```bash
# Test emergency recovery system

# Simulate terminal corruption
TERM=dumb ./builddir/lusush -c 'echo test'
# Expected: Highlighting disabled automatically

# Test recovery after errors
./builddir/lusush -i
# Enable highlighting, cause some error condition, verify recovery
```

---

## 🔍 VERIFICATION CHECKLIST

### Phase 2 Completion Criteria

#### Visual Functionality ✅
- [ ] Real-time syntax highlighting visually working
- [ ] Colors applied correctly to all syntax elements
- [ ] Cursor positioning accurate with colored text
- [ ] No visual artifacts or display corruption

#### Safety and Stability ✅
- [ ] All special readline modes work perfectly
- [ ] No display corruption in any scenario
- [ ] Emergency recovery system functional
- [ ] Memory management stable (no leaks)

#### Performance ✅
- [ ] Highlighting performance acceptable (< 5ms per update)
- [ ] No significant impact on typing speed
- [ ] Phase 1 performance maintained
- [ ] Memory usage reasonable (< 100KB overhead)

#### Integration ✅
- [ ] All existing shell functionality preserved
- [ ] Theme system works with highlighting
- [ ] Git integration unaffected
- [ ] Tab completion fully functional

### Expected Visual Output

**Before Phase 2:**
```
$ echo "hello world"
```

**After Phase 2:**
```
$ echo "hello world"
  ^^^^       ^^^^
  green      yellow
```

**Complex example:**
```
$ if test -f /etc/passwd; then echo "found"; fi
  ^^      ^               ^^^^      ^^^^^^^  ^^
  blue    green           blue      yellow   blue
```

---

## 🔧 TROUBLESHOOTING GUIDE

### Common Issues and Solutions

#### Issue 1: Display Corruption
**Symptoms**: Garbled text, cursor in wrong position
**Solution**: Check `is_safe_for_highlighting()` conditions
**Debug**: Add more safety checks, verify terminal state

#### Issue 2: Performance Degradation
**Symptoms**: Slow typing, laggy response
**Solution**: Review buffer management, optimize color generation
**Debug**: Profile `generate_colored_line()` function

#### Issue 3: Memory Leaks
**Symptoms**: Increasing memory usage over time
**Solution**: Verify `cleanup_highlight_buffer()` is called
**Debug**: Check all malloc/free pairs

#### Issue 4: Special Mode Conflicts
**Symptoms**: Tab completion or search broken
**Solution**: Enhance safety checks for more readline states
**Debug**: Add logging to see which states cause issues

---

## 📝 COMPLETION CHECKLIST

### Before Proceeding to Phase 3

- [ ] All visual highlighting working correctly
- [ ] Zero display corruption in extensive testing
- [ ] Performance meets Phase 1 baseline
- [ ] All existing functionality preserved
- [ ] Emergency recovery tested and working

### Code Quality Standards

- [ ] All buffer management functions properly handle edge cases
- [ ] Error handling comprehensive and tested
- [ ] Memory allocation/deallocation balanced
- [ ] Code follows project style guidelines
- [ ] Documentation updated for new functions

### Documentation Updates

- [ ] Update CHANGELOG.md with Phase 2 completion
- [ ] Document any new configuration options
- [ ] Update performance benchmarks
- [ ] Prepare handoff notes for Phase 3

---

## 🚀 PHASE 3 PREPARATION

Phase 2 completion provides a fully functional real-time syntax highlighting system. Phase 3 will focus on performance optimization and advanced features:

### Expected State After Phase 2
- Real-time syntax highlighting working and stable
- All safety mechanisms proven in testing
- Performance baseline for optimization established
- Foundation ready for advanced optimization

### Phase 3 Integration Points
1. **Buffer Management**: Optimize for incremental updates
2. **Performance Monitoring**: Use Phase 2 baseline for comparison
3. **Advanced Features**: Build on stable Phase 2 foundation
4. **User Configuration**: Extend Phase 2 safety framework

**Phase 2 Success = Production-Ready Syntax Highlighting** ✅