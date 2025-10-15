// Debug line_end behavior
#include "../buffer/buffer.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "first line\nsecond line\nthird", 28);
    
    printf("Buffer: 'first line\\nsecond line\\nthird'\n");
    printf("Total size: %zu\n\n", lle_buffer_size(&buffer));
    
    // Print with positions
    char content[100];
    lle_buffer_get_contents(&buffer, content, sizeof(content));
    
    for (size_t i = 0; i < strlen(content); i++) {
        if (content[i] == '\n') {
            printf("[%2zu]='\\n' ", i);
        } else {
            printf("[%2zu]='%c' ", i, content[i]);
        }
        if ((i + 1) % 5 == 0) printf("\n");
    }
    printf("\n\n");
    
    // Test line_end from position 15
    size_t pos = 15;
    printf("Testing from position %zu (char '%c')\n", pos, content[pos]);
    
    lle_buffer_pos_t line_end = lle_buffer_line_end(&buffer, pos);
    printf("line_end returned: %zu\n", line_end);
    
    if (line_end < strlen(content)) {
        printf("Character at line_end: '%c'\n", content[line_end]);
    }
    
    printf("\nExpected: position 22 (the newline after 'line')\n");
    
    lle_buffer_cleanup(&buffer);
    return 0;
}
