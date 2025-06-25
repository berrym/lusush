# LUSUSH SHELL PROJECT WORKFLOW REFERENCE

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    LUSUSH SHELL WORKFLOW CONTROL BOX                       │
│                         [CLICK TO STAY CURRENT]                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  STATUS: 85% POSIX Compliant | Tests: 49/49 | Build: Clean | Docs: Current │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  WORKFLOW COMMANDS [Execute in sequence for each development cycle]        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  [BUILD]     ninja -C builddir                                             │
│  [TEST]      ./test_posix_regression.sh                                    │
│  [FORMAT]    ./clang-format-all .                                          │
│  [COMMIT]    git add -A && git commit -m "Text-only message"               │
│  [DOCS]      Update PROJECT_STATUS_CURRENT.md and README.md                │
│  [VERIFY]    ./test_posix_regression.sh | tail -n 5                        │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  CRITICAL FILES [Monitor and maintain these files]                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  CORE SOURCE:                                                               │
│  • src/executor.c      - Main execution engine                             │
│  • src/tokenizer.c     - Input parsing and tokens                          │
│  • src/parser.c        - AST generation                                    │
│  • src/symtable.c      - Symbol management                                 │
│                                                                             │
│  DOCUMENTATION:                                                             │
│  • PROJECT_STATUS_CURRENT.md - Project status                              │
│  • README.md                 - Overview                                    │
│  • ACHIEVEMENT_SUMMARY_DECEMBER_2024.md - Monthly tracking                 │
│                                                                             │
│  TESTING:                                                                   │
│  • test_posix_regression.sh - 49 POSIX compliance tests                    │
│  • test-comprehensive.sh    - Feature validation                           │
│  • test_broken_features_diagnosis.sh - Edge case detection                 │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  RECENT ACHIEVEMENTS [Last completed work]                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  DECEMBER 2024 - COMMAND SUBSTITUTION ENHANCEMENT:                         │
│  • Fixed nested quotes: echo "Count: $(echo "a b c" | wc -w)"              │
│  • Added backtick support: echo "Today is `date +%A`"                      │
│  • Enhanced tokenizer boundary detection                                   │
│  • Files: src/tokenizer.c, src/executor.c                                  │
│                                                                             │
│  DECEMBER 2024 - ENHANCED SYMBOL TABLE SYSTEM:                             │
│  • 3-4x performance improvement with libhashtable                          │
│  • Fixed critical unset variable bug                                       │
│  • Maintained API compatibility                                            │
│  • Files: src/symtable.c, include/symtable.h                               │
│                                                                             │
│  DECEMBER 2024 - EDGE CASE RESOLUTION:                                     │
│  • Multiple parameter expansions: ${n1=4} ${n2=6}                          │
│  • Arithmetic variables: $(($n1*$n2))                                      │
│  • Enhanced error handling                                                 │
│  • Files: src/executor.c, src/arithmetic.c                                 │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  COMMIT STANDARDS [Required format for all commits]                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  FORMAT: Text-only, no emojis, no symbols                                  │
│  STRUCTURE:                                                                 │
│  • Title line describing technical achievement                             │
│  • Blank line                                                              │
│  • Detailed technical content                                              │
│  • What was fixed, how implemented, test results                           │
│                                                                             │
│  EXAMPLE:                                                                   │
│  "Fix nested quotes command substitution                                   │
│                                                                             │
│  Enhanced tokenizer to handle command substitution boundaries              │
│  within double-quoted strings. Fixed position calculation                  │
│  in expand_quoted_string function.                                         │
│                                                                             │
│  All 49/49 POSIX regression tests maintained."                             │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  QUALITY CHECKLIST [Complete for each development cycle]                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  PRE-DEVELOPMENT:                                                           │
│  [ ] Review PROJECT_STATUS_CURRENT.md                                      │
│  [ ] Identify specific target issue                                        │
│  [ ] Plan surgical implementation                                          │
│                                                                             │
│  DEVELOPMENT:                                                               │
│  [ ] Make focused changes                                                   │
│  [ ] Test with relevant scripts                                            │
│  [ ] Verify ninja build success                                            │
│  [ ] Confirm 49/49 regression tests pass                                   │
│                                                                             │
│  POST-DEVELOPMENT:                                                          │
│  [ ] Execute ./clang-format-all .                                          │
│  [ ] Create text-only git commit                                           │
│  [ ] Update core documentation                                             │
│  [ ] Final regression validation                                           │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  PROJECT METRICS [Current status indicators]                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  POSIX COMPLIANCE: 85% achieved, 49/49 regression tests passing            │
│  CODE QUALITY: clang-format enforced, minimal warnings                     │
│  PERFORMANCE: 3-4x improvement in symbol operations                        │
│  RELIABILITY: Comprehensive test coverage with edge case validation        │
│                                                                             │
│  ARCHITECTURE: ISO C99, Meson build, libhashtable dependency               │
│  TARGET: POSIX shell specification compliance                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

USAGE: This clickable reference box maintains familiarity with the Lusush
Shell project status and established development workflow. Follow the workflow
commands in sequence for each development cycle to ensure consistency with
project standards and maintain the excellent foundation and compliance record.