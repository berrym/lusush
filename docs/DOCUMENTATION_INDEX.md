# LUSUSH DOCUMENTATION INDEX

**Last Updated**: December 21, 2024  
**Shell Version**: 0.7.0-dev

## Current Active Documentation

### Core Project Documentation
- **[Project Status](../PROJECT_STATUS_CURRENT.md)** - Current functional status and technical overview
- **[Development Next Steps](../NEXT_STEPS_CURRENT.md)** - Immediate priorities and development roadmap
- **[README](../README.md)** - Main project overview, quick start, and usage guide
- **[POSIX Roadmap](../POSIX_ROADMAP.md)** - POSIX feature implementation roadmap and priorities

### Implementation Documentation
- **[Test Builtin Implementation](../TEST_BUILTIN_IMPLEMENTATION.md)** - Complete documentation of test/[ builtin implementation
- **[Case Statement Completion](../CASE_STATEMENT_COMPLETION.md)** - Documentation of case statement implementation
- **[Modern Parameter Expansion Summary](../MODERN_PARAMETER_EXPANSION_SUMMARY.md)** - Parameter expansion implementation details

### Technical Reference
- **Build System**: See `meson.build` for current build configuration
- **Testing**: See test files in project root (`test_*.c`, `test_*.sh`)
- **Code Structure**: Modern implementation in `src/` with headers in `include/`
- **Symbol Table**: Modern POSIX-compliant implementation in `src/symtable_modern.c`, `include/symtable_modern.h`

### Bug Fixes and Design Documents
- **[Assignment Parsing Bug Fix](ASSIGNMENT_PARSING_BUG_FIX.md)** - Documentation of critical assignment parsing bug fix
- **[Explicit List Types Design](EXPLICIT_LIST_TYPES_DESIGN.md)** - Design document for explicit AST list node types

## Archived Documentation

Legacy documentation has been moved to `docs/archived/` to maintain project history while keeping current documentation focused and up-to-date.

### Archived Files
- Migration phase documentation (MIGRATION_*.md)
- Legacy development checkpoints (DEVELOPMENT_*.md, CRITICAL_*.md)
- Historical technical analysis (TECHNICAL_*.md, PARSING_*.md)
- Legacy status summaries (CURRENT_STATUS_*.md)

## Quick Navigation

**For Current Status**: → [PROJECT_STATUS_CURRENT.md](../PROJECT_STATUS_CURRENT.md)  
**For Next Steps**: → [NEXT_STEPS_CURRENT.md](../NEXT_STEPS_CURRENT.md)  
**For Usage**: → [README.md](../README.md)  
**For Build Instructions**: → [README.md#quick-start](../README.md#quick-start)

## Documentation Guidelines

- Keep current docs in project root for easy access
- Archive completed phases and obsolete analysis in `docs/archived/`
- Update this index when adding new documentation
- Maintain clear references from README to active documentation

---
*This index helps maintain clean, current documentation while preserving project history.*
