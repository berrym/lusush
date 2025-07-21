#!/bin/bash
# LLE Meson Build Helper Script - With Integrated Termcap System

BUILDDIR=${BUILDDIR:-builddir}

case "$1" in
    "setup")
        echo "Setting up build directory for LLE with integrated termcap..."
        meson setup $BUILDDIR
        ;;
    "build")
        echo "Building LLE components with integrated termcap..."
        meson compile -C $BUILDDIR
        ;;
    "test")
        echo "Running LLE tests including termcap integration..."
        meson test -C $BUILDDIR --suite lle
        ;;
    "test-verbose")
        echo "Running LLE tests (verbose) including termcap..."
        meson test -C $BUILDDIR --suite lle --verbose
        ;;
    "test-termcap")
        echo "Running termcap-specific tests..."
        meson test -C $BUILDDIR --suite termcap
        ;;
    "test-all")
        echo "Running all LLE tests (including termcap)..."
        meson test -C $BUILDDIR
        ;;
    "benchmark")
        echo "Running LLE benchmarks..."
        $BUILDDIR/tests/line_editor/lle_benchmark
        ;;
    "clean")
        echo "Cleaning build directory..."
        rm -rf $BUILDDIR
        ;;
    "reconfigure")
        echo "Reconfiguring build for termcap integration..."
        meson setup --reconfigure $BUILDDIR
        ;;
    "verify-termcap")
        echo "Verifying termcap integration..."
        echo "Checking for lle_termcap functions in library..."
        nm $BUILDDIR/src/line_editor/liblle.a | grep lle_termcap || echo "No termcap functions found (integration pending)"
        ;;
    "status")
        echo "LLE Development Status:"
        echo "======================"
        echo "Architecture: Standalone library with integrated termcap"
        echo "Current Phase: Foundation (Phase 1 of 4)"
        echo "Build System: Meson"
        echo "Next Task: LLE-009 (Termcap System Integration)"
        echo ""
        echo "Key Features:"
        echo "- Complete termcap system from Lusush (2000+ lines)"
        echo "- iTerm2 and macOS support preserved"
        echo "- No external termcap dependencies"
        echo "- Standalone library like libhashtable"
        ;;
    *)
        echo "Usage: $0 {setup|build|test|test-verbose|test-termcap|test-all|benchmark|clean|reconfigure|verify-termcap|status}"
        echo ""
        echo "Commands:"
        echo "  setup         - Initialize meson build directory for LLE+termcap"
        echo "  build         - Build LLE components with integrated termcap"
        echo "  test          - Run LLE tests"
        echo "  test-verbose  - Run LLE tests with verbose output"
        echo "  test-termcap  - Run termcap-specific tests"
        echo "  test-all      - Run all tests (LLE + termcap)"
        echo "  benchmark     - Run performance benchmarks"
        echo "  clean         - Clean build directory"
        echo "  reconfigure   - Reconfigure build system"
        echo "  verify-termcap- Verify termcap integration status"
        echo "  status        - Show LLE development status"
        echo ""
        echo "Termcap Integration Status: PLANNED (LLE-009)"
        echo "Architecture: Standalone library with integrated termcap system"
        exit 1
        ;;
esac
