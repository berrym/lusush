#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Convert multiline input to single-line format for history storage
// This makes multiline commands more manageable in history recall
static char *convert_multiline_for_history(const char *input) {
    if (!input || !*input) {
        return NULL;
    }
    
    size_t input_len = strlen(input);
    // Allocate buffer (worst case: every newline becomes "; ")
    char *result = malloc(input_len * 2 + 1);
    if (!result) {
        return NULL;
    }
    
    const char *src = input;
    char *dst = result;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;
    bool need_space = false;
    
    while (*src) {
        char ch = *src;
        
        // Handle escaping
        if (escaped) {
            *dst++ = ch;
            escaped = false;
            src++;
            continue;
        }
        
        if (ch == '\\') {
            *dst++ = ch;
            escaped = true;
            src++;
            continue;
        }
        
        // Handle quotes
        if (ch == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            *dst++ = ch;
        } else if (ch == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            *dst++ = ch;
        } else if (ch == '\n') {
            // Convert newlines to appropriate separators
            if (!in_single_quote && !in_double_quote) {
                // Look ahead to see if we need a semicolon
                const char *next = src + 1;
                while (*next && isspace(*next) && *next != '\n') next++;
                
                if (*next && *next != '\n') {
                    // Check if the previous token needs a semicolon
                    if (dst > result && *(dst-1) != ';' && *(dst-1) != '{' && *(dst-1) != '(' && !isspace(*(dst-1))) {
                        // Check if next token is a continuation keyword
                        if (strncmp(next, "then", 4) == 0 || strncmp(next, "do", 2) == 0 || 
                            strncmp(next, "else", 4) == 0 || strncmp(next, "elif", 4) == 0 ||
                            strncmp(next, "fi", 2) == 0 || strncmp(next, "done", 4) == 0 ||
                            strncmp(next, "esac", 4) == 0) {
                            *dst++ = ' ';
                        } else {
                            *dst++ = ';';
                            *dst++ = ' ';
                        }
                    } else {
                        *dst++ = ' ';
                    }
                    need_space = false;
                } else {
                    // Skip empty lines
                }
            } else {
                // Inside quotes - preserve newline as literal
                *dst++ = '\\';
                *dst++ = 'n';
            }
        } else if (isspace(ch)) {
            // Collapse multiple spaces
            if (!need_space) {
                *dst++ = ' ';
                need_space = true;
            }
        } else {
            *dst++ = ch;
            need_space = false;
        }
        
        src++;
    }
    
    // Remove trailing whitespace
    while (dst > result && isspace(*(dst-1))) {
        dst--;
    }
    
    *dst = '\0';
    
    // If the result is empty or just whitespace, return NULL
    if (dst == result || (dst == result + 1 && *result == ' ')) {
        free(result);
        return NULL;
    }
    
    return result;
}

// Test cases
typedef struct {
    const char *input;
    const char *expected;
    const char *description;
} test_case_t;

test_case_t test_cases[] = {
    {
        "if true\nthen\n    echo success\nfi",
        "if true then echo success fi",
        "Basic IF statement"
    },
    {
        "for i in 1 2 3\ndo\n    echo $i\ndone",
        "for i in 1 2 3 do echo $i done",
        "Basic FOR loop"
    },
    {
        "while [ $x -le 3 ]\ndo\n    echo $x\n    x=$((x + 1))\ndone",
        "while [ $x -le 3 ] do echo $x x=$((x + 1)) done",
        "WHILE loop with multiple commands"
    },
    {
        "{\n    echo line1\n    echo line2\n}",
        "{ echo line1 echo line2 }",
        "Brace group"
    },
    {
        "echo hello \\\nworld",
        "echo hello world",
        "Backslash continuation"
    },
    {
        "echo 'multi\nline\nstring'",
        "echo 'multi\\nline\\nstring'",
        "Multiline single-quoted string"
    },
    {
        "echo \"multi\nline\nstring\"",
        "echo \"multi\\nline\\nstring\"",
        "Multiline double-quoted string"
    },
    {
        "echo hello\necho world",
        "echo hello echo world",
        "Multiple simple commands"
    },
    {
        "case $var in\n    pattern1)\n        echo match1\n        ;;\n    *)\n        echo default\n        ;;\nesac",
        "case $var in pattern1) echo match1 ;; *) echo default ;; esac",
        "CASE statement"
    },
    {
        "function myfunc() {\n    echo \"in function\"\n    return 0\n}",
        "function myfunc() { echo \"in function\" return 0 }",
        "Function definition"
    }
};

int main() {
    int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    int passed = 0;
    int failed = 0;
    
    printf("=== Multiline to Single-line Conversion Tests ===\n\n");
    
    for (int i = 0; i < total_tests; i++) {
        test_case_t *test = &test_cases[i];
        
        printf("Test %d: %s\n", i + 1, test->description);
        printf("Input:\n%s\n", test->input);
        
        char *result = convert_multiline_for_history(test->input);
        
        if (result) {
            printf("Output: %s\n", result);
            printf("Expected: %s\n", test->expected);
            
            if (strcmp(result, test->expected) == 0) {
                printf("✓ PASSED\n");
                passed++;
            } else {
                printf("✗ FAILED\n");
                failed++;
            }
            
            free(result);
        } else {
            printf("Output: (NULL)\n");
            printf("Expected: %s\n", test->expected);
            printf("✗ FAILED - conversion returned NULL\n");
            failed++;
        }
        
        printf("\n");
    }
    
    printf("=== Test Results ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    
    if (failed == 0) {
        printf("All tests passed! ✓\n");
        return 0;
    } else {
        printf("Some tests failed! ✗\n");
        return 1;
    }
}