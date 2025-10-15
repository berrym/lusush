// Debug gap buffer state during delete_range
#include "../buffer/buffer.h"
#include <stdio.h>

// Access internal buffer state
extern size_t text_size(const lle_buffer_t *buffer);

void print_gap_state(const lle_buffer_t *buffer, const char *label) {
    printf("%s:\n", label);
    printf("  capacity: %zu\n", buffer->capacity);
    printf("  gap_start: %zu\n", buffer->gap_start);
    printf("  gap_end: %zu\n", buffer->gap_end);
    printf("  gap_size: %zu\n", buffer->gap_end - buffer->gap_start);
    printf("  text_size: %zu\n", lle_buffer_size(buffer));
    printf("\n");
}

int main(void) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "first line\nsecond line\nthird", 28);
    
    print_gap_state(&buffer, "After init");
    
    printf("About to delete range [15, 22)\n");
    printf("delete_len should be: %d\n\n", 22 - 15);
    
    // Manually do what delete_range does
    printf("Step 1: Move gap to position 15\n");
    lle_buffer_move_gap(&buffer, 15);
    print_gap_state(&buffer, "After move_gap(15)");
    
    printf("Step 2: Expand gap by 7 (delete_len)\n");
    size_t old_gap_end = buffer.gap_end;
    buffer.gap_end += 7;
    printf("  old gap_end: %zu\n", old_gap_end);
    printf("  new gap_end: %zu\n", buffer.gap_end);
    print_gap_state(&buffer, "After expanding gap");
    
    char content[100];
    lle_buffer_get_contents(&buffer, content, sizeof(content));
    printf("Result: '%s'\n", content);
    printf("Expected: 'first line\\nsecond\\nthird'\n");
    
    lle_buffer_cleanup(&buffer);
    return 0;
}
