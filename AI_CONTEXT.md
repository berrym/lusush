# AI Context: Lusush Line Editor (LLE) Development

## 🔥 INSTANT CONTEXT FOR AI ASSISTANTS

**Project**: Replacing linenoise with custom Lusush Line Editor (LLE)  
**Language**: C99  
**Build**: Meson (NOT Make)  
**Branch**: `feature/lusush-line-editor`  
**Status**: Active development, 50 atomic tasks, Phase 1 of 4  

## 📋 IMMEDIATE NEXT STEPS
1. Check `LLE_PROGRESS.md` for current TODO task
2. Read task spec in `LLE_DEVELOPMENT_TASKS.md` 
3. Implement following `.cursorrules` patterns
4. Write tests, run `scripts/lle_build.sh test`
5. Commit with format: `LLE-XXX: Task description`

## 🚀 ESSENTIAL COMMANDS
```bash
# Build & Test
scripts/lle_build.sh setup    # First time
scripts/lle_build.sh build    # Compile
scripts/lle_build.sh test     # Run tests

# Development
git checkout -b task/lle-XXX-desc  # New task
scripts/complete_task.sh XXX       # Finish task
scripts/update_progress.sh         # Check status
```

## 💻 CODE STANDARDS (CRITICAL)
```c
// Naming: lle_component_action
bool lle_text_insert_char(lle_text_buffer_t *buffer, char c);

// Structures: lle_component_t  
typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
} lle_text_buffer_t;

// Error handling: always return bool
bool lle_function(args) {
    if (!args) return false;
    // implementation
    return true;
}

// Tests: LLE_TEST macro
LLE_TEST(function_name) {
    // setup, test, assert, cleanup
}
```

## 🎯 ARCHITECTURE OVERVIEW
- **Text Engine**: `text_buffer.c/h` - UTF-8 text manipulation
- **Cursor Math**: `cursor_math.c/h` - Mathematical position calculations  
- **Terminal**: `terminal_manager.c/h` - Terminal interface
- **Display**: `display.c/h` - Rendering system
- **Theme**: `theme_integration.c/h` - Lusush theme support
- **History**: `history.c/h` - Command history
- **Completion**: `completion.c/h` - Tab completion
- **API**: `line_editor.c/h` - Main public interface

## 🧪 TESTING REQUIREMENTS
- Unit tests for every function
- Edge cases and error conditions
- Performance < 1ms for core operations
- No memory leaks (Valgrind clean)
- Integration with existing Lusush components

## 📐 PERFORMANCE TARGETS
- Character insertion: < 1ms
- Cursor movement: < 1ms  
- Display update: < 5ms
- Memory: < 1MB base, < 50 bytes per char
- Support: 100KB text, 10K history, 500 char width

## 🔧 CRITICAL GOTCHAS
1. **UTF-8**: Byte vs character positions are different
2. **Cursor Math**: Must be mathematically provable
3. **Memory**: Every malloc needs corresponding free
4. **Terminal**: ANSI escape sequences affect width calculations
5. **Build**: Use Meson, not Make
6. **Thread Safety**: Single-threaded design
7. **Error Handling**: Never ignore return values

## 📁 KEY FILES TO READ
- `LLE_PROGRESS.md` - What to work on next
- `LLE_DEVELOPMENT_TASKS.md` - All 50 tasks with specs
- `.cursorrules` - Complete coding standards
- `LLE_AI_DEVELOPMENT_GUIDE.md` - Detailed development guide
- `LINE_EDITOR_STRATEGIC_ANALYSIS.md` - Why we're building this
- `LLE_TERMCAP_INTEGRATION_PLAN.md` - How we'll integrate termcap

## 🎯 SUCCESS CRITERIA
- Perfect multiline prompts across all terminals
- Sub-millisecond response times
- Zero mathematical errors in cursor positioning  
- 100% theme integration
- Extensible architecture

## 🆘 QUICK DEBUG
```bash
# Memory leaks
valgrind --leak-check=full builddir/lusush

# Performance  
scripts/lle_build.sh benchmark

# Debug output
export LLE_DEBUG=1
export LLE_DEBUG_CURSOR=1
gdb builddir/lusush
```

## 🔄 DEVELOPMENT PHASES
1. **Phase 1**: Foundation (text buffer, cursor math, terminal)
2. **Phase 2**: Core (prompts, themes, basic editing)  
3. **Phase 3**: Advanced (Unicode, completion, undo/redo)
4. **Phase 4**: Integration (API, optimization, docs)

## 📦 BUILD INTEGRATION
- LLE builds as static library `liblle.a`
- Links into main lusush executable
- Replaces linenoise calls gradually
- Maintains compatibility during transition

**Read `LLE_AI_DEVELOPMENT_GUIDE.md` for complete context. This file gives you enough to start any LLE task immediately.**
