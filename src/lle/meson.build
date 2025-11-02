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
# LUSUSH MEMORY POOL (Foundation Dependency)
# ============================================================================
# LLE memory management depends on Lusush memory pool system
# Must be included before any LLE components that use memory

lle_sources += files('../lusush_memory_pool.c')

# ============================================================================
# LIBHASHTABLE INTEGRATION (Spec 05)
# ============================================================================
# libhashtable is the exclusive hashtable solution for all LLE components.
# Per Spec 05, integrate libhashtable sources directly into liblle.a

libhashtable_root = '../libhashtable'
lle_sources += files(
  libhashtable_root + '/ht.c',
  libhashtable_root + '/ht_fnv1a.c',
  libhashtable_root + '/ht_strdouble.c',
  libhashtable_root + '/ht_strfloat.c',
  libhashtable_root + '/ht_strint.c',
  libhashtable_root + '/ht_strstr.c',
)

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
# Provides: Terminal capability detection, state management, display generation
# Depends on: Spec 16, 15, 14, 17
# Subsystem 1: Terminal capability detection
if fs.exists('terminal_capabilities.c')
  lle_sources += files('terminal_capabilities.c')
endif
# Subsystem 2: Internal State Authority Model
if fs.exists('terminal_internal_state.c')
  lle_sources += files('terminal_internal_state.c')
endif
# Subsystem 3: Display Content Generation
if fs.exists('terminal_display_generator.c')
  lle_sources += files('terminal_display_generator.c')
endif
# Subsystem 4: Lusush Display Integration
if fs.exists('terminal_lusush_client.c')
  lle_sources += files('terminal_lusush_client.c')
endif
# Subsystem 5: Input Event Processing
if fs.exists('terminal_input_processor.c')
  lle_sources += files('terminal_input_processor.c')
endif
# Subsystem 6: Unix Terminal Interface (renamed from terminal_state.c)
if fs.exists('terminal_unix_interface.c')
  lle_sources += files('terminal_unix_interface.c')
endif
# Subsystem 7: Error Handling
if fs.exists('terminal_error_handler.c')
  lle_sources += files('terminal_error_handler.c')
endif
# Subsystem 8: Performance Monitoring
if fs.exists('terminal_perf_monitor.c')
  lle_sources += files('terminal_perf_monitor.c')
endif
# Main Terminal Abstraction
if fs.exists('terminal_abstraction.c')
  lle_sources += files('terminal_abstraction.c')
endif

# Spec 04: Event System - Phase 1 + Phase 2
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
# Phase 2B: Enhanced statistics and processing control
if fs.exists('event_stats.c')
  lle_sources += files('event_stats.c')
endif
# Phase 2C: Event filtering and system state
if fs.exists('event_filter.c')
  lle_sources += files('event_filter.c')
endif
# Phase 2D: Timer events
if fs.exists('event_timer.c')
  lle_sources += files('event_timer.c')
endif

# Spec 05: libhashtable Integration
# Provides: Hash table data structure for caching/lookup
# Depends on: Spec 16, 15, 14, 17
if fs.exists('hashtable.c')
  lle_sources += files('hashtable.c')
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
#   - utf8_index.c: Fast O(1) UTF-8 position mapping (byte/codepoint/grapheme)
#   - buffer_validator.c: Buffer integrity validation (UTF-8, bounds, structure)
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
if fs.exists('utf8_index.c')
  lle_sources += files('utf8_index.c')
endif
if fs.exists('buffer_validator.c')
  lle_sources += files('buffer_validator.c')
endif
if fs.exists('buffer_management.c')
  lle_sources += files('buffer_management.c')
endif
if fs.exists('multiline_manager.c')
  lle_sources += files('multiline_manager.c')
endif

# Spec 08: Display Integration
# Provides: Display rendering through Lusush, cursor management
# Depends on: Spec 16, 15, 14, 17, 02, 03, 04
# Phase 1: Foundation - Display Bridge and Event Coordination
if fs.exists('display_bridge.c')
  lle_sources += files('display_bridge.c')
endif
if fs.exists('event_coordinator.c')
  lle_sources += files('event_coordinator.c')
endif
# Phase 2: Rendering System - Render Controller
if fs.exists('render_controller.c')
  lle_sources += files('render_controller.c')
endif

# Phase 2: Rendering System - Rendering Pipeline
if fs.exists('render_pipeline.c')
  lle_sources += files('render_pipeline.c')
endif

# Phase 2: Rendering System - Render Cache (libhashtable integration per Spec 05)
if fs.exists('render_cache.c')
  lle_sources += files('render_cache.c')
endif

# Phase 2: Rendering System - Dirty Region Tracking
if fs.exists('dirty_tracker.c')
  lle_sources += files('dirty_tracker.c')
endif

# Phase 2: Rendering System - Terminal Adapter
if fs.exists('terminal_adapter.c')
  lle_sources += files('terminal_adapter.c')
endif

# Phase 2: Rendering System - Theme Integration
if fs.exists('theme_integration.c')
  lle_sources += files('theme_integration.c')
endif

# ============================================================================
# PHASE 3: INPUT AND INTEGRATION
# ============================================================================
# Implement after Phase 2 complete and tested.

# Spec 06: Input Parsing
# Provides: Input stream parsing, key sequence detection, mouse input
# Depends on: Spec 16, 15, 14, 17, 02, 04
# Phase 1: Input Stream Management
if fs.exists('input_stream.c')
  lle_sources += files('input_stream.c')
endif
# Phase 2: UTF-8 Processor (will reuse utf8_support.c from Spec 03)
if fs.exists('input_utf8_processor.c')
  lle_sources += files('input_utf8_processor.c')
endif
# Phase 3: Terminal Sequence Parser
if fs.exists('sequence_parser.c')
  lle_sources += files('sequence_parser.c')
endif
# Phase 4: Key Sequence Detector
if fs.exists('key_detector.c')
  lle_sources += files('key_detector.c')
endif
# Phase 5: Mouse Parser
if fs.exists('mouse_parser.c')
  lle_sources += files('mouse_parser.c')
endif
# Phase 6: Parser State Machine
if fs.exists('parser_state_machine.c')
  lle_sources += files('parser_state_machine.c')
endif
# Phase 7-9: Integration (combined file)
if fs.exists('input_parser_integration.c')
  lle_sources += files('input_parser_integration.c')
endif

if fs.exists('input_keybinding_integration.c')
  lle_sources += files('input_keybinding_integration.c')
endif

if fs.exists('input_widget_hooks.c')
  lle_sources += files('input_widget_hooks.c')
endif

if fs.exists('input_parser_error_recovery.c')
  lle_sources += files('input_parser_error_recovery.c')
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
# Phase 1: Core engine, indexing, persistence
if fs.exists('history_core.c')
  lle_sources += files('history_core.c')
endif
if fs.exists('history_index.c')
  lle_sources += files('history_index.c')
endif
if fs.exists('history_storage.c')
  lle_sources += files('history_storage.c')
endif
# Phase 2: Lusush integration
if fs.exists('history_lusush_bridge.c')
  lle_sources += files('history_lusush_bridge.c')
endif
if fs.exists('history_events.c')
  lle_sources += files('history_events.c')
endif
# Phase 3: Search and navigation
if fs.exists('history_search.c')
  lle_sources += files('history_search.c')
endif
if fs.exists('history_interactive_search.c')
  lle_sources += files('history_interactive_search.c')
endif
if fs.exists('history_expansion.c')
  lle_sources += files('history_expansion.c')
endif
if fs.exists('history_navigation.c')
  lle_sources += files('history_navigation.c')
endif
# Critical Gap Spec 22: History-Buffer Integration
if fs.exists('history_buffer_integration.c')
  lle_sources += files('history_buffer_integration.c')
endif
# Spec 22 Phase 2: Multiline Reconstruction Engine
if fs.exists('command_structure.c')
  lle_sources += files('command_structure.c')
endif
if fs.exists('structure_analyzer.c')
  lle_sources += files('structure_analyzer.c')
endif
if fs.exists('multiline_parser.c')
  lle_sources += files('multiline_parser.c')
endif
if fs.exists('reconstruction_engine.c')
  lle_sources += files('reconstruction_engine.c')
endif
if fs.exists('formatting_engine.c')
  lle_sources += files('formatting_engine.c')
endif
# Spec 22 Phase 3: Interactive Editing System
if fs.exists('edit_session_manager.c')
  lle_sources += files('edit_session_manager.c')
endif
if fs.exists('history_buffer_bridge.c')
  lle_sources += files('history_buffer_bridge.c')
endif
# Spec 22 Phase 4: Performance Optimization
if fs.exists('edit_cache.c')
  lle_sources += files('edit_cache.c')
endif
# Phase 4: Advanced features
if fs.exists('history_forensics.c')
  lle_sources += files('history_forensics.c')
endif
if fs.exists('history_dedup.c')
  lle_sources += files('history_dedup.c')
endif
if fs.exists('history_multiline.c')
  lle_sources += files('history_multiline.c')
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
# LLE READLINE - CORE ORCHESTRATION
# ============================================================================
# The main readline function that ties all subsystems together
# Depends on: Spec 02, 03, 04, 08, 16, 15, 14, 17
if fs.exists('lle_readline.c')
  lle_sources += files('lle_readline.c')
  # lle_readline uses Lusush display system and other utilities
  # These are already built into the lusush executable, so we don't duplicate them here
  # The LLE library will link against the full lusush at link time
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
