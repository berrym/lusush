# LLE Foundation Layer Build Configuration
#
# This builds the Lusush Line Editor (LLE) foundation layer as a static library
# with comprehensive test coverage.

# Include directories for LLE foundation
lle_inc = include_directories('terminal', 'display', 'buffer', 'editor')

# LLE Foundation source files
lle_sources = [
  'terminal/terminal.c',
  'terminal/capability.c',
  'display/display.c',
  'buffer/buffer.c',
  'editor/editor.c',
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

# Test executables
test_sources = {
  'terminal_unit_test': 'test/terminal_unit_test.c',
  'buffer_test': 'test/buffer_test.c',
  'editor_test': 'test/editor_test.c',
  'editor_advanced_test_v2': 'test/editor_advanced_test_v2.c',
  'editor_kill_ring_test': 'test/editor_kill_ring_test.c',
  'editor_search_test': 'test/editor_search_test.c',
}

# Build and register tests
foreach test_name, test_source : test_sources
  test_exe = executable(test_name,
                       test_source,
                       link_with: liblle_foundation,
                       include_directories: lle_inc,
                       c_args: ['-std=c99', '-Wall', '-Wextra'])

  # Register with Meson test framework
  test(test_name, test_exe, timeout: 30)
endforeach

# Summary message
message('LLE Foundation: Building library and 6 test suites')
