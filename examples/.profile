#!/bin/sh
# Example .profile script for Lush Shell
# This file is executed by login shells following POSIX conventions
# Compatible with traditional shell users migrating from bash/zsh

# Set PATH for common directories
export PATH="$HOME/bin:$HOME/.local/bin:/usr/local/bin:$PATH"

# Set default editor and pager
export EDITOR=vi
export PAGER=less

# Set locale
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8

# Set default permissions
umask 022

# XDG Base Directory Specification
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_DATA_HOME="$HOME/.local/share"
export XDG_CACHE_HOME="$HOME/.cache"

# Development environment variables
export CLICOLOR=1
export GREP_OPTIONS='--color=auto'

# History configuration (can be overridden by .lushrc)
export HISTSIZE=10000
export HISTFILE="$HOME/.lush_history"

# Set terminal title capability
case "$TERM" in
    xterm*|rxvt*|screen*)
        export TERM_TITLE_SUPPORT=1
        ;;
esac

# Load system-wide profile if it exists
if [ -f /etc/profile ]; then
    . /etc/profile
fi

# Load user-specific profile extensions
if [ -d "$HOME/.profile.d" ]; then
    for script in "$HOME/.profile.d"/*.sh; do
        if [ -r "$script" ]; then
            . "$script"
        fi
    done
fi

# Print login message for interactive login shells
if [ -n "$PS1" ]; then
    echo "Welcome to Lush Shell - Modern POSIX-compliant shell"
    echo "Profile loaded: $(date)"

    # Show system information
    if command -v uname >/dev/null 2>&1; then
        echo "System: $(uname -s) $(uname -r)"
    fi

    # Show available themes
    if command -v theme >/dev/null 2>&1; then
        echo "Available themes: corporate, dark, light, colorful, minimal, classic"
        echo "Current theme: $(theme info 2>/dev/null | head -1 || echo 'default')"
    fi
fi
