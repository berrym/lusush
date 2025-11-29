# src/libfuzzy/meson.build
# Fuzzy String Matching Library Build Configuration
#
# This library provides Unicode-aware fuzzy string matching algorithms
# used by both the shell's autocorrect system and LLE's completion/history.

libfuzzy_sources = files('fuzzy_match.c')

# Export sources to parent scope for integration into lusush
# Note: libfuzzy depends on LLE's unicode_compare.c and utf8_support.c
