# LLE Continuation Prompts: Display Controller Layer Architecture

**Document**: lle_continuation_prompt_display_layer_architecture.md  
**Date**: 2025-11-08  
**Status**: Architectural Brainstorming  
**Purpose**: Explore display controller layer-based solutions for continuation prompts

---

## Context

### Current Display Controller Architecture

The display controller uses a **layered composition model**:

```
┌──────────────────────────────────────────────┐
│ Display Controller                           │
│  ┌────────────────────────────────────────┐ │
│  │ Compositor (composition_engine)        │ │
│  │  ├─ prompt_layer                       │ │
│  │  ├─ command_layer                      │ │
│  │  ├─ suggestion_layer                   │ │
│  │  └─ status_line_layer                  │ │
│  └────────────────────────────────────────┘ │
│           ↓                                   │
│  ┌────────────────────────────────────────┐ │
│  │ Screen Buffer (critical breakthrough)  │ │
│  │  - Double buffering                    │ │
│  │  - Accurate dirty region tracking      │ │
│  │  - Minimal terminal updates            │ │
│  └────────────────────────────────────────┘ │
│           ↓                                   │
│  Terminal Output (ANSI escape sequences)     │
└──────────────────────────────────────────────┘
```

**Key Innovation**: Screen buffer made accurate display rendering viable by:
- Tracking what's currently displayed
- Computing minimal diffs
- Only updating changed regions
- Preventing flicker and render artifacts

### The Continuation Prompt Challenge

Need to display:
```
$ for i in 1 2 3; do     ← prompt_layer: "$"
> echo $i                ← continuation prompt: ">"
> done                   ← continuation prompt: ">"
```

Where continuation prompts appear on lines **within the command_layer's multiline content**.

---

## Architectural Approach 1: Dedicated Continuation Prompt Layer

### Concept: New Layer in Compositor

Add `continuation_prompt_layer` to compositor alongside existing layers.

```
┌────────────────────────────────────────────┐
│ Compositor                                 │
│  ├─ prompt_layer          (line 0)        │
│  ├─ command_layer         (lines 0-N)     │
│  ├─ continuation_prompt_layer  ← NEW      │
│  ├─ suggestion_layer                       │
│  └─ status_line_layer                      │
└────────────────────────────────────────────┘
```

### How It Works

**1. Layer Responsibilities**:
```c
// prompt_layer: Primary prompt (PS1)
"$ "  at row 0, col 0

// command_layer: Buffer content with embedded newlines
"for i in 1 2 3; do\necho $i\ndone"

// continuation_prompt_layer: Injected prompts at specific positions
"> " at row 1, col 0  (after first \n)
"> " at row 2, col 0  (after second \n)
```

**2. Composition Strategy**:
```c
composition_engine_compose_with_continuation() {
    // Step 1: Render prompt_layer
    output[row=0] = prompt_layer_content + " ";
    
    // Step 2: Split command_layer by newlines
    lines[] = split(command_layer_content, '\n');
    
    // Step 3: First line goes on same row as prompt
    output[row=0] += lines[0];
    
    // Step 4: Subsequent lines get continuation prompts
    for (i = 1; i < line_count; i++) {
        // Get continuation prompt from continuation_prompt_layer
        cont_prompt = continuation_prompt_layer_get_prompt_for_line(i);
        output[row=i] = cont_prompt + lines[i];
    }
}
```

**3. Continuation Prompt Layer API**:
```c
typedef struct continuation_prompt_layer_t {
    char default_prompt[32];           // "> " or custom PS2
    bool context_aware;                 // true = loop>, if>, etc.
    continuation_state_t *parser_state; // For context-aware prompts
} continuation_prompt_layer_t;

// Get prompt for specific line number
const char* continuation_prompt_layer_get_prompt_for_line(
    continuation_prompt_layer_t *layer,
    size_t line_number,
    const char *command_content  // For context analysis
);

// Update when command changes
void continuation_prompt_layer_update_state(
    continuation_prompt_layer_t *layer,
    const char *current_command
);
```

### Advantages
- ✅ **Clean separation of concerns**: Each layer has single responsibility
- ✅ **Fits existing architecture**: Natural extension of layer model
- ✅ **Context-aware support**: Layer can analyze command content for smart prompts
- ✅ **Configurable**: Easy to toggle between simple/context-aware prompts
- ✅ **Testable**: Layer can be unit tested independently

### Disadvantages
- ⚠️ **Cursor position complexity**: Still need translation (continuation prompts inject text)
- ⚠️ **Composition coordination**: Compositor needs to coordinate 3 layers (prompt, command, continuation)
- ⚠️ **Line-aware composition**: Compositor must understand line boundaries

### Implementation Complexity
- **New layer module**: Medium (similar to existing layers)
- **Compositor changes**: Medium (multi-layer coordination)
- **Cursor translation**: Still needed (prompts insert display characters)
- **Estimated effort**: 2-3 days

---

## Architectural Approach 2: Screen Buffer Native Support

### Concept: Screen Buffer Understands Line Prefixes

Enhance screen buffer to natively handle per-line prefixes (not just full-screen buffers).

```c
typedef struct screen_buffer_line_t {
    char *prefix;        // "> " continuation prompt
    char *content;       // "echo $i"
    size_t prefix_len;
    size_t content_len;
    bool prefix_dirty;   // Track if prefix changed
    bool content_dirty;  // Track if content changed
} screen_buffer_line_t;

typedef struct screen_buffer_t {
    screen_buffer_line_t *lines;  // Array of lines with prefixes
    size_t line_count;
    // ... existing fields ...
} screen_buffer_t;
```

### How It Works

**1. Separate Prefix and Content**:
```c
// Line 0
lines[0].prefix = "$ ";
lines[0].content = "for i in 1 2 3; do";

// Line 1
lines[1].prefix = "> ";
lines[1].content = "echo $i";

// Line 2
lines[2].prefix = "> ";
lines[2].content = "done";
```

**2. Rendering with Prefixes**:
```c
screen_buffer_render_line(screen_buffer_t *sb, size_t line_num) {
    line = &sb->lines[line_num];
    
    // Cursor position calculation
    if (cursor_in_prefix) {
        // Don't allow cursor in prefix (prompts are read-only)
        cursor_col = line->prefix_len;
    } else {
        cursor_col = line->prefix_len + cursor_offset_in_content;
    }
    
    // Render prefix (if changed)
    if (line->prefix_dirty) {
        move_cursor(line_num, 0);
        write_terminal(line->prefix);
    }
    
    // Render content (if changed)
    if (line->content_dirty) {
        move_cursor(line_num, line->prefix_len);
        write_terminal(line->content);
    }
}
```

**3. Dirty Tracking Per Component**:
- Prefix dirty: Only redraw prefix region
- Content dirty: Only redraw content region
- Neither dirty: Skip line entirely (screen buffer optimization!)

### Advantages
- ✅ **Leverages screen buffer power**: Uses existing diff/dirty tracking
- ✅ **Efficient updates**: Can update prefix without touching content
- ✅ **Natural cursor handling**: Screen buffer already tracks cursor positions
- ✅ **Line-oriented**: Matches terminal's line-based model
- ✅ **Minimal composition changes**: Compositor just sets prefixes

### Disadvantages
- ⚠️ **Screen buffer complexity increase**: More complex data structure
- ⚠️ **Breaks abstraction slightly**: Screen buffer becomes aware of "prefixes" concept
- ⚠️ **Migration effort**: Need to update screen buffer API consumers

### Implementation Complexity
- **Screen buffer changes**: Medium-High (core data structure change)
- **Compositor adaptation**: Low (just set prefixes)
- **Cursor management**: Low (screen buffer handles it)
- **Estimated effort**: 3-4 days (most in screen buffer refactor)

---

## Architectural Approach 3: Hybrid - Continuation Layer + Screen Buffer Enhancement

### Concept: Combine Both Approaches

- **Continuation layer**: Manages prompt logic (simple vs context-aware)
- **Screen buffer**: Native prefix support for efficient rendering
- **Composition engine**: Coordinates between layers

```
┌────────────────────────────────────────────────────┐
│ Continuation Prompt Layer                         │
│  - Analyzes command content                       │
│  - Determines prompt for each line                │
│  - Provides prompt strings                        │
└────────────────┬───────────────────────────────────┘
                 │ get_prompts_for_lines()
                 ↓
┌────────────────────────────────────────────────────┐
│ Composition Engine                                 │
│  - Splits command into lines                      │
│  - Requests prompts from continuation layer       │
│  - Passes to screen buffer as prefixes            │
└────────────────┬───────────────────────────────────┘
                 │ set_line_prefixes()
                 ↓
┌────────────────────────────────────────────────────┐
│ Screen Buffer                                      │
│  - Stores lines with prefixes                     │
│  - Tracks prefix/content dirty separately         │
│  - Renders efficiently with cursor handling       │
└────────────────────────────────────────────────────┘
```

### How It Works

**1. Continuation Layer API**:
```c
// Get prompts for all lines in multiline command
char** continuation_layer_get_prompts(
    continuation_layer_t *layer,
    const char *command_content,  // Full buffer
    size_t *line_count            // Output: number of lines
) {
    // Split by newlines
    char **lines = split_lines(command_content, line_count);
    
    // Allocate prompt array
    char **prompts = malloc(*line_count * sizeof(char*));
    
    // First line: primary prompt (from prompt_layer)
    prompts[0] = NULL;  // Special: use prompt_layer
    
    // Continuation lines
    for (size_t i = 1; i < *line_count; i++) {
        if (layer->context_aware) {
            // Analyze accumulated content up to this line
            char *accumulated = join_lines(lines, i);
            prompts[i] = get_context_prompt(layer->parser, accumulated);
            free(accumulated);
        } else {
            // Simple fixed prompt
            prompts[i] = strdup(layer->default_prompt);
        }
    }
    
    return prompts;
}
```

**2. Composition Engine Integration**:
```c
composition_engine_compose_multiline() {
    // Get command content from command_layer
    const char *command = command_layer_get_content(compositor->command_layer);
    
    // Get prompts from continuation_layer
    size_t line_count;
    char **prompts = continuation_layer_get_prompts(
        compositor->continuation_layer,
        command,
        &line_count
    );
    
    // Split command into lines
    char **lines = split_lines(command, &line_count);
    
    // Build screen buffer with prefixes
    screen_buffer_begin_update(compositor->screen_buffer);
    
    for (size_t i = 0; i < line_count; i++) {
        const char *prefix = (i == 0) 
            ? prompt_layer_get_content(compositor->prompt_layer)
            : prompts[i];
            
        screen_buffer_set_line_with_prefix(
            compositor->screen_buffer,
            i,              // line number
            prefix,         // prefix (prompt)
            lines[i]        // content
        );
    }
    
    screen_buffer_end_update(compositor->screen_buffer);
}
```

**3. Screen Buffer API Enhancement**:
```c
// Set line with separate prefix and content
void screen_buffer_set_line_with_prefix(
    screen_buffer_t *sb,
    size_t line_num,
    const char *prefix,
    const char *content
) {
    screen_buffer_line_t *line = &sb->lines[line_num];
    
    // Check if prefix changed
    if (!line->prefix || strcmp(line->prefix, prefix) != 0) {
        free(line->prefix);
        line->prefix = strdup(prefix);
        line->prefix_len = strlen(prefix);
        line->prefix_dirty = true;
    }
    
    // Check if content changed
    if (!line->content || strcmp(line->content, content) != 0) {
        free(line->content);
        line->content = strdup(content);
        line->content_len = strlen(content);
        line->content_dirty = true;
    }
}

// Cursor position accounting for prefix
size_t screen_buffer_get_display_cursor_column(
    screen_buffer_t *sb,
    size_t line_num,
    size_t cursor_in_content
) {
    return sb->lines[line_num].prefix_len + cursor_in_content;
}
```

### Advantages
- ✅ **Best of both worlds**: Clean layer separation + efficient rendering
- ✅ **Each component has clear purpose**:
  - Continuation layer: Prompt logic
  - Composition: Coordination
  - Screen buffer: Efficient display
- ✅ **Natural cursor handling**: Screen buffer knows about prefixes
- ✅ **Extensible**: Easy to add features (custom prompts, colors, etc.)
- ✅ **Testable**: Each component independently testable

### Disadvantages
- ⚠️ **Most complex solution**: Changes to three components
- ⚠️ **Coordination overhead**: More moving parts
- ⚠️ **Learning curve**: Developers need to understand full pipeline

### Implementation Complexity
- **Continuation layer**: Medium (new module, 1 day)
- **Screen buffer enhancement**: Medium-High (core changes, 2 days)
- **Composition coordination**: Medium (1 day)
- **Testing & integration**: High (1-2 days)
- **Estimated effort**: 5-7 days total

---

## Architectural Approach 4: Virtual Text Overlay System (Future-Proof)

### Concept: Generic Overlay System

Build a **general-purpose virtual text system** that can overlay any content at any position without modifying buffer.

```
┌────────────────────────────────────────────────────┐
│ Virtual Text Overlay System                       │
│  ┌──────────────────────────────────────────────┐ │
│  │ Overlay Regions                              │ │
│  │  - Line number gutter                        │ │
│  │  - Inline hints (type annotations)           │ │
│  │  - Continuation prompts  ← our use case      │ │
│  │  - Git blame info                            │ │
│  │  - Diagnostic messages                       │ │
│  └──────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────┘
```

### How It Works

**1. Overlay Region Definition**:
```c
typedef enum {
    OVERLAY_POSITION_LINE_START,    // Before line content (continuation prompts)
    OVERLAY_POSITION_LINE_END,      // After line content
    OVERLAY_POSITION_INLINE,        // Within line at specific offset
    OVERLAY_POSITION_GUTTER         // Separate gutter column
} overlay_position_type_t;

typedef struct overlay_region_t {
    overlay_position_type_t type;
    size_t line_number;             // Which line
    size_t offset;                  // Position within line (if INLINE)
    char *text;                     // Text to display
    bool read_only;                 // Cannot place cursor here
    // Styling
    uint32_t fg_color;
    uint32_t bg_color;
    bool bold, italic, underline;
} overlay_region_t;

typedef struct virtual_text_system_t {
    overlay_region_t *regions;
    size_t region_count;
    size_t capacity;
} virtual_text_system_t;
```

**2. For Continuation Prompts**:
```c
// Register continuation prompts as overlays
void setup_continuation_prompts(virtual_text_system_t *vts, 
                                const char *command_buffer) {
    // Clear existing continuation overlays
    virtual_text_clear_overlays_of_type(vts, "continuation_prompt");
    
    // Find newlines in buffer
    size_t line_num = 1;
    for (size_t i = 0; i < strlen(command_buffer); i++) {
        if (command_buffer[i] == '\n') {
            // Add overlay for next line
            overlay_region_t overlay = {
                .type = OVERLAY_POSITION_LINE_START,
                .line_number = line_num,
                .offset = 0,
                .text = strdup("> "),
                .read_only = true,
                .fg_color = 0x808080  // Gray
            };
            virtual_text_add_overlay(vts, overlay);
            line_num++;
        }
    }
}
```

**3. Rendering with Overlays**:
```c
screen_buffer_render_with_overlays(screen_buffer_t *sb, 
                                   virtual_text_system_t *vts) {
    for (size_t line_num = 0; line_num < sb->line_count; line_num++) {
        size_t col = 0;
        
        // Render overlays at LINE_START
        overlay_region_t *overlays = virtual_text_get_overlays_for_line(
            vts, line_num, OVERLAY_POSITION_LINE_START
        );
        for (size_t i = 0; i < overlay_count; i++) {
            render_text(line_num, col, overlays[i].text, overlays[i].style);
            col += strlen(overlays[i].text);
        }
        
        // Render actual line content
        render_text(line_num, col, sb->lines[line_num].content, normal_style);
        col += sb->lines[line_num].length;
        
        // Render overlays at LINE_END
        // ... similar ...
    }
}
```

**4. Cursor Position Handling**:
```c
size_t translate_cursor_position(virtual_text_system_t *vts,
                                 size_t line_num,
                                 size_t buffer_offset) {
    size_t display_offset = buffer_offset;
    
    // Add length of all overlays before cursor
    overlay_region_t *overlays = virtual_text_get_overlays_before(
        vts, line_num, buffer_offset
    );
    
    for (size_t i = 0; i < overlay_count; i++) {
        if (overlays[i].read_only) {
            display_offset += strlen(overlays[i].text);
        }
    }
    
    return display_offset;
}
```

### Advantages
- ✅ **Maximum flexibility**: Solves continuation prompts AND future features
- ✅ **Zero buffer pollution**: Overlays never touch buffer content
- ✅ **Rich styling**: Colors, fonts, etc. per overlay
- ✅ **Modern editor pattern**: Used by VSCode, Neovim, etc.
- ✅ **Extensible**: Easy to add new overlay types

### Disadvantages
- ⚠️ **Highest complexity**: Full-featured system
- ⚠️ **Over-engineering for immediate need**: Just need continuation prompts
- ⚠️ **Performance overhead**: Managing overlay regions
- ⚠️ **Long implementation time**: Full system design needed

### Implementation Complexity
- **Virtual text system**: High (new subsystem, 3-4 days)
- **Screen buffer integration**: Medium (2 days)
- **Cursor management**: Medium (1 day)
- **Testing**: High (2-3 days)
- **Estimated effort**: 8-12 days

### Future Value
- Line numbers in LLE editor mode
- Inline type hints
- Git blame information
- Diagnostic messages (syntax errors)
- Autocomplete suggestions inline
- **This is how modern editors work** (VSCode, Neovim LSP)

---

## Screen Buffer Architectural Considerations

### Current Screen Buffer Design

```c
typedef struct screen_buffer_t {
    char **lines;              // Array of line contents
    size_t line_count;
    size_t *line_lengths;
    bool *dirty_lines;         // Which lines need redraw
    
    // Double buffering
    screen_buffer_cell_t *current_buffer;
    screen_buffer_cell_t *previous_buffer;
    
    // Cursor tracking
    size_t cursor_row;
    size_t cursor_col;
} screen_buffer_t;
```

### Enhancement Option A: Line Prefix Support (Approach 2 & 3)

**Add prefix tracking to screen buffer**:
```c
typedef struct screen_buffer_line_t {
    char *prefix;              // Read-only prompt/prefix
    char *content;             // Editable content
    size_t prefix_len;
    size_t content_len;
    bool prefix_dirty;
    bool content_dirty;
    
    // Styling (optional)
    ansi_style_t prefix_style;
    ansi_style_t content_style;
} screen_buffer_line_t;

typedef struct screen_buffer_t {
    screen_buffer_line_t *lines;  // ← Changed from char **
    // ... rest same ...
} screen_buffer_t;
```

**Benefits**:
- Prefix separate from content (clean separation)
- Independent dirty tracking (efficient updates)
- Natural cursor handling (prefix_len + content_offset)

**Challenges**:
- API change: All consumers need updates
- Migration: Update existing code using old API
- Testing: Verify all display paths still work

### Enhancement Option B: Cell-Level Metadata (Approach 4)

**Add metadata to each screen cell**:
```c
typedef struct screen_buffer_cell_t {
    char ch;                   // Character
    bool read_only;            // Cannot place cursor here
    bool is_overlay;           // Part of virtual text
    uint32_t fg_color;
    uint32_t bg_color;
    // ... styling ...
} screen_buffer_cell_t;
```

**Benefits**:
- Fine-grained control (per-character metadata)
- Supports any overlay pattern
- Screen buffer becomes "smart canvas"

**Challenges**:
- Memory overhead (metadata per cell)
- Complexity increase
- May be over-engineering

### Enhancement Option C: Layer Composition (No Screen Buffer Changes)

**Keep screen buffer simple, handle in composition**:
- Screen buffer stays as char **lines
- Composition engine builds final output with prompts
- Screen buffer just renders what it receives

**Benefits**:
- Minimal changes to proven screen buffer
- Composition engine owns the complexity
- Screen buffer remains simple rendering layer

**Challenges**:
- Composition engine more complex
- Cursor translation done outside screen buffer
- Less efficient (can't track prefix vs content dirty separately)

---

## Comparison Matrix

| Approach | Clean Architecture | Complexity | Future-Proof | Effort | Performance |
|----------|-------------------|------------|--------------|--------|-------------|
| **1. Continuation Layer** | ⭐⭐⭐⭐ | Medium | ⭐⭐⭐ | 2-3 days | ⭐⭐⭐⭐ |
| **2. Screen Buffer Prefix** | ⭐⭐⭐ | Medium-High | ⭐⭐⭐ | 3-4 days | ⭐⭐⭐⭐⭐ |
| **3. Hybrid (1 + 2)** | ⭐⭐⭐⭐⭐ | High | ⭐⭐⭐⭐ | 5-7 days | ⭐⭐⭐⭐⭐ |
| **4. Virtual Text System** | ⭐⭐⭐⭐⭐ | Very High | ⭐⭐⭐⭐⭐ | 8-12 days | ⭐⭐⭐⭐ |
| **Composition Translation** | ⭐⭐⭐ | Medium | ⭐⭐ | 4-5 hours | ⭐⭐⭐ |

---

## Recommendations

### For Immediate Need (Next Week)
**Composition Translation Approach** (from first document)
- Fastest implementation
- Provides user value quickly
- Can be replaced later if needed
- Use as prototype/proof of concept

### For Solid Long-Term Solution (Next Month)
**Approach 3: Hybrid (Continuation Layer + Screen Buffer Enhancement)**
- Clean architecture with proper separation
- Leverages screen buffer power
- Natural extension of existing layer model
- Balances complexity vs capability

### For Maximum Future-Proofing (Next Quarter)
**Approach 4: Virtual Text Overlay System**
- Solves continuation prompts + many future features
- Modern editor pattern
- Highly extensible
- Requires significant design and implementation time

---

## Open Architecture Questions

### 1. Screen Buffer API Stability
**Question**: How stable does the screen buffer API need to be?
- If consumers are limited (just compositor), refactoring is easier
- If widely used, API changes require careful migration

### 2. Performance Requirements
**Question**: What's acceptable rendering performance?
- Overlay system adds per-frame overhead
- Screen buffer prefix approach is most efficient
- Simple translation is fast but less optimal

### 3. Feature Roadmap
**Question**: What other display features are planned?
- If many overlays needed (line numbers, hints, etc.) → Virtual text system
- If just continuation prompts → Simpler approach sufficient

### 4. Cursor Interaction Model
**Question**: Should users interact with overlaid content?
- Read-only overlays (our case) → Simpler cursor handling
- Interactive overlays → Need full position mapping

### 5. Multi-Editor Support
**Question**: Will LLE support multiple simultaneous editors?
- Single global editor → Global screen buffer okay
- Multiple editors → Need per-editor screen buffers

---

## Next Steps for Architecture Decision

### Phase 1: Research & Validation (Current)
- ✅ Document architectural approaches (this document)
- ⏳ Examine screen buffer implementation details
- ⏳ Prototype cursor translation in isolation
- ⏳ Review screen buffer change impact
- ⏳ Consult with user on preferences

### Phase 2: Prototype (if pursuing layer approach)
- Spike: Screen buffer with prefix support (1 day)
- Spike: Continuation layer interface (1 day)
- Validate cursor handling works
- Measure performance impact

### Phase 3: Implementation Decision
- Choose approach based on:
  - User requirements (immediate vs long-term)
  - Available development time
  - Future feature plans
  - Architectural preferences

---

## Conclusion

Multiple viable architectural approaches exist:

1. **Quick Win**: Translation in composition (half day)
2. **Solid Architecture**: Continuation layer + Screen buffer (1 week)
3. **Future-Proof**: Virtual text system (2-3 weeks)

All approaches are properly engineered - the choice depends on timeline and future vision for LLE.

The display controller's layered architecture and screen buffer innovation provide excellent foundations for any of these approaches.

---

**Document Status**: Architectural exploration complete  
**Last Updated**: 2025-11-08  
**Authors**: AI Assistant + User collaborative design
