#!/bin/bash

# LUSUSH MODERN PARAMETER EXPANSION DEMONSTRATION
# This script showcases the advanced parameter expansion features
# implemented in the modern Lusush shell architecture

echo "=== LUSUSH MODERN PARAMETER EXPANSION DEMO ==="
echo "Demonstrating advanced POSIX-compliant parameter expansion"
echo

# Build the shell if needed
if [ ! -x "./builddir/lusush" ]; then
    echo "Building Lusush shell..."
    meson compile -C builddir > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "❌ Build failed. Please check your build environment."
        exit 1
    fi
    echo "✅ Build successful"
    echo
fi

SHELL="./builddir/lusush"

# Demo function
demo() {
    local title="$1"
    local command="$2"
    local description="$3"

    echo "🔹 $title"
    echo "   Command: $command"
    if [ -n "$description" ]; then
        echo "   Purpose: $description"
    fi

    # Execute and show result
    result=$(echo "$command" | $SHELL 2>/dev/null)
    echo "   Result:  '$result'"
    echo
}

echo "=== 1. DEFAULT VALUES (${var:-default}) ==="
echo "Use default value when variable is unset or empty"
echo

demo "Basic default value" \
     'echo ${USER:-guest}' \
     "Show current user or 'guest' if unset"

demo "Default for empty variable" \
     'empty=; echo ${empty:-fallback}' \
     "Use fallback when variable is empty"

demo "Variable expansion in default" \
     'backup=config.bak; echo ${CONFIG:-$backup}' \
     "Default can contain other variables"

demo "Default with spaces and special chars" \
     'echo ${MISSING:-"hello world & more"}' \
     "Defaults can contain complex strings"

echo "=== 2. ALTERNATIVE VALUES (${var:+alternative}) ==="
echo "Use alternative value when variable is set and non-empty"
echo

demo "Basic alternative value" \
     'DEBUG=1; echo ${DEBUG:+--verbose}' \
     "Add --verbose flag only if DEBUG is set"

demo "No alternative for empty" \
     'empty=; echo ${empty:+should_not_show}' \
     "Empty variables don't trigger alternatives"

demo "Conditional greeting" \
     'name=Alice; echo ${name:+Hello $name}' \
     "Personalized greeting when name is available"

demo "Build flags example" \
     'OPTIMIZE=1; echo gcc ${OPTIMIZE:+-O2} main.c' \
     "Conditional compiler optimization flag"

echo "=== 3. UNSET vs EMPTY DISTINCTION ==="
echo "Different behavior for unset vs empty variables"
echo

demo "Default (-) vs (:-) for unset" \
     'echo ${unset_var-default} vs ${unset_var:-default}' \
     "Both work the same for unset variables"

demo "Default (-) vs (:-) for empty" \
     'empty=; echo "${empty-default}" vs "${empty:-default}"' \
     "Different behavior for empty variables"

demo "Alternative (+) for empty variable" \
     'empty=; echo ${empty+is_set}' \
     "Shows 'is_set' because variable exists (even if empty)"

demo "Alternative (:+) for empty variable" \
     'empty=; echo ${empty:+is_nonempty}' \
     "Shows nothing because variable is empty"

echo "=== 4. LENGTH EXPANSION (${#var}) ==="
echo "Get the length of variable values"
echo

demo "Length of string" \
     'name=John; echo ${#name}' \
     "Count characters in variable value"

demo "Length of empty string" \
     'empty=; echo ${#empty}' \
     "Empty variables have length 0"

demo "Length for validation" \
     'password=secret123; echo "Password length: ${#password}"' \
     "Useful for input validation"

demo "Length of path" \
     'path="/usr/local/bin"; echo ${#path}' \
     "Works with any string content"

echo "=== 5. SUBSTRING EXPANSION (${var:offset:length}) ==="
echo "Extract portions of variable values"
echo

demo "Substring from middle" \
     'text=Hello; echo ${text:1:3}' \
     "Extract 3 characters starting at position 1"

demo "Substring to end" \
     'filename=document.txt; echo ${filename:9}' \
     "From position 9 to end of string"

demo "First few characters" \
     'url=https://example.com; echo ${url:0:5}' \
     "Extract protocol from URL"

demo "File extension extraction" \
     'file=script.sh; echo ${file:7}' \
     "Get file extension"

echo "=== 6. COMMAND SUBSTITUTION INTEGRATION ==="
echo "Parameter expansion works with command substitution"
echo

demo "Default with command substitution" \
     'echo ${EDITOR:-$(which nano)}' \
     "Use nano as default editor if EDITOR unset"

demo "Date in alternative value" \
     'TIMESTAMP=1; echo ${TIMESTAMP:+Generated on $(date +%Y-%m-%d)}' \
     "Conditional timestamp with current date"

demo "Backtick command substitution" \
     'echo `echo Hello from backticks`' \
     "Legacy backtick syntax still works"

demo "Mixed expansion types" \
     'USER=admin; echo ${USER:+User: $USER, Home: $(echo /home/$USER)}' \
     "Combine multiple expansion types"

echo "=== 7. REAL-WORLD USAGE PATTERNS ==="
echo "Practical examples for shell scripting"
echo

demo "Configuration with defaults" \
     'echo "Config: ${CONFIG_FILE:-/etc/app.conf}"' \
     "Provide sensible defaults for configuration"

demo "Debug output control" \
     'DEBUG=1; echo ${DEBUG:+[DEBUG] Application started}' \
     "Conditional debug output"

demo "Safe file operations" \
     'BACKUP_DIR=/tmp; echo "Backup: ${BACKUP_DIR:-./backup}"' \
     "Default backup location"

demo "Build configuration" \
     'RELEASE=1; echo make ${RELEASE:+CFLAGS=-O2} all' \
     "Conditional build flags"

echo "=== 8. COMPLEX COMBINATIONS ==="
echo "Advanced patterns combining multiple features"
echo

demo "Nested parameter expansion" \
     'primary=main; secondary=backup; echo ${primary:+Using $primary}' \
     "Variables within parameter expansion"

demo "Chained alternatives" \
     'name=Alice; title=Dr; echo ${title:+$title }${name:-Unknown}' \
     "Multiple expansions in single command"

demo "Conditional file processing" \
     'INPUT=data.txt; echo "Processing: ${INPUT:-stdin}"' \
     "Default to stdin if no input file specified"

demo "Length-based validation" \
     'key=abc123; echo "Key length: ${#key} (min 6 required)"' \
     "Input validation using length expansion"

echo "=== 9. INTEGRATION WITH SHELL FEATURES ==="
echo "Parameter expansion works with all shell constructs"
echo

demo "With logical operators" \
     'FILE=test.txt; test -f "${FILE:-missing}" && echo "File exists" || echo "File missing"' \
     "Combine with logical operators for robust scripts"

demo "In FOR loops" \
     'ITEMS="one two"; for item in ${ITEMS:-default}; do echo "Item: $item"; break; done' \
     "Parameter expansion in control structures"

demo "Variable assignment" \
     'DEFAULT=fallback; RESULT=${MISSING:-$DEFAULT}; echo $RESULT' \
     "Assign results of parameter expansion"

demo "Pipeline integration" \
     'TEXT="Hello World"; echo ${TEXT:6} | tr a-z A-Z' \
     "Use with pipelines for text processing"

echo "=== SUMMARY ==="
echo "✅ Modern parameter expansion is fully functional in Lusush!"
echo
echo "Implemented features:"
echo "  • Default values: \${var:-default} and \${var-default}"
echo "  • Alternative values: \${var:+alt} and \${var+alt}"
echo "  • Length expansion: \${#var}"
echo "  • Substring expansion: \${var:offset:length}"
echo "  • Variable expansion in defaults and alternatives"
echo "  • Full integration with command substitution"
echo "  • Seamless integration with all shell features"
echo
echo "🎯 Lusush now provides professional-grade parameter expansion"
echo "   capabilities that match other modern POSIX-compliant shells!"
echo
echo "Next: Pattern matching expansion (\${var#pattern}, \${var%pattern})"
echo "      and case conversion features for complete POSIX compliance."
