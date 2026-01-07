# Changelog

All notable changes to Lusush are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2026-01-XX

### Major Release: LLE, Multi-Mode Architecture, Extended Language Support

This release transforms Lusush from a POSIX shell with debugging into a full-featured
interactive shell with Bash/Zsh compatibility, a native line editor, and comprehensive
extended syntax support.

### Added

#### LLE - Lusush Line Editor
- Complete native line editor replacing GNU Readline dependency
- Emacs editing mode with 42 keybinding actions
- Context-aware tab completion for 45 shell builtins
- Real-time syntax highlighting with 30+ token types
- Semantic coloring (commands, strings, variables, paths, errors)
- Multi-line command editing with visual continuation
- Kill ring with yank/pop operations
- History integration with Ctrl-R search
- Prompt theming system

#### Shell Modes
- **POSIX Mode**: Strict POSIX sh compliance
- **Bash Mode**: Bash 5.x compatibility
- **Zsh Mode**: Zsh compatibility
- **Lusush Mode** (default): Curated best of Bash and Zsh

#### Extended Language Support

**Arrays (Phase 1)**
- Indexed arrays: `arr=(a b c)`, `${arr[0]}`, `${arr[@]}`
- Associative arrays: `declare -A map`, `${map[key]}`
- Array operations: append `+=`, length `${#arr[@]}`, keys `${!arr[@]}`

**Arithmetic (Phase 1)**
- Arithmetic command: `(( expression ))`
- `let` builtin for arithmetic evaluation
- Full operator support: `+ - * / % ** ++ -- << >> & | ^ ~ ! && ||`

**Extended Test (Phase 2)**
- Extended test syntax: `[[ expression ]]`
- Pattern matching: `[[ $var == pattern ]]`
- Regex matching: `[[ $var =~ regex ]]`
- Logical operators without quoting requirements

**Process Substitution (Phase 3)**
- Input substitution: `<(command)`
- Output substitution: `>(command)`
- Pipe stderr: `|&`
- Append both streams: `&>>`

**Extended Parameter Expansion (Phase 4)**
- Case modification: `${var^^}`, `${var,,}`, `${var^}`, `${var,}`
- Substring extraction: `${var:offset:length}`
- Pattern substitution: `${var/pattern/replacement}`, `${var//pattern/replacement}`
- Indirect expansion: `${!var}`, `${!prefix*}`
- Transformations: `${var@Q}`, `${var@E}`, `${var@A}`, `${var@U}`, `${var@L}`

**Extended Globbing (Phase 4)**
- Extended patterns: `?(pat)`, `*(pat)`, `+(pat)`, `@(pat)`, `!(pat)`
- Null glob: unmatched patterns expand to nothing
- Glob qualifiers: `*(.)` files, `*(/)` directories, `*(@)` symlinks, `*(*)` executables

**Control Flow Extensions (Phase 5)**
- Case fall-through: `;&`
- Case continue testing: `;;&`
- Select loop: `select var in list; do ... done`
- Time keyword: `time [-p] pipeline`

**Function Enhancements (Phase 6)**
- Name references: `local -n ref=varname`
- Anonymous functions: `() { body }`
- Return from sourced scripts

**Hook System (Phase 7)**
- `precmd`: Called before each prompt
- `preexec`: Called before command execution (receives command as $1)
- `chpwd`: Called after directory change
- `periodic`: Called every PERIOD seconds
- Hook arrays: `precmd_functions`, `preexec_functions`, `chpwd_functions`

**Plugin System Foundation (Phase 7)**
- Plugin manager architecture
- Permission-based sandboxing
- Dynamic loading infrastructure

### Changed

- Shell description updated to reflect new capabilities
- Build system no longer requires GNU Readline
- Default mode is now Lusush mode (combines best of Bash/Zsh)

### Removed

- GNU Readline dependency (replaced by LLE)

### Fixed

- Memory leaks in AST sibling node handling
- Double-strdup leak in variable expansion
- Arithmetic stack item cleanup after evaluation
- Zero memory leaks verified with macOS `leaks` tool

---

## [1.3.0] - 2024-10-XX

### Added
- Integrated interactive debugger with GDB-like interface
- Breakpoints, stepping, variable inspection
- Layered display architecture
- 6 enterprise themes
- 100% POSIX shell option compliance (24 options)
- Context-aware tab completion
- Git integration in prompts
- Configuration system with INI-style files

### Changed
- Complete rewrite of display system
- Improved memory management with pool optimization

---

## [1.2.0] - 2024-XX-XX

Initial public release with core POSIX shell functionality.
