# AI Context: Lusush Line Editor (LLE) Development
**Last Updated**: December 2024 | **Version**: Phase 3 Advanced Features | **Next Task**: LLE-036

## 🚨 CRITICAL: READ DOCUMENTATION FIRST - NO EXCEPTIONS

⚠️ **STOP! Before any code changes, you MUST read these files in order:**

1. **`.cursorrules`** - LLE coding standards and patterns (REQUIRED)
2. **`LLE_AI_DEVELOPMENT_GUIDE.md`** - Complete development context (REQUIRED)
3. **`LLE_PROGRESS.md`** - Current task status (REQUIRED)
4. **`LLE_DEVELOPMENT_TASKS.md`** - Task specifications (REQUIRED)

**DO NOT proceed without reading these files. Failure to follow established patterns will result in code rejection.**

## 🔥 INSTANT CONTEXT FOR AI ASSISTANTS

**Project**: Replacing linenoise with custom Lusush Line Editor (LLE)  
**Language**: C99  
**Build**: Meson (NOT Make)  
**Branch**: `feature/lusush-line-editor`  
**Status**: 35/50 tasks complete (70%) + hist_no_dups enhancement, Phase 3 Advanced Features IN PROGRESS ✅

## 🎯 CURRENT CAPABILITIES (WHAT WORKS NOW)
**✅ FULLY OPERATIONAL:**
- **Unicode Text Editing**: Complete UTF-8 support with character-aware cursor movement
- **International Languages**: CJK, emojis, accented characters, complex Unicode
- **Word Navigation**: Unicode-aware word boundaries for any language
- **Professional Terminal**: 50+ terminal types, iTerm2 optimizations, 24-bit color
- **Multiline Prompts**: ANSI escape code support, dynamic sizing, theme integration
- **Command History**: File persistence, navigation, circular buffer (10-50K entries)
- **Theme System**: 18 visual elements, fallback colors, terminal capability detection
- **Key Input**: 60+ key types, modifiers, escape sequences, comprehensive event handling
- **Mathematical Precision**: Cursor positioning with byte/character position accuracy
- **Completion Framework**: Extensible provider architecture with context analysis (LLE-029 ✅)
- **File Completion**: Basic file and directory completion with word extraction (LLE-030 ✅)
- **Completion Display**: Visual interface with scrolling and navigation (LLE-031 ✅)
- **Undo System Structure**: Complete undo/redo data structures and validation (LLE-032 ✅)
- **Undo Operation Recording**: Complete operation recording with capacity management (LLE-033 ✅)
- **Undo/Redo Execution**: Complete operation reversal with all action types supported (LLE-034 ✅)
- **Syntax Highlighting Framework**: Complete framework with shell syntax detection and extensible architecture (LLE-035 ✅)

## 📋 MANDATORY WORKFLOW - FOLLOW EXACTLY

**STEP 1: READ DOCUMENTATION (REQUIRED)**
- Read `.cursorrules` - Coding standards, naming conventions, error patterns
- Read `LLE_AI_DEVELOPMENT_GUIDE.md` - Complete development context
- Read current task in `LLE_DEVELOPMENT_TASKS.md` - Exact requirements

**STEP 2: UNDERSTAND CONTEXT**
- Check `LLE_PROGRESS.md` for current TODO task (LLE-036: Basic Shell Syntax)
- Review existing code patterns in similar completed tasks
- Understand dependencies and integration points

**STEP 3: IMPLEMENT WITH STANDARDS**
- Follow EXACT naming patterns: `lle_component_action()`
- Use EXACT error handling: `bool` returns, parameter validation
- Write Doxygen documentation: `@brief`, `@param`, `@return`, `@note`
- Follow memory patterns: `memcpy()` not `strcpy()`, proper cleanup

**STEP 4: TEST COMPREHENSIVELY**
- Write tests using `LLE_TEST(name)` macro
- Call tests as `test_name()` in main()
- Include edge cases, error conditions, parameter validation
- Run `scripts/lle_build.sh test`

**STEP 5: COMMIT PROPERLY**
- Format: `LLE-XXX: Task description with detailed implementation notes`
- Update progress tracking files

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

## 💻 CODE STANDARDS (CRITICAL - MUST FOLLOW EXACTLY)

⚠️ **These are NON-NEGOTIABLE. Violations will be rejected.**

```c
// 1. NAMING: lle_component_action (EXACT pattern)
bool lle_text_insert_char(lle_text_buffer_t *buffer, char c);
bool lle_completion_extract_word(const char *input, size_t cursor_pos, ...);

// 2. STRUCTURES: lle_component_t (EXACT pattern)
typedef struct lle_completion_item {
    char *text;                    // Required fields
    char *description;             // Optional fields
    int priority;                  // Sorting/display
} lle_completion_item_t;

// 3. ERROR HANDLING: Always return bool (MANDATORY)
bool lle_function(args) {
    // ALWAYS validate parameters first
    if (!args) return false;
    if (invalid_condition) return false;
    
    // Implementation
    return true; // Success
}

// 4. DOCUMENTATION: Comprehensive Doxygen (REQUIRED)
/**
 * @brief Insert character at cursor position in text buffer
 *
 * Automatically resizes buffer if needed and advances cursor by one.
 * Handles UTF-8 encoding correctly and maintains text integrity.
 *
 * @param buffer Text buffer to modify (must not be NULL)
 * @param c Character to insert
 * @return true on success, false on error or invalid parameters
 *
 * @note Buffer capacity will be doubled if insertion would exceed current size
 * @note Cursor position is automatically advanced past inserted character
 */

// 5. MEMORY MANAGEMENT: Use memcpy(), not strcpy() (MANDATORY)
if (len > 0) {
    memcpy(dest, src, len);
}
dest[len] = '\0';

// 6. TESTS: LLE_TEST macro with proper calls (EXACT pattern)
LLE_TEST(function_basic_behavior) {  // NO test_ prefix in LLE_TEST()
    printf("Testing function basic behavior... ");
    
    // Setup
    component_t *comp = create_component();
    LLE_ASSERT_NOT_NULL(comp);
    
    // Test
    LLE_ASSERT(lle_function_call(comp, valid_input));
    
    // Verify
    LLE_ASSERT_EQ(comp->expected_field, expected_value);
    
    // Cleanup
    destroy_component(comp);
    printf("PASSED\n");
}

// Main function calls: test_function_basic_behavior()  (WITH test_ prefix)
int main(void) {
    test_function_basic_behavior();  // LLE_TEST creates test_ prefixed function
    return 0;
}
```

## 🚨 COMMON MISTAKES TO AVOID (THESE WILL BE REJECTED)

❌ **Naming Violations**
- Using non-LLE prefixes
- Inconsistent component naming
- Missing action verbs in function names

❌ **Documentation Violations**  
- Missing `@brief`, `@param`, `@return`
- No behavior descriptions
- Missing error condition documentation

❌ **Memory Safety Violations**
- Using `strcpy()` instead of `memcpy()`
- Missing buffer bounds checking  
- Incorrect parameter validation order

❌ **Test Framework Violations**
- Using `test_` prefix in `LLE_TEST()` macro
- Calling wrong function names in main()
- Missing comprehensive test coverage

❌ **Build Integration Violations**
- Missing includes for used functions
- Compiler warnings not addressed
- Test files not added to meson.build

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

**✅ PHASE 2 CORE FUNCTIONALITY COMPLETE (12/12):**
- **Prompt System**: `src/line_editor/prompt.c/h` - Multiline prompt support with ANSI handling (LLE-015)
- **Prompt Parsing**: ANSI detection and line splitting with display width calculation (LLE-016)
- **Prompt Rendering**: Complete prompt rendering with cursor positioning (LLE-017)
- **Multiline Display**: Full display state management with input rendering (LLE-018)
- **Theme Integration**: `src/line_editor/theme_integration.c/h` - Complete theme system with fallback colors (LLE-019/020)
- **Key Input Handling**: `src/line_editor/input_handler.c/h` - Comprehensive 60+ key type definitions (LLE-021)
- **LLE-021 COMPLETED**: Key input handling structures with 60+ key types (23 tests) ✅
- **LLE-022 COMPLETED**: Key event processing with raw input reading and escape sequence parsing (20 tests) ✅
- **LLE-023 COMPLETED**: Basic editing commands with comprehensive text manipulation (15+ tests) ✅ [CRITICAL BUG FIXED]
- **LLE-024 COMPLETED**: History structure with complete command storage and circular buffer navigation (20+ tests) ✅
- **LLE-025 COMPLETED**: History management with file persistence and save/load operations (13+ tests) ✅
- **LLE-026 COMPLETED**: History navigation with convenience functions and position management (12+ tests) ✅

**✅ PHASE 3 ADVANCED FEATURES IN PROGRESS (9/11):**
- **LLE-027 COMPLETED**: UTF-8 text handling with comprehensive Unicode support (22+ tests) ✅
- **LLE-028 COMPLETED**: Unicode cursor movement with character-aware navigation and word boundaries (13+ tests) ✅
- **LLE-029 COMPLETED**: Completion framework with extensible provider architecture (18+ tests) ✅
- **LLE-030 COMPLETED**: Basic file completion with filesystem integration and word extraction (14+ tests) ✅
- **LLE-031 COMPLETED**: Completion display with visual interface, scrolling, and navigation (13+ tests) ✅
- **LLE-032 COMPLETED**: Undo stack structure with comprehensive data structures and validation (23+ tests) ✅
- **LLE-033 COMPLETED**: Undo operation recording with action recording, capacity management, and memory efficiency (8+ tests) ✅
- **LLE-034 COMPLETED**: Undo/redo execution with operation reversal and all action types supported (12+ tests) ✅
- **ENHANCEMENT COMPLETED**: hist_no_dups implementation with runtime toggle and move-to-end behavior (15+ tests) ✅
- **Unicode Support**: `src/line_editor/unicode.c/h` - Complete UTF-8 text analysis and navigation
- **Unicode Cursor Movement**: Character-based LEFT/RIGHT movement and Unicode word boundaries
- **Runtime hist_no_dups**: Complete unique history with move-to-end behavior and runtime toggle
- **Completion System**: `src/line_editor/completion.c/h` - Complete framework, file completion, and display interface
- **Complete Undo/Redo System**: `src/line_editor/undo.c/h` - Data structures, operation recording, and execution
- **Syntax Highlighting Framework**: `src/line_editor/syntax.c/h` - Complete framework with shell syntax detection

**🚧 TODO COMPONENTS:**
- **Enhanced Shell Syntax**: Advanced shell syntax support (Phase 3) ← CURRENT
- **Syntax Display Integration**: Visual highlighting display (Phase 3)
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

### Complete History System (LLE-024):
- **Circular buffer architecture** with configurable size limits (10-50,000 entries)
- **Navigation system** with prev/next/first/last movement and state tracking
- **Temporary buffer management** for preserving current edit during navigation
- **Statistics engine** with memory usage, command analysis, and temporal tracking
- **Professional memory management** with both heap and stack allocation patterns

### Complete Unicode Support System (LLE-027/028):
- **UTF-8 Text Handling**: Comprehensive analysis and navigation for any Unicode content
- **Character-Aware Cursor Movement**: LEFT/RIGHT by logical characters, not bytes
- **Unicode Word Boundaries**: International word navigation for CJK, emojis, accented text
- **Position Conversion System**: Seamless byte ↔ character position translation
- **Performance Optimized**: Sub-millisecond operations with minimal overhead for ASCII
- **International Ready**: Full support for global languages and modern Unicode

### Complete hist_no_dups Enhancement:
- **Runtime Toggle**: Enable/disable unique history during shell session
- **Move-to-End Behavior**: Duplicate commands move to end with updated timestamps
- **Chronological Order**: Preserves order of latest occurrences of each command
- **Lusush Integration Ready**: Full support for `config.history_no_dups` setting
- **Comprehensive API**: Create, toggle, query, and manually clean duplicates
- **Professional Grade**: Sophisticated duplicate management rivaling bash/zsh

### Complete Completion Framework (LLE-029):
- **Extensible Provider Architecture**: Clean interface for multiple completion sources
- **Intelligent Context Analysis**: Word boundaries, quote detection, command position
- **Dynamic List Management**: Auto-resizing arrays with priority-based sorting
- **Performance Optimized**: Sub-millisecond operations, efficient memory usage
- **Comprehensive API**: Items, lists, context, utilities with full error handling
- **Foundation Ready**: Architecture prepared for file, command, and variable completion

### Complete Syntax Highlighting Framework (LLE-035):
- **Extensible Architecture**: Region-based highlighting with 10 syntax types
- **Shell Syntax Detection**: Keywords, strings, variables, comments, operators
- **Runtime Configuration**: Dynamic enable/disable of highlighting types
- **Performance Optimized**: Sub-5ms highlighting for typical shell commands
- **Display Integration Ready**: Clean interface for visual rendering systems
- **Memory Efficient**: Dynamic allocation with safety limits and bounds checking

## 🧪 COMPREHENSIVE TESTING FRAMEWORK
**Extensive Test Coverage (413+ tests):**
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
- `tests/line_editor/test_lle_022_key_event_processing.c` - Key event processing (20 tests)
- `tests/line_editor/test_lle_023_basic_editing_commands.c` - Basic editing commands (15+ tests)
- `tests/line_editor/test_lle_024_history_structure.c` - History structure (8 tests)
- `tests/line_editor/test_lle_025_history_management.c` - History management (13 tests)
- `tests/line_editor/test_lle_026_history_navigation.c` - History navigation (12 tests)
- `tests/line_editor/test_lle_027_utf8_text_handling.c` - UTF-8 text handling (22 tests)
- `tests/line_editor/test_lle_028_unicode_cursor_movement.c` - Unicode cursor movement (13 tests)
- `tests/line_editor/test_lle_029_completion_framework.c` - Completion framework (18 tests)
- tests/line_editor/test_lle_030_basic_file_completion.c - Basic file completion (14 tests)
- tests/line_editor/test_lle_031_completion_display.c - Completion display (13 tests)
- `tests/line_editor/test_lle_032_undo_stack_structure.c` - Undo stack structure (23 tests)
- `tests/line_editor/test_lle_033_undo_operation_recording.c` - Undo operation recording (8 tests)
- `tests/line_editor/test_lle_034_undo_redo_execution.c` - Undo/redo execution (12 tests)
- `tests/line_editor/test_lle_hist_no_dups.c` - History duplicate management (15 tests)
- `tests/line_editor/test_lle_035_syntax_highlighting_framework.c` - Syntax highlighting framework (17 tests)

**Total: 413+ tests covering all implemented functionality**

## 📐 PERFORMANCE TARGETS (VALIDATED)
- Character insertion: < 1ms ✅
- Cursor movement: < 1ms ✅  
- Terminal operations: < 5ms ✅
- Prompt operations: < 2ms ✅
- Display updates: < 5ms ✅
- Theme color access: < 1ms ✅
- Key event processing: < 1ms ✅
- UTF-8 text analysis: < 1ms ✅
- Unicode cursor movement: < 1ms ✅
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
13. **Command Results**: Commands return valid results even in non-terminal environments when text operations succeed
14. **Terminal Environment**: Check if issues are terminal/TTY related before debugging complex logic errors
15. **Parameter Validation**: Always verify function parameter contracts - mixing (start, length) vs (start, end) causes bugs
16. **Naming Conflicts**: Use proper LLE prefixes to avoid conflicts with existing project files (e.g., command_history.h with lle_history_* functions)
17. **Unicode Character Counting**: Manual character count validation often reveals test expectation errors
18. **UTF-8 Navigation**: Use lle_utf8_prev_char/next_char for cursor movement, never byte arithmetic
19. **Empty String Edge Cases**: UTF-8 functions must handle empty strings (return SIZE_MAX for invalid positions)
20. **Unicode Word Boundaries**: CJK characters should be treated as individual word boundaries
21. **Function Placement**: Be careful with function placement in large files - use proper scope
22. **SIZE_MAX Constants**: Include <stdint.h> for SIZE_MAX in test files
23. **Character vs Byte Positions**: Always distinguish between character indices and byte offsets
24. **Unicode Display Width**: Start with character count approximation, enhance for zero-width/double-width later

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
3. **Unicode Considerations**: For text handling, always consider UTF-8 implications
4. **Start with Core Logic**: Implement text operations before display integration
5. **Character vs Byte Positions**: Distinguish between character indices and byte offsets
6. **Test Incrementally**: Build simple tests first, add complexity gradually
7. **Debug with Sample Data**: Create debug programs to validate text processing logic
8. **Handle Environment**: Make functions work in both terminal and non-terminal environments
9. **Validate Assumptions**: Don't assume test expectations are correct - verify output
10. **Unicode Testing**: Include ASCII, accented characters, CJK, and emojis in tests

### **Critical Bug Discovery and Resolution**
- **Parameter Contract Bugs**: Found critical bug in LLE-023 where functions were passing (start, length) instead of (start, end) to lle_text_delete_range()
- **Systematic Impact**: Bug affected kill_line, delete_word, and backspace_word operations causing incorrect text deletion
- **Unicode Character Counting**: LLE-027 revealed manual character count errors in test expectations
- **UTF-8 Navigation Logic**: LLE-028 required careful validation logic for character boundary detection
- **Debug Process**: Isolated with standalone test, traced exact behavior, identified mismatch, applied systematic fix
- **Quality Importance**: Demonstrates why no failing tests can be accepted in critical components
- **Resolution Pattern**: Create debug program → trace behavior → identify root cause → systematic fix → comprehensive validation
- **Unicode Debugging**: Use hex byte analysis to understand UTF-8 encoding issues

## 🏆 WHAT ANY AI ASSISTANT CAN DO IMMEDIATELY

**ONLY AFTER READING REQUIRED DOCUMENTATION:**

- **Start Next Task**: LLE-033 (Undo Operation Recording) - BUT READ TASK SPEC FIRST
- **Run Full Test Suite**: 370+ tests - `meson test -C builddir`
- **Debug Issues**: Use existing patterns from completed tasks
- **Extend Systems**: Follow established architectural patterns
- **Add Features**: MUST follow existing code patterns exactly

## ⚠️ BEFORE ANY CODE CHANGES

1. **Read `.cursorrules`** - Know the exact standards
2. **Read `LLE_AI_DEVELOPMENT_GUIDE.md`** - Understand the context  
3. **Read current task specification** - Know the requirements
4. **Study existing code patterns** - See how similar tasks were implemented
5. **Plan implementation** - Design before coding
6. **Write tests first** - Test-driven development

## 📁 MANDATORY READING ORDER (READ BEFORE ANY WORK)

**REQUIRED DOCUMENTATION (NO EXCEPTIONS):**
1. **`.cursorrules`** - LLE coding standards and patterns
2. **`LLE_AI_DEVELOPMENT_GUIDE.md`** - Complete development context and standards
3. **`LLE_PROGRESS.md`** - Current task status (LLE-033 next)
4. **`LLE_DEVELOPMENT_TASKS.md`** - Task LLE-033 specification
5. **`LLE_TERMCAP_QUICK_REFERENCE.md`** - If working with terminal functions

**REFERENCE FOR PATTERNS:**
6. **Recent completion summaries**: `LLE-030_COMPLETION_SUMMARY.md`, `LLE-031_COMPLETION_SUMMARY.md`, `LLE-032_COMPLETION_SUMMARY.md`, `LLE-033_COMPLETION_SUMMARY.md`
7. **Existing code**: Study `src/line_editor/completion.c` and `src/line_editor/undo.c` for established patterns
8. **Test patterns**: Study `tests/line_editor/test_lle_030_basic_file_completion.c` and `tests/line_editor/test_lle_033_undo_operation_recording.c`

**DO NOT START CODING WITHOUT READING ITEMS 1-4 ABOVE**

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
- ✅ **Basic editing commands with unified command interface** [CRITICAL BUG FIXED]
- ✅ **Complete history system with structure, management, file persistence, and navigation**
- ✅ **hist_no_dups enhancement with runtime toggle and move-to-end behavior**
- ✅ **PHASE 2 COMPLETE: All core functionality implemented and tested**
- 🚧 Unicode support and advanced features (Phase 3)
- 🚧 Extensible architecture (Phase 4)

## 🆘 QUICK DEBUG & TESTING
```bash
# Run all LLE tests (300+ tests)
meson test -C builddir

# Run specific test categories
meson test -C builddir test_text_buffer -v          # Text operations
meson test -C builddir test_cursor_math -v          # Mathematical correctness
meson test -C builddir test_lle_020_basic_theme_application -v  # Theme system
meson test -C builddir test_lle_021_key_input_handling -v       # Key input
meson test -C builddir test_lle_022_key_event_processing -v     # Key events
meson test -C builddir test_lle_023_basic_editing_commands -v   # Edit commands (CRITICAL BUG FIXED)
meson test -C builddir test_lle_024_history_structure -v         # History structure
meson test -C builddir test_lle_025_history_management -v        # History management
meson test -C builddir test_lle_026_history_navigation -v        # History navigation
meson test -C builddir test_lle_027_utf8_text_handling -v        # UTF-8 text handling
meson test -C builddir test_lle_028_unicode_cursor_movement -v   # Unicode cursor movement
meson test -C builddir test_lle_hist_no_dups -v                 # hist_no_dups functionality

# Memory leak detection
valgrind --leak-check=full builddir/tests/line_editor/test_lle_027_utf8_text_handling

# Debug specific functionality
export LLE_DEBUG=1
export LLE_TERMCAP_DEBUG=1
gdb builddir/tests/line_editor/test_lle_028_unicode_cursor_movement

# Build from scratch
scripts/lle_build.sh clean && scripts/lle_build.sh setup && scripts/lle_build.sh build
```

## 🔄 DEVELOPMENT PHASES (CURRENT STATUS)
1. **Phase 1: Foundation** ✅ **COMPLETE** (LLE-001 to LLE-014) - Text buffer, cursor math, termcap integration, terminal I/O, testing
2. **Phase 2: Core** ✅ **COMPLETE** (LLE-015 to LLE-026) - Prompts, themes, editing commands **[12/12 DONE]**
3. **Phase 3: Advanced** 🚧 **IN PROGRESS** (LLE-027 to LLE-037) - Unicode, completion, undo/redo, syntax highlighting **[5/11 DONE + hist_no_dups enhancement]**
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
├── text_buffer.c/h             # Text manipulation (LLE-001 to LLE-004, LLE-028)
├── cursor_math.c/h             # Cursor calculations (LLE-005 to LLE-008)
├── terminal_manager.c/h        # Terminal interface (LLE-010, LLE-011)
├── prompt.c/h                  # Complete prompt system (LLE-015 to LLE-017)
├── display.c/h                 # Multiline input display (LLE-018)
├── theme_integration.c/h       # Complete theme system (LLE-019, LLE-020)
├── input_handler.c/h           # Key input and event processing (LLE-021, LLE-022)
├── edit_commands.c/h           # Basic editing commands (LLE-023)
├── command_history.c/h         # Complete history system (LLE-024, LLE-025, LLE-026)
├── unicode.c/h                 # UTF-8 text handling (LLE-027)
├── completion.c/h              # Completion framework and file completion (LLE-029, LLE-030)
├── completion_display.c        # Completion display system (LLE-031)
├── undo.c/h                    # Complete undo/redo system (LLE-032, LLE-033, LLE-034)
├── syntax.c/h                  # Syntax highlighting framework (LLE-035)
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
├── test_lle_024_history_structure.c # LLE-024 tests (8 tests)
├── test_lle_025_history_management.c # LLE-025 tests (13 tests)
├── test_lle_026_history_navigation.c # LLE-026 tests (12 tests)
├── test_lle_027_utf8_text_handling.c # LLE-027 tests (22 tests)
├── test_lle_028_unicode_cursor_movement.c # LLE-028 tests (13 tests)
├── test_lle_029_completion_framework.c # LLE-029 tests (18 tests)
├── test_lle_030_basic_file_completion.c # LLE-030 tests (14 tests)
├── test_lle_031_completion_display.c # LLE-031 tests (13 tests)
├── test_lle_032_undo_stack_structure.c # LLE-032 tests (23 tests)
├── test_lle_033_undo_operation_recording.c # LLE-033 tests (8 tests)
├── test_lle_034_undo_redo_execution.c # LLE-034 tests (12 tests)
├── test_lle_035_syntax_highlighting_framework.c # LLE-035 tests (17 tests)
├── test_lle_hist_no_dups.c     # hist_no_dups tests (15 tests)
├── test_framework.h            # Testing infrastructure
└── meson.build                 # Test configuration
```
</text>

<old_text line=380>
**✅ COMPLETED (9/11 tasks):**
- LLE-027: UTF-8 Text Handling [COMPLETE] - Comprehensive Unicode support with 22 tests
- LLE-028: Unicode Cursor Movement [COMPLETE] - Character-aware navigation with 13 tests  
- LLE-029: Completion Framework [COMPLETE] - Extensible provider architecture with 18 tests
- LLE-030: Basic File Completion [COMPLETE] - File system integration with word extraction with 14 tests
- LLE-031: Completion Display [COMPLETE] - Visual interface with scrolling and navigation with 13 tests
- LLE-032: Undo Stack Structure [COMPLETE] - Comprehensive undo/redo data structures with 23 tests
- LLE-033: Undo Operation Recording [COMPLETE] - Action recording with capacity management with 8 tests
- LLE-034: Undo/Redo Execution [COMPLETE] - Operation reversal with all action types supported with 12 tests
- LLE-035: Syntax Highlighting Framework [COMPLETE] - Complete framework with shell syntax detection with 17 tests

## 🚀 IMMEDIATE DEVELOPER ONBOARDING
**For any developer starting work:**

1. **Current Status**: 70% complete, Phase 1 foundation + Phase 2 COMPLETE + Phase 3 IN PROGRESS
- **Next Task**: LLE-036 (Basic Shell Syntax) - continue Phase 3 advanced features
3. **Key Achievement**: Complete core functionality + Unicode support + completion system + undo/redo system
4. **Test Everything**: `scripts/lle_build.sh test` runs 413+ comprehensive tests
5. **Code Quality**: All code follows strict C99 standards with comprehensive error handling

**🎯 IMMEDIATE PRODUCTIVITY:**
- **Architecture is solid**: Foundation systems are production-ready
- **Unicode support is complete**: Full international text handling
- **Test framework is comprehensive**: 413+ tests cover all functionality
- **Development patterns are established**: Clear examples in 35 completed tasks
- **Next task is well-defined**: LLE-036 has clear 4-hour scope with acceptance criteria

**Phase 1 foundation + Phase 2 core systems + Unicode support + undo/redo system + syntax highlighting framework are COMPLETE.**

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

## 🎉 MAJOR MILESTONE: PHASE 3 ADVANCED FEATURES PROGRESSING
- **Complete Unicode Support**: UTF-8 text handling and character-aware cursor movement
- **Professional hist_no_dups**: Runtime toggle unique history with move-to-end behavior
- **Complete Completion System**: Extensible framework, file completion, visual interface
- **Complete Undo/Redo System**: Full operation recording and execution with all action types
- **Undo/Redo Execution**: Operation reversal, redo capability, cursor position management
- **Complete Syntax Highlighting Framework**: Shell syntax detection with extensible architecture
- **International Text Editing**: Proper navigation for CJK, emojis, accented characters
- **Character Position System**: Seamless byte ↔ character position conversion
- **Unicode Word Boundaries**: International word navigation and selection
- **Advanced Features Foundation**: Complete undo/redo system ready for syntax highlighting
- **413+ Comprehensive Tests**: All systems including complete undo/redo system and syntax highlighting validated
- **Performance Validated**: Sub-millisecond operations including undo/redo execution
- **Zero Memory Leaks**: Valgrind-verified memory management
- **Production Ready**: Professional Unicode-aware line editor with complete undo/redo system

**Phase 3 (Advanced Features) progressing: 9/11 tasks complete + hist_no_dups enhancement.**

## 🔑 CURRENT PHASE 3 STATUS
**✅ COMPLETED (9/11 tasks):**
- **LLE-027 COMPLETED**: UTF-8 Text Handling [COMPLETE] - Comprehensive Unicode support with 22+ tests
- **LLE-028 COMPLETED**: Unicode Cursor Movement [COMPLETE] - Character-aware navigation with 13+ tests
- **LLE-029 COMPLETED**: Completion Framework [COMPLETE] - Extensible provider architecture with 18+ tests
- **LLE-030 COMPLETED**: Basic File Completion [COMPLETE] - File system integration with 14+ tests
- **LLE-031 COMPLETED**: Completion Display [COMPLETE] - Visual interface with scrolling with 13+ tests
- **LLE-032 COMPLETED**: Undo Stack Structure [COMPLETE] - Comprehensive data structures with 23+ tests
- **LLE-033 COMPLETED**: Undo Operation Recording [COMPLETE] - Action recording with capacity management with 8+ tests
- **LLE-034 COMPLETED**: Undo/Redo Execution [COMPLETE] - Operation reversal with all action types supported with 12+ tests
- **LLE-035 COMPLETED**: Syntax Highlighting Framework [COMPLETE] - Complete framework with shell syntax detection with 17+ tests
- **ENHANCEMENT COMPLETED**: hist_no_dups Implementation [COMPLETE] - Runtime toggle unique history with 15+ tests

**🚧 TODO (2/11 tasks):**
- LLE-036: Basic Shell Syntax
- LLE-037: Syntax Display Integration

**📈 PHASE 3 PROGRESS: 81.8% (9/11 tasks) + hist_no_dups enhancement**

**Unicode foundation, hist_no_dups enhancement, completion system, complete undo/redo system, and syntax highlighting framework ready! Next: enhanced shell syntax.**

## 🎯 CURRENT DEVELOPMENT FOCUS (PHASE 3)
**Primary Goal**: Advanced Features Implementation
- **Completion System**: ✅ Complete (framework, file completion, display interface)
- **Syntax Highlighting**: Enhanced shell syntax support and display integration

**Development Priority**: 
1. **LLE-036**: Basic Shell Syntax (4h) - enhanced shell syntax support ← CURRENT
3. **LLE-037**: Syntax Display Integration (3h) - syntax highlighting display

**Key Success Metrics**:
- Maintain sub-millisecond performance for all operations
- Comprehensive test coverage (target: 350+ tests by Phase 3 completion) - currently at 413+
- Zero memory leaks and robust error handling
- International text support throughout all new features
- Professional shell feature parity (hist_no_dups + complete undo/redo + syntax highlighting)

## 🚀 AI DEVELOPMENT CONFIDENCE LEVEL: HIGH (WITH PROPER PREPARATION)

**Any AI assistant can succeed, BUT ONLY if they:**

✅ **Read Documentation First**: `.cursorrules`, `LLE_AI_DEVELOPMENT_GUIDE.md`, task specs  
✅ **Follow Exact Patterns**: Proven architecture with 30 tasks completed consistently  
- **Test Framework**: 413+ tests provide safety net for validation
✅ **Study Existing Code**: Learn from completed implementations  
✅ **Follow Naming Standards**: Exact `lle_component_action` patterns  
✅ **Write Proper Documentation**: Comprehensive Doxygen comments required  
✅ **Test Comprehensively**: Edge cases, error conditions, parameter validation  

**Why Success is Guaranteed (When Standards Are Followed):**
- **Clear Task Specifications**: Each task has detailed acceptance criteria
- **Established Patterns**: Consistent code patterns across all components
- **Professional Features**: Complex systems (Unicode, completion, undo operation recording, syntax highlighting) already working
- **Comprehensive Testing**: Safety net prevents regressions
- **Build Integration**: Meson system handles all dependencies

**Estimated Time to Productivity**: 
- 30 minutes reading documentation (REQUIRED)
- 2-4 hours per task implementation (following patterns)

## 🚨 SUCCESS REQUIRES DISCIPLINE

**AI assistants who skip documentation or violate standards WILL FAIL**  
**AI assistants who read documentation and follow patterns WILL SUCCEED**

**Current Next Task**: LLE-036 (Basic Shell Syntax) - Enhance shell syntax detection with additional constructs, built-in commands, and advanced parsing capabilities.
