# Lusush Line Editor (LLE) Build Configuration

# Include termcap subdirectory first
subdir('termcap')

lle_sources = [
  'text_buffer.c',
  'cursor_math.c',
  'terminal_manager.c'
]

lle_headers = [
  'text_buffer.h',
  'cursor_math.h',
  'terminal_manager.h'
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
