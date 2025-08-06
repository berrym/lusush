# Lusush Line Editor (LLE) Build Configuration

# Include termcap subdirectory first
subdir('termcap')

lle_sources = [
  'text_buffer.c',
  'cursor_math.c',
  'terminal_manager.c',
  'prompt.c',
  'display.c',
  'display_performance.c',
  'display_stabilization.c',
  'display_state_sync.c',
  'display_state_integration.c',
  'lle_state_observer.c',
  'theme_integration.c',
  'input_handler.c',
  'edit_commands.c',
  'command_history.c',
  'unicode.c',
  'completion.c',
  'completion_display.c',
  'undo.c',
  'syntax.c',
  'line_editor.c',
  'lle_integration.c',
  'config.c',
  'enhanced_terminal_detection.c',
  'enhanced_terminal_integration.c',
  'enhanced_tab_completion.c',
  'buffer_trace.c',
  'platform_detection.c'
]

lle_headers = [
  'text_buffer.h',
  'cursor_math.h',
  'terminal_manager.h',
  'prompt.h',
  'display.h',
  'display_performance.h',
  'display_stabilization.h',
  'display_state_sync.h',
  'display_state_integration.h',
  'lle_state_observer.h',
  'theme_integration.h',
  'input_handler.h',
  'edit_commands.h',
  'command_history.h',
  'unicode.h',
  'completion.h',
  'undo.h',
  'syntax.h',
  'line_editor.h',
  'lle_integration.h',
  'config.h',
  'enhanced_terminal_detection.h',
  'enhanced_terminal_integration.h',
  'enhanced_tab_completion.h',
  'buffer_trace.h',
  'platform_detection.h'
]

# Create line editor static library
lle_lib = static_library('lle',
                        lle_sources,
                        include_directories: inc,
                        dependencies: [lle_termcap_dependency],
                        install: false)

# Line editor dependency for main lusush build
lle_dep = declare_dependency(link_with: lle_lib,
                            dependencies: [lle_termcap_dependency],
                            include_directories: include_directories('.'))

# Linux platform verification utility
linux_platform_check = executable('linux_platform_check',
                                  'linux_platform_check.c',
                                  dependencies: [lle_dep],
                                  install: false)
