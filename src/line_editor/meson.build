# Lusush Line Editor (LLE) Build Configuration

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
                        dependencies: [],
                        install: false)

# Line editor dependency for main lusush build
lle_dep = declare_dependency(link_with: lle_lib,
                            include_directories: include_directories('.'))
