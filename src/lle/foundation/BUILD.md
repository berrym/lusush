# LLE Foundation Layer - Meson Build System

The LLE (Lusush Line Editor) foundation layer is now fully integrated into the Meson build system.

## Building

From the project root:

```bash
# Configure (first time or after changes to meson.build)
meson setup builddir

# Compile
meson compile -C builddir

# Run tests
meson test -C builddir

# Run tests with verbose output
meson test -C builddir --print-errorlogs
```

## Build Artifacts

The build creates:
- **Static Library**: `builddir/src/lle/foundation/liblle_foundation.a`
- **Test Executables**: `builddir/src/lle/foundation/*_test`

## Test Suite

Six test suites are integrated:

1. **terminal_unit_test** - Terminal abstraction unit tests (no TTY required)
2. **buffer_test** - Gap buffer operations (14 tests)
3. **editor_test** - Editor integration tests (requires TTY)
4. **editor_advanced_test_v2** - Advanced editing operations (requires TTY)
5. **editor_kill_ring_test** - Kill ring system (10 tests)
6. **editor_search_test** - Incremental search (11 tests)

**Note**: Tests marked "requires TTY" will fail in CI/non-interactive environments. This is expected.

## Test Results

Expected passing tests (non-TTY):
- ✅ terminal_unit_test (8/8)
- ✅ buffer_test (14/14)
- ✅ editor_kill_ring_test (10/10)
- ✅ editor_search_test (11/11)

Interactive tests (require TTY):
- ⚠️ editor_test (10 tests, requires interactive terminal)
- ⚠️ editor_advanced_test_v2 (12 tests, requires interactive terminal)

**Total**: 4/6 automated test suites pass, 2/6 require manual testing in TTY

## Architecture

The LLE foundation is built as a static library (`liblle_foundation.a`) containing:
- Terminal abstraction layer
- Display system
- Gap buffer implementation
- Editor integration layer

Tests link against this library, ensuring consistent build and proper dependency management.

## Integration with Main Project

The main `meson.build` conditionally includes LLE if the directory exists:

```meson
if fs.is_dir('src/lle/foundation')
  subdir('src/lle/foundation')
endif
```

This allows the feature branch to include LLE while master branch remains unaffected.
