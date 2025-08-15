/*
 * Prompt-Aware Syntax Highlighting Prototype
 * Demonstrates how to integrate syntax highlighting with complex themed prompts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>

// Prompt boundary detection structure
typedef struct {
    size_t prompt_end_position;     // Where the prompt ends
    size_t command_start_position;  // Where user command begins
    bool is_themed_prompt;          // Is this a complex themed prompt?
    char *command_marker;           // The actual marker found ("$ ", "└─$ ", etc.)
    bool detection_successful;      // Did we successfully parse the prompt?
} prompt_boundaries_t;

// Detect where the prompt ends and command begins
static prompt_boundaries_t detect_prompt_boundaries(const char *prompt) {
    prompt_boundaries_t bounds = {0};
    
    if (!prompt) {
        return bounds;
    }
    
    // Common prompt patterns in order of specificity
    const char *patterns[] = {
        "└─$ ",         // Themed prompt ending
        "┌─$ ",         // Alternative themed ending  
        "> ",           // Continuation prompt (loop>, if>, etc.)
        "$ ",           // Simple prompt
        "# ",           // Root prompt
        "% ",           // Some shell variants
        NULL
    };
    
    // Search for prompt patterns from end of string (most reliable)
    size_t prompt_len = strlen(prompt);
    
    for (int i = 0; patterns[i]; i++) {
        size_t pattern_len = strlen(patterns[i]);
        
        // Look for pattern at the end of prompt
        if (prompt_len >= pattern_len) {
            char *pattern_pos = strstr(prompt + (prompt_len - pattern_len - 10), patterns[i]);
            if (pattern_pos) {
                bounds.command_start_position = (pattern_pos - prompt) + pattern_len;
                bounds.prompt_end_position = bounds.command_start_position;
                bounds.command_marker = (char*)patterns[i];
                bounds.is_themed_prompt = (i < 2); // First two are themed
                bounds.detection_successful = true;
                break;
            }
        }
    }
    
    return bounds;
}

// Apply syntax highlighting only to the command portion
static void apply_command_syntax_highlighting(const char *command_text, int cursor_pos) {
    if (!command_text || !*command_text) {
        return;
    }
    
    // Simplified syntax highlighting (proof of concept)
    size_t len = strlen(command_text);
    size_t i = 0;
    bool in_string = false;
    char string_char = '\0';
    
    // Color definitions
    const char *keyword_color = "\033[1;34m";    // Bright blue
    const char *command_color = "\033[1;32m";    // Bright green
    const char *string_color = "\033[1;33m";     // Bright yellow
    const char *variable_color = "\033[1;35m";   // Bright magenta
    const char *operator_color = "\033[1;31m";   // Bright red
    const char *reset_color = "\033[0m";         // Reset
    
    while (i < len) {
        char c = command_text[i];
        
        // Handle string literals
        if (!in_string && (c == '"' || c == '\'')) {
            in_string = true;
            string_char = c;
            printf("%s%c", string_color, c);
            i++;
            continue;
        } else if (in_string && c == string_char) {
            printf("%c%s", c, reset_color);
            in_string = false;
            string_char = '\0';
            i++;
            continue;
        } else if (in_string) {
            printf("%c", c);
            i++;
            continue;
        }
        
        // Handle variables
        if (c == '$') {
            printf("%s%c", variable_color, c);
            i++;
            while (i < len && (isalnum(command_text[i]) || command_text[i] == '_' || 
                              command_text[i] == '{' || command_text[i] == '}')) {
                printf("%c", command_text[i]);
                i++;
            }
            printf("%s", reset_color);
            continue;
        }
        
        // Handle operators
        if (c == '|' || c == '&' || c == ';' || c == '<' || c == '>') {
            printf("%s%c%s", operator_color, c, reset_color);
            i++;
            continue;
        }
        
        // Handle words (commands, keywords)
        if (isalnum(c) || c == '_' || c == '-' || c == '.') {
            size_t word_start = i;
            while (i < len && (isalnum(command_text[i]) || command_text[i] == '_' || 
                              command_text[i] == '-' || command_text[i] == '.')) {
                i++;
            }
            
            size_t word_len = i - word_start;
            const char *word = command_text + word_start;
            
            // Check if it's a keyword or command
            bool is_keyword = false;
            const char *keywords[] = {"if", "then", "else", "fi", "for", "do", "done", 
                                     "while", "case", "esac", NULL};
            const char *commands[] = {"echo", "ls", "grep", "cat", "git", "make", NULL};
            
            for (int j = 0; keywords[j]; j++) {
                if (strlen(keywords[j]) == word_len && 
                    strncmp(word, keywords[j], word_len) == 0) {
                    printf("%s", keyword_color);
                    is_keyword = true;
                    break;
                }
            }
            
            if (!is_keyword) {
                for (int j = 0; commands[j]; j++) {
                    if (strlen(commands[j]) == word_len && 
                        strncmp(word, commands[j], word_len) == 0) {
                        printf("%s", command_color);
                        break;
                    }
                }
            }
            
            // Print the word
            for (size_t j = 0; j < word_len; j++) {
                printf("%c", word[j]);
            }
            printf("%s", reset_color);
            continue;
        }
        
        // Default: print character as-is
        printf("%c", c);
        i++;
    }
}

// Main prompt-aware redisplay function
static void lusush_prompt_aware_redisplay(void) {
    // Get current readline state
    if (!rl_line_buffer || !rl_prompt) {
        rl_redisplay();
        return;
    }
    
    // Detect prompt boundaries
    prompt_boundaries_t bounds = detect_prompt_boundaries(rl_prompt);
    
    if (!bounds.detection_successful) {
        // Fallback to standard redisplay if we can't parse the prompt
        rl_redisplay();
        return;
    }
    
    // Clear the current line
    printf("\r\033[K");
    
    // Print the full prompt (unchanged)
    printf("%s", rl_prompt);
    
    // Apply syntax highlighting only to the command portion
    apply_command_syntax_highlighting(rl_line_buffer, rl_point);
    
    // Position cursor correctly
    // Calculate where cursor should be: prompt + command position
    printf("\r");
    
    // Move to the start of command area
    for (size_t i = 0; i < bounds.command_start_position; i++) {
        printf("\033[C");
    }
    
    // Move cursor to correct position within command
    for (int i = 0; i < rl_point && i < rl_end; i++) {
        printf("\033[C");
    }
    
    fflush(stdout);
}

// Enhanced boundary detection with theme awareness
static prompt_boundaries_t enhanced_prompt_detection(const char *prompt) {
    prompt_boundaries_t bounds = detect_prompt_boundaries(prompt);
    
    if (bounds.detection_successful && bounds.is_themed_prompt) {
        // Additional processing for themed prompts
        
        // Handle multi-line prompts by finding the last line
        char *last_newline = strrchr(prompt, '\n');
        if (last_newline) {
            // Recalculate boundaries based on last line only
            char *last_line = last_newline + 1;
            prompt_boundaries_t last_line_bounds = detect_prompt_boundaries(last_line);
            
            if (last_line_bounds.detection_successful) {
                // Adjust positions to account for previous lines
                size_t offset = last_line - prompt;
                bounds.command_start_position = offset + last_line_bounds.command_start_position;
                bounds.prompt_end_position = bounds.command_start_position;
            }
        }
    }
    
    return bounds;
}

// Configuration structure for prompt-aware highlighting
typedef struct {
    bool enabled;
    bool debug_mode;
    bool fallback_on_unknown_prompt;
    int max_prompt_analysis_length;
} prompt_aware_config_t;

static prompt_aware_config_t config = {
    .enabled = true,
    .debug_mode = false,
    .fallback_on_unknown_prompt = true,
    .max_prompt_analysis_length = 1024
};

// Public API functions
void lusush_enable_prompt_aware_highlighting(bool enable) {
    config.enabled = enable;
    
    if (enable) {
        rl_redisplay_function = lusush_prompt_aware_redisplay;
    } else {
        rl_redisplay_function = rl_redisplay;
    }
}

void lusush_set_prompt_aware_debug(bool debug) {
    config.debug_mode = debug;
}

bool lusush_test_prompt_compatibility(const char *test_prompt) {
    prompt_boundaries_t bounds = detect_prompt_boundaries(test_prompt);
    return bounds.detection_successful;
}

// Example usage and testing
#ifdef PROTOTYPE_TESTING
int main() {
    // Test various prompt patterns
    const char *test_prompts[] = {
        "$ ",
        "└─$ ",
        "┌─[user@host]─[~/path] (master)\n└─$ ",
        "merry@fedora-xps13.local:~/Lab/c/lusush (master)$ ",
        "loop> ",
        "if> ",
        NULL
    };
    
    printf("Testing prompt boundary detection:\n\n");
    
    for (int i = 0; test_prompts[i]; i++) {
        prompt_boundaries_t bounds = detect_prompt_boundaries(test_prompts[i]);
        
        printf("Prompt: %s\n", test_prompts[i]);
        printf("  Detection successful: %s\n", bounds.detection_successful ? "YES" : "NO");
        printf("  Command start: %zu\n", bounds.command_start_position);
        printf("  Is themed: %s\n", bounds.is_themed_prompt ? "YES" : "NO");
        printf("  Marker: %s\n", bounds.command_marker ? bounds.command_marker : "NONE");
        printf("\n");
    }
    
    return 0;
}
#endif

/* 
 * Implementation Notes:
 * 
 * 1. This prototype demonstrates the core concept of prompt-aware syntax highlighting
 * 2. The detect_prompt_boundaries() function can be extended for more prompt patterns
 * 3. The enhanced_prompt_detection() handles multi-line themed prompts
 * 4. Configuration allows runtime control of the feature
 * 5. Fallback to standard redisplay ensures safety
 * 
 * Integration with Lusush:
 * 1. Replace lusush_safe_redisplay() with lusush_prompt_aware_redisplay()
 * 2. Add prompt boundary detection during theme initialization
 * 3. Use lusush_enable_prompt_aware_highlighting() in syntax highlighting setup
 * 4. Test with all 6 themes to validate prompt pattern detection
 */