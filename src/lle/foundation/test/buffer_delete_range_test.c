// Test buffer delete_range directly
#include "../buffer/buffer.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "first line\nsecond line\nthird", 28);
    
    printf("Initial buffer:\n");
    char content[100];
    lle_buffer_get_contents(&buffer, content, sizeof(content));
    printf("'%s'\n", content);
    printf("Size: %zu\n\n", lle_buffer_size(&buffer));
    
    printf("Deleting range [15, 22)...\n");
    int result = lle_buffer_delete_range(&buffer, 15, 22);
    printf("Result: %d\n", result);
    
    lle_buffer_get_contents(&buffer, content, sizeof(content));
    printf("\nAfter delete:\n");
    printf("'%s'\n", content);
    printf("Size: %zu\n", lle_buffer_size(&buffer));
    printf("Expected: 'first line\\nsecond\\nthird'\n");
    
    lle_buffer_cleanup(&buffer);
    return 0;
}
