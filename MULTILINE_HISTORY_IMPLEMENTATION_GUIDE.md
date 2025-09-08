# Multiline History Implementation Guide for Next AI Assistant

**Priority:** High - Next Major Enhancement  
**Estimated Time:** 4-6 hours  
**Complexity:** Medium  
**Status:** Ready for Implementation  

## 🎯 OBJECTIVE

Implement configurable multiline history support in Lusush, enabling bash-like multiline command recall and editing capabilities using GNU Readline's native multiline support.

---

## 🔍 TECHNICAL BACKGROUND

### Current State Analysis
- ✅ **GNU Readline supports multiline history natively**
- ✅ **`add_history()` accepts strings with embedded `\n` characters**
- ✅ **Full editing support across line boundaries available**
- ❌ **Lusush currently converts newlines to spaces** - limiting multiline support

### Root Cause
The `convert_multiline_for_history()` function in `src/input.c` (around line 365) converts all newlines to spaces before storing commands in history:

```c
// Current implementation - LIMITS multiline support
while (*src) {
    if (*src == '\n') {
        *dst++ = ' ';  // ← This conversion prevents multiline history
    } else {
        *dst++ = *src;
    }
    src++;
}
```

---

## 🛠️ IMPLEMENTATION PLAN

### Step 1: Add Configuration Option (1 hour)

**File:** `src/config.c`

Add multiline history configuration:
```c
// In config_option_t config_options[]
{ "multiline_history_enabled", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
  &config.multiline_history_enabled, "Enable multiline history recall and editing",
  config_validate_bool },
```

**File:** `include/config.h`
```c
// In config_t struct
bool multiline_history_enabled;
```

**File:** `src/config.c` in `config_set_defaults()`
```c
config.multiline_history_enabled = true;  // Enable by default for modern experience
```

### Step 2: Modify History Conversion Logic (2 hours)

**File:** `src/input.c`

Replace the current `convert_multiline_for_history()` function:

```c
static char *convert_multiline_for_history(const char *input) {
    if (!input) return NULL;
    
    // If multiline history is enabled, preserve original formatting
    if (config.multiline_history_enabled) {
        return strdup(input);  // Keep newlines intact for multiline support
    }
    
    // Legacy behavior: convert newlines to spaces
    size_t len = strlen(input);
    char *converted = malloc(len + 1);
    if (!converted) return NULL;
    
    char *dst = converted;
    const char *src = input;
    
    while (*src) {
        if (*src == '\n') {
            *dst++ = ' ';
        } else {
            *dst++ = *src;
        }
        src++;
    }
    
    *dst = '\0';
    return converted;
}
```

### Step 3: Enhance Autosuggestion System (1 hour)

**File:** `src/autosuggestions.c`

The autosuggestion system may need updates to handle multiline history entries properly. Add a multiline comparison function:

```c
static char *normalize_for_suggestion_matching(const char *input) {
    if (!input || !config.multiline_history_enabled) {
        return strdup(input ? input : "");
    }
    
    // For suggestion matching, temporarily normalize multiline to single line
    size_t len = strlen(input);
    char *normalized = malloc(len + 1);
    if (!normalized) return NULL;
    
    char *dst = normalized;
    const char *src = input;
    
    while (*src) {
        if (*src == '\n') {
            *dst++ = ' ';
        } else {
            *dst++ = *src;
        }
        src++;
    }
    
    *dst = '\0';
    return normalized;
}
```

### Step 4: Add Configuration Command (0.5 hours)

**File:** `src/builtins/builtins.c`

Add ability to toggle multiline history:
```c
// In appropriate builtin command or add new command
if (strcmp(argv[1], "multiline_history") == 0) {
    if (argc == 3) {
        if (strcmp(argv[2], "on") == 0) {
            config.multiline_history_enabled = true;
            printf("Multiline history enabled\n");
        } else if (strcmp(argv[2], "off") == 0) {
            config.multiline_history_enabled = false;
            printf("Multiline history disabled\n");
        }
    } else {
        printf("Multiline history: %s\n", 
               config.multiline_history_enabled ? "enabled" : "disabled");
    }
    return 0;
}
```

### Step 5: Testing and Validation (1.5 hours)

Create comprehensive tests:

**Test Script:** `test_multiline_history.sh`
```bash
#!/bin/bash
echo "Testing multiline history implementation..."

echo -e "for i in 1 2 3\ndo\n  echo \$i\ndone\n\nexit" | ./builddir/lusush -i

# Test cases:
# 1. Multiline for loop recall and editing
# 2. Multiline if statement recall and editing  
# 3. Complex nested constructs
# 4. Configuration toggling
# 5. Backward compatibility with multiline_history=false
```

---

## 🎯 EXPECTED USER EXPERIENCE

### Before (Current Behavior)
```bash
# User types:
for i in 1 2 3
do
  echo $i  
done

# History stores: "for i in 1 2 3; do echo $i; done"
# UP arrow recalls: for i in 1 2 3; do echo $i; done
```

### After (Enhanced Behavior)  
```bash
# User types:
for i in 1 2 3
do
  echo $i
done

# History stores with newlines intact
# UP arrow recalls:
for i in 1 2 3
do
  echo $i    # ← User can edit here with full readline support
done
```

---

## 🔧 TECHNICAL CONSIDERATIONS

### GNU Readline Integration
- **No changes needed** to readline integration - it already supports multiline
- **History navigation** (UP/DOWN arrows) will work across line boundaries automatically
- **Editing capabilities** include insert/delete/navigation within multiline entries

### Performance Impact
- **Minimal** - Only affects history storage format
- **Memory usage** - Negligible difference between storing newlines vs spaces
- **Display performance** - No impact on interactive responsiveness

### Compatibility
- **Backward compatibility** - Preserved through configuration option
- **Cross-platform** - GNU Readline behavior consistent across Linux/macOS/BSD
- **Migration path** - Users can toggle between old and new behavior

---

## 🚨 POTENTIAL PITFALLS & SOLUTIONS

### Issue 1: History File Format
**Problem:** Existing history files have single-line format  
**Solution:** History loading should handle both formats gracefully

### Issue 2: Terminal Display Width
**Problem:** Long multiline commands might not display well in narrow terminals  
**Solution:** Leverage readline's built-in line wrapping capabilities

### Issue 3: Autosuggestion Matching
**Problem:** Multiline history entries might not match single-line partial input  
**Solution:** Use normalization function for matching while preserving original format

---

## ✅ VALIDATION CRITERIA

### Must Pass Before Merging
1. **Functionality Test:** Multiline commands recall and edit properly
2. **Configuration Test:** Toggle between multiline/single-line modes works
3. **Backward Compatibility:** Existing functionality unchanged when disabled
4. **Performance Test:** No measurable performance regression
5. **Cross-platform Test:** Works identically on Linux and macOS
6. **Memory Safety:** No leaks in multiline handling code

---

## 📚 REFERENCE MATERIALS

### GNU Readline Documentation
- History expansion: https://tiswww.cwru.edu/php/chet/readline/history.html
- Programming with history: Focus on `add_history()` and `HIST_ENTRY`

### Bash Implementation Reference
```bash
# Enable bash multiline history for reference
shopt -s lithist    # Literal history (preserve newlines)  
shopt -s cmdhist    # Command history (multi-line commands as one entry)
```

### Key Files to Study
- `src/input.c` - Current multiline handling
- `src/readline_integration.c` - History integration
- `src/autosuggestions.c` - May need multiline awareness updates

---

## 🎯 SUCCESS METRICS

Upon completion, users should experience:
- ✅ **Bash-like multiline editing** for complex commands
- ✅ **Seamless UP/DOWN navigation** across multiline history
- ✅ **Full readline editing** within recalled multiline commands  
- ✅ **Professional shell experience** comparable to modern shells
- ✅ **Zero regression** in existing single-line functionality

**Implementation Priority:** This enhancement will significantly improve Lusush's competitive position among modern interactive shells.