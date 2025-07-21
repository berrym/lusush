# AI Context: Lusush Line Editor (LLE) Development

## 🔥 INSTANT CONTEXT FOR AI ASSISTANTS

**Project**: Replacing linenoise with custom Lusush Line Editor (LLE)  
**Language**: C99  
**Build**: Meson (NOT Make)  
**Branch**: `feature/lusush-line-editor`  
**Status**: 15/50 tasks complete (30%), Phase 2 Core Functionality ✅

## 📋 IMMEDIATE NEXT STEPS
1. Check `LLE_PROGRESS.md` for current TODO task (LLE-016: Prompt Parsing)
2. Read task spec in `LLE_DEVELOPMENT_TASKS.md` 
3. Implement following `.cursorrules` patterns
4. Write tests, run `scripts/lle_build.sh test`
5. Commit with format: `LLE-XXX: Task description`

## 🚀 ESSENTIAL COMMANDS
```bash
# Build & Test
scripts/lle_build.sh setup    # First time
scripts/lle_build.sh build    # Compile
scripts/lle_build.sh test     # Run all tests

# Development
git checkout -b task/lle-XXX-desc  # New task branch
meson test -C builddir test_lle_XXX_integration -v  # Run specific test
```

## 💻 CODE STANDARDS (CRITICAL)
```c
// Naming: lle_component_action
bool lle_text_insert_char(lle_text_buffer_t *buffer, char c);
bool lle_prompt_parse(lle_prompt_t *prompt, const char *text);

// Structures: lle_component_t  
typedef struct {
    char *text;
    size_t length;
    lle_prompt_geometry_t geometry;
    bool has_ansi_codes;
    char **lines;
    size_t line_count;
    size_t capacity;
} lle_prompt_t;

// Error handling: always return bool
bool lle_function(args) {
    if (!args) return false;
    // implementation
    return true;
}

// Tests: LLE_TEST macro with proper framework
LLE_TEST(function_name) {
    // setup, test, assert, cleanup
    LLE_ASSERT(condition);
}
```

## 🎯 CURRENT ARCHITECTURE (IMPLEMENTED)
**✅ PHASE 1 FOUNDATION COMPLETE:**
- **Text Buffer System**: `src/line_editor/text_buffer.c/h` - UTF-8 text manipulation (LLE-001 to LLE-004)
- **Cursor Mathematics**: `src/line_editor/cursor_math.c/h` - Mathematical position calculations (LLE-005 to LLE-008)
- **Integrated Termcap**: `src/line_editor/termcap/` - Complete standalone termcap system (LLE-009)
- **Terminal Manager**: `src/line_editor/terminal_manager.c/h` - Professional terminal interface (LLE-010)
- **Terminal Output**: Full output API with cursor, color, clearing functions (LLE-011)
- **Test Framework**: `test_framework.h` - Professional testing infrastructure (LLE-012)
- **Text Buffer Tests**: Comprehensive 57-test suite (LLE-013)
- **Cursor Math Tests**: Complete 30-test mathematical validation (LLE-014)

**✅ PHASE 2 CORE FUNCTIONALITY STARTED:**
- **Prompt System**: `src/line_editor/prompt.c/h` - Multiline prompt support with ANSI handling (LLE-015)

**🚧 TODO COMPONENTS:**
- **Prompt Parsing**: ANSI detection and line splitting (LLE-016)
- **Prompt Rendering**: Display and geometry calculations (LLE-017)
- **Multiline Display**: Advanced input display (LLE-018)
- **Theme Integration**: `theme_integration.c/h` - Lusush theme support
- **History Management**: `history.c/h` - Command history
- **Tab Completion**: `completion.c/h` - Intelligent completion
- **Main API**: `line_editor.c/h` - Public interface

## 🏆 MAJOR ACHIEVEMENTS

### Revolutionary Termcap Integration (LLE-009 to LLE-011):
- **2000+ lines** of proven Lusush termcap code integrated into LLE
- **118+ functions** namespaced with `lle_termcap_` prefix
- **50+ terminal profiles** including modern terminals (iTerm2, etc.)
- **Zero external dependencies** - completely standalone library
- **Professional features**: 24-bit color, mouse support, bracketed paste
- **Cross-platform**: macOS, Linux, BSD with iTerm2 optimizations

### Advanced Prompt System (LLE-015):
- **Multiline prompt support** with dynamic line arrays
- **ANSI code detection** and handling architecture
- **Integrated geometry calculations** with cursor math system
- **Memory-safe operations** with comprehensive validation
- **Clean API design** ready for parsing and rendering

## 🧪 COMPREHENSIVE TESTING FRAMEWORK
**Extensive Test Coverage (101+ tests):**
- `tests/line_editor/test_text_buffer.c` - Text buffer operations (57 tests)
- `tests/line_editor/test_cursor_math.c` - Cursor mathematics (30 tests)
- `tests/line_editor/test_terminal_manager.c` - Terminal management (22 tests)
- `tests/line_editor/test_termcap_integration.c` - Termcap system (11 tests)
- `tests/line_editor/test_lle_010_integration.c` - Terminal manager integration (9 tests)
- `tests/line_editor/test_lle_011_terminal_output.c` - Terminal output (14 tests)
- `tests/line_editor/test_lle_015_prompt_structure.c` - Prompt structures (14 tests)

**Total: 101+ tests covering all implemented functionality**

## 📐 PERFORMANCE TARGETS (VALIDATED)
- Character insertion: < 1ms ✅
- Cursor movement: < 1ms ✅  
- Terminal operations: < 5ms ✅
- Prompt operations: < 2ms ✅
- Memory: < 1MB base, < 50 bytes per char ✅
- Support: 100KB text, 10K history, 500 char width (designed for)

## 🔧 CRITICAL GOTCHAS (LEARNED FROM IMPLEMENTATION)
1. **UTF-8**: Byte vs character positions - handled in text_buffer system
2. **Termcap Integration**: Use `LLE_TERMCAP_OK || LLE_TERMCAP_NOT_TERMINAL || LLE_TERMCAP_INVALID_PARAMETER`
3. **Memory**: Every malloc needs corresponding free - validated with Valgrind
4. **Terminal Bounds**: Always check geometry before cursor movement
5. **Build**: Use Meson, scripts in `scripts/` directory
6. **Non-Terminal Environments**: Functions must handle CI/testing gracefully
7. **Error Handling**: 5-layer validation (NULL, init, capability, bounds, data)
8. **Prompt Lines**: Always validate line_count <= capacity and null-terminate arrays

## 📁 KEY FILES TO READ (PRIORITY ORDER)
1. **`LLE_PROGRESS.md`** - Current status (LLE-016 next)
2. **`LLE_DEVELOPMENT_TASKS.md`** - All 50 tasks with specs
3. **`LLE_TERMCAP_QUICK_REFERENCE.md`** - Termcap integration overview
4. **`.cursorrules`** - Complete coding standards
5. **`LLE_AI_DEVELOPMENT_GUIDE.md`** - Detailed development guide
6. **Task completion summaries**: `LLE-009_COMPLETION_SUMMARY.md`, `LLE-010_COMPLETION_SUMMARY.md`, `LLE-011_COMPLETION_SUMMARY.md`

## 🎯 SUCCESS CRITERIA (PROGRESS)
- ✅ Professional terminal handling across all platforms
- ✅ Sub-millisecond response times for core operations
- ✅ Zero crashes with comprehensive error handling
- ✅ Complete foundation with mathematical correctness
- ✅ Advanced prompt structure with multiline support
- 🚧 Prompt parsing with ANSI handling (Phase 2)
- 🚧 Perfect multiline prompts (Phase 2)
- 🚧 100% theme integration (Phase 2)
- 🚧 Extensible architecture (Phase 4)

## 🆘 QUICK DEBUG & TESTING
```bash
# Run all LLE tests (101+ tests)
meson test -C builddir

# Run specific test categories
meson test -C builddir test_text_buffer -v      # Text operations
meson test -C builddir test_cursor_math -v      # Mathematical correctness
meson test -C builddir test_lle_015_prompt_structure -v  # Prompt system

# Memory leak detection
valgrind --leak-check=full builddir/tests/line_editor/test_lle_015_prompt_structure

# Debug specific functionality
export LLE_DEBUG=1
export LLE_TERMCAP_DEBUG=1
gdb builddir/tests/line_editor/test_terminal_manager

# Build from scratch
scripts/lle_build.sh clean && scripts/lle_build.sh setup && scripts/lle_build.sh build
```

## 🔄 DEVELOPMENT PHASES (CURRENT STATUS)
1. **Phase 1: Foundation** ✅ **COMPLETE** (LLE-001 to LLE-014) - Text buffer, cursor math, termcap integration, terminal I/O, testing
2. **Phase 2: Core** 🚧 **STARTED** (LLE-015 to LLE-026) - Prompts, themes, basic editing **[1/12 DONE]**
3. **Phase 3: Advanced** 📋 (LLE-027 to LLE-037) - Unicode, completion, undo/redo, syntax highlighting
4. **Phase 4: Integration** 📋 (LLE-038 to LLE-050) - API, optimization, documentation, final integration

## 📦 BUILD INTEGRATION (CURRENT)
- **LLE builds as static library**: `builddir/src/line_editor/liblle.a`
- **Termcap builds as static library**: `builddir/src/line_editor/termcap/liblle_termcap.a`
- **Links into main lusush executable**: Professional terminal handling
- **Meson build system**: Complete integration with dependency management
- **Test framework**: Comprehensive coverage with `meson test`

## 🏗️ CURRENT DIRECTORY STRUCTURE
```
lusush/src/line_editor/
├── termcap/                     # Integrated termcap system (LLE-009)
│   ├── lle_termcap.h           # Public termcap API
│   ├── lle_termcap.c           # Implementation (1300+ lines)
│   ├── lle_termcap_internal.h  # Internal structures
│   └── meson.build             # Termcap build config
├── text_buffer.c/h             # Text manipulation (LLE-001 to LLE-004)
├── cursor_math.c/h             # Cursor calculations (LLE-005 to LLE-008)
├── terminal_manager.c/h        # Terminal interface (LLE-010, LLE-011)
├── prompt.c/h                  # Prompt system (LLE-015)
└── meson.build                 # Main LLE build config

lusush/tests/line_editor/
├── test_text_buffer.c          # Text buffer tests (57 tests)
├── test_cursor_math.c          # Cursor math tests (30 tests)
├── test_terminal_manager.c     # Terminal manager tests (22 tests)
├── test_termcap_integration.c  # Termcap tests (11 tests)
├── test_lle_010_integration.c  # LLE-010 tests (9 tests)
├── test_lle_011_terminal_output.c # LLE-011 tests (14 tests)
├── test_lle_015_prompt_structure.c # LLE-015 tests (14 tests)
├── test_framework.h            # Testing infrastructure
└── meson.build                 # Test configuration
```

## 🚀 IMMEDIATE DEVELOPER ONBOARDING
**For any developer starting work:**

1. **Current Status**: 30% complete, Phase 1 foundation complete + Phase 2 started
2. **Next Task**: LLE-016 (Prompt Parsing) - implement ANSI detection and line splitting
3. **Key Achievement**: Complete foundation + advanced prompt structure with multiline support
4. **Test Everything**: `scripts/lle_build.sh test` runs 101+ comprehensive tests
5. **Code Quality**: All code follows strict C99 standards with comprehensive error handling

**Phase 1 foundation is rock-solid. Phase 2 builds advanced prompt functionality.**

## 📚 STRATEGIC CONTEXT
LLE replaces basic linenoise with a professional-grade line editor featuring:
- **Standalone Operation**: No external dependencies, complete termcap integration
- **Professional Terminal Handling**: 50+ terminal profiles, iTerm2 optimizations
- **Mathematical Correctness**: Provable cursor positioning algorithms
- **Advanced Prompt System**: Multiline prompts with ANSI code support
- **Extensible Architecture**: Clean APIs for themes, completion, history
- **Performance Focus**: Sub-millisecond core operations

**Read `LINE_EDITOR_STRATEGIC_ANALYSIS.md` for complete strategic context.**

## 🎯 RECENT MAJOR MILESTONE: LLE-015 COMPLETED
- **Advanced Prompt Structure**: Complete `lle_prompt_t` with multiline and ANSI support
- **15 API Functions**: Full prompt lifecycle management with validation
- **14 Comprehensive Tests**: All prompt operations validated
- **Phase 2 Started**: Beginning core functionality implementation
- **Architecture Ready**: Foundation for prompt parsing, rendering, and display

**Next: LLE-016 will implement the parsing logic to detect ANSI codes and split prompt lines.**