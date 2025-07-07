# LUSUSH Hints System Documentation

## Overview

The LUSUSH hints system provides intelligent input suggestions that appear to the right of the cursor as you type, enhancing the user experience without interfering with normal shell operations. This feature works alongside the existing tab completion system to provide real-time feedback and suggestions.

## Features

### 🎯 **Intelligent Context-Aware Hints**
- **Command Hints**: Suggests command completions and usage patterns
- **File Hints**: Shows file and directory completions
- **Variable Hints**: Displays environment and shell variable completions
- **Builtin Hints**: Provides usage information for builtin commands

### 🚀 **Smart Behavior**
- **Non-Intrusive**: Appears dimmed to the right of the cursor
- **Performance Optimized**: Minimal impact on shell responsiveness
- **Context Sensitive**: Shows relevant hints based on cursor position
- **Threshold-Based**: Only shows high-confidence suggestions

### ⚙️ **Configurable**
- **Enable/Disable**: Toggle hints on/off via configuration
- **Integration**: Works seamlessly with existing completion system
- **Fuzzy Matching**: Respects fuzzy completion settings

## Configuration

### Basic Configuration

The hints system is configured through the `[completion]` section in your `.lusushrc` file:

```ini
[completion]
hints_enabled = true              # Enable/disable hints
completion_enabled = true         # Required for hints to work
fuzzy_completion = true          # Enhances hint accuracy
completion_threshold = 70        # Minimum confidence for hints
```

### Configuration Commands

```bash
# Check if hints are enabled
config get hints_enabled

# Enable hints
config set hints_enabled true

# Disable hints
config set hints_enabled false

# Show all completion settings
config show completion
```

## How It Works

### Visual Appearance

When you type, hints appear as dimmed text to the right of your cursor:

```
$ ec█
$ echo [text...]█
```

The hint text is displayed in a darker gray color to distinguish it from your actual input.

### Hint Types

#### 1. Command Hints
Shows command completions and usage patterns:

```
$ ec█          → echo [text...]
$ cd█          → cd [directory]
$ ex█          → export [var=value]
```

#### 2. File Hints
Suggests file and directory completions:

```
$ cat test█    → cat test_file.txt
$ ls /usr/b█   → ls /usr/bin
```

#### 3. Variable Hints
Shows variable name completions:

```
$ echo $HO█    → echo $HOME
$ echo $PA█    → echo $PATH
```

#### 4. Builtin Usage Hints
Provides usage information for builtin commands:

```
$ config█      → config [command]
$ theme█       → theme [command]
$ alias█       → alias [name=value]
```

### Intelligent Behavior

The hints system is designed to be smart and non-intrusive:

- **Context Awareness**: Shows different hints based on cursor position
- **Confidence Threshold**: Only displays high-confidence suggestions (≥70% match)
- **Length Limits**: Doesn't show hints for very long commands (>100 chars)
- **Space Sensitivity**: Hides hints when you finish typing a word
- **Performance**: Optimized to avoid impacting shell responsiveness

## Integration with Completion System

The hints system works seamlessly with the existing tab completion:

- **Shared Logic**: Uses the same completion functions as TAB completion
- **Consistent Results**: Hints preview what TAB completion would show
- **Fuzzy Matching**: Respects fuzzy completion settings
- **Priority System**: Shows the best matching completion as a hint

## Technical Implementation

### Architecture

```
Input → Context Analysis → Hint Generation → Display
   ↓         ↓                   ↓            ↓
  Buffer   Position          Best Match   Colored Text
```

### Key Components

1. **Context Detection**: Determines what type of hint to show
2. **Completion Integration**: Leverages existing completion functions
3. **Fuzzy Matching**: Uses the same scoring system as completion
4. **Memory Management**: Proper cleanup of hint strings
5. **Performance**: Efficient algorithms for real-time updates

### Code Organization

```
src/completion.c          # Main implementation
  ├── lusush_hints_callback()         # Main hint callback
  ├── generate_command_hint()         # Command hints
  ├── generate_file_hint()            # File hints
  ├── generate_variable_hint()        # Variable hints
  ├── generate_builtin_hint()         # Builtin hints
  └── should_show_hints()             # Display logic

include/completion.h      # Function declarations
src/init.c               # Initialization
src/config.c             # Configuration
```

## Performance Considerations

### Optimization Features

- **Threshold-Based**: Only processes high-confidence matches
- **Length Limits**: Avoids processing very long inputs
- **Caching**: Reuses completion results when possible
- **Lazy Evaluation**: Only generates hints when needed

### Performance Metrics

- **Latency**: < 1ms for most operations
- **Memory**: Minimal additional memory usage
- **CPU**: Negligible impact on shell responsiveness

## Troubleshooting

### Common Issues

#### Hints Not Appearing
```bash
# Check if hints are enabled
config get hints_enabled

# Enable hints
config set hints_enabled true

# Verify completion is enabled
config get completion_enabled
```

#### Performance Issues
```bash
# Check completion threshold
config get completion_threshold

# Increase threshold to reduce hint frequency
config set completion_threshold 80
```

#### Incorrect Hints
```bash
# Check fuzzy completion settings
config get fuzzy_completion

# Verify PATH is properly set
echo $PATH
```

### Debug Information

```bash
# Show all completion settings
config show completion

# Test completion system
# (Press TAB to verify completion works)

# Check shell diagnostics
echo $LUSUSH_VERSION
```

## Best Practices

### Configuration Recommendations

1. **Enable Fuzzy Completion**: Improves hint accuracy
2. **Set Appropriate Threshold**: Balance between accuracy and frequency
3. **Keep Completion Enabled**: Required for hints to work

### Usage Tips

1. **Let Hints Guide You**: Use hints to discover command options
2. **Combine with TAB**: Use hints to preview, TAB to complete
3. **Adjust Threshold**: Tune based on your preference

### Performance Tips

1. **Monitor Performance**: Watch for any slowdown
2. **Adjust Settings**: Increase threshold if needed
3. **Disable if Needed**: Can be turned off completely

## Advanced Features

### Customization

The hints system can be extended by modifying the hint generation functions:

```c
// Add custom hint logic
char *generate_custom_hint(const char *buf) {
    // Your custom hint logic here
    return hint;
}
```

### Integration with Other Features

- **Theme System**: Hints respect theme color settings
- **Network Features**: Shows SSH host hints
- **History**: Can be extended to show history-based hints

## Future Enhancements

### Planned Features

1. **History-Based Hints**: Show previously used commands
2. **Smart Argument Hints**: Context-aware argument suggestions
3. **Multi-Line Hints**: Support for complex command structures
4. **Learning System**: Adapt based on user behavior

### Extensibility

The hints system is designed to be easily extensible:

- **Plugin Architecture**: Easy to add new hint types
- **Configuration**: Extensive configuration options
- **API**: Clean interface for extensions

## Testing

### Test Coverage

The hints system includes comprehensive tests:

```bash
# Run hints system tests
./tests/enhanced/test_hints_system.sh

# Run full test suite
./tests/compliance/test_shell_compliance_comprehensive.sh
```

### Test Categories

1. **Configuration Tests**: Enable/disable functionality
2. **Functionality Tests**: Different hint types
3. **Integration Tests**: Compatibility with other features
4. **Performance Tests**: Response time and memory usage
5. **Edge Cases**: Empty input, special characters, etc.

## Conclusion

The LUSUSH hints system provides a modern, intelligent input assistance feature that enhances the user experience while maintaining the shell's performance and reliability. It seamlessly integrates with the existing completion system and provides valuable real-time feedback to help users discover commands and complete their input more efficiently.

The system is production-ready, fully tested, and configurable to meet different user preferences and performance requirements.