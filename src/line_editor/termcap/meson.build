# LLE Termcap Subdirectory Build Configuration
# This builds the integrated termcap system for the Lusush Line Editor (LLE)

# Termcap source files
lle_termcap_sources = files(
    'lle_termcap.c',
)

# Termcap headers (for installation and dependencies)
lle_termcap_headers = files(
    'lle_termcap.h',
    'lle_termcap_internal.h',
)

# Include directories for termcap
lle_termcap_inc = include_directories('.')

# Build termcap as a static library
lle_termcap_lib = static_library(
    'lle_termcap',
    lle_termcap_sources,
    include_directories: [lle_termcap_inc],
    c_args: [
        '-DLLE_TERMCAP_STANDALONE=1',
        '-std=c99',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-pedantic',
    ],
    dependencies: [],
    install: false,  # Internal library, not installed
)

# Dependency object for other parts of LLE to link against
lle_termcap_dep = declare_dependency(
    link_with: lle_termcap_lib,
    include_directories: lle_termcap_inc,
    sources: lle_termcap_headers,
)

# Export for parent build file
lle_termcap_dependency = lle_termcap_dep
