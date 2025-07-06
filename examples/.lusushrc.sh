#!/usr/bin/env lusush
# Example .lusushrc.sh script for LUSUSH Shell
# This file is executed by interactive shells for shell-specific configuration
# Provides traditional shell script configuration alongside the modern config system

# ============================================================================
# LUSUSH Shell Configuration Script
# ============================================================================
# This script demonstrates how to configure LUSUSH using traditional shell
# commands while leveraging the modern config system. Advanced users can
# combine both approaches for maximum flexibility.

# Set shell options using traditional commands
set -o emacs          # Use emacs-style line editing
set +o noclobber      # Allow output redirection to overwrite files

# Define useful aliases (these work alongside config-defined aliases)
alias ls='ls --color=auto'
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'
alias dir='ls -la'
alias ..='cd ..'
alias ...='cd ../..'
alias ....='cd ../../..'

# Git aliases for developers
alias gs='git status'
alias ga='git add'
alias gc='git commit'
alias gd='git diff'
alias gl='git log --oneline'
alias gp='git push'
alias gpl='git pull'
alias gb='git branch'
alias gco='git checkout'

# System aliases
alias df='df -h'
alias du='du -h'
alias free='free -h'
alias ps='ps aux'
alias top='top -o cpu'
alias grep='grep --color=auto'
alias fgrep='fgrep --color=auto'
alias egrep='egrep --color=auto'

# Development aliases
alias make='make -j$(nproc)'
alias python='python3'
alias pip='pip3'
alias serve='python -m http.server'
alias json='python -m json.tool'

# Safety aliases
alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'
alias ln='ln -i'

# Define useful functions
# Function to create directory and cd into it
mkcd() {
    mkdir -p "$1" && cd "$1"
}

# Function to extract various archive types
extract() {
    if [ -f "$1" ]; then
        case "$1" in
            *.tar.bz2)   tar xjf "$1"     ;;
            *.tar.gz)    tar xzf "$1"     ;;
            *.bz2)       bunzip2 "$1"     ;;
            *.rar)       unrar x "$1"     ;;
            *.gz)        gunzip "$1"      ;;
            *.tar)       tar xf "$1"      ;;
            *.tbz2)      tar xjf "$1"     ;;
            *.tgz)       tar xzf "$1"     ;;
            *.zip)       unzip "$1"       ;;
            *.Z)         uncompress "$1"  ;;
            *.7z)        7z x "$1"        ;;
            *)           echo "'$1' cannot be extracted via extract()" ;;
        esac
    else
        echo "'$1' is not a valid file"
    fi
}

# Function to show directory tree
tree() {
    if command -v tree >/dev/null 2>&1; then
        command tree "$@"
    else
        find "${1:-.}" -print | sed -e 's;[^/]*/;|____;g;s;____|; |;g'
    fi
}

# Function to find files by name
ff() {
    find . -type f -iname "*$1*" 2>/dev/null
}

# Function to find directories by name
fd() {
    find . -type d -iname "*$1*" 2>/dev/null
}

# Function to show PATH in readable format
showpath() {
    echo "$PATH" | tr ':' '\n' | nl
}

# Function to backup a file
backup() {
    cp "$1" "$1.backup.$(date +%Y%m%d%H%M%S)"
}

# Function to show disk usage of current directory
usage() {
    du -sh * | sort -hr
}

# Function to show network connections
netstat() {
    if command -v ss >/dev/null 2>&1; then
        ss -tuln
    else
        command netstat -tuln
    fi
}

# Set up development environment
if [ -d "$HOME/Development" ]; then
    export PROJECTS_DIR="$HOME/Development"
    alias projects="cd $PROJECTS_DIR"
fi

# Set up Node.js environment if available
if command -v node >/dev/null 2>&1; then
    export NODE_ENV=development
    alias nls='npm list --depth=0'
    alias ngs='npm list -g --depth=0'
fi

# Set up Python environment if available
if command -v python3 >/dev/null 2>&1; then
    export PYTHONDONTWRITEBYTECODE=1
    alias pyserver='python3 -m http.server'
    alias pyjson='python3 -m json.tool'
fi

# Set up Go environment if available
if command -v go >/dev/null 2>&1; then
    export GOPATH="$HOME/go"
    export PATH="$GOPATH/bin:$PATH"
fi

# Set up Rust environment if available
if [ -f "$HOME/.cargo/env" ]; then
    . "$HOME/.cargo/env"
fi

# Configure modern shell features using config commands
# These work alongside the .lusushrc configuration file

# Enable advanced features if running interactively
if [ -n "$PS1" ]; then
    # Set theme based on terminal capabilities
    if [ "$TERM_PROGRAM" = "iTerm.app" ] || [ "$COLORTERM" = "truecolor" ]; then
        theme set corporate
    else
        theme set minimal
    fi

    # Configure completion
    config set completion_enabled true
    config set fuzzy_completion true
    config set completion_threshold 70

    # Configure history
    config set history_enabled true
    config set history_size 10000
    config set history_no_dups true

    # Configure auto-correction
    config set spell_correction true
    config set autocorrect_interactive true
    config set autocorrect_threshold 60
fi

# Load user-specific extensions
if [ -d "$HOME/.lusush.d" ]; then
    for script in "$HOME/.lusush.d"/*.sh; do
        if [ -r "$script" ]; then
            . "$script"
        fi
    done
fi

# Load project-specific configuration
if [ -f "$PWD/.lusushrc.local" ]; then
    . "$PWD/.lusushrc.local"
fi

# Print startup message for interactive shells
if [ -n "$PS1" ]; then
    echo "LUSUSH configuration loaded from .lusushrc.sh"
    echo "Traditional shell commands and modern config system active"
    echo "Type 'config show' to see current configuration"
    echo "Type 'theme list' to see available themes"
fi

# Set up completion for common commands
if command -v complete >/dev/null 2>&1; then
    complete -d cd
    complete -c which
    complete -f -X '!*.@(tar|tgz|tar.gz|tar.bz2|tbz2)' tar
    complete -f -X '!*.@(zip|jar|war|ear)' unzip
fi

# Export functions for use in subshells
export -f mkcd extract tree ff fd showpath backup usage
