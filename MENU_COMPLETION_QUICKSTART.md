# Lusush Menu Completion - Quick Start Implementation Template

**Branch**: `feature/menu-completion`  
**Target**: 2-4 day implementation  
**Goal**: Fish-style menu completion with TAB cycling

## 🚀 Quick Start Checklist

### Day 1: Basic TAB Cycling (4-6 hours)

#### Step 1: Add Configuration (30 minutes)
- [ ] Add to `include/config.h`:
```c
typedef struct {
    bool enabled;
    bool show_descriptions;
    int max_columns;
    int max_rows;
    char *selection_color;
    char *description_color;
} menu_completion_config_t;

// Add to main config struct
extern menu_completion_config_t menu_completion;
```

- [ ] Add to `src/config.c`:
```c
// Add these entries to config_entries array
{"menu_completion_enabled", CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
 &config.menu_completion.enabled, "Enable menu completion cycling"},
{"menu_completion_show_descriptions", CONFIG_TYPE_BOOL, CONFIG_SECTION_COMPLETION,
 &config.menu_completion.show_descriptions, "Show descriptions in completions"},
{"menu_completion_max_columns", CONFIG_TYPE_INT, CONFIG_SECTION_COMPLETION,
 &config.menu_completion.max_columns, "Maximum columns (0=auto)"},
```

#### Step 2: Create Menu Completion Module (2 hours)
- [ ] Create `src/menu_completion.c`:
```c
#include "../include/menu_completion.h"
#include <readline/readline.h>

static menu_completion_state_t menu_state = {0};

bool lusush_menu_completion_init(void) {
    // Initialize menu completion system
    menu_state.enabled = config.menu_completion.enabled;
    return true;
}

int lusush_menu_complete_handler(int count, int key) {
    // Handle TAB key for menu completion
    if (!menu_state.enabled) {
        return rl_complete(count, key);
    }
    
    // Use readline's built-in menu complete
    return rl_menu_complete(count, key);
}

void lusush_menu_completion_cleanup(void) {
    // Cleanup resources
}
```

- [ ] Create `include/menu_completion.h`:
```c
#ifndef MENU_COMPLETION_H
#define MENU_COMPLETION_H

#include <stdbool.h>

typedef struct {
    char **completions;
    char **descriptions;
    int count;
    int selected_index;
    bool enabled;
    bool active;
} menu_completion_state_t;

bool lusush_menu_completion_init(void);
int lusush_menu_complete_handler(int count, int key);
void lusush_menu_completion_cleanup(void);

#endif /* MENU_COMPLETION_H */
```

#### Step 3: Integrate with Readline (1 hour)
- [ ] Modify `src/readline_integration.c`:
```c
#include "../include/menu_completion.h"

// In lusush_readline_init():
if (!lusush_menu_completion_init()) {
    fprintf(stderr, "Warning: Failed to initialize menu completion\n");
}

// In lusush_completion_setup():
if (config.menu_completion.enabled) {
    rl_bind_key('\t', lusush_menu_complete_handler);
} else {
    rl_bind_key('\t', rl_complete);
}
```

#### Step 4: Update Build System (15 minutes)
- [ ] Add to `meson.build`:
```meson
# Add to src array:
'src/menu_completion.c',
```

#### Step 5: Test Basic Functionality (1 hour)
```bash
ninja -C builddir
echo -e 'config set menu_completion_enabled true\nls /usr/bin/g\t\t\t\nexit' | ./builddir/lusush -i
```

**Expected Day 1 Result**: TAB cycles through completions using readline's built-in menu complete.

---

### Day 2: Enhanced Display (4-6 hours)

#### Step 1: Custom Display Hook (2 hours)
- [ ] Add to `src/menu_completion.c`:
```c
void lusush_display_completion_menu(char **matches, int len, int max) {
    if (!config.menu_completion.enabled) {
        rl_display_matches(matches, len, max);
        return;
    }
    
    // Custom multi-column display with descriptions
    display_menu_completions(matches, len);
}

static void display_menu_completions(char **matches, int len) {
    // Calculate terminal width
    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        term_width = ws.ws_col;
    }
    
    // Calculate columns
    int max_columns = config.menu_completion.max_columns;
    if (max_columns == 0) {
        max_columns = term_width / 20; // Auto-calculate
    }
    
    // Display in columns
    printf("\n");
    for (int i = 1; i < len; i++) { // Skip first element (common prefix)
        printf("%-20s", matches[i]);
        if (i % max_columns == 0) {
            printf("\n");
        }
    }
    printf("\n");
}
```

#### Step 2: Selection Highlighting (2 hours)
- [ ] Add selection tracking and highlighting
- [ ] Implement visual feedback for current selection

#### Step 3: Integration Testing (1 hour)
```bash
./test_fish_features_macos.sh  # Must still pass all tests
```

---

### Day 3: Theme Integration & Polish (4-6 hours)

#### Step 1: Theme Integration (2 hours)
- [ ] Add menu colors to `src/themes.c`
- [ ] Apply theme colors to menu display

#### Step 2: Configuration Commands (1 hour)
- [ ] Test runtime configuration changes
- [ ] Verify config persistence

#### Step 3: Performance Optimization (2 hours)
- [ ] Add caching for large completion sets
- [ ] Performance profiling and optimization

#### Step 4: Error Handling (1 hour)
- [ ] Comprehensive error handling
- [ ] Graceful degradation

---

### Day 4: Final Integration (2-4 hours)

#### Step 1: Testing (2 hours)
```bash
# Regression testing
./test_fish_features_macos.sh

# Menu-specific testing
./test_menu_completion_integration.sh  # Create this

# Performance testing
time echo -e 'ls /usr/bin/\t\t\t\nexit' | ./builddir/lusush -i
```

#### Step 2: Documentation (1 hour)
- [ ] Update README.md with menu completion features
- [ ] Update help system

#### Step 3: Version Update (30 minutes)
```bash
# Update to v1.3.0
sed -i 's/1\.2\.0/1.3.0/g' include/version.h meson.build src/init.c
```

#### Step 4: Prepare for Merge (30 minutes)
```bash
git add .
git commit -m "feat: Implement Fish-style menu completion with TAB cycling"
git push origin feature/menu-completion
# Ready for review and merge to master
```

## 🧪 Testing Commands

### Basic Functionality Test
```bash
echo -e 'ls /usr/bin/g\t\t\t\nexit' | ./builddir/lusush -i
```

### Configuration Test
```bash
echo -e 'config set menu_completion_enabled true\nconfig get menu_completion_enabled\nexit' | ./builddir/lusush -i
```

### Performance Test
```bash
time echo -e 'ls /usr/bin/\t\nexit' | ./builddir/lusush -i
```

### Regression Test
```bash
./test_fish_features_macos.sh
```

## 🎯 Success Criteria

**Day 1**: Basic TAB cycling works
**Day 2**: Multi-column display with descriptions  
**Day 3**: Theme integration and optimization
**Day 4**: All tests pass, ready for production

**Final Goal**: `ls /usr/bin/g<TAB><TAB><TAB>` cycles through completions in organized columns with visual selection highlighting.

## 🚨 Critical Requirements

1. **Zero Regressions**: All existing tests must continue to pass
2. **Performance**: < 50ms display time, < 10ms cycling time
3. **Memory Safety**: No leaks detected by valgrind
4. **Cross-platform**: Works on macOS and Linux
5. **Professional Quality**: Enterprise-grade stability and appearance

This template provides a clear path from basic functionality to production-ready menu completion system in 2-4 days.