# AI Context: Lusush Line Editor (LLE) Development

## 🔥 INSTANT CONTEXT FOR AI ASSISTANTS

**Project**: Replacing linenoise with custom Lusush Line Editor (LLE)  
**Language**: C99  
**Build**: Meson (NOT Make)  
**Branch**: `feature/lusush-line-editor`  
**Status**: 23/50 tasks complete (46%), Phase 2 Core Functionality 75% complete ✅

## 📋 IMMEDIATE NEXT STEPS
1. Check `LLE_PROGRESS.md` for current TODO task (LLE-024: History Structure)
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
bool lle_theme_get_color(lle_theme_integration_t *ti, lle_theme_element_t element);
bool lle_key_event_copy(lle_key_event_t *dest, const lle_key_event_t *src);

// Structures: lle_component_t  
typedef struct {
    lle_key_type_t type;
    char character;
    uint32_t unicode;
    bool ctrl, alt, shift, super;
    char raw_sequence[16];
    uint64_t timestamp;
} lle_key_event_t;

// Error handling: always return bool
bool lle_function(args) {
    if (!args) return false;
    // implementation
    return true;
}

// Tests: LLE_TEST macro with proper framework
LLE_TEST(function_name) {
    printf("Testing function behavior... ");
    // setup, test, assert, cleanup
    LLE_ASSERT(condition);
    printf("PASSED\n");
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

**✅ PHASE 2 CORE FUNCTIONALITY (7/12 COMPLETE):**
- **Prompt System**: `src/line_editor/prompt.c/h` - Multiline prompt support with ANSI handling (LLE-015)
- **Prompt Parsing**: ANSI detection and line splitting with display width calculation (LLE-016)
- **Prompt Rendering**: Complete prompt rendering with cursor positioning (LLE-017)
- **Multiline Display**: Full display state management with input rendering (LLE-018)
- **Theme Integration**: `src/line_editor/theme_integration.c/h` - Complete theme system with fallback colors (LLE-019/020)
- **Key Input Handling**: `src/line_editor/input_handler.c/h` - Comprehensive 60+ key type definitions (LLE-021)
- **LLE-021 COMPLETED**: Key input handling structures with 60+ key types (23 tests) ✅
- **LLE-022 COMPLETED**: Key event processing with raw input reading and escape sequence parsing (20 tests) ✅
- **LLE-023 COMPLETED**: Basic editing commands with comprehensive text manipulation (15+ tests) ✅

**🚧 TODO COMPONENTS:**
- **History Structure**: Command history data structures (LLE-024)
- **History Management**: History operations and navigation (LLE-025-026)
- **History Management**: `history.c/h` - Command history (LLE-024 to LLE-026)
- **Advanced Features**: Unicode, completion, undo/redo (Phase 3)
- **Main API**: `line_editor.c/h` - Public interface (Phase 4)

## 🏆 MAJOR ACHIEVEMENTS

### Revolutionary Termcap Integration (LLE-009 to LLE-011):
- **2000+ lines** of proven Lusush termcap code integrated into LLE
- **118+ functions** namespaced with `lle_termcap_` prefix
- **50+ terminal profiles** including modern terminals (iTerm2, etc.)
- **Zero external dependencies** - completely standalone library
- **Professional features**: 24-bit color, mouse support, bracketed paste
- **Cross-platform**: macOS, Linux, BSD with iTerm2 optimizations

### Complete Prompt System (LLE-015 to LLE-017):
- **Multiline prompt support** with dynamic line arrays and ANSI code handling
- **Advanced parsing** with display width calculation and line splitting
- **Professional rendering** with terminal output and cursor positioning
- **Integrated geometry calculations** with mathematical precision
- **Memory-safe operations** with comprehensive validation

### Complete Display System (LLE-018):
- **Multiline input display** with state management and cursor positioning
- **Efficient rendering** with prompt integration and line wrapping
- **Professional architecture** coordinating 4 major LLE components
- **Comprehensive testing** with 19 tests covering all scenarios

### Complete Theme System (LLE-019/020):
- **Standalone theme integration** with 18 visual element mappings
- **Fallback color system** with ANSI codes for universal compatibility
- **Performance caching** with hit/miss tracking and statistics
- **Terminal capability detection** (true color, 256 color, basic)
- **Debug and diagnostics** with comprehensive configuration management

### Complete Key Input System (LLE-021):
- **60+ key type definitions** covering all essential editing operations
- **Comprehensive event structure** with modifiers, Unicode, and metadata
- **Professional binding system** for runtime key-to-action mapping
- **Classification utilities** (printable, control, navigation, editing, function)
- **String conversion system** for debugging and configuration

## 🧪 COMPREHENSIVE TESTING FRAMEWORK
**Extensive Test Coverage (160+ tests):**
- `tests/line_editor/test_text_buffer.c` - Text buffer operations (57 tests)
- `tests/line_editor/test_cursor_math.c` - Cursor mathematics (30 tests)
- `tests/line_editor/test_terminal_manager.c` - Terminal management (22 tests)
- `tests/line_editor/test_termcap_integration.c` - Termcap system (11 tests)
- `tests/line_editor/test_lle_010_integration.c` - Terminal manager integration (9 tests)
- `tests/line_editor/test_lle_011_terminal_output.c` - Terminal output (14 tests)
- `tests/line_editor/test_lle_015_prompt_structure.c` - Prompt structures (14 tests)
- `tests/line_editor/test_lle_016_prompt_parsing.c` - Prompt parsing (17 tests)
- `tests/line_editor/test_lle_017_prompt_rendering.c` - Prompt rendering (16 tests)
- `tests/line_editor/test_lle_018_multiline_input_display.c` - Display system (19 tests)
- `tests/line_editor/test_lle_019_theme_interface.c` - Theme interface (13 tests)
- `tests/line_editor/test_lle_020_basic_theme_application.c` - Theme application (22 tests)
- `tests/line_editor/test_lle_021_key_input_handling.c` - Key input handling (23 tests)

**Total: 195+ tests covering all implemented functionality**

## 📐 PERFORMANCE TARGETS (VALIDATED)
- Character insertion: < 1ms ✅
- Cursor movement: < 1ms ✅  
- Terminal operations: < 5ms ✅
- Prompt operations: < 2ms ✅
- Display updates: < 5ms ✅
- Theme color access: < 1ms ✅
- Key event processing: < 1ms (target)
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
9. **Theme Colors**: Use fallback colors when Lusush theme system unavailable
10. **Key Events**: Always initialize event structures before use with lle_key_event_init()
11. **Text Buffer Memory**: Use `lle_text_buffer_create()` + `lle_text_buffer_destroy()`, NOT `lle_text_buffer_init()` in tests
12. **Display Validation**: Commands should handle display validation failures gracefully for non-terminal environments
13. **Test Setup**: Simplify test setup with minimal components - only connect required parts to avoid complex initialization
14. **Command Results**: Commands return valid results even in non-terminal environments when text operations succeed
15. **Terminal Environment**: Check if issues are terminal/TTY related before debugging complex logic errors

## 🎓 CRITICAL LESSONS LEARNED (AI DEVELOPMENT INSIGHTS)

### **Test Environment Challenges**
- **CI/Non-Terminal Issues**: When tests fail with display/terminal errors, the issue is usually that commands expect full terminal capability but run in non-terminal CI environment
- **Quick Fix**: Make commands handle `lle_display_validate()` failure gracefully - continue with text operations, skip display updates
- **Symptom**: `LLE_CMD_ERROR_DISPLAY_UPDATE` or segfaults in display functions indicate terminal environment issues
- **Solution Pattern**: `if (lle_display_validate(state)) { update_display(); }` instead of requiring display validation

### **Memory Management Patterns**
- **Text Buffer Creation**: Always use `lle_text_buffer_create()` + `lle_text_buffer_destroy()` for heap allocation
- **Text Buffer Init**: `lle_text_buffer_init()` is for stack-allocated structures, not suitable for test patterns
- **Common Error**: "free(): invalid pointer" means mixing init/create with destroy functions
- **Test Pattern**: Create pointer (`lle_text_buffer_t *buffer`), create with function, destroy in cleanup

### **Function Naming Conflicts**
- **Cursor Movement Enums**: Watch for conflicts between `lle_cursor_movement_t` (text_buffer.h) and custom enums
- **Solution**: Use prefixed names like `lle_cmd_cursor_movement_t` for command-specific enums
- **Display Functions**: All exist and work - check `src/line_editor/display.c` for available functions
- **Text Buffer API**: Use `lle_text_move_cursor(buffer, LLE_MOVE_LEFT)` not individual direction functions

### **Test Framework Patterns**
- **Function Calls**: Call `test_function_name()` directly in main(), NOT `RUN_TEST(function_name)`
- **Test Definition**: Use `LLE_TEST(name)` macro, then call `test_name()` in main()
- **Test Structure**: Keep setup simple - only initialize components actually needed
- **Debug Strategy**: Add printf statements early, disable assertions temporarily to see actual vs expected results

### **Command Implementation Strategy**
- **Graceful Degradation**: Commands should work in any environment (terminal, non-terminal, CI)
- **Core Logic First**: Implement text buffer operations first, display integration second
- **Return Valid Results**: Even if display updates fail, return success if text operations succeed
- **Validation Pattern**: Check state and buffer validity, make display updates optional

### **Build and Test Debugging**
- **Compilation Errors**: Usually indicate missing function declarations or conflicting types
- **Memory Errors**: Often text buffer creation/destruction pattern issues
- **Assertion Failures**: Check if test expects different output than what's actually produced
- **Segfaults**: Usually display validation failing in non-terminal environment

### **Progressive Development Approach**
- **Start Simple**: Create minimal test setup, test core functionality first
- **Add Complexity Gradually**: Start with text buffer operations, add display integration later
- **Validate Results**: Don't assume test expectations are correct - verify actual output first
- **Debug Environment**: Always consider if issues are environmental (terminal/CI) vs logical

### **AI Assistant Workflow**
1. **Read Current Task**: Check `LLE_PROGRESS.md` for current TODO task
2. **Check Existing Patterns**: Look at completed tests for setup patterns
3. **Start with Core Logic**: Implement text operations before display integration
4. **Test Incrementally**: Build simple tests first, add complexity gradually
5. **Handle Environment**: Make functions work in both terminal and non-terminal environments
6. **Validate Assumptions**: Don't assume test expectations are correct - verify output

## 📁 KEY FILES TO READ (PRIORITY ORDER)
1. **`LLE_PROGRESS.md`** - Current status (LLE-022 next)
2. **`LLE_DEVELOPMENT_TASKS.md`** - All 50 tasks with specs
3. **`LLE_TERMCAP_QUICK_REFERENCE.md`** - Termcap integration overview
4. **`.cursorrules`** - Complete coding standards
5. **`LLE_AI_DEVELOPMENT_GUIDE.md`** - Detailed development guide
6. **Recent completion summaries**: `LLE-019_COMPLETION_SUMMARY.md`, `LLE-020_COMPLETION_SUMMARY.md`, `LLE-021_COMPLETION_SUMMARY.md`

## 🎯 SUCCESS CRITERIA (PROGRESS)
- ✅ Professional terminal handling across all platforms
- ✅ Sub-millisecond response times for core operations
- ✅ Zero crashes with comprehensive error handling
- ✅ Complete foundation with mathematical correctness
- ✅ Advanced prompt system with multiline and ANSI support
- ✅ Perfect multiline prompt parsing and rendering
- ✅ Complete display system with input rendering
- ✅ **Complete theme integration with fallback system**
- ✅ **Comprehensive key input handling with 60+ key types**
- ✅ **Key event processing from terminal input with 80+ escape sequences**
- ✅ **Basic editing commands with unified command interface**
- 🚧 Command history structure and management (Phase 2 - next task)
- 🚧 Extensible architecture (Phase 4)

## 🆘 QUICK DEBUG & TESTING
```bash
# Run all LLE tests (160+ tests)
meson test -C builddir

# Run specific test categories
meson test -C builddir test_text_buffer -v          # Text operations
meson test -C builddir test_cursor_math -v          # Mathematical correctness
- meson test -C builddir test_lle_020_basic_theme_application -v  # Theme system
- meson test -C builddir test_lle_021_key_input_handling -v       # Key input
- meson test -C builddir test_lle_022_key_event_processing -v     # Key events
- meson test -C builddir test_lle_023_basic_editing_commands -v   # Edit commands

# Memory leak detection
valgrind --leak-check=full builddir/tests/line_editor/test_lle_021_key_input_handling

# Debug specific functionality
export LLE_DEBUG=1
export LLE_TERMCAP_DEBUG=1
gdb builddir/tests/line_editor/test_lle_021_key_input_handling

# Build from scratch
scripts/lle_build.sh clean && scripts/lle_build.sh setup && scripts/lle_build.sh build
```

## 🔄 DEVELOPMENT PHASES (CURRENT STATUS)
1. **Phase 1: Foundation** ✅ **COMPLETE** (LLE-001 to LLE-014) - Text buffer, cursor math, termcap integration, terminal I/O, testing
2. **Phase 2: Core** 🚧 **75% COMPLETE** (LLE-015 to LLE-026) - Prompts, themes, editing commands **[9/12 DONE]**
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
├── prompt.c/h                  # Complete prompt system (LLE-015 to LLE-017)
├── display.c/h                 # Multiline input display (LLE-018)
├── theme_integration.c/h       # Complete theme system (LLE-019, LLE-020)
├── input_handler.c/h           # Key input and event processing (LLE-021, LLE-022)
├── edit_commands.c/h           # Basic editing commands (LLE-023)
└── meson.build                 # Main LLE build config

lusush/tests/line_editor/
├── test_text_buffer.c          # Text buffer tests (57 tests)
├── test_cursor_math.c          # Cursor math tests (30 tests)
├── test_terminal_manager.c     # Terminal manager tests (22 tests)
├── test_termcap_integration.c  # Termcap tests (11 tests)
├── test_lle_010_integration.c  # LLE-010 tests (9 tests)
├── test_lle_011_terminal_output.c # LLE-011 tests (14 tests)
├── test_lle_015_prompt_structure.c # LLE-015 tests (14 tests)
├── test_lle_016_prompt_parsing.c # LLE-016 tests (17 tests)
├── test_lle_017_prompt_rendering.c # LLE-017 tests (16 tests)
├── test_lle_018_multiline_input_display.c # LLE-018 tests (19 tests)
├── test_lle_019_theme_interface.c # LLE-019 tests (13 tests)
├── test_lle_020_basic_theme_application.c # LLE-020 tests (22 tests)
├── test_lle_021_key_input_handling.c # LLE-021 tests (23 tests)
├── test_lle_022_key_event_processing.c # LLE-022 tests (20 tests)
├── test_lle_023_basic_editing_commands.c # LLE-023 tests (15+ tests)
├── test_framework.h            # Testing infrastructure
└── meson.build                 # Test configuration
```

## 🚀 IMMEDIATE DEVELOPER ONBOARDING
**For any developer starting work:**

1. **Current Status**: 42% complete, Phase 1 foundation complete + Phase 2 58% complete
2. **Next Task**: LLE-022 (Key Event Processing) - implement raw input reading and escape sequence parsing
3. **Key Achievement**: Complete input handling foundation with theme system and 60+ key types
4. **Test Everything**: `scripts/lle_build.sh test` runs 160+ comprehensive tests
5. **Code Quality**: All code follows strict C99 standards with comprehensive error handling

**Phase 1 foundation is rock-solid. Phase 2 core systems (prompts, themes, input, editing) are 75% complete.**

## 📚 STRATEGIC CONTEXT
LLE replaces basic linenoise with a professional-grade line editor featuring:
- **Standalone Operation**: No external dependencies, complete termcap integration
- **Professional Terminal Handling**: 50+ terminal profiles, iTerm2 optimizations
- **Mathematical Correctness**: Provable cursor positioning algorithms
- **Advanced Prompt System**: Multiline prompts with ANSI code support
- **Complete Theme Integration**: 18 visual elements with fallback support
- **Comprehensive Input Handling**: 60+ key types with modifier support
- **Extensible Architecture**: Clean APIs for completion, history, advanced features
- **Performance Focus**: Sub-millisecond core operations

**Read `LINE_EDITOR_STRATEGIC_ANALYSIS.md` for complete strategic context.**

## 🎯 RECENT MAJOR MILESTONE: PHASE 2 FOUNDATION COMPLETE (58%)
- **Complete Theme System**: Standalone theme integration with fallback colors
- **Complete Key Input System**: 60+ key types with comprehensive event structures
- **Professional Architecture**: Ready for event processing and editing commands
- **160+ Comprehensive Tests**: All foundation systems validated
- **Performance Validated**: Sub-millisecond operations across all systems
- **Zero Memory Leaks**: Valgrind-verified memory management

**LLE-024 will implement command history data structures and management.**

## 🔑 CURRENT PHASE 2 STATUS
**✅ COMPLETED (9/12 tasks):**
- LLE-015: Prompt Structure Definition
- LLE-016: Prompt Parsing 
- LLE-017: Prompt Rendering
- LLE-018: Multiline Input Display
- LLE-019: Theme Interface Definition
- LLE-020: Basic Theme Application
- LLE-021: Key Input Handling
- LLE-022: Key Event Processing
- LLE-023: Basic Editing Commands

**🚧 REMAINING (3/12 tasks):**
- LLE-024: History Structure ← **NEXT TASK**
- LLE-025: History Management
- LLE-026: History Navigation

**Phase 2 is 75% complete with solid editing command foundation ready for history implementation.**
