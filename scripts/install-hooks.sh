#!/bin/sh
# install-hooks.sh - Install git hooks for lusush development
#
# This script creates symlinks from .git/hooks/ to scripts/hooks/
# ensuring all developers use the same pre-commit and commit-msg hooks.
#
# Usage: ./scripts/install-hooks.sh
#
# The script is POSIX-compliant for portability across Linux, macOS, and BSD.

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_SOURCE="$SCRIPT_DIR/hooks"
HOOKS_TARGET="$REPO_ROOT/.git/hooks"

# Verify we're in a git repository
if [ ! -d "$REPO_ROOT/.git" ]; then
    echo "Error: Not a git repository (or .git directory not found)"
    echo "Please run this script from within the lusush repository."
    exit 1
fi

# Verify hooks source directory exists
if [ ! -d "$HOOKS_SOURCE" ]; then
    echo "Error: Hooks source directory not found: $HOOKS_SOURCE"
    exit 1
fi

echo "Installing git hooks for lusush..."
echo "  Source: $HOOKS_SOURCE"
echo "  Target: $HOOKS_TARGET"
echo ""

# Create .git/hooks directory if it doesn't exist
mkdir -p "$HOOKS_TARGET"

# Install each hook
installed=0
for hook in "$HOOKS_SOURCE"/*; do
    if [ -f "$hook" ]; then
        hook_name="$(basename "$hook")"
        target="$HOOKS_TARGET/$hook_name"

        # Remove existing hook (file or symlink)
        if [ -e "$target" ] || [ -L "$target" ]; then
            echo "  Removing existing: $hook_name"
            rm -f "$target"
        fi

        # Create symlink using relative path for portability
        # From .git/hooks/ to scripts/hooks/ is ../../scripts/hooks/
        ln -s "../../scripts/hooks/$hook_name" "$target"

        # Verify the symlink works
        if [ -x "$target" ]; then
            echo "  Installed: $hook_name"
            installed=$((installed + 1))
        else
            echo "  Warning: $hook_name installed but not executable"
        fi
    fi
done

echo ""
if [ $installed -gt 0 ]; then
    echo "Successfully installed $installed hook(s)."
    echo ""
    echo "Installed hooks:"
    ls -la "$HOOKS_TARGET" | grep -v "\.sample$" | grep "^l"
else
    echo "No hooks found to install."
fi

echo ""
echo "Done."
