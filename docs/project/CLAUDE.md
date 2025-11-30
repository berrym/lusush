# Claude Code Instructions for Lusush

## CRITICAL: Build Directory

**ALWAYS use `builddir` as the meson build directory. NEVER use `build` or any other name.**

```bash
# Correct - ALWAYS use this:
meson compile -C builddir
meson compile -C builddir lusush
./builddir/lusush

# WRONG - NEVER use these:
meson compile -C build        # NO!
./build/lusush                 # NO!
```

If you see a `build/` directory exists alongside `builddir/`, alert the user immediately and recommend removing the duplicate to prevent confusion.

## Build Commands

```bash
# Configure (only needed once or after meson.build changes)
meson setup builddir

# Build the main binary
meson compile -C builddir lusush

# Build everything
meson compile -C builddir

# Run tests
meson test -C builddir

# Test with LLE enabled
LLE_ENABLED=1 ./builddir/lusush
```

## Code Style

- C11 standard
- 4-space indentation
- No trailing whitespace
- Function comments use `/** @brief */` doxygen style for public APIs
- Use `snake_case` for functions and variables
- Use `UPPER_SNAKE_CASE` for constants and macros
- Prefix LLE functions with `lle_`
- Prefix Lusush functions with `lusush_` where disambiguation needed

## Commit Messages

- LLE-related commits MUST start with "LLE:" prefix
- No emojis in commit messages
- Keep first line under 72 characters
- Provide detailed body for non-trivial changes

## Key Documentation

- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - Session handoff, current status, priorities
- `docs/lle_specification/` - LLE spec documents
- `docs/lle_implementation/` - Implementation plans and status

## User Preferences

1. NO destructive git operations without explicit approval
2. Themeable everything - core design philosophy
3. Opinionated defaults - features like dedup should be ON by default
4. Unicode/UTF-8 completeness preferred over "good enough"
5. Test changes before committing when practical
