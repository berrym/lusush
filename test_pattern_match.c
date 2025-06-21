#include <stdio.h>
#include <string.h>

int main() {
    const char *line = "a=test; for i in 1; do echo \"hi\"; done";
    const char *new_parser_control_keywords[] = {
        "if ", "while ", "for "
    };
    
    printf("Testing line: '%s'\n", line);
    
    // Check if line contains control keywords after semicolons
    for (size_t i = 0; i < sizeof(new_parser_control_keywords) / sizeof(new_parser_control_keywords[0]); i++) {
        char search_pattern1[32], search_pattern2[32];
        snprintf(search_pattern1, sizeof(search_pattern1), "; %s", new_parser_control_keywords[i]);
        snprintf(search_pattern2, sizeof(search_pattern2), ";%s", new_parser_control_keywords[i]);
        
        printf("Searching for patterns '%s' and '%s'\n", search_pattern1, search_pattern2);
        
        char *found1 = strstr(line, search_pattern1);
        char *found2 = strstr(line, search_pattern2);
        
        if (found1) {
            printf("Found pattern '%s' at position %ld\n", search_pattern1, found1 - line);
            return 0; // Should return CMD_CONTROL_STRUCTURE
        }
        if (found2) {
            printf("Found pattern '%s' at position %ld\n", search_pattern2, found2 - line);
            return 0; // Should return CMD_CONTROL_STRUCTURE
        }
    }
    
    printf("No control structure patterns found\n");
    return 1; // Would fall through to other checks
}
