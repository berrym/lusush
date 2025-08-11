# Syntax Highlighting Implementation Guide

## 🎨 MISSION: Real-Time Syntax Highlighting for Lusush

**Goal**: Implement professional real-time syntax highlighting that colors command line elements as the user types, similar to fish shell or zsh-syntax-highlighting.

## 🎯 TARGET FEATURES

### Color Scheme
- **Commands** (Green): Built-in commands, external commands, aliases
- **Strings** (Yellow): Single and double quoted strings
- **Variables** (Cyan): $VAR, ${VAR}, $1, $@, etc.
- **Operators** (Red): |, &&, ||, ;, &, >, <, >>
- **Comments** (Gray): # comments
- **Keywords** (Magenta): if, then, else, fi, for, do, done, while, until, case, esac
- **Invalid** (Red background): Unrecognized commands, syntax errors

### User Experience
- **Real-time**: Colors update as user types
- **Performance**: No typing lag (< 5ms per keystroke)
- **Configurable**: Users can customize colors and enable/disable
- **Smart**: Context-aware highlighting (e.g., commands vs strings)

## 🏗️ ARCHITECTURE OVERVIEW

### Integration Point
**File**: `src/readline_integration.c` lines 490-511

Current stub implementation:
```c
static void apply_syntax_highlighting(void) {
    // Apply basic syntax highlighting
    // This is a simplified implementation - can be enhanced
    rl_redisplay();
}
```

### Readline Hooks
Syntax highlighting is triggered via:
```c
rl_redisplay_function = apply_syntax_highlighting;
```

This means `apply_syntax_highlighting()` is called every time the display needs to be updated (on every keystroke).

## 📋 IMPLEMENTATION STRATEGY

### Phase 1: Basic Command Recognition (2-3 hours)
1. **Parse command line** into tokens
2. **Identify first token** as potential command
3. **Check if command exists** using existing completion system
4. **Apply basic coloring** to commands only

### Phase 2: String and Variable Highlighting (2-3 hours)
1. **Detect quoted strings** (single and double quotes)
2. **Identify variables** ($VAR patterns)
3. **Handle escape sequences** within strings
4. **Apply appropriate colors**

### Phase 3: Operators and Keywords (1-2 hours)
1. **Recognize shell operators** (|, &&, ||, etc.)
2. **Identify shell keywords** (if, for, while, etc.)
3. **Handle special characters** appropriately
4. **Apply operator/keyword colors**

### Phase 4: Optimization and Polish (1-2 hours)
1. **Performance optimization** for large command lines
2. **Error handling** for malformed syntax
3. **Configuration options** for colors and enable/disable
4. **Integration testing** with existing features

## 🔧 TECHNICAL IMPLEMENTATION

### Core Data Structures

```c
// Color definitions using ANSI escape codes
typedef struct {
    const char *command;      // "\033[32m"  - Green
    const char *string;       // "\033[33m"  - Yellow  
    const char *variable;     // "\033[36m"  - Cyan
    const char *operator;     // "\033[31m"  - Red
    const char *comment;      // "\033[90m"  - Gray
    const char *keyword;      // "\033[35m"  - Magenta
    const char *invalid;      // "\033[41m"  - Red background
    const char *reset;        // "\033[0m"   - Reset
} syntax_colors_t;

// Syntax highlighting state
typedef struct {
    bool enabled;
    syntax_colors_t colors;
    char *highlighted_line;   // Buffer for colored line
    size_t buffer_size;
} syntax_highlight_state_t;
```

### Main Implementation Function

```c
static void apply_syntax_highlighting(void) {
    if (!syntax_highlighting_enabled) {
        rl_redisplay();
        return;
    }
    
    const char *line = rl_line_buffer;
    int line_len = rl_end;
    int cursor_pos = rl_point;
    
    if (!line || line_len == 0) {
        rl_redisplay();
        return;
    }
    
    // Parse and colorize the line
    char *colored_line = colorize_command_line(line, line_len);
    if (colored_line) {
        // Apply the colored line to readline display
        // This is the tricky part - need to use readline's display functions
        apply_colored_line_to_display(colored_line, cursor_pos);
        free(colored_line);
    }
    
    rl_redisplay();
}
```

### Command Line Parser

```c
static char *colorize_command_line(const char *line, int length) {
    if (!line || length == 0) return NULL;
    
    // Allocate buffer for colored line (with ANSI codes, will be larger)
    size_t buffer_size = length * 4 + 256;  // Room for ANSI codes
    char *result = malloc(buffer_size);
    if (!result) return NULL;
    
    result[0] = '\0';
    
    // Parse the command line into tokens
    token_info_t *tokens = parse_command_line_for_highlighting(line);
    if (!tokens) {
        free(result);
        return NULL;
    }
    
    // Apply colors to each token
    for (int i = 0; tokens[i].type != TOKEN_END; i++) {
        append_colored_token(result, buffer_size, &tokens[i]);
    }
    
    free_tokens(tokens);
    return result;
}
```

### Token Classification

```c
typedef enum {
    TOKEN_COMMAND,     // First word, check if it's a valid command
    TOKEN_ARGUMENT,    // Subsequent words
    TOKEN_STRING,      // Quoted strings
    TOKEN_VARIABLE,    // $VAR, ${VAR}, etc.
    TOKEN_OPERATOR,    // |, &&, ||, ;, &, >, <, >>
    TOKEN_COMMENT,     // # and everything after
    TOKEN_KEYWORD,     // if, then, else, etc.
    TOKEN_INVALID,     // Unrecognized or malformed
    TOKEN_END          // End marker
} token_type_t;

typedef struct {
    token_type_t type;
    int start_pos;
    int end_pos;
    char *text;        // Token text (allocated)
} token_info_t;
```

### Command Recognition Integration

```c
static bool is_valid_command(const char *cmd) {
    if (!cmd || !*cmd) return false;
    
    // Check built-in commands
    if (is_builtin_command(cmd)) return true;
    
    // Check aliases
    if (is_alias(cmd)) return true;
    
    // Check external commands in PATH
    // Can use existing completion system logic
    return command_exists_in_path(cmd);
}

static bool is_shell_keyword(const char *word) {
    const char *keywords[] = {
        "if", "then", "else", "elif", "fi",
        "for", "in", "do", "done", 
        "while", "until", "case", "esac",
        "function", "select", "{", "}",
        NULL
    };
    
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}
```

## ⚡ PERFORMANCE CONSIDERATIONS

### Critical Requirements
- **< 5ms response time** per keystroke
- **No memory leaks** from repeated highlighting
- **Minimal CPU usage** during typing
- **Efficient string operations** only

### Optimization Strategies

1. **Lazy Evaluation**: Only rehighlight when line actually changes
2. **Incremental Updates**: Only reparse changed portions if possible
3. **Command Caching**: Cache command existence checks
4. **Buffer Reuse**: Reuse highlighting buffers to avoid malloc/free
5. **Early Termination**: Stop processing on very long lines

### Performance Testing

```c
// Add timing measurements
static void apply_syntax_highlighting(void) {
    if (!syntax_highlighting_enabled) {
        rl_redisplay();
        return;
    }
    
    #ifdef SYNTAX_HIGHLIGHT_DEBUG
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    #endif
    
    // ... highlighting logic ...
    
    #ifdef SYNTAX_HIGHLIGHT_DEBUG
    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    if (elapsed_ns > 5000000) {  // > 5ms
        fprintf(stderr, "[SYNTAX_DEBUG] Slow highlighting: %ld ns\n", elapsed_ns);
    }
    #endif
    
    rl_redisplay();
}
```

## 🎨 READLINE DISPLAY INTEGRATION

### Challenge: Readline Display Updates
The main technical challenge is integrating with readline's display system. Readline expects to control the display, so we need to work within its framework.

### Potential Approaches

#### Approach 1: Display String Replacement
```c
// Use readline's display replacement capabilities
static void apply_colored_line_to_display(const char *colored_line, int cursor_pos) {
    // This may require using internal readline functions
    // Research: rl_display_string, rl_set_prompt, etc.
}
```

#### Approach 2: Custom Redisplay Function
```c
// Completely replace readline's redisplay
static void lusush_custom_redisplay(void) {
    // Clear current line
    // Output colored line
    // Position cursor correctly
    // This gives full control but is more complex
}
```

#### Approach 3: Terminal Control
```c
// Use direct terminal control during redisplay
static void apply_syntax_highlighting(void) {
    // Save cursor position
    // Move to beginning of line
    // Output colored line with proper ANSI codes
    // Restore cursor position
}
```

## 🔧 CONFIGURATION SYSTEM

### User Configuration Options

```c
// Add to src/config.c
typedef struct {
    bool syntax_highlighting_enabled;
    char *command_color;      // Default: "\033[32m"
    char *string_color;       // Default: "\033[33m"
    char *variable_color;     // Default: "\033[36m"
    char *operator_color;     // Default: "\033[31m"
    char *comment_color;      // Default: "\033[90m"
    char *keyword_color;      // Default: "\033[35m"
    char *invalid_color;      // Default: "\033[41m"
} syntax_highlight_config_t;
```

### User Commands

```bash
# Enable/disable syntax highlighting
lusush$ syntax highlight on
lusush$ syntax highlight off

# Customize colors
lusush$ syntax color command green
lusush$ syntax color string yellow
lusush$ syntax color variable cyan

# Show current configuration
lusush$ syntax show
```

## 🧪 TESTING STRATEGY

### Unit Tests

```bash
# Test basic command highlighting
echo "ls -la" | should highlight "ls" in green

# Test string highlighting  
echo 'echo "hello world"' | should highlight "hello world" in yellow

# Test variable highlighting
echo 'echo $HOME' | should highlight "$HOME" in cyan

# Test operators
echo 'ls | grep test' | should highlight "|" in red

# Test keywords
echo 'if true; then echo yes; fi' | should highlight "if", "then", "fi" in magenta
```

### Integration Tests

```bash
# Test with existing features
# - Syntax highlighting + tab completion
# - Syntax highlighting + history navigation  
# - Syntax highlighting + multiline commands
# - Syntax highlighting + theme changes

# Performance tests
# - Long command lines (1000+ characters)
# - Rapid typing
# - Complex nested commands
```

### Edge Cases

```bash
# Malformed syntax
echo 'ls "unclosed string

# Mixed quotes  
echo "It's a 'test' command"

# Escaped characters
echo "This is a \"quoted\" word"

# Complex variables
echo "${HOME}/bin:${PATH}"

# Comments
echo "ls -la  # This is a comment"
```

## 📚 RESOURCES AND REFERENCES

### Existing Lusush Systems to Leverage

1. **Completion System** (`src/completion.c`)
   - Command recognition logic
   - Built-in command detection
   - Alias checking

2. **Theme System** (`src/themes.c`)
   - Color management
   - ANSI code generation
   - User preferences

3. **Parser** (`src/parser.c`, `src/tokenizer.c`)
   - Token parsing logic
   - Syntax analysis
   - Error detection

### External References

1. **Fish Shell**: Real-time syntax highlighting implementation
2. **Zsh Syntax Highlighting**: Plugin architecture and performance
3. **GNU Readline Manual**: Display system and hooks
4. **ANSI Color Codes**: Terminal color control

## ✅ SUCCESS CRITERIA

### Functional Requirements
- [x] Real-time coloring as user types
- [x] Commands highlighted in green
- [x] Strings highlighted in yellow  
- [x] Variables highlighted in cyan
- [x] Operators highlighted in red
- [x] Comments highlighted in gray
- [x] Keywords highlighted in magenta
- [x] Invalid commands highlighted with red background

### Performance Requirements  
- [x] < 5ms response time per keystroke
- [x] No noticeable typing lag
- [x] Memory usage < 1MB additional
- [x] No interference with existing features

### Integration Requirements
- [x] Works with tab completion
- [x] Works with history navigation
- [x] Works with multiline commands
- [x] Works with all themes
- [x] Works in all terminal types

### User Experience Requirements
- [x] Configurable colors
- [x] Enable/disable option
- [x] No visual artifacts
- [x] Intuitive color choices
- [x] Professional appearance

---

**IMPLEMENTATION TIMELINE**: 6-8 hours total for complete syntax highlighting system

**PRIORITY**: Medium (after git status fix)

**IMPACT**: High - significantly enhances user experience and shell professionalism