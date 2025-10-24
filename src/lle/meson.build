# src/lle/meson.build
# LLE (Lusush Line Editor) Build Configuration
#
# This file defines the source files for the LLE static library.
# New modules are automatically detected via fs.exists() checks.
#
# LAYERED IMPLEMENTATION STRATEGY:
# - Layer 0: Only headers exist (.h files only)
#   Result: lle_sources empty, library not built
#
# - Layer 1: Individual .c files created (WON'T COMPILE ALONE)
#   DO NOT attempt to build during Layer 1!
#   Source files reference functions from other specs that don't exist yet.
#   This is expected - circular dependencies resolve in Layer 2.
#
# - Layer 2: ALL Phase 0 .c files exist simultaneously
#   Result: lle_sources has all 4 files, library builds, dependencies resolve
#
# Add files in phase order (Phase 0 -> Phase 1 -> ... -> Phase 5)

# ============================================================================
# LLE Compilation Flags
# ============================================================================
# Define feature test macros for POSIX/XOPEN functionality
# These enable pthread_rwlock_t and other POSIX types used in LLE headers

lle_c_args = [
  '-D_DEFAULT_SOURCE',
  '-D_XOPEN_SOURCE=700',
  '-D_XOPEN_SOURCE_EXTENDED',
  '-D_POSIX_C_SOURCE=200809L',
]

# LLE source files - add incrementally as specs are implemented
lle_sources = []

# ============================================================================
# PHASE 0: FOUNDATION (IMPLEMENT FIRST)
# ============================================================================
# These define foundational types used by ALL other specs.
# Must be completed before ANY other implementation.

# Spec 16: Error Handling
# Defines: lle_result_t, lle_error_context_t
# Used by: ALL specs
if fs.exists('error_handling.c')
  lle_sources += files('error_handling.c')
endif

# Spec 15: Memory Management
# Defines: lusush_memory_pool_t
# Depends on: Spec 16
# Used by: ALL specs
if fs.exists('memory_management.c')
  lle_sources += files('memory_management.c')
endif

# Spec 14: Performance Optimization
# Defines: lle_performance_monitor_t, lle_performance_context_t
# Depends on: Spec 16, 15
# Used by: ALL specs
if fs.exists('performance.c')
  lle_sources += files('performance.c')
endif

# Spec 17: Testing Framework
# Provides: Testing infrastructure for all specs
# Depends on: Spec 16, 15, 14
# Used by: ALL specs
if fs.exists('testing.c')
  lle_sources += files('testing.c')
endif

# ============================================================================
# PHASE 1: CORE SYSTEMS
# ============================================================================
# Implement after Phase 0 complete and tested.

# Spec 02: Terminal Abstraction
# Provides: Terminal capability detection, state management
# Depends on: Spec 16, 15, 14, 17
if fs.exists('src/lle/terminal.c')
  lle_sources += 'terminal.c'
endif

# Spec 04: Event System - Phase 1
# Provides: Core event infrastructure, queue, handlers, dispatch
# Depends on: Spec 16, 15
# Phase 1: Basic event system (3 files)
if fs.exists('event_system.c')
  lle_sources += files('event_system.c')
endif
if fs.exists('event_queue.c')
  lle_sources += files('event_queue.c')
endif
if fs.exists('event_handlers.c')
  lle_sources += files('event_handlers.c')
endif

# Spec 05: libhashtable Integration
# Provides: Hash table data structure for caching/lookup
# Depends on: Spec 16, 15, 14, 17
if fs.exists('src/lle/hashtable_integration.c')
  lle_sources += 'hashtable_integration.c'
endif

# ============================================================================
# PHASE 2: BUFFER AND DISPLAY
# ============================================================================
# Implement after Phase 1 complete and tested.

# Spec 03: Buffer Management
# Provides: Complete buffer management system
# Depends on: Spec 16, 15, 14
# Supporting modules:
#   - utf8_support.c: Complete UTF-8 encoding/decoding and validation
#   - unicode_grapheme.c: Unicode TR#29 grapheme cluster boundary detection
#   - change_tracker.c: Complete undo/redo with atomic operation tracking
#   - cursor_manager.c: Complete cursor position tracking and movement
#   - buffer_management.c: Complete buffer system with all subsystems
if fs.exists('utf8_support.c')
  lle_sources += files('utf8_support.c')
endif
if fs.exists('unicode_grapheme.c')
  lle_sources += files('unicode_grapheme.c')
endif
if fs.exists('change_tracker.c')
  lle_sources += files('change_tracker.c')
endif
if fs.exists('cursor_manager.c')
  lle_sources += files('cursor_manager.c')
endif
if fs.exists('buffer_management.c')
  lle_sources += files('buffer_management.c')
endif

# Spec 08: Display Integration
# Provides: Display rendering through Lusush, cursor management
# Depends on: Spec 16, 15, 14, 17, 02, 03, 04
if fs.exists('src/lle/display.c')
  lle_sources += 'display.c'
endif

# ============================================================================
# PHASE 3: INPUT AND INTEGRATION
# ============================================================================
# Implement after Phase 2 complete and tested.

# Spec 06: Input Parsing
# Provides: Input stream parsing, key sequence detection, mouse input
# Depends on: Spec 16, 15, 14, 17, 02, 04
if fs.exists('src/lle/input_parsing.c')
  lle_sources += 'input_parsing.c'
endif

# Spec 07: Extensibility Framework
# Provides: Plugin system, widget hooks, extension API
# Depends on: Spec 16, 15, 14, 17, 04
if fs.exists('src/lle/extensibility.c')
  lle_sources += 'extensibility.c'
endif

# ============================================================================
# PHASE 4: FEATURES
# ============================================================================
# Implement after Phase 3 complete and tested.

# Spec 09: History System
# Provides: Command history, persistent storage, history search
# Depends on: Spec 16, 15, 14, 17, 03, 05
if fs.exists('src/lle/history.c')
  lle_sources += 'history.c'
endif

# Spec 11: Syntax Highlighting
# Provides: Real-time syntax highlighting, token-based coloring
# Depends on: Spec 16, 15, 14, 17, 03, 08
if fs.exists('src/lle/syntax_highlighting.c')
  lle_sources += 'syntax_highlighting.c'
endif

# Spec 12: Completion System
# Provides: Command/path completion, completion menu
# Depends on: Spec 16, 15, 14, 17, 03, 05
if fs.exists('src/lle/completion.c')
  lle_sources += 'completion.c'
endif

# Spec 10: Autosuggestions
# Provides: Intelligent command suggestions
# Depends on: Spec 16, 15, 14, 17, 09, 12
if fs.exists('src/lle/autosuggestions.c')
  lle_sources += 'autosuggestions.c'
endif

# Spec 13: User Customization
# Provides: User configuration, keybindings, themes
# Depends on: Spec 16, 15, 14, 17, 07
if fs.exists('src/lle/customization.c')
  lle_sources += 'customization.c'
endif

# ============================================================================
# PHASE 5: INFRASTRUCTURE
# ============================================================================
# Implement after Phase 4 complete and tested.

# Spec 18: Plugin API
# Provides: Stable plugin API, plugin development SDK
# Depends on: Spec 16, 15, 14, 17, 07
if fs.exists('src/lle/plugin_api.c')
  lle_sources += 'plugin_api.c'
endif

# Spec 19: Security Analysis
# Provides: Security framework, sandboxing, audit logging
# Depends on: Spec 16, 15, 14, 17, ALL other specs
if fs.exists('src/lle/security.c')
  lle_sources += 'security.c'
endif

# ============================================================================
# Export lle_sources to parent scope
# ============================================================================
# The root meson.build will use this to build liblle.a static library
