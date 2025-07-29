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
  'enhanced_tab_completion.c'
]

lle_headers = [
  'text_buffer.h',
  'cursor_math.h',
  'terminal_manager.h',
  'prompt.h',
  'display.h',
  'display_performance.h',
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
  'enhanced_tab_completion.h'
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
