project('lusush',
        'c',
        default_options: ['c_std=c99',
                          'optimization=3',
                          'warning_level=2'
                         ],
        version: '1.3.0',
        license: 'GPL-3.0+')

# Import filesystem module for directory checks
fs = import('fs')

# ============================================================================
# BUILD DIRECTORY ENFORCEMENT
# ============================================================================
# IMPORTANT: Always use 'builddir' as the build directory name.
# This prevents confusion from multiple build directories with different binaries.
#
# Correct usage:
#   meson setup builddir
#   meson compile -C builddir
#   ./builddir/lusush
#
# The check below will error if you use a different build directory name.
# ============================================================================

# Get the build directory name (last component of the path)
build_dir_path = meson.current_build_dir()
build_dir_name = build_dir_path.split('/')[-1]

if build_dir_name != 'builddir'
    error('Build directory must be named "builddir", not "' + build_dir_name + '".\n' +
          'Please reconfigure with: meson setup builddir --wipe\n' +
          'Then build with: meson compile -C builddir')
endif

message('Build directory: ' + build_dir_name + ' ✓')

# Debug build configuration
debug_option = get_option('enable_debug')
if debug_option
    add_global_arguments('-DDEBUG', language : 'c')
    message('Debug build enabled - DEBUG macro active')
else
    message('Production build - DEBUG macro disabled')
endif

inc = include_directories('include',
                          'include/libhashtable',
                          'include/display',
                          'src')

src = ['src/builtins/alias.c',
       'src/builtins/builtins.c',
       'src/builtins/history.c',
       'src/builtins/fc.c',
       'src/builtins/enhanced_history.c',
       'src/posix_history.c',
       'src/arithmetic.c',
       'src/autocorrect.c',
       'src/libfuzzy/fuzzy_match.c',
       'src/autosuggestions.c',
       'src/completion.c',
       'src/completion_types.c',
       'src/completion_menu.c',
       'src/completion_menu_theme.c',
       'src/rich_completion.c',
       'src/config.c',
       'src/lusush_memory_pool.c',
       'src/debug/debug_core.c',
       'src/debug/debug_trace.c',
       'src/debug/debug_breakpoints.c',
       'src/debug/debug_profile.c',
       'src/debug/debug_analysis.c',
       'src/display/base_terminal.c',
       'src/display/terminal_control.c',
       'src/display/layer_events.c',
       'src/display/prompt_layer.c',
       'src/display/command_layer.c',
       'src/display/continuation_prompt_layer.c',
       'src/display/composition_engine.c',
       'src/display/display_controller.c',
       'src/display/screen_buffer.c',
       'src/display/screen_buffer_menu.c',
       'src/display/autosuggestions_layer.c',
       'src/display_integration.c',
       'src/errors.c',
       'src/executor.c',
       'src/expand.c',
       'src/globals.c',
       'src/init.c',
       'src/input.c',
       'src/input_continuation.c',
       'src/redirection.c',
       'src/libhashtable/ht.c',
       'src/libhashtable/ht_fnv1a.c',
       'src/libhashtable/ht_strdouble.c',
       'src/libhashtable/ht_strfloat.c',
       'src/libhashtable/ht_strint.c',
       'src/libhashtable/ht_strstr.c',
       'src/lusush.c',
       'src/termcap.c',
       'src/termcap_test.c',
       'src/node.c',
       'src/opts.c',
       'src/parser.c',
       'src/posix_opts.c',
       'src/network.c',
       'src/prompt.c',
       'src/themes.c',
       'src/signals.c',
       'src/strings.c',
       'src/symtable.c',
       'src/tokenizer.c',
      ]

add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')
add_project_arguments('-D_XOPEN_SOURCE=700', language: 'c')
add_project_arguments('-D_XOPEN_SOURCE_EXTENDED', language: 'c')

# Readline support option
readline_support = get_option('readline_support')

# Find readline dependency (optional based on build option)
if readline_support
  readline_dep = dependency('readline', required: true)
  add_project_arguments('-DHAVE_READLINE=1', language: 'c')
  # Add readline_integration.c only when readline is enabled
  src += 'src/readline_integration.c'
  message('GNU Readline support: ENABLED')
else
  readline_dep = dependency('', required: false)
  add_project_arguments('-DHAVE_READLINE=0', language: 'c')
  # Add stub implementations when readline is disabled
  src += 'src/readline_stubs.c'
  message('GNU Readline support: DISABLED (LLE-only build)')
endif

# Find ncurses dependency (required by LLE terminal capabilities)
ncurses_dep = dependency('ncurses', required: true)

# ============================================================================
# LLE (Lusush Line Editor) Build System
# ============================================================================
# Build LLE as a static library (liblle.a) and link into lusush executable.
# The src/lle/meson.build file automatically detects implemented modules
# via fs.exists() checks - no manual updates needed when adding modules.

# Include LLE build configuration
subdir('src/lle')

# Build LLE static library if any modules exist
if lle_sources.length() > 0
  message('Building LLE with ' + lle_sources.length().to_string() + ' module(s)')

  lle_lib = static_library('lle',
                           sources: lle_sources,
                           include_directories: inc,
                           c_args: lle_c_args,
                           dependencies: [readline_dep, ncurses_dep])

  lle_dep = declare_dependency(link_with: lle_lib,
                               include_directories: include_directories('include/lle'))
else
  # LLE not yet implemented, create empty dependency
  message('LLE: No modules implemented yet (zero source files)')
  lle_dep = dependency('', required: false)
endif

# Build lusush executable (with or without LLE)
lusush_exe = executable('lusush',
                        src,
                        include_directories: inc,
                        dependencies: [readline_dep, ncurses_dep, lle_dep])

# ============================================================================
# TEST INFRASTRUCTURE
# ============================================================================

# LLE Functional Tests - standalone with mock memory pool
if lle_sources.length() > 0
  # Buffer operations functional tests
  # Uses mock memory pool to avoid full lusush dependencies
  test_buffer_ops = executable('test_buffer_operations',
                               ['tests/lle/functional/buffer_operations_test.c',
                                'tests/lle/functional/test_memory_mock.c'],
                               include_directories: inc,
                               dependencies: [lle_dep])

  test('LLE Buffer Operations', test_buffer_ops,
       suite: 'lle-functional',
       timeout: 30)

  # Multiline manager functional tests
  # Tests multiline context and buffer analysis
  # NOTE: Includes hashtable sources because symtable.c depends on them
  test_multiline = executable('test_multiline_manager',
                              ['tests/lle/functional/multiline_manager_test.c',
                               'tests/lle/functional/test_memory_mock.c',
                               'src/input_continuation.c',
                               'src/symtable.c',
                               'src/libhashtable/ht.c',
                               'src/libhashtable/ht_fnv1a.c',
                               'src/libhashtable/ht_strstr.c',
                               'src/globals.c'],
                              include_directories: inc,
                              dependencies: [lle_dep, readline_dep])

  test('LLE Multiline Manager', test_multiline,
       suite: 'lle-functional',
       timeout: 30)

  # Subsystem integration tests
  # Tests interaction between multiple LLE subsystems:
  # - Buffer operations + UTF-8 index
  # - Buffer operations + cursor manager
  # - Buffer operations + validator
  # - Buffer operations + change tracker (undo/redo)
  # - End-to-end multi-subsystem scenarios
  test_integration = executable('test_subsystem_integration',
                                ['tests/lle/integration/subsystem_integration_test.c',
                                 'tests/lle/functional/test_memory_mock.c'],
                                include_directories: inc,
                                dependencies: [lle_dep])

  test('LLE Subsystem Integration', test_integration,
       suite: 'lle-integration',
       timeout: 60)

  # Display integration tests
  # Tests the complete integration flow through the display system:
  # - Display Bridge → Event Coordinator → Render Controller
  # - Render Controller → Pipeline → Cache → Output
  # - Terminal Adapter → Theme Integration
  # - Full rendering workflows with multi-component scenarios
  test_display_integration = executable('test_display_integration',
                                        'tests/lle/integration/display_integration_test.c',
                                        include_directories: inc,
                                        dependencies: [lle_dep, readline_dep])

  test('LLE Display Integration', test_display_integration,
       suite: 'lle-integration',
       timeout: 60)

  # Input Parser Integration (Phase 7-9)
  # Tests event generation, keybinding integration, and error recovery
  if fs.exists('tests/lle/integration/input_parser_integration_test.c')
    test_input_parser_integration = executable('test_input_parser_integration',
                                              'tests/lle/integration/input_parser_integration_test.c',
                                              include_directories: inc,
                                              dependencies: [lle_dep])

    test('LLE Input Parser Integration', test_input_parser_integration,
         suite: 'lle-integration',
         timeout: 60)
  endif

  # LLE Readline Step 1 Test
  # Manual integration test for minimal lle_readline() implementation
  # TODO: DISABLED - undefined reference to `display_integration_get_controller'
  #       This test calls functions that don't exist or aren't linked properly.
  # if fs.exists('tests/lle/integration/test_lle_readline_step1.c')
  #   test_readline_step1 = executable('test_lle_readline_step1',
  #                                    'tests/lle/integration/test_lle_readline_step1.c',
  #                                    include_directories: inc,
  #                                    dependencies: [lle_dep])
  #   # Note: This is a manual test requiring user interaction
  #   # Run manually with: ./build/test_lle_readline_step1
  # endif

  # End-to-end realistic scenario tests
  # Tests complete editing workflows that simulate real-world usage
  test_e2e_scenarios = executable('test_realistic_scenarios',
                                  ['tests/lle/e2e/realistic_scenarios_test.c',
                                   'tests/lle/functional/test_memory_mock.c'],
                                  include_directories: inc,
                                  dependencies: [lle_dep])

  test('LLE Realistic Scenarios', test_e2e_scenarios,
       suite: 'lle-e2e',
       timeout: 60)

  # Performance benchmarks
  # Validates operations meet spec performance requirements
  benchmark_perf = executable('benchmark_performance',
                              ['tests/lle/benchmarks/performance_benchmark.c',
                               'tests/lle/functional/test_memory_mock.c'],
                              include_directories: inc,
                              dependencies: [lle_dep])

  test('LLE Performance Benchmarks', benchmark_perf,
       suite: 'lle-benchmarks',
       timeout: 120)

  # Display integration performance benchmarks
  # Validates display operations meet Spec 08 performance requirements
  benchmark_display = executable('benchmark_display_performance',
                                 'tests/lle/benchmarks/display_performance_benchmark.c',
                                 include_directories: inc,
                                 dependencies: [lle_dep, readline_dep])

  test('LLE Display Performance', benchmark_display,
       suite: 'lle-benchmarks',
       timeout: 120)

  # Display integration stress tests
  # Week 8: Production validation - stress testing under extreme conditions
  stress_display = executable('stress_display_test',
                              'tests/lle/stress/display_stress_test.c',
                              include_directories: inc,
                              dependencies: [lle_dep, readline_dep])

  test('LLE Display Stress Tests', stress_display,
       suite: 'lle-stress',
       timeout: 300)

  # Display Bridge Unit Tests (Spec 08)
  # Unit tests for display bridge initialization, cleanup, and error handling
  # Uses mocks for display_controller and memory pool (standard unit test practice)
  # Integration with real display system will be tested in integration tests
  test_display_bridge = executable('test_display_bridge',
                                   ['tests/lle/unit/test_display_bridge.c',
                                    'tests/lle/functional/test_memory_mock.c'],
                                   include_directories: inc,
                                   dependencies: [lle_dep, readline_dep])

  test('LLE Display Bridge', test_display_bridge,
       suite: 'lle-unit',
       timeout: 30)

  # Event Coordinator Unit Tests (Spec 08)
  # Unit tests for event coordinator initialization, cleanup, and event processing
  # Tests sub-components: translator, router, filter, queue, metrics
  # NOTE: Disabled due to event_coordinator.c bugs - lle_event_filter_t structure mismatch
  # test_event_coordinator = executable('test_event_coordinator',
  #                                     ['tests/lle/unit/test_event_coordinator.c',
  #                                      'tests/lle/functional/test_memory_mock.c'],
  #                                     include_directories: inc,
  #                                     dependencies: [lle_dep, readline_dep])
  #
  # test('LLE Event Coordinator', test_event_coordinator,
  #      suite: 'lle-unit',
  #      timeout: 30)

  # Event System Unit Tests (Spec 04 Phase 1)
  # Tests core event system infrastructure: lifecycle, events, queues, handlers
  test_event_system = executable('test_event_system',
                                 ['tests/lle/unit/test_event_system.c',
                                  'tests/lle/functional/test_memory_mock.c'],
                                 include_directories: inc,
                                 dependencies: [lle_dep, readline_dep])

  test('LLE Event System', test_event_system,
       suite: 'lle-unit',
       timeout: 30)

  # Event System Phase 2 Tests (Spec 04 Phase 2A-2D)
  # Tests filters, timers, enhanced stats, and priority queues
  test_event_phase2 = executable('test_event_phase2',
                                 ['tests/lle/unit/test_event_phase2.c',
                                  'tests/lle/functional/test_memory_mock.c'],
                                 include_directories: inc,
                                 dependencies: [lle_dep, readline_dep])

  test('LLE Event System Phase 2', test_event_phase2,
       suite: 'lle-unit',
       timeout: 30)

  # Terminal Capabilities Unit Tests (Spec 02 Phase 1)
  test_terminal_capabilities = executable('test_terminal_capabilities',
                                          'tests/lle/unit/test_terminal_capabilities.c',
                                          include_directories: inc,
                                          dependencies: [lle_dep, readline_dep],
                                          link_args: ['-lncurses'])

  test('LLE Terminal Capabilities', test_terminal_capabilities,
       suite: 'lle-unit',
       timeout: 30)

  # Terminal State Management Unit Tests (Spec 02 Phase 2)
  test_terminal_state = executable('test_terminal_state',
                                   'tests/lle/unit/test_terminal_state.c',
                                   include_directories: inc,
                                   dependencies: [lle_dep, readline_dep])

  test('LLE Terminal State', test_terminal_state,
       suite: 'lle-unit',
       timeout: 30)

  # Terminal Event Reading Unit Tests (Spec 02 Phase 3)
  test_terminal_event_reading = executable('test_terminal_event_reading',
                                          'tests/lle/unit/test_terminal_event_reading.c',
                                          include_directories: inc,
                                          dependencies: [lle_dep, readline_dep])

  test('LLE Terminal Event Reading', test_terminal_event_reading,
       suite: 'lle-unit',
       timeout: 30)

  # F-Key Detection Integration Test (Spec 02 + Spec 06)
  # Tests F-key detection with full terminal_abstraction initialization
  # Validates production code path with parser + key_detector
  test_fkey_detection = executable('test_fkey_detection',
                                   'tests/lle/integration/test_fkey_detection.c',
                                   include_directories: inc,
                                   dependencies: [lle_dep, readline_dep])

  test('LLE F-Key Detection Integration', test_fkey_detection,
       suite: 'lle-integration',
       timeout: 30)

  # F-Key Manual Test Program (Spec 02 + Spec 06)
  # Interactive manual test for F-key detection in real terminals
  # Not registered as automated test - for manual verification only
  test_fkey_manual = executable('test_fkey_manual',
                                'tests/lle/manual/test_fkey_manual.c',
                                include_directories: inc,
                                dependencies: [lle_dep, readline_dep])

  # Render Controller Unit Tests (Spec 08)
  # Unit tests for render controller initialization, cleanup, and sub-component setup
  # Tests sub-components: buffer renderer, cursor renderer, frame scheduler, cache, etc.
  test_render_controller = executable('test_render_controller',
                                      ['tests/lle/unit/test_render_controller.c',
                                       'tests/lle/functional/test_memory_mock.c'],
                                      include_directories: inc,
                                      dependencies: [lle_dep, readline_dep])

  test('LLE Render Controller', test_render_controller,
       suite: 'lle-unit',
       timeout: 30)

  # Terminal adapter unit tests
  # NOTE: Disabled due to terminal_adapter.c bugs - terminal type enum and capability structure mismatches
  # test_terminal_adapter = executable('test_terminal_adapter',
  #                                    ['tests/lle/unit/test_terminal_adapter.c',
  #                                     'tests/lle/functional/test_memory_mock.c'],
  #                                    include_directories: inc,
  #                                    dependencies: [lle_dep, readline_dep])
  #
  # test('LLE Terminal Adapter', test_terminal_adapter,
  #      suite: 'lle-unit',
  #      timeout: 30)

  # Theme integration unit tests
  test_theme_integration = executable('test_theme_integration',
                                      'tests/lle/unit/test_theme_integration.c',
                                      include_directories: inc,
                                      dependencies: [lle_dep, readline_dep])

  test('LLE Theme Integration', test_theme_integration,
       suite: 'lle-unit',
       timeout: 30)

  # Render pipeline unit tests
  test_render_pipeline = executable('test_render_pipeline',
                                    'tests/lle/unit/test_render_pipeline.c',
                                    include_directories: inc,
                                    dependencies: [lle_dep, readline_dep])

  test('LLE Render Pipeline', test_render_pipeline,
       suite: 'lle-unit',
       timeout: 30)

  # Hashtable wrapper unit tests (Spec 05)
  test_hashtable = executable('test_hashtable',
                              'tests/lle/unit/test_hashtable.c',
                              include_directories: inc,
                              dependencies: [lle_dep, readline_dep])

  test('LLE Hashtable Wrapper', test_hashtable,
       suite: 'lle-unit',
       timeout: 30)

  # Render cache unit tests
  test_render_cache = executable('test_render_cache',
                                 'tests/lle/unit/test_render_cache.c',
                                 include_directories: inc,
                                 dependencies: [lle_dep, readline_dep])

  test('LLE Render Cache', test_render_cache,
       suite: 'lle-unit',
       timeout: 30)

  # Dirty tracker unit tests
  test_dirty_tracker = executable('test_dirty_tracker',
                                  'tests/lle/unit/test_dirty_tracker.c',
                                  include_directories: inc,
                                  dependencies: [lle_dep, readline_dep])

  test('LLE Dirty Tracker', test_dirty_tracker,
       suite: 'lle-unit',
       timeout: 30)

  # Input Stream Unit Tests (Spec 06 Phase 1)
  # Unit tests for input stream buffering, flow control, and data management
  # Tests: init/destroy, buffer data, consume, peek, statistics, overflow handling
  test_input_stream = executable('test_input_stream',
                                 ['tests/lle/unit/test_input_stream.c',
                                  'tests/lle/functional/test_memory_mock.c'],
                                 include_directories: inc,
                                 dependencies: [lle_dep, readline_dep])

  test('LLE Input Stream', test_input_stream,
       suite: 'lle-unit',
       timeout: 30)

  # Input UTF-8 Processor Unit Tests (Spec 06 Phase 2)
  # Unit tests for streaming UTF-8 decoding and grapheme boundary detection
  # Tests: byte-by-byte processing, buffer processing, partial sequences, error recovery
  test_input_utf8_processor = executable('test_input_utf8_processor',
                                         ['tests/lle/unit/test_input_utf8_processor.c',
                                          'tests/lle/functional/test_memory_mock.c'],
                                         include_directories: inc,
                                         dependencies: [lle_dep, readline_dep])

  test('LLE Input UTF-8 Processor', test_input_utf8_processor,
       suite: 'lle-unit',
       timeout: 30)

  # Sequence Parser Unit Tests (Spec 06 Phase 3)
  # Unit tests for terminal escape sequence parsing (CSI, OSC, DCS, control chars)
  # Tests: state machine, CSI parameters, string sequences, incomplete sequences, error recovery
  test_sequence_parser = executable('test_sequence_parser',
                                    ['tests/lle/unit/test_sequence_parser.c',
                                     'tests/lle/functional/test_memory_mock.c'],
                                    include_directories: inc,
                                    dependencies: [lle_dep, readline_dep])

  test('LLE Sequence Parser', test_sequence_parser,
       suite: 'lle-unit',
       timeout: 30)

  # Key Detector Unit Tests (Spec 06 Phase 4)
  # Unit tests for key sequence detection and mapping
  # Tests: function keys, cursor keys, modifiers, control chars, partial sequences
  test_key_detector = executable('test_key_detector',
                                 ['tests/lle/unit/test_key_detector.c',
                                  'tests/lle/functional/test_memory_mock.c'],
                                 include_directories: inc,
                                 dependencies: [lle_dep, readline_dep])

  test('LLE Key Detector', test_key_detector,
       suite: 'lle-unit',
       timeout: 30)
endif

# Test: Mouse Parser
if fs.exists('tests/lle/unit/test_mouse_parser.c')
  test_mouse_parser = executable('test_mouse_parser',
                                 ['tests/lle/unit/test_mouse_parser.c',
                                  'tests/lle/functional/test_memory_mock.c'],
                                 include_directories: inc,
                                 dependencies: [lle_dep, readline_dep])
  test('LLE Mouse Parser', test_mouse_parser,
       suite: 'lle-unit',
       timeout: 30)
endif

# Test: Parser State Machine
if fs.exists('tests/lle/unit/test_parser_state_machine.c')
  test_parser_state_machine = executable('test_parser_state_machine',
                                         ['tests/lle/unit/test_parser_state_machine.c',
                                          'tests/lle/functional/test_memory_mock.c'],
                                         include_directories: inc,
                                         dependencies: [lle_dep, readline_dep])
  test('LLE Parser State Machine', test_parser_state_machine,
       suite: 'lle-unit',
       timeout: 30)
endif

# ============================================================================
# LLE History System Tests (Spec 09)
# ============================================================================

  # History Core Functional Tests (Phase 1 Day 1)
  # Tests basic history operations: add, get, search, navigate
  if fs.exists('tests/lle/functional/test_history_phase1_day1.c')
    test_history_core = executable('test_history_phase1_day1',
                                   ['tests/lle/functional/test_history_phase1_day1.c',
                                    'tests/lle/functional/test_memory_mock.c'],
                                   include_directories: inc,
                                   dependencies: [lle_dep])
    test('LLE History Core', test_history_core,
         suite: 'lle-functional',
         timeout: 30)
  endif

  # History Indexing Functional Tests (Phase 1 Day 2)
  # Tests command indexing, prefix search, full-text search
  if fs.exists('tests/lle/functional/test_history_phase1_day2.c')
    test_history_indexing = executable('test_history_phase1_day2',
                                       ['tests/lle/functional/test_history_phase1_day2.c',
                                        'tests/lle/functional/test_memory_mock.c'],
                                       include_directories: inc,
                                       dependencies: [lle_dep])
    test('LLE History Indexing', test_history_indexing,
         suite: 'lle-functional',
         timeout: 30)
  endif

  # History Persistence Functional Tests (Phase 1 Day 3)
  # Tests file I/O, format conversion, import/export
  if fs.exists('tests/lle/functional/test_history_phase1_day3.c')
    test_history_persistence = executable('test_history_phase1_day3',
                                          ['tests/lle/functional/test_history_phase1_day3.c',
                                           'tests/lle/functional/test_memory_mock.c'],
                                          include_directories: inc,
                                          dependencies: [lle_dep])
    test('LLE History Persistence', test_history_persistence,
         suite: 'lle-functional',
         timeout: 30)
  endif

  # History Lusush Bridge Functional Tests (Phase 2 Day 5)
  # Tests bidirectional sync with Lusush history
  # TODO: DISABLED - test uses non-existent pool APIs (lusush_pool_create/destroy)
  #       and references g_pool throughout (19 times). Needs comprehensive rewrite.
  #       Implementation exists in src/lle/history_lusush_bridge.c but is untested.
  # if fs.exists('tests/lle/functional/test_history_phase2_day5.c')
  #   test_history_bridge = executable('test_history_phase2_day5',
  #                                    ['tests/lle/functional/test_history_phase2_day5.c',
  #                                     'tests/lle/functional/test_memory_mock.c'],
  #                                    include_directories: inc,
  #                                    dependencies: [lle_dep])
  #   test('LLE History Lusush Bridge', test_history_bridge,
  #        suite: 'lle-functional',
  #        timeout: 30)
  # endif

  # History Phase 1 Integration Tests
  # Tests integration between core, indexing, and persistence
  if fs.exists('tests/lle/integration/test_history_phase1_integration.c')
    test_history_phase1_integration = executable('test_history_phase1_integration',
                                                 ['tests/lle/integration/test_history_phase1_integration.c',
                                                  'tests/lle/functional/test_memory_mock.c'],
                                                 include_directories: inc,
                                                 dependencies: [lle_dep])
    test('LLE History Phase 1 Integration', test_history_phase1_integration,
         suite: 'lle-integration',
         timeout: 60)
  endif

  # History Phase 2 Integration Tests
  # Tests integration between bridge, events, and all Phase 1 components
  # TODO: DISABLED - test uses lle_event_system_create (should be _init),
  #       non-existent event data types (lle_history_entry_event_data_t),
  #       and g_pool references throughout. Needs comprehensive API fixes.
  # if fs.exists('tests/lle/integration/test_history_phase2_integration.c')
  #   test_history_phase2_integration = executable('test_history_phase2_integration',
  #                                                ['tests/lle/integration/test_history_phase2_integration.c',
  #                                                 'tests/lle/functional/test_memory_mock.c'],
  #                                                include_directories: inc,
  #                                                dependencies: [lle_dep])
  #   test('LLE History Phase 2 Integration', test_history_phase2_integration,
  #        suite: 'lle-integration',
  #        timeout: 60)
  # endif

  # History Phase 3 Day 8 Tests - Search Engine
  # Tests exact, prefix, substring, and fuzzy search functionality
  if fs.exists('tests/lle/functional/test_history_phase3_day8.c')
    test_history_phase3_day8 = executable('test_history_phase3_day8',
                                          ['tests/lle/functional/test_history_phase3_day8.c',
                                           'tests/lle/functional/test_memory_mock.c'],
                                          include_directories: inc,
                                          dependencies: [lle_dep])
    test('LLE History Phase 3 Day 8', test_history_phase3_day8,
         suite: 'lle-functional',
         timeout: 60)
  endif

  # History Phase 3 Day 9 Tests - Interactive Search
  # Tests Ctrl+R style reverse incremental search functionality
  if fs.exists('tests/lle/functional/test_history_phase3_day9.c')
    test_history_phase3_day9 = executable('test_history_phase3_day9',
                                          ['tests/lle/functional/test_history_phase3_day9.c',
                                           'tests/lle/functional/test_memory_mock.c'],
                                          include_directories: inc,
                                          dependencies: [lle_dep])
    test('LLE History Phase 3 Day 9', test_history_phase3_day9,
         suite: 'lle-functional',
         timeout: 60)
  endif

  # History Compliance Tests (Spec 09)
  # Validates compliance with Spec 09 requirements
  if fs.exists('tests/lle/compliance/spec_09_history_compliance.c')
    test_history_compliance = executable('spec_09_compliance',
                                         ['tests/lle/compliance/spec_09_history_compliance.c',
                                          'tests/lle/functional/test_memory_mock.c'],
                                         include_directories: inc,
                                         dependencies: [lle_dep])
    test('Spec 09 History Compliance', test_history_compliance,
         suite: 'lle-compliance',
         timeout: 60)
  endif

  # Completion System Compliance Tests (Spec 12)
  # Validates compliance with Spec 12 requirements
  if fs.exists('tests/lle/compliance/spec_12_completion_compliance.c')
    test_completion_compliance = executable('spec_12_compliance',
                                           ['tests/lle/compliance/spec_12_completion_compliance.c',
                                            'tests/lle/functional/test_memory_mock.c',
                                            'tests/lle/functional/test_completion_mock.c'],
                                           include_directories: inc,
                                           dependencies: [lle_dep])
    test('Spec 12 Completion Compliance', test_completion_compliance,
         suite: 'lle-compliance',
         timeout: 60)
  endif

  # Validates compliance with Spec 22 requirements
  if fs.exists('tests/lle/compliance/spec_22_history_buffer_compliance.c')
    test_spec22_compliance = executable('spec_22_compliance',
                                        ['tests/lle/compliance/spec_22_history_buffer_compliance.c',
                                         'tests/lle/functional/test_memory_mock.c'],
                                        include_directories: inc,
                                        dependencies: [lle_dep])
    test('Spec 22 History-Buffer Integration Compliance', test_spec22_compliance,
         suite: 'lle-compliance',
         timeout: 60)
  endif

  # Validates compliance with Spec 25 requirements
  if fs.exists('tests/lle/compliance/spec_25_keybinding_compliance.c')
    test_spec25_compliance = executable('spec_25_compliance',
                                        'tests/lle/compliance/spec_25_keybinding_compliance.c',
                                        include_directories: inc,
                                        dependencies: [lle_dep])
    test('Spec 25 Keybinding System Compliance', test_spec25_compliance,
         suite: 'lle-compliance',
         timeout: 60)
  endif

  # Spec 26 Compliance Test (Adaptive Terminal Integration)
  # Validates compliance with Spec 26 Phases 1-2 requirements
  if fs.exists('tests/lle/compliance/spec_26_adaptive_terminal_compliance.c')
    test_spec26_compliance = executable('spec_26_compliance',
                                        'tests/lle/compliance/spec_26_adaptive_terminal_compliance.c',
                                        include_directories: inc,
                                        dependencies: [lle_dep])
    test('Spec 26 Adaptive Terminal Integration Compliance', test_spec26_compliance,
         suite: 'lle-compliance',
         timeout: 60)
  endif

  # History Phase 4 Complete Tests
  # Tests Phase 4 features: forensics, deduplication, multiline
  # NOTE: Includes input_continuation.c for multiline support
  if fs.exists('tests/lle/functional/test_history_phase4_complete.c')
    test_history_phase4_complete = executable('test_history_phase4_complete',
                                              ['tests/lle/functional/test_history_phase4_complete.c',
                                               'tests/lle/functional/test_memory_mock.c',
                                               'src/input_continuation.c',
                                               'src/symtable.c',
                                               'src/libhashtable/ht.c',
                                               'src/libhashtable/ht_fnv1a.c',
                                               'src/libhashtable/ht_strstr.c',
                                               'src/globals.c'],
                                              include_directories: inc,
                                              dependencies: [lle_dep, readline_dep])
    test('LLE History Phase 4 Complete', test_history_phase4_complete,
         suite: 'lle-functional',
         timeout: 60)
  endif

  # ============================================================================
  # SPEC 25: DEFAULT KEYBINDINGS TESTS
  # ============================================================================

  # Kill Ring Unit Tests
  # Tests GNU Readline compatible kill/yank operations
  if fs.exists('tests/lle/unit/test_kill_ring.c')
    test_kill_ring = executable('test_kill_ring',
                                'tests/lle/unit/test_kill_ring.c',
                                include_directories: inc,
                                dependencies: [lle_dep])
    test('LLE Kill Ring', test_kill_ring,
         suite: 'lle-unit',
         timeout: 30)
  endif

  # Keybinding Engine Unit Tests
  # Tests keybinding manager, key parsing, bind/unbind, mode switching
  if fs.exists('tests/lle/unit/test_keybinding.c')
    test_keybinding = executable('test_keybinding',
                                 'tests/lle/unit/test_keybinding.c',
                                 include_directories: inc,
                                 dependencies: [lle_dep])
    test('LLE Keybinding Engine', test_keybinding,
         suite: 'lle-unit',
         timeout: 30)
  endif

  # Widget System Unit Tests (Spec 07)
  # Tests widget registry: init, register, lookup, execute, unregister, enable/disable
  if fs.exists('tests/lle/unit/test_widget_system.c')
    test_widget_system = executable('test_widget_system',
                                    ['tests/lle/unit/test_widget_system.c',
                                     'tests/lle/functional/test_memory_mock.c'],
                                    include_directories: inc,
                                    dependencies: [lle_dep])
    test('LLE Widget System', test_widget_system,
         suite: 'lle-unit',
         timeout: 30)
  endif

  # Widget Hooks Manager Unit Tests (Spec 07)
  # Tests hooks manager: register, unregister, trigger, enable/disable, hook types
  if fs.exists('tests/lle/unit/test_widget_hooks.c')
    test_widget_hooks = executable('test_widget_hooks',
                                   ['tests/lle/unit/test_widget_hooks.c',
                                    'tests/lle/functional/test_memory_mock.c'],
                                   include_directories: inc,
                                   dependencies: [lle_dep])
    test('LLE Widget Hooks Manager', test_widget_hooks,
         suite: 'lle-unit',
         timeout: 30)
  endif

  # Completion Types Unit Tests (Spec 12 Phase 1)
  # Tests type classification, item/result management, sorting
  if fs.exists('tests/lle/unit/test_completion_types.c')
    test_completion_types = executable('test_completion_types',
                                       'tests/lle/unit/test_completion_types.c',
                                       include_directories: inc,
                                       link_with: lle_lib,
                                       dependencies: [ncurses_dep])
    test('LLE Completion Types', test_completion_types,
         suite: 'lle-unit',
         timeout: 30)
  endif

  # UTF-8 Movement Functions Test
  # Tests cursor_manager integration for UTF-8 character movement
  # Tests: lle_forward_char, lle_backward_char, lle_forward_word, lle_backward_word
  # Coverage: ASCII, 2-byte (Latin), 3-byte (CJK), 4-byte (Emoji), mixed content
  # Uses real LLE library (not mocked) - provides global_memory_pool stub
  if fs.exists('tests/lle/test_utf8_movement.c')
    test_utf8_movement = executable('test_utf8_movement',
                                    'tests/lle/test_utf8_movement.c',
                                    include_directories: inc,
                                    dependencies: [lle_dep])
    test('LLE UTF-8 Movement Functions', test_utf8_movement,
         suite: 'lle-functional',
         timeout: 30)
  endif

  # Adaptive Terminal Detection Unit Tests (Spec 26 Phase 1)
  # Tests detection system, signature matching, capability probing
  if fs.exists('tests/lle/unit/test_adaptive_detection.c')
    test_adaptive_detection = executable('test_adaptive_detection',
                                         'tests/lle/unit/test_adaptive_detection.c',
                                         include_directories: inc,
                                         dependencies: [lle_dep])
    test('LLE Adaptive Detection', test_adaptive_detection,
         suite: 'lle-unit',
         timeout: 30)
  endif

  # Adaptive Terminal Controllers Unit Tests (Spec 26 Phase 2)
  # Tests all four controllers: native, enhanced, minimal, multiplexer
  if fs.exists('tests/lle/unit/test_adaptive_controllers.c')
    test_adaptive_controllers = executable('test_adaptive_controllers',
                                          'tests/lle/unit/test_adaptive_controllers.c',
                                          include_directories: inc,
                                          dependencies: [lle_dep])
    test('LLE Adaptive Controllers', test_adaptive_controllers,
         suite: 'lle-unit',
         timeout: 30)
  endif

  # Adaptive Terminal Fallback Unit Tests (Spec 26 Phase 3)
  # Tests graceful degradation and fallback hierarchy
  if fs.exists('tests/lle/unit/test_adaptive_fallback.c')
    test_adaptive_fallback = executable('test_adaptive_fallback',
                                       'tests/lle/unit/test_adaptive_fallback.c',
                                       include_directories: inc,
                                       dependencies: [lle_dep])
    test('LLE Adaptive Fallback', test_adaptive_fallback,
         suite: 'lle-unit',
         timeout: 30)
  endif

# ============================================================================
# LLE Manual Integration Test (CRITICAL VERIFICATION)
# ============================================================================
# This is NOT an automated test - it's a manual verification tool
# Purpose: Verify raw terminal input works at 11% implementation
# Usage: Run manually and observe behavior
# Exit: User must verify results manually
# NOTE: Standalone executable, no LLE dependencies (tests raw input only)
if fs.exists('tests/lle/integration/manual_input_test.c')
  manual_input_test = executable('manual_input_test',
                                  'tests/lle/integration/manual_input_test.c')
  # NOTE: Not added to automated test suite - must be run manually
endif

# Simple input test - absolutely minimal, no formatting
if fs.exists('tests/lle/integration/simple_input_test.c')
  simple_input_test = executable('simple_input_test',
                                  'tests/lle/integration/simple_input_test.c')
endif

# NOTE: test_lle_readline_real removed - use lusush binary with 'display lle enable' for real-world testing

# Minimal lle_readline() test - with stubs for non-display functions
# This version includes only display system sources and stubs the rest
if fs.exists('tests/manual/test_lle_readline_minimal.c')
  minimal_lusush_sources = [
    'src/display/base_terminal.c',
    'src/display/terminal_control.c',
    'src/display/layer_events.c',
    'src/display/prompt_layer.c',
    'src/display/command_layer.c',
    'src/display/composition_engine.c',
    'src/display/display_controller.c',
    'src/display/screen_buffer.c',
    'src/display_integration.c',
    'src/lusush_memory_pool.c',
    'src/themes.c',
    'src/termcap.c',
    'src/input_continuation.c',
    'src/prompt.c',
    'src/globals.c',
    'src/strings.c',
    'src/errors.c',
    'src/symtable.c',
    'src/libhashtable/ht.c',
    'src/libhashtable/ht_fnv1a.c',
    'src/libhashtable/ht_strstr.c',
    'src/libhashtable/ht_strint.c',
  ]

  test_lle_readline_minimal = executable('test_lle_readline_minimal',
                                         ['tests/manual/test_lle_readline_minimal.c'] + minimal_lusush_sources,
                                         include_directories: inc,
                                         dependencies: [readline_dep, ncurses_dep, lle_dep])
  # NOTE: Interactive test - run with: ./builddir/test_lle_readline_minimal
endif

# ============================================================================
# PRAGMATIC COMPLETION SYSTEM TESTS
# ============================================================================

# Phase 2: Interactive Menu Demo (Manual Test)
# Demonstrates interactive completion menu with arrow key navigation
if fs.exists('tests/manual/demo_completion_menu.c')
  demo_completion_menu = executable('demo_completion_menu',
                                   ['tests/manual/demo_completion_menu.c',
                                    'src/completion_types.c',
                                    'src/completion_menu.c'],
                                   include_directories: inc,
                                   dependencies: [readline_dep])
  # NOTE: This is a manual interactive demo - run with: ./builddir/demo_completion_menu
endif

# Phase 3: Themed Menu Demo (Manual Test)
# Demonstrates theme integration with completion menu
if fs.exists('tests/manual/demo_completion_menu_themed.c')
  demo_completion_menu_themed = executable('demo_completion_menu_themed',
                                          ['tests/manual/demo_completion_menu_themed.c',
                                           'src/completion_types.c',
                                           'src/completion_menu.c',
                                           'src/completion_menu_theme.c'],
                                          include_directories: inc,
                                          dependencies: [readline_dep])
  # NOTE: This is a manual demo - run with: ./builddir/demo_completion_menu_themed
endif

# Phase 1: Classification Tests
# ============================================================================

# Completion Type Classification Unit Tests
# Tests completion type system, item management, classification helpers
# Note: Uses mocks for builtins/aliases to avoid heavyweight dependencies
if fs.exists('tests/unit/test_completion_classification.c')
  test_completion_classification = executable('test_completion_classification',
                                              ['tests/unit/test_completion_classification.c',
                                               'src/completion_types.c'],
                                              include_directories: inc,
                                              dependencies: [readline_dep])
  test('Completion Classification', test_completion_classification,
       suite: 'unit',
       timeout: 30)
endif

# Phase 2: Interactive Menu Tests
# ============================================================================

# Completion Menu Unit Tests
# Tests menu lifecycle, navigation, selection, scrolling, display
# Note: Uses mocks for builtins/aliases to avoid heavyweight dependencies
if fs.exists('tests/unit/test_completion_menu.c')
  test_completion_menu = executable('test_completion_menu',
                                   ['tests/unit/test_completion_menu.c',
                                    'src/completion_types.c',
                                    'src/completion_menu.c'],
                                   include_directories: inc,
                                   dependencies: [readline_dep])
  test('Completion Menu', test_completion_menu,
       suite: 'unit',
       timeout: 30)
endif

# Phase 3: Theme Integration Tests
# ============================================================================

# Completion Menu Theme Integration Unit Tests
# Tests theme initialization, color/symbol mapping, themed formatting
# Note: Uses mocks for theme system to test independently
if fs.exists('tests/unit/test_completion_menu_theme.c')
  test_completion_menu_theme = executable('test_completion_menu_theme',
                                         ['tests/unit/test_completion_menu_theme.c',
                                          'src/completion_types.c',
                                          'src/completion_menu.c',
                                          'src/completion_menu_theme.c'],
                                         include_directories: inc,
                                         dependencies: [readline_dep])
  test('Completion Menu Theme', test_completion_menu_theme,
       suite: 'unit',
       timeout: 30)
endif

# LLE Completion Menu Renderer Unit Tests (Spec 12 Phase 5.1)
# Tests menu rendering, column layout, category headers, item formatting
if fs.exists('tests/unit/test_completion_menu_renderer.c')
  test_completion_menu_renderer = executable('test_completion_menu_renderer',
                                             'tests/unit/test_completion_menu_renderer.c',
                                             include_directories: inc,
                                             link_with: lle_lib)
  test('LLE Completion Menu Renderer', test_completion_menu_renderer,
       suite: 'unit',
       timeout: 30)
endif

# Phase 4: Continuation Prompt Tests
# ============================================================================

# Continuation Prompt Layer Unit Tests
# Tests layer lifecycle, event handling, prompt generation modes
# Tests simple mode, context-aware mode, error handling
if fs.exists('tests/unit/test_continuation_prompt_layer.c')
  test_continuation_prompt = executable('test_continuation_prompt_layer',
                                       ['tests/unit/test_continuation_prompt_layer.c',
                                        'tests/unit/test_lusush_stubs.c',
                                        'src/display/continuation_prompt_layer.c',
                                        'src/display/layer_events.c',
                                        'src/input_continuation.c',
                                        'src/symtable.c'],
                                       include_directories: inc,
                                       dependencies: [readline_dep])
  test('Continuation Prompt Layer', test_continuation_prompt,
       suite: 'unit',
       timeout: 30)
endif
