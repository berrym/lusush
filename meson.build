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
       'src/autosuggestions.c',
       'src/completion.c',
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
       'src/display/composition_engine.c',
       'src/display/display_controller.c',
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
       'src/readline_integration.c',
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

# Find readline dependency
readline_dep = dependency('readline', required: true)

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
                           dependencies: [readline_dep])

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
                        dependencies: [readline_dep, lle_dep])

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
  test_event_coordinator = executable('test_event_coordinator',
                                      ['tests/lle/unit/test_event_coordinator.c',
                                       'tests/lle/functional/test_memory_mock.c'],
                                      include_directories: inc,
                                      dependencies: [lle_dep, readline_dep])

  test('LLE Event Coordinator', test_event_coordinator,
       suite: 'lle-unit',
       timeout: 30)

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
  test_terminal_adapter = executable('test_terminal_adapter',
                                     ['tests/lle/unit/test_terminal_adapter.c',
                                      'tests/lle/functional/test_memory_mock.c'],
                                     include_directories: inc,
                                     dependencies: [lle_dep, readline_dep])

  test('LLE Terminal Adapter', test_terminal_adapter,
       suite: 'lle-unit',
       timeout: 30)

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
endif
