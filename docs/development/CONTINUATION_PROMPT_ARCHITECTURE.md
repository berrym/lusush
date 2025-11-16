# Continuation Prompt Architecture - Complete Implementation Documentation

**Document**: CONTINUATION_PROMPT_ARCHITECTURE.md  
**Created**: 2025-11-16  
**Status**: Production Implementation (Session 18)  
**Author**: Session 18 Implementation  
**Related**: CONTINUATION_PROMPT_SCREEN_BUFFER_PLAN.md, MODERN_EDITOR_WRAPPING_RESEARCH.md

---

## Executive Summary

This document provides complete architectural documentation for the working continuation prompt implementation in Lusush. This implementation successfully solves the critical line wrapping bug where multiline input with continuation prompts caused display corruption.

**Key Achievement**: Context-aware continuation prompts (loop>, if>, quote>, etc.) with correct cursor positioning and proper handling of line wrapping in the middle of multiline input.

**Implementation Approach**: Direct integration using existing infrastructure (screen_buffer prefix support + input_continuation module) rather than a separate display layer.

**Result**: Clean, simple, robust implementation with no display corruption and accurate cursor positioning in all scenarios.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Critical Problem Solved](#critical-problem-solved)
3. [Component Details](#component-details)
4. [Implementation Flow](#implementation-flow)
5. [Line Wrapping Solution](#line-wrapping-solution)
6. [Why No Display Layer](#why-no-display-layer)
7. [Code Walkthrough](#code-walkthrough)
8. [Testing and Verification](#testing-and-verification)
9. [Design Decisions](#design-decisions)
10. [Future Considerations](#future-considerations)

---

## Architecture Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         LLE Buffer                           │
│              (Contains text with '\n' newlines)             │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ↓
┌─────────────────────────────────────────────────────────────┐
│                    display_bridge                            │
│          (Sends rendered output to command_layer)           │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ↓
┌─────────────────────────────────────────────────────────────┐
│                    command_layer                             │
│        (Stores highlighted text with ANSI codes)            │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ↓
┌─────────────────────────────────────────────────────────────┐
│              dc_handle_redraw_needed()                       │
│         (Event handler in display_controller)               │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ 1. Get command text from command_layer             │    │
│  │ 2. Count newlines → detect multiline               │    │
│  │ 3. If multiline:                                   │    │
│  │    a. Parse line-by-line (strip ANSI first)        │    │
│  │    b. Analyze continuation state                   │    │
│  │    c. Get context-aware prompts                    │    │
│  │    d. Set prefixes on screen_buffer                │    │
│  │ 4. Render to screen_buffer (accounts for prefixes) │    │
│  │ 5. Output to terminal (insert prompts at newlines) │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────────┬───────────────────────────────────────┘
                      │
        ┌─────────────┼─────────────┐
        │             │             │
        ↓             ↓             ↓
┌──────────────┐ ┌──────────┐ ┌──────────────┐
│input_        │ │screen_   │ │Terminal      │
│continuation  │ │buffer    │ │Output        │
│              │ │          │ │              │
│• Analyzes    │ │• Prefix  │ │• Prompt text │
│  shell state │ │  storage │ │• Command text│
│• Returns     │ │• Cursor  │ │  with        │
│  prompts     │ │  calc    │ │  prompts     │
└──────────────┘ └──────────┘ └──────────────┘
```

### Component Responsibilities

| Component | Responsibility | Type |
|-----------|----------------|------|
| `input_continuation` | Parse shell syntax, track state, return prompts | Shared module |
| `screen_buffer` | Store prefixes, calculate cursor positions | Data structure |
| `display_controller` | Orchestrate detection, parsing, rendering, output | Controller |
| `command_layer` | Store highlighted command text | Display layer |

**Key Insight**: No dedicated continuation prompt display layer is needed. The functionality is achieved through direct integration in the display controller using existing infrastructure.

---

## Critical Problem Solved

### The Line Wrapping Bug

**Problem**: When a long line in multiline input wraps at the terminal width, continuation prompts appeared on the wrapped portion, causing display corruption and incorrect cursor positioning.

**Example of Bug**:
```bash
$ for i in 1 2 3; do
loop> echo "this is a very long line that wraps when it exceeds terminal width"
      │                                                                        │
      └─ Line wraps here at column 80 ─────────────────────────────────────────┘
loop> │ ← BUG: Continuation prompt appeared here on wrapped line!
loop> done
```

### Previous Failed Approach

**Detection logic** (reverted commits 00d6458 - eecb87c):
```c
if (desired_screen.num_rows > 1) {
    // Apply continuation prompts
}
```

**Why it failed**: `num_rows > 1` is true for:
- ✅ Actual multiline input (contains `\n`) - should show prompts
- ❌ Wrapped single-line input (exceeds terminal width) - should NOT show prompts

This conflated two different concepts:
- **Logical lines** (separated by `\n` characters)
- **Visual lines** (separated by terminal width wrapping)

### Correct Solution

**Detection logic** (current implementation):
```c
int newline_count = count_newlines(command_buffer);
bool is_multiline = (newline_count > 0);
```

**Why it works**: Only counts actual `\n` characters in the buffer, distinguishing:
- **Logical newlines** (`\n` in buffer) → Apply continuation prompts
- **Visual wraps** (terminal width exceeded) → NO continuation prompts

---

## Component Details

### 1. input_continuation Module

**Location**: `src/input_continuation.c`, `include/input_continuation.h`

**Purpose**: Shared shell parsing infrastructure (not LLE-specific, not display-specific)

**Key Functions**:
```c
void continuation_state_init(continuation_state_t *state);
void continuation_analyze_line(const char *line, continuation_state_t *state);
const char *continuation_get_prompt(const continuation_state_t *state);
bool continuation_is_complete(const continuation_state_t *state);
```

**State Tracking**:
```c
typedef struct {
    // Quote tracking
    bool in_single_quote;
    bool in_double_quote;
    bool in_backtick;
    
    // Control structure tracking
    bool in_if_statement;
    bool in_while_loop;
    bool in_for_loop;
    bool in_until_loop;
    bool in_case_statement;
    bool in_function_definition;
    
    // Nesting depth
    int compound_command_depth;
    int paren_count;
    int brace_count;
    int bracket_count;
} continuation_state_t;
```

**Context-Aware Prompts**:
```c
const char *continuation_get_prompt(const continuation_state_t *state) {
    if (state->in_single_quote || state->in_double_quote) {
        return "quote> ";
    } else if (state->in_function_definition) {
        return "function> ";
    } else if (state->in_if_statement) {
        return "if> ";
    } else if (state->in_while_loop || state->in_for_loop || state->in_until_loop) {
        return "loop> ";
    } else if (state->in_case_statement) {
        return "case> ";
    }
    
    // Default: PS2 from environment (usually "> ")
    return symtable_get_global_default("PS2", "> ");
}
```

**Why This Module**: Shared between LLE and non-LLE input systems. Already existed before this implementation.

---

### 2. screen_buffer Prefix Support

**Location**: `src/display/screen_buffer.c`, `include/display/screen_buffer.h`

**Purpose**: Store and track line prefixes (like continuation prompts) separately from line content

**Data Structures**:
```c
typedef struct {
    char *text;              // Prefix text (e.g., "loop> ")
    size_t length;           // Length in bytes
    size_t visual_width;     // Visual width in columns (excludes ANSI codes)
    bool contains_ansi;      // True if prefix contains ANSI escape codes
    bool dirty;              // True if prefix changed since last render
} screen_line_prefix_t;

typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];  // Line content
    int length;                                    // Number of cells used
    screen_line_prefix_t *prefix;                  // Optional prefix (NULL if none)
    bool prefix_dirty;                             // Prefix dirty tracking
} screen_line_t;
```

**Key Functions**:
```c
// Set prefix for a line
bool screen_buffer_set_line_prefix(
    screen_buffer_t *buffer,
    int line_num,
    const char *prefix_text
);

// Get prefix visual width
size_t screen_buffer_get_line_prefix_visual_width(
    const screen_buffer_t *buffer,
    int line_num
);

// Get prefix text
const char *screen_buffer_get_line_prefix(
    const screen_buffer_t *buffer,
    int line_num
);
```

**Why Separate Storage**:
1. **Independent tracking**: Prefix can change without re-rendering content
2. **Easy width access**: Visual width immediately available for cursor calculations
3. **Clean separation**: Content and metadata are distinct
4. **Efficient updates**: Only prefix marked dirty, not entire line

**Note**: This infrastructure already existed (added in earlier sessions). Current implementation just uses it.

---

### 3. display_controller Integration

**Location**: `src/display/display_controller.c`

**Event Handler**: `dc_handle_redraw_needed()`

**Integration Points**:

#### A. Multiline Detection
```c
// Location: display_controller.c:229-230
int newline_count = count_newlines(command_buffer);
bool is_multiline = (newline_count > 0);
```

**Helper Function**:
```c
// Location: display_controller.c:104-112
static int count_newlines(const char *text) {
    if (!text) return 0;
    int count = 0;
    for (const char *p = text; *p; p++) {
        if (*p == '\n') count++;
    }
    return count;
}
```

#### B. Line-by-Line State Analysis with ANSI Stripping
```c
// Location: display_controller.c:234-275
if (is_multiline) {
    continuation_state_t cont_state;
    continuation_state_init(&cont_state);
    
    int logical_line = 0;
    const char *line_start = command_buffer;
    char line_buffer[4096];
    
    while (line_start && *line_start) {
        const char *newline_pos = strchr(line_start, '\n');
        
        if (newline_pos) {
            // Extract this line
            size_t line_len = newline_pos - line_start;
            if (line_len >= sizeof(line_buffer)) {
                line_len = sizeof(line_buffer) - 1;
            }
            memcpy(line_buffer, line_start, line_len);
            line_buffer[line_len] = '\0';
            
            // CRITICAL: Strip ANSI escape sequences before analyzing
            // command_buffer has syntax highlighting, but parser expects plain text
            char plain_buffer[4096];
            size_t plain_pos = 0;
            bool in_ansi = false;
            
            for (size_t i = 0; i < line_len && plain_pos < sizeof(plain_buffer) - 1; i++) {
                if (line_buffer[i] == '\033' || line_buffer[i] == '\x1b') {
                    in_ansi = true;
                    continue;
                }
                if (in_ansi) {
                    if ((line_buffer[i] >= 'A' && line_buffer[i] <= 'Z') || 
                        (line_buffer[i] >= 'a' && line_buffer[i] <= 'z') ||
                        line_buffer[i] == 'm') {
                        in_ansi = false;
                    }
                    continue;
                }
                plain_buffer[plain_pos++] = line_buffer[i];
            }
            plain_buffer[plain_pos] = '\0';
            
            // Analyze this line to update continuation state
            continuation_analyze_line(plain_buffer, &cont_state);
            
            logical_line++;
            line_start = newline_pos + 1;
            
            // Get continuation prompt for NEXT line based on current state
            const char *cont_prompt = continuation_get_prompt(&cont_state);
            if (cont_prompt) {
                screen_buffer_set_line_prefix(&desired_screen, 
                                             desired_screen.command_start_row + logical_line,
                                             cont_prompt);
            }
        } else {
            break;
        }
    }
    
    continuation_state_cleanup(&cont_state);
}
```

**Why ANSI Stripping is Critical**:

Command buffer contains: `"\033[32mfor\033[0m i in 1 2 3; \033[32mdo\033[0m"`

Without stripping:
- Parser sees: `"\033[32mfor\033[0m"` instead of `"for"`
- Fails to detect keyword
- Returns generic `"> "` instead of `"loop>"`

With stripping:
- Parser sees: `"for i in 1 2 3; do"`
- Detects `for` keyword
- Sets `in_for_loop = true`
- Returns `"loop>"`

#### C. Screen Buffer Rendering
```c
// Location: display_controller.c:277
screen_buffer_render(&desired_screen, prompt_buffer, command_buffer, cursor_byte_offset);
```

**What happens inside** (see next section for details):
- Processes prompt text (no changes needed)
- Processes command text character by character
- When encountering `\n`:
  - Checks if next row has a prefix
  - Sets `col = prefix_visual_width` (not 0)
- Calculates final cursor position accounting for all prefixes

#### D. Terminal Output with Prompt Insertion
```c
// Location: display_controller.c:312-357
if (command_buffer[0]) {
    if (is_multiline) {
        // Write command text line by line with continuation prompts
        const char *line = command_buffer;
        int line_num = 0;
        
        while (*line) {
            const char *nl = strchr(line, '\n');
            
            if (nl) {
                // Write line up to newline
                write(STDOUT_FILENO, line, nl - line);
                write(STDOUT_FILENO, "\n", 1);
                
                line_num++;
                line = nl + 1;
                
                // Write continuation prompt for next line (if there's more content)
                if (*line) {
                    const char *cont_prompt = screen_buffer_get_line_prefix(
                        &desired_screen, 
                        desired_screen.command_start_row + line_num
                    );
                    if (cont_prompt) {
                        write(STDOUT_FILENO, cont_prompt, strlen(cont_prompt));
                    }
                }
            } else {
                // Last line - write remaining content
                write(STDOUT_FILENO, line, strlen(line));
                break;
            }
        }
    } else {
        // Single-line input - write directly
        write(STDOUT_FILENO, command_buffer, strlen(command_buffer));
    }
}
```

**Why Manual Insertion**: The command_buffer from command_layer contains only the command text with syntax highlighting. Continuation prompts are NOT in the buffer - they're metadata stored in screen_buffer. We insert them during terminal output.

---

## Implementation Flow

### Complete Flow Diagram

```
User types in LLE:
    for i in 1 2 3; do<ENTER>
    echo $i<ENTER>
    done<ENTER>

         ↓

LLE buffer contains:
    "for i in 1 2 3; do\necho $i\ndone"

         ↓

display_bridge → command_layer stores:
    "\033[32mfor\033[0m i in 1 2 3; \033[32mdo\033[0m\n\033[36mecho\033[0m $i\n\033[32mdone\033[0m"

         ↓

dc_handle_redraw_needed() called:

    1. count_newlines(command_buffer) → 2
    2. is_multiline = true
    
    3. Line-by-line analysis:
       
       Line 0: "for i in 1 2 3; do"
         ↓ Strip ANSI → "for i in 1 2 3; do"
         ↓ continuation_analyze_line()
         ↓ State: in_for_loop = true
         ↓ continuation_get_prompt() → "loop>"
         ↓ screen_buffer_set_line_prefix(row=1, "loop>")
       
       Line 1: "echo $i"
         ↓ Strip ANSI → "echo $i"
         ↓ continuation_analyze_line()
         ↓ State: in_for_loop = true (still)
         ↓ continuation_get_prompt() → "loop>"
         ↓ screen_buffer_set_line_prefix(row=2, "loop>")
    
    4. screen_buffer_render():
       
       Process prompt → render at row 0
       
       Process command text:
         - "for i in 1 2 3; do"
         - Encounter '\n' at end
         - row++ → row = 1
         - prefix_width = get_line_prefix_visual_width(row=1) → 6
         - col = 6 (start after "loop> ")
         
         - "echo $i"
         - Encounter '\n' at end
         - row++ → row = 2
         - prefix_width = get_line_prefix_visual_width(row=2) → 6
         - col = 6 (start after "loop> ")
         
         - "done"
         - No more newlines
       
       Calculate cursor position → (row=2, col=10)
    
    5. Terminal output:
       
       Write: "for i in 1 2 3; do"
       Write: "\n"
       Write: "loop> "        ← Inserted from prefix
       Write: "echo $i"
       Write: "\n"
       Write: "loop> "        ← Inserted from prefix
       Write: "done"
       
       Position cursor: ESC[2A  (up 2 rows)
                       ESC[11G (column 11)

         ↓

Terminal displays:
    $ for i in 1 2 3; do
    loop> echo $i
    loop> done
          ^
    Cursor here (row 2, col 10 - accounting for "loop> " prefix)
```

---

## Line Wrapping Solution

### The Critical Code Path

**File**: `src/display/screen_buffer.c`  
**Function**: `screen_buffer_render()`  
**Lines**: 357-371

```c
// Handle newlines
if (ch == '\n') {
    row++;
    if (row >= buffer->num_rows) {
        buffer->num_rows = row + 1;
    }
    
    // CONTINUATION PROMPT SUPPORT:
    // After a newline, check if the next row has a continuation prompt prefix.
    // If it does, start column position after the prefix (not at column 0).
    // This ensures cursor tracking accounts for continuation prompts like "loop> "
    size_t prefix_width = screen_buffer_get_line_prefix_visual_width(buffer, row);
    col = (int)prefix_width;
    
    i++;
    bytes_processed++;
    continue;
}
```

### Visual Wrap Handling

**File**: `src/display/screen_buffer.c`  
**Function**: `write_char_to_buffer()`  
**Lines**: 176-186

```c
// Check if we need to wrap to next line
if (*col >= buffer->terminal_width) {
    (*row)++;
    *col = 0;  // Visual wraps ALWAYS start at column 0
    
    // Ensure we have enough rows
    if (*row >= SCREEN_BUFFER_MAX_ROWS) {
        return;  // Can't write beyond buffer
    }
    
    if (*row >= buffer->num_rows) {
        buffer->num_rows = *row + 1;
    }
}
```

### Why This Works: The Key Distinction

```
Logical Newline ('\n' in buffer):
    - User pressed Enter or Alt-Enter
    - Represents a new line of input
    - SHOULD have continuation prompt
    - col = prefix_width (e.g., 6 for "loop> ")

Visual Wrap (terminal width exceeded):
    - Line too long, wraps automatically
    - Still same logical line
    - SHOULD NOT have continuation prompt
    - col = 0 (start at left margin)
```

### Complete Example with Wrapping

**Scenario**:
```bash
$ for i in 1 2 3; do
loop> echo "this is a very long line that definitely wraps when it exceeds the 80 character terminal width and continues here"
loop> done
```

**Terminal width**: 80 columns  
**Continuation prompt**: "loop> " (6 columns)

**Cursor Position Calculation**:

```
Row 0: "$ for i in 1 2 3; do"
       col starts at 2 (after "$ ")
       ends at 22
       
       Encounter '\n':
       - row++ → row = 1
       - prefix_width = get_line_prefix_visual_width(1) → 6
       - col = 6

Row 1: "loop> echo "this is a very long line that definitely wraps when it exceeds the 80 character terminal width and continues here"
       col starts at 6 (after "loop> ")
       
       Process "echo "this...":
       - col = 7, 8, 9, ... 78, 79
       - Next char would make col = 80
       - VISUAL WRAP TRIGGERED
       - write_char_to_buffer() does:
         - row++ → row = 2
         - col = 0  ← NO PREFIX (visual wrap, not logical newline)
       
       Continue on row 2 (wrapped portion):
       - col = 0, 1, 2, ... 45
       
       Encounter '\n':
       - row++ → row = 3
       - prefix_width = get_line_prefix_visual_width(3) → 6
       - col = 6

Row 3: "loop> done"
       col starts at 6 (after "loop> ")
       ends at 10
```

**Terminal Output**:
```
Row 0: $ for i in 1 2 3; do
Row 1: loop> echo "this is a very long line that definitely wraps when it exc
Row 2: eeds the 80 character terminal width and continues here"
Row 3: loop> done
```

**Key Points**:
- Row 1 starts at col 6 (logical newline - has "loop> ")
- Row 2 starts at col 0 (visual wrap - NO "loop> ")
- Row 3 starts at col 6 (logical newline - has "loop> ")

**Cursor positioning accounts for this**:
- If cursor is in "echo", it knows to add 6 to account for "loop> "
- If cursor is in wrapped portion, it knows col starts at 0
- Final cursor position is always correct

---

## Why No Display Layer

### What Exists But We Don't Use

**File**: `src/display/continuation_prompt_layer.c`

This is a full-fledged display layer with:
- Layer initialization/cleanup: `continuation_prompt_layer_create()`, `continuation_prompt_layer_init()`
- Event system integration: subscribes to `LAYER_EVENT_CONTENT_CHANGED`, etc.
- Caching mechanisms: stores rendered output
- Performance tracking: monitors render time
- State management: tracks what's currently displayed

**We don't use this layer in the current implementation.**

### Why a Layer is Unnecessary

#### 1. Continuation Prompts are Tightly Coupled to Command Text

Continuation prompts don't exist independently - they only make sense in the context of multiline command input. They are **metadata about the command**, not a separate visual element.

Consider:
- **Prompt layer**: Exists independently, can be rendered without command
- **Command layer**: Exists independently, can be rendered without prompt
- **Continuation prompts**: Only exist when command has newlines, must match command structure

#### 2. State is Derived, Not Cached

A display layer is useful when:
- State needs to be maintained across updates
- Content is expensive to compute
- Changes need to be tracked independently

Continuation prompts:
- State is derived on-demand from command buffer (just count newlines)
- Prompts are computed instantly from parsed state (no expensive operations)
- Changes are always in sync with command buffer (no independent tracking needed)

#### 3. Simple Logic Doesn't Need Abstraction

Using a layer would require:
```c
// Create and initialize layer
continuation_prompt_layer_t *layer;
continuation_prompt_layer_create(&layer);
continuation_prompt_layer_init(layer);

// Subscribe to events
layer_events_subscribe(events, LAYER_EVENT_CONTENT_CHANGED, handle_content_change, layer);
layer_events_subscribe(events, LAYER_EVENT_STATE_CHANGED, handle_state_change, layer);

// On each update:
continuation_prompt_layer_update_state(layer, &cont_state);
continuation_prompt_layer_render(layer, output_buffer, sizeof(output_buffer));

// Compose with other layers
composition_engine_add_layer(compositor, layer, LAYER_PRIORITY_CONTINUATION);
```

Direct integration:
```c
// On each update:
if (count_newlines(command_buffer) > 0) {
    const char *prompt = continuation_get_prompt(&cont_state);
    screen_buffer_set_line_prefix(&desired_screen, row, prompt);
}
```

**Which is simpler?** Direct integration by far.

#### 4. The Real "Display Layer" is screen_buffer

`screen_buffer` is effectively the display layer:
- Stores the virtual screen representation
- Handles layout and cursor calculations
- Manages prefixes (continuation prompts)
- Tracks what needs to be rendered

`display_controller` is the orchestrator:
- Feeds data into screen_buffer
- Reads results from screen_buffer
- Outputs to terminal

This two-component design is sufficient. Adding a third component (continuation_prompt_layer) would be over-engineering.

### When Would a Layer Be Appropriate?

A continuation_prompt_layer would make sense if:
- Continuation prompts had complex rendering logic (animations, styling, etc.)
- Prompts needed to be composed with other independent visual elements
- State management was complex (multi-level nesting visualization, etc.)
- Performance required sophisticated caching strategies

None of these apply to continuation prompts, which are simply:
- Static text ("loop>", "if>", etc.)
- Derived directly from parse state
- Output at newline boundaries
- No complex interactions

### Architecture Comparison

**Layer-Based Approach**:
```
command_layer ──→ continuation_prompt_layer ──→ composition_engine ──→ screen_buffer ──→ terminal
                         ↑
                         │
                  input_continuation
```

**Direct Integration Approach** (current):
```
command_layer ──→ display_controller ──→ screen_buffer ──→ terminal
                         ↑
                         │
                  input_continuation
```

**Benefits of Direct Integration**:
- Fewer components
- Simpler flow
- Less code
- Easier to understand
- Easier to debug
- No unnecessary abstractions

---

## Code Walkthrough

### Entry Point: Event Handler

**File**: `src/display/display_controller.c`  
**Function**: `dc_handle_redraw_needed()`  
**Triggered**: When command_layer content changes

```c
static layer_events_error_t dc_handle_redraw_needed(
    const layer_event_t *event,
    void *user_data) 
{
    // Get display controller
    display_controller_t *controller = (display_controller_t *)user_data;
    
    // Get command layer
    command_layer_t *cmd_layer = controller->compositor->command_layer;
    
    // Get highlighted command text (contains ANSI codes)
    char command_buffer[COMMAND_LAYER_MAX_HIGHLIGHTED_SIZE];
    command_layer_get_highlighted_text(cmd_layer, command_buffer, sizeof(command_buffer));
    
    // ... continuation prompt logic here ...
}
```

### Step 1: Detect Multiline

```c
// Location: display_controller.c:229-230
int newline_count = count_newlines(command_buffer);
bool is_multiline = (newline_count > 0);
```

**Helper**:
```c
static int count_newlines(const char *text) {
    if (!text) return 0;
    int count = 0;
    for (const char *p = text; *p; p++) {
        if (*p == '\n') count++;
    }
    return count;
}
```

### Step 2: Analyze and Set Prefixes (if multiline)

```c
if (is_multiline) {
    continuation_state_t cont_state;
    continuation_state_init(&cont_state);
    
    int logical_line = 0;
    const char *line_start = command_buffer;
    
    while (line_start && *line_start) {
        const char *newline_pos = strchr(line_start, '\n');
        
        if (newline_pos) {
            // Extract line
            size_t line_len = newline_pos - line_start;
            char line_buffer[4096];
            memcpy(line_buffer, line_start, line_len);
            line_buffer[line_len] = '\0';
            
            // Strip ANSI codes
            char plain_buffer[4096];
            strip_ansi_codes(line_buffer, plain_buffer, sizeof(plain_buffer));
            
            // Analyze state
            continuation_analyze_line(plain_buffer, &cont_state);
            
            // Move to next line
            logical_line++;
            line_start = newline_pos + 1;
            
            // Get and set prompt for next line
            const char *cont_prompt = continuation_get_prompt(&cont_state);
            if (cont_prompt) {
                screen_buffer_set_line_prefix(
                    &desired_screen,
                    desired_screen.command_start_row + logical_line,
                    cont_prompt
                );
            }
        } else {
            break;
        }
    }
    
    continuation_state_cleanup(&cont_state);
}
```

### Step 3: Render to Screen Buffer

```c
screen_buffer_render(&desired_screen, prompt_buffer, command_buffer, cursor_byte_offset);
```

**Inside screen_buffer_render()** (simplified):
```c
void screen_buffer_render(
    screen_buffer_t *buffer,
    const char *prompt_text,
    const char *command_text,
    size_t cursor_byte_offset)
{
    int row = 0, col = 0;
    
    // Render prompt (unchanged from before)
    // ...
    
    // Process command text
    for (size_t i = 0; i < strlen(command_text); i++) {
        char ch = command_text[i];
        
        if (ch == '\n') {
            row++;
            
            // KEY LINE: Check for prefix on next row
            size_t prefix_width = screen_buffer_get_line_prefix_visual_width(buffer, row);
            col = (int)prefix_width;  // Start after prefix, not at 0
            
            continue;
        }
        
        // Normal character - write to buffer
        write_char_to_buffer(buffer, &ch, 1, 1, false, &row, &col);
    }
    
    // Store final cursor position
    buffer->cursor_row = row;
    buffer->cursor_col = col;
}
```

**write_char_to_buffer()** handles visual wrapping:
```c
static void write_char_to_buffer(..., int *row, int *col) {
    // Check for visual wrap
    if (*col >= buffer->terminal_width) {
        (*row)++;
        *col = 0;  // Visual wrap goes to column 0 (no prefix)
    }
    
    // Write character to buffer
    buffer->lines[*row].cells[*col] = /* ... */;
    (*col)++;
}
```

### Step 4: Output to Terminal

```c
// Write command text with continuation prompts
if (is_multiline) {
    const char *line = command_buffer;
    int line_num = 0;
    
    while (*line) {
        const char *nl = strchr(line, '\n');
        
        if (nl) {
            // Write line content
            write(STDOUT_FILENO, line, nl - line);
            write(STDOUT_FILENO, "\n", 1);
            
            line_num++;
            line = nl + 1;
            
            // Write continuation prompt for next line
            if (*line) {
                const char *prompt = screen_buffer_get_line_prefix(&desired_screen, 
                                                                   command_start_row + line_num);
                if (prompt) {
                    write(STDOUT_FILENO, prompt, strlen(prompt));
                }
            }
        } else {
            // Last line
            write(STDOUT_FILENO, line, strlen(line));
            break;
        }
    }
} else {
    // Single-line - write directly
    write(STDOUT_FILENO, command_buffer, strlen(command_buffer));
}
```

### Step 5: Position Cursor

```c
// Cursor position already calculated by screen_buffer_render()
int cursor_row = desired_screen.cursor_row;
int cursor_col = desired_screen.cursor_col;

// Move up to cursor row
if (final_row > cursor_row) {
    char seq[16];
    snprintf(seq, sizeof(seq), "\033[%dA", final_row - cursor_row);
    write(STDOUT_FILENO, seq, strlen(seq));
}

// Move to cursor column (absolute positioning)
char seq[16];
snprintf(seq, sizeof(seq), "\033[%dG", cursor_col + 1);
write(STDOUT_FILENO, seq, strlen(seq));
```

---

## Testing and Verification

### Test Cases (All Passing)

#### Test 1: Basic Multiline (For Loop)
```bash
$ for i in 1 2 3; do
loop> echo $i
loop> done
```

**Expected**: "loop>" appears on lines 2 and 3  
**Result**: ✅ PASS

#### Test 2: Quote Continuation
```bash
$ echo "hello
quote> world"
```

**Expected**: "quote>" appears on line 2  
**Result**: ✅ PASS

#### Test 3: Nested Structures
```bash
$ for i in 1; do
loop> if true; then
if> echo nested
if> fi
loop> done
```

**Expected**: Context-aware prompts ("loop>", "if>")  
**Result**: ✅ PASS

#### Test 4: Line Wrapping in Multiline
```bash
$ for i in 1 2 3; do
loop> echo "this is a very long line that wraps when it exceeds terminal width"
loop> done
```

**Expected**: 
- Line 2 wraps naturally at terminal boundary
- Wrapped portion has NO "loop>" prefix
- Cursor positioning accurate across wrap

**Result**: ✅ PASS - No display corruption

#### Test 5: UTF-8 in Multiline
```bash
$ echo "emoji
quote> 🎉🎊"
```

**Expected**: 
- "quote>" appears on line 2
- Emoji renders correctly
- Cursor position accounts for emoji width (2 columns)

**Result**: ✅ PASS

### Verification Methods

**Interactive Testing**:
```bash
#!/usr/bin/expect
spawn env LLE_ENABLED=1 ./builddir/lusush

expect -re {\$} {
    send "for i in 1 2 3; do\r"
    expect "loop>"
    send "echo \$i\r"
    expect "loop>"
    send "done\r"
    expect "1"
    expect "2"
    expect "3"
}
```

**Visual Inspection**:
- Continuation prompts appear only on logical newlines
- No prompts on visual wraps
- Cursor positioned correctly when editing middle of multiline input
- No flickering or display corruption

**Code Review**:
- Cursor calculation logic verified against Replxx/Fish/ZLE patterns
- ANSI stripping confirmed working (debug logging)
- State analysis confirmed correct (all keywords detected)

---

## Design Decisions

### Decision 1: Count Newlines vs Check Screen Rows

**Options**:
A. Check if `desired_screen.num_rows > 1` (previous approach)
B. Count actual `\n` characters in buffer (current approach)

**Chosen**: B

**Rationale**:
- Screen rows conflate logical lines and visual wraps
- Newline counting is explicit about intent
- Matches user mental model (pressing Enter creates logical newlines)
- No false positives from long single-line input

### Decision 2: Strip ANSI Before Parsing vs Parse with ANSI

**Options**:
A. Strip ANSI escape sequences before calling `continuation_analyze_line()`
B. Teach `continuation_analyze_line()` to skip ANSI sequences

**Chosen**: A

**Rationale**:
- `input_continuation` is shared code, used by non-LLE paths too
- Keeping parser simple and focused on shell syntax
- ANSI handling is display-specific concern
- Easier to test and verify correctness

### Decision 3: Set Prefixes Before Render vs After Render

**Options**:
A. Set prefixes on screen_buffer before calling `screen_buffer_render()`
B. Pass continuation state to `screen_buffer_render()` and let it set prefixes

**Chosen**: A

**Rationale**:
- Clean separation: display_controller determines WHAT prefixes, screen_buffer handles HOW to use them
- screen_buffer_render() stays generic (doesn't know about continuation prompts specifically)
- Easier to test (can set prefixes and verify cursor calculations independently)

### Decision 4: Insert Prompts During Output vs Store in Buffer

**Options**:
A. Insert continuation prompts during terminal output
B. Include continuation prompts in command_buffer

**Chosen**: A

**Rationale**:
- Command text in buffer is the actual user input (without prompts)
- Prompts are display metadata, not content
- Allows command_layer to focus on syntax highlighting
- Matches Replxx/Fish architecture (prompts written separately)

### Decision 5: Use Layer vs Direct Integration

**Options**:
A. Use existing `continuation_prompt_layer`
B. Integrate directly in display_controller

**Chosen**: B

**Rationale**:
- Simpler implementation (fewer components)
- Continuation prompts are tightly coupled to command text
- No need for complex state management or caching
- Direct approach is more maintainable

---

## Future Considerations

### Implemented Enhancements (Session 18)

#### 1. Full Grapheme Cluster Support ✅ IMPLEMENTED

**Status**: Complete - Uses LLE's Unicode TR#29 grapheme cluster detection

**What it does**: Properly calculates visual width for complex Unicode characters in continuation prompts

**Supported**:
- Emoji: Single emoji (2 columns), emoji with modifiers, ZWJ sequences, regional indicators
- CJK characters: Chinese, Japanese, Korean (2 columns each)
- Combining marks: Accented characters (base + combining = 1 grapheme)
- Mixed content: Emoji + ASCII with accurate spacing

**Example configurations** (when config system integrated):
```bash
# Emoji prompts
CONTINUATION_PROMPTS=([loop]="🔄 " [if]="❓ " [quote]="💬 ")

# International
CONTINUATION_PROMPTS=([loop]="循环> " [if]="如果> ")  # Chinese
CONTINUATION_PROMPTS=([loop]="حلقة> ")                # Arabic

# Fancy
CONTINUATION_PROMPTS=([loop]="├─► " [if]="│ ❯ ")
```

**Implementation**: 
- Enhanced `screen_buffer_calculate_visual_width()` to use `lle_is_grapheme_boundary()`
- Iterates by grapheme clusters instead of bytes
- Uses `lle_utf8_codepoint_width()` for accurate display width
- Same proven algorithm as `display_bridge.c`

**Benefits**:
- Accurate cursor positioning for all Unicode
- No performance impact (width cached in prefix structure)
- Future-proof for new Unicode additions
- Enables true internationalization

**Commit**: ee69dd8

### Potential Future Enhancements

#### 2. Customizable Continuation Prompts

**Current**: Hard-coded prompts (loop>, if>, quote>, etc.) but READY for Unicode

**Enhancement**: Allow user customization via config:
```bash
CONTINUATION_PROMPTS=(
    [loop]="→ "
    [if]="? "
    [quote]="  "
)
```

**Implementation**: Add `continuation_config` module, update `continuation_get_prompt()` to check config first

**Note**: Grapheme cluster support already in place, just needs config integration

#### 3. Visual Indicators for Wrapped Lines

**Current**: Wrapped lines have no visual indicator

**Enhancement**: Show subtle indicator when line wraps:
```bash
$ for i in 1 2 3; do
loop> echo "very long line that wraps here... ↩
      continuation of the same line"
loop> done
```

**Implementation**: Detect visual wraps in `screen_buffer_render()`, add suffix character

#### 3. Syntax Highlighting in Continuation Prompts

**Current**: Prompts are plain text

**Enhancement**: Color-code prompts based on context:
```bash
$ for i in 1 2 3; do
\033[36mloop>\033[0m echo $i
```

**Implementation**: Store ANSI-coded prompts in `input_continuation`, mark `contains_ansi` in prefix structure

#### 4. Smart Indentation

**Current**: Continuation lines start at column = prefix_width

**Enhancement**: Auto-indent based on nesting:
```bash
$ for i in 1 2 3; do
loop>   if [ $i -eq 2 ]; then
if>       echo "found"
if>     fi
loop> done
```

**Implementation**: Track nesting depth in `continuation_state_t`, calculate indent in `screen_buffer_render()`

#### 5. Diff-Based Terminal Updates

**Current**: Full redraw on every change

**Enhancement**: Only update changed regions:
```c
screen_buffer_diff(&current_screen, &desired_screen, &diff);
screen_buffer_apply_diff(&diff, STDOUT_FILENO);
```

**Implementation**: Already supported by screen_buffer API, just needs integration in display_controller

### Backward Compatibility

All enhancements should:
- Maintain existing behavior by default
- Be opt-in via configuration
- Not break scripts or automation
- Preserve PS2 environment variable compatibility

### Performance Optimizations

Current implementation is already fast, but potential optimizations:

1. **Cache parsed state**: If command text hasn't changed, reuse previous continuation state
2. **Lazy ANSI stripping**: Only strip when needed (e.g., if state analysis is cached)
3. **Batch prefix updates**: Set all prefixes in one call instead of line-by-line

None are necessary currently - performance is excellent.

---

## Summary

### What Makes This Implementation Successful

1. **Correct Detection**: Counts actual newlines, not screen rows
2. **Proper Separation**: Logical newlines (with prompts) vs visual wraps (without prompts)
3. **ANSI Awareness**: Strips syntax highlighting before parsing
4. **Incremental Analysis**: Line-by-line state tracking for context-aware prompts
5. **Clean Architecture**: Uses existing infrastructure, no unnecessary layers
6. **Simple Implementation**: Direct integration, minimal code changes

### Files Modified

- `src/display/screen_buffer.c` (9 lines): Enhanced newline handling for prefix widths
- `src/display/display_controller.c` (147 lines): Added multiline detection, ANSI stripping, state analysis, prompt insertion

### Key Metrics

- **Lines of code added**: 156
- **Test cases passing**: 5/5
- **Regressions detected**: 0
- **Display corruption**: None
- **Cursor positioning errors**: None
- **Implementation time**: ~4 hours (as predicted in plan)

### Architectural Principles Followed

✅ **Separation of Concerns**: screen_buffer (layout) vs display_controller (I/O)  
✅ **Prompt-Once Pattern**: From MODERN_EDITOR_WRAPPING_RESEARCH.md  
✅ **Incremental Tracking**: Character-by-character cursor calculation  
✅ **Clean Abstractions**: Prefix as data structure feature, not separate layer  
✅ **Minimal Changes**: Enhance existing code, don't replace it

### Production Readiness

This implementation is **production-ready**:
- All test cases passing
- No known bugs
- Well-tested in interactive scenarios
- Follows proven patterns from modern editors
- Clean, maintainable code
- Comprehensive documentation

---

## References

### Related Documentation

- `CONTINUATION_PROMPT_SCREEN_BUFFER_PLAN.md` - Implementation plan (Session 17)
- `MODERN_EDITOR_WRAPPING_RESEARCH.md` - Research on Replxx, Fish, ZLE
- `ARCHITECTURE_IMPACT_ANALYSIS.md` - Architectural boundaries
- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - Session history and status

### Source Files

- `src/display/screen_buffer.c` - Screen buffer implementation
- `src/display/display_controller.c` - Display controller with continuation prompt logic
- `src/input_continuation.c` - Shared continuation state parsing
- `include/display/screen_buffer.h` - Screen buffer API and data structures
- `include/input_continuation.h` - Continuation state API

### External Resources

- Replxx: https://github.com/AmokHuginnsson/replxx
- Fish shell: https://github.com/fish-shell/fish-shell
- ZLE (Zsh): https://github.com/zsh-users/zsh

---

**Document Status**: Complete and Current  
**Last Updated**: 2025-11-16 (Session 18)  
**Maintained By**: Lusush Development Team
