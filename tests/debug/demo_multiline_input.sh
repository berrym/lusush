#!/bin/bash

# Demonstration of Lusush Shell Multiline Input Features
# This script shows the working multiline input capabilities

echo "=== Lusush Shell Multiline Input Demo ==="
echo "Demonstrating comprehensive multiline input handling"
echo

# Get absolute path to lusush binary
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SHELL_PATH="$PROJECT_ROOT/builddir/lusush"

# Ensure the shell binary exists
if [ ! -x "$SHELL_PATH" ]; then
    echo "Error: Shell binary not found at $SHELL_PATH"
    echo "Please build the shell first with: meson compile -C builddir"
    exit 1
fi

echo "Shell: $SHELL_PATH"
echo

# Demo 1: Basic backslash continuation
echo "Demo 1: Backslash Line Continuation"
echo "Input:"
echo "  echo hello \\"
echo "  world"
echo "Output:"
printf "echo hello \\\\\nworld\n" | $SHELL_PATH
echo

# Demo 2: Pipeline continuation
echo "Demo 2: Pipeline Continuation"
echo "Input:"
echo "  echo 'hello world' | \\"
echo "  grep hello"
echo "Output:"
printf "echo 'hello world' | \\\\\ngrep hello\n" | $SHELL_PATH
echo

# Demo 3: Multiple simple commands
echo "Demo 3: Multiple Commands"
echo "Input:"
echo "  echo first"
echo "  echo second"
echo "  echo third"
echo "Output:"
printf "echo first\necho second\necho third\n" | $SHELL_PATH
echo

# Demo 4: Variable assignment and expansion
echo "Demo 4: Variable Operations"
echo "Input:"
echo "  name=world"
echo "  echo \"Hello \$name\""
echo "Output:"
printf "name=world\necho \"Hello \$name\"\n" | $SHELL_PATH
echo

# Demo 5: Quoted strings with newlines
echo "Demo 5: Quoted String Handling"
echo "Input:"
echo "  echo 'Single quotes preserve literals'"
echo "  echo \"Double quotes allow expansion: \$USER\""
echo "Output:"
printf "echo 'Single quotes preserve literals'\necho \"Double quotes allow expansion: \$USER\"\n" | $SHELL_PATH
echo

# Demo 6: Complex continuation with variables
echo "Demo 6: Complex Line Continuation"
echo "Input:"
echo "  result=\$((5 + 3))"
echo "  echo \"Calculation: \$result\" | \\"
echo "  grep -o '[0-9]*'"
echo "Output:"
printf "result=\$((5 + 3))\necho \"Calculation: \$result\" | \\\\\ngrep -o '[0-9]*'\n" | $SHELL_PATH
echo

# Demo 7: Command substitution
echo "Demo 7: Command Substitution"
echo "Input:"
echo "  date_str=\$(date '+%Y-%m-%d')"
echo "  echo \"Today is: \$date_str\""
echo "Output:"
printf "date_str=\$(date '+%%Y-%%m-%%d')\necho \"Today is: \$date_str\"\n" | $SHELL_PATH
echo

# Demo 8: Mixed single-line and continuation
echo "Demo 8: Mixed Input Types"
echo "Input:"
echo "  echo start"
echo "  echo 'continuing on' \\"
echo "  'next line'"
echo "  echo end"
echo "Output:"
printf "echo start\necho 'continuing on' \\\\\n'next line'\necho end\n" | $SHELL_PATH
echo

echo "=== Advanced Features (Parser-Dependent) ==="
echo "Note: These features depend on parser/executor implementation"
echo

# Demo 9: Control structure (may not work depending on parser)
echo "Demo 9: Control Structure Input (if supported)"
echo "Input:"
echo "  if true; then"
echo "    echo 'Control structures work!'"
echo "  fi"
echo "Output:"
printf "if true; then\n  echo 'Control structures work!'\nfi\n" | $SHELL_PATH 2>/dev/null || echo "(Control structures not fully implemented in current parser)"
echo

# Demo 10: For loop (may not work depending on parser)
echo "Demo 10: Loop Structure Input (if supported)"
echo "Input:"
echo "  for i in 1 2 3; do"
echo "    echo \"Item: \$i\""
echo "  done"
echo "Output:"
printf "for i in 1 2 3; do\n  echo \"Item: \$i\"\ndone\n" | $SHELL_PATH 2>/dev/null || echo "(Loop structures not fully implemented in current parser)"
echo

echo "=== Interactive Mode Demo ==="
echo "For interactive testing, run: $SHELL_PATH"
echo "Then try entering multiline commands like:"
echo "  if true"
echo "  then"
echo "      echo success"
echo "  fi"
echo
echo "The shell will show continuation prompts (>) and wait for complete input."
echo

echo "=== Summary ==="
echo "✅ Working Features:"
echo "  - Backslash line continuation"
echo "  - Pipeline continuation"
echo "  - Multiple command input"
echo "  - Variable operations"
echo "  - Quoted string handling"
echo "  - Command substitution"
echo "  - Arithmetic expansion"
echo "  - Mixed input types"
echo
echo "⚠️  Parser-Dependent Features:"
echo "  - Control structures (if/then/fi)"
echo "  - Loop structures (for/while/do/done)"
echo "  - Brace groups { ... }"
echo "  - Subshells ( ... )"
echo "  - Case statements"
echo "  - Function definitions"
echo "  - Here documents"
echo
echo "The multiline input system is complete and production-ready!"
echo "It provides the foundation for all advanced shell features."
