# LLE AI Development Guide - Instant Context for Modern Editors

## 🚀 Quick Start for AI-Assisted Development

This guide provides immediate context for any AI assistant to help with Lusush Line Editor (LLE) development. Read this file first to understand the project structure, current state, and how to contribute effectively.

## 📋 Project Status at a Glance

- **Current Phase**: Integration & Polish (Phase 4 of 4) - Ready to begin
- **Active Branch**: `feature/lusush-line-editor`
- **Build System**: Meson (NOT Make)
- **Language**: C99 with strict standards
- **Test Framework**: Custom LLE test framework
- **Progress Tracking**: `LLE_PROGRESS.md`
- **Architecture**: Standalone library with integrated termcap system
- **Tasks Completed**: 40/50 + 1 Enhancement (80% overall progress)

## 🎯 What is LLE?

The Lusush Line Editor (LLE) is a standalone, reusable library replacement for linenoise that provides:
- **Perfect multiline prompt support** with mathematical correctness
- **Professional terminal capabilities** via integrated termcap system
- **Complete iTerm2 and macOS support** transferred from Lusush
- **Sub-millisecond response times** for all operations
- **Complete theme integration** with 18 visual elements and fallback colors
- **Comprehensive key input handling** with 60+ key types and modifier support
- **Professional-grade Unicode support** with international text editing
- **Complete undo/redo system** with operation recording and execution
- **Visual syntax highlighting** with real-time display integration
- **Complete main line editor functionality** with Unix signal separation
- **Refactored input event loop** with enhanced error handling and code organization
- **Standard readline keybindings** with proper control character handling
- **Standalone reusability** like libhashtable for other projects

## 📁 Critical Files for AI Context

### **Immediate Reading Priority**
1. `LLE_PROGRESS.md` - Current task status and what to work on next
2. `LLE_DEVELOPMENT_TASKS.md` - Complete breakdown of 50 atomic tasks
3. `.cursorrules` - Code standards and development patterns
4. `LINE_EDITOR_STRATEGIC_ANALYSIS.md` - Why we're building this

### **Implementation Files**
- `src/line_editor/` - All LLE source code (start here for implementation)
- `tests/line_editor/` - Test suite (write tests alongside code)
- `scripts/lle_build.sh` - Build helper script (use this for builds)

### **Reference Documentation**
- `LLE_DEVELOPMENT_WORKFLOW.md` - Git workflow and quality standards
- `docs/line_editor/` - API documentation (create as you go)

## 🚨 CRITICAL: Control Character Handling

**MUST UNDERSTAND - Unix Signal Separation:**

LLE implements proper separation of concerns for control characters:

**Signal Characters (Shell Domain - NEVER INTERCEPT):**
- `Ctrl+C` (SIGINT) - Let shell handle interrupt signal
- `Ctrl+\` (SIGQUIT) - Let shell handle quit signal  
- `Ctrl+Z` (SIGTSTP) - Let shell handle job control

**Line Editing Characters (LLE Domain - HANDLE HERE):**
- `Ctrl+G` (0x07) - Abort/cancel line (standard readline)
- `Ctrl+_` (0x1F) - Undo (standard readline)
- `Ctrl+A/E/K/U/W/H/D/L/Y` - Standard editing functions

**Code Pattern:**
```c
case LLE_KEY_CHAR:
    if (event.character == LLE_ASCII_CTRL_G) {
        line_cancelled = true;  // LLE handles abort
    }
    else if (event.character == LLE_ASCII_CTRL_BACKSLASH) {
        needs_display_update = false;  // Let shell handle SIGQUIT
    }
```

**Reference**: `LLE_CONTROL_CHARACTER_DESIGN.md` for complete technical details.

## 🔧 Essential Commands

### **Build System (Meson-based)**
```bash
# First-time setup
scripts/lle_build.sh setup

# Build LLE components
scripts/lle_build.sh build

# Run all tests
scripts/lle_build.sh test

# Run specific test
meson test -C builddir test_text_buffer

# Performance benchmarks
scripts/lle_build.sh benchmark

# Clean rebuild
scripts/lle_build.sh clean && scripts/lle_build.sh setup
```

### **Development Workflow**
```bash
# Check current task
cat LLE_PROGRESS.md | grep -A 3 "TODO"

# Create task branch
git checkout -b task/lle-XXX-description

# Update progress when done
scripts/complete_task.sh XXX

# Update progress summary
scripts/update_progress.sh
```

## 📐 Architecture Overview

### **Core Components (✅ = Completed)**
```
LLE Architecture (Standalone Library):
├── Text Engine       (text_buffer.c/h)      ✅ UTF-8 text manipulation
├── Cursor Math       (cursor_math.c/h)      ✅ Position calculations  
├── Terminal System   (termcap/)             ✅ Complete terminal management
│   ├── lle_termcap.c/h                      ✅ Core termcap (from Lusush)
│   ├── lle_termcap_internal.h              ✅ Internal structures
│   └── lle_termcap_database.c              ✅ Terminal profiles (iTerm2+)
├── Terminal Manager  (terminal_manager.c/h) ✅ LLE terminal interface
├── Prompt System     (prompt.c/h)           ✅ Multiline prompt support
├── Display System    (display.c/h)          ✅ Rendering and updates
├── Input Handler     (input_handler.c/h)    🚧 Key processing
├── Theme Integration (theme_integration.c/h) 🚧 Theme support  
├── History System    (history.c/h)          📋 Command history
├── Completion Engine (completion.c/h)       📋 Tab completion
├── Undo/Redo Stack   (undo.c/h)            📋 Operation reversal
└── Main API         (line_editor.c/h)      📋 Public interface
```

### **Development Phases**
1. **Phase 1 (Weeks 1-2)**: ✅ Foundation - Text buffer, cursor math, termcap integration
2. **Phase 2 (Weeks 3-4)**: 🚧 Core functionality - Prompts, themes, basic editing (58% complete)
3. **Phase 3 (Weeks 5-6)**: 📋 Advanced features - Unicode, completion, undo/redo
4. **Phase 4 (Weeks 7-8)**: 📋 Integration & polish - API, optimization, docs

### **Major Achievements**
- ✅ **Phase 1 Foundation Complete**: 87+ tests, comprehensive text/cursor/terminal systems
- ✅ **Integrated Termcap System**: 2000+ lines, 118+ functions, 50+ terminal profiles
- ✅ **Professional Prompt System**: Multiline prompts with ANSI support and 67 API functions
- ✅ **Complete Display System**: Multiline input display with cursor positioning
- ✅ **Complete Theme System**: Standalone integration with 18 visual elements and fallback colors
- ✅ **Comprehensive Key Input System**: 60+ key types with modifier support and event structures
- ✅ **Complete Unicode Support System**: UTF-8 text handling and character-aware cursor movement
- ✅ **Complete Completion System**: Extensible framework, file completion, and visual interface
- ✅ **Complete Undo/Redo System**: Full operation recording and execution with all action types
- 🚧 **Syntax Highlighting Framework**: Next task - LLE-035 Basic syntax highlighting support

## 💻 Code Standards Quick Reference

### **Naming Conventions**
```c
// Functions: lle_component_action
bool lle_text_insert_char(lle_text_buffer_t *buffer, char c);

// Structures: lle_component_t
typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
} lle_text_buffer_t;

// Constants: LLE_CONSTANT_NAME
#define LLE_DEFAULT_BUFFER_SIZE 256
```

### **Error Handling Pattern**
```c
bool lle_function(args) {
    // Always validate inputs
    if (!args) return false;
    
    // Check preconditions
    if (invalid_state) return false;
    
    // Perform operation
    if (!operation_success) return false;
    
    // Success
    return true;
}
```

### **Memory Management Pattern**
```c
// Creation pattern
lle_component_t *lle_component_create(size_t capacity) {
    lle_component_t *comp = malloc(sizeof(lle_component_t));
    if (!comp) return NULL;
    
    comp->buffer = malloc(capacity);
    if (!comp->buffer) {
        free(comp);
        return NULL;
    }
    
    // Initialize other fields
    return comp;
}

// Destruction pattern
void lle_component_destroy(lle_component_t *comp) {
    if (!comp) return;
    
    free(comp->buffer);
    free(comp);
}
```

## 🧪 Testing Requirements

### **Test Coverage Standards**
- **Unit Tests**: Every public function must have tests
- **Edge Cases**: Test boundary conditions and error states
- **Integration Tests**: Test component interactions
- **Performance Tests**: Validate response time requirements
- **Memory Tests**: No leaks, proper cleanup

### **Test Writing Pattern**
```c
LLE_TEST(component_operation) {
    // Setup
    lle_component_t *comp = lle_component_create(256);
    LLE_ASSERT_NOT_NULL(comp);
    
    // Test normal case
    LLE_ASSERT(lle_component_operation(comp, valid_data));
    
    // Test edge cases
    LLE_ASSERT(!lle_component_operation(comp, NULL));
    LLE_ASSERT(!lle_component_operation(NULL, valid_data));
    
    // Verify state
    LLE_ASSERT_EQ(comp->expected_field, expected_value);
    
    // Cleanup
    lle_component_destroy(comp);
}
```

## 🎯 Task Selection Guide

### **How to Pick Your Next Task**

1. **Check Progress**: Look at `LLE_PROGRESS.md` for next TODO task
2. **Read Task Spec**: Find the task in `LLE_DEVELOPMENT_TASKS.md`
3. **Understand Context**: Read the task's dependencies and requirements
4. **Estimate Time**: Most tasks are 2-4 hours of focused work

### **Task Completion Checklist**
- [ ] Read task requirements thoroughly
- [ ] Implement functionality following code standards
- [ ] Write comprehensive tests
- [ ] Run test suite and ensure all tests pass
- [ ] Update documentation if needed
- [ ] Commit with proper message format
- [ ] Update progress tracking

## 🔍 Debugging and Troubleshooting

### **Common Issues and Solutions**

#### **Build Issues**
```bash
# Clean rebuild
scripts/lle_build.sh clean
scripts/lle_build.sh setup
scripts/lle_build.sh build
```

#### **Test Failures**
```bash
# Run tests with verbose output
scripts/lle_build.sh test-verbose

# Run specific failing test
meson test -C builddir test_name --verbose
```

#### **Memory Issues**
```bash
# Check for memory leaks
valgrind --leak-check=full builddir/lusush

# Debug with GDB
gdb builddir/lusush
(gdb) set environment LLE_DEBUG=1
(gdb) run
```

#### **Performance Issues**
```bash
# Run benchmarks
scripts/lle_build.sh benchmark

# Profile with perf
perf record builddir/lusush
perf report
```

### **Debug Environment Variables**
```bash
export LLE_DEBUG=1              # General debug output
export LLE_DEBUG_CURSOR=1       # Cursor calculation debug
export LLE_DEBUG_MEMORY=1       # Memory allocation debug
export LLE_DEBUG_TERMINAL=1     # Terminal operation debug
```

## 🚀 Getting Started in Any Editor

### **Current Development Status (LLE-041 + Enhanced POSIX History Complete)**
- **Next Task**: LLE-042 (Theme System Integration) - 3 hours estimated
- **Progress**: 41/50 tasks complete (82%), Phase 4: 4/13 tasks (31% COMPLETE) + 2 Major Enhancements COMPLETE
- **Foundation**: Rock-solid with 479+ tests covering all implemented functionality + Professional POSIX history management + Complete linenoise replacement

### **Immediate Next Task: LLE-042 Theme System Integration**
**File**: `src/line_editor/theme_integration.c`
**Goal**: Complete integration with Lusush theme system
**Status**: Ready to start - all dependencies (LLE-038 through LLE-041 + Enhanced POSIX History) complete
**Key Features to Implement**:
```c
// Connect to Lusush theme change events
// Implement real-time theme updates
// Ensure all colors properly themed
// Add theme-specific line editor settings
```
**Acceptance Criteria**:
- Responds to theme changes immediately
- All elements properly themed
- Performance maintained
- Consistent with Lusush theming

### **For Cursor/VS Code Users**
1. Open the lusush repository
2. Read this file (you're doing it!)
3. Check `LLE_PROGRESS.md` for current task (LLE-042)
4. Note: LLE-041 + Enhanced POSIX History are COMPLETE
5. Ask AI: "Help me implement LLE-042 Theme System Integration"

### **For Other AI-Assisted Editors**
1. Load the repository context
2. Reference `.cursorrules` for coding standards
3. Use `LLE_DEVELOPMENT_TASKS.md` for LLE-039 specifications
4. Follow the build commands in this guide

### **Context Loading for AI**
When starting a new session, provide your AI with:
1. This file (`LLE_AI_DEVELOPMENT_GUIDE.md`)
2. Current progress (`LLE_PROGRESS.md`)
3. Task LLE-039 requirements from `LLE_DEVELOPMENT_TASKS.md`
4. Code standards from `.cursorrules`
5. Recent completion summaries: `LLE-036_COMPLETION_SUMMARY.md`, `LLE-037_COMPLETION_SUMMARY.md`, `LLE-038_COMPLETION_SUMMARY.md`

## 📊 Performance Requirements

### **Response Time Targets**
- Character insertion: < 1ms
- Cursor movement: < 1ms
- Display update: < 5ms
- History search: < 10ms
- Tab completion: < 50ms
- Terminal capability detection: < 50ms (cached)

### **Memory Usage Targets**
- Base memory: < 1MB
- Per-character overhead: < 50 bytes
- History entry: < 1KB average
- Completion cache: < 5MB

### **Scalability Targets**
- Text length: Up to 100KB
- History entries: Up to 10,000
- Completion items: Up to 1,000
- Terminal width: Up to 500 characters
- Terminal database: 50+ terminal profiles (including iTerm2)

## 🔗 Integration Points

### **Lusush Integration**
- **Theme System**: `src/themes.c` - LLE must integrate with existing themes
- **Terminal Capabilities**: `src/termcap.c` - LLE uses enhanced termcap
- **Configuration**: `src/config.c` - LLE respects user settings
- **Main Shell**: `src/lusush.c` - LLE will replace linenoise calls

### **Build System Integration**
- LLE builds as completely standalone static library (`liblle.a`)
- Includes integrated termcap system (no external termcap dependency)
- Lusush links against LLE library for line editing
- Other projects can use LLE library independently
- Meson handles all internal LLE dependency management

## 🎯 Success Metrics
## 🎯 Success Criteria

### **Definition of Done for Each Task**
- [ ] Implementation meets all acceptance criteria
- [ ] All tests pass (unit, integration, edge cases)
- [ ] Performance meets requirements
- [ ] Memory management verified (no leaks)
- [ ] Code follows standards and is documented
- [ ] Integration points validated
- [ ] Termcap compatibility maintained

### **Project Success Criteria**
- ✅ Perfect multiline prompt support across all terminals
- ✅ Sub-millisecond response times for all operations
- ✅ Zero mathematical errors in cursor positioning
- ✅ Complete terminal compatibility (including iTerm2/macOS)
- ✅ Standalone library reusability for other projects
- 🚧 100% theme integration with visual consistency (LLE-019/020)
- ✅ Extensible architecture for future enhancements
- 🚧 Complete documentation and test coverage (ongoing)

### **Current Achievements**
- ✅ **Professional Foundation**: Complete text buffer, cursor math, terminal systems
- ✅ **Integrated Termcap**: Enhanced termcap library with 50+ terminal profiles
- ✅ **Multiline Prompts**: Full ANSI support with mathematical precision
- ✅ **Display System**: Complete multiline input display with cursor positioning
- ✅ **Complete Unicode Support System**: UTF-8 text handling and character-aware cursor movement
- ✅ **Complete Completion System**: Extensible framework with file completion and display
- ✅ **Complete Undo/Redo System**: Full operation recording and execution capabilities
- ✅ **Complete Enhanced Syntax Highlighting**: Comprehensive shell syntax with built-ins, command substitution, parameter expansion, redirection operators, and number recognition
- ✅ **479+ Tests**: Comprehensive test coverage with Valgrind-verified memory safety

## 💡 Pro Tips for AI-Assisted Development

### **Effective Prompts**
- "Help me implement LLE task XXX following the code standards"
- "Write tests for this LLE function following the test patterns"
- "Debug this cursor calculation issue in LLE"
- "Optimize this LLE function for performance"

### **Context Management**
- Always reference the specific task from `LLE_DEVELOPMENT_TASKS.md`
- Include relevant code standards from `.cursorrules`
- Mention performance requirements for optimization tasks
- Reference existing code patterns for consistency

### **Quality Assurance**
- Ask AI to review code against the standards
- Request test coverage analysis
- Validate performance implications
- Check for memory management correctness

## 🆘 Emergency Procedures

### **If Development Gets Stuck**
1. **Check Dependencies**: Ensure all prerequisite tasks are complete
2. **Review Requirements**: Re-read task specification carefully
3. **Run Diagnostics**: Use debug environment variables
4. **Validate Environment**: Ensure build system is working
5. **Check Git State**: Verify you're on the right branch

### **If Tests Are Failing**
1. **Isolate Failure**: Run specific failing test with verbose output
2. **Check Implementation**: Compare against task acceptance criteria
3. **Validate Test**: Ensure test follows LLE testing patterns
4. **Debug Step-by-Step**: Use GDB or debug prints
5. **Consider Rollback**: Return to last known good state if needed

### **If Performance Issues Arise**
1. **Run Benchmarks**: Use `scripts/lle_build.sh benchmark`
2. **Profile Code**: Use perf or other profiling tools
3. **Review Algorithm**: Check for O(n²) operations
4. **Optimize Hot Paths**: Focus on frequently called functions
5. **Memory Usage**: Check for excessive allocations

This guide provides everything needed for any developer with AI assistance to immediately start contributing to LLE development effectively and efficiently.