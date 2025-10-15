# LLE Foundation Layer Build Configuration
#
# This builds the Lusush Line Editor (LLE) foundation layer as a static library
# with comprehensive test coverage.

# Include directories for LLE foundation
lle_inc = include_directories('terminal', 'display', 'buffer', 'editor', 'history', 'fuzzy_matching')

# LLE Foundation source files
lle_sources = [
  'terminal/terminal.c',
  'terminal/capability.c',
  'display/display.c',
  'buffer/buffer.c',
  'editor/editor.c',
  'history/history.c',
  'fuzzy_matching/fuzzy_matching.c',
]

# Build LLE foundation as a static library
liblle_foundation = static_library('lle_foundation',
                                   lle_sources,
                                   include_directories: lle_inc,
                                   c_args: ['-std=c99', '-Wall', '-Wextra'])

# Declare dependency for other parts of the project to use
lle_foundation_dep = declare_dependency(
  link_with: liblle_foundation,
  include_directories: lle_inc
)

# Test executables (automated - no TTY required)
test_sources = {
  'terminal_unit_test': 'test/terminal_unit_test.c',
  'buffer_test': 'test/buffer_test.c',
  'editor_kill_ring_test': 'test/editor_kill_ring_test.c',
  'editor_search_test': 'test/editor_search_test.c',
  'display_scroll_test': 'test/display_scroll_test.c',
  'history_test': 'test/history_test.c',
  'fuzzy_matching_test': 'test/test_fuzzy_matching.c',
  # 'editor_history_integration_test': 'test/editor_history_integration_test.c',  # TODO: needs lle_editor_set_history() implementation
}

# TTY-required tests (run manually with run_tty_tests.sh)
tty_test_sources = {
  'editor_test_tty': 'test/editor_test_tty.c',
  'editor_advanced_test_tty': 'test/editor_advanced_test_tty.c',
}

# Build and register automated tests
foreach test_name, test_source : test_sources
  test_exe = executable(test_name,
                       test_source,
                       link_with: liblle_foundation,
                       include_directories: lle_inc,
                       c_args: ['-std=c99', '-Wall', '-Wextra'])

  # Register with Meson test framework
  test(test_name, test_exe, timeout: 30)
endforeach

# Build TTY-required tests (not registered with meson test - run manually)
foreach test_name, test_source : tty_test_sources
  executable(test_name,
            test_source,
            link_with: liblle_foundation,
            include_directories: lle_inc,
            c_args: ['-std=c99', '-Wall', '-Wextra'])
endforeach

# Summary message
message('LLE Foundation: Building library with 7 automated tests + 2 TTY tests (run ./run_tty_tests.sh)')
