// Hex dump of buffer to see physical layout
#include "../buffer/buffer.h"
#include <stdio.h>
#include <ctype.h>

void hex_dump(const lle_buffer_t *buffer, size_t start, size_t len) {
    printf("Physical buffer dump [%zu, %zu):\n", start, start + len);
    for (size_t i = start; i < start + len && i < buffer->capacity; i++) {
        if (i == buffer->gap_start) {
            printf("\n--- GAP START (gap_start=%zu) ---\n", buffer->gap_start);
        }
        if (i == buffer->gap_end) {
            printf("\n--- GAP END (gap_end=%zu) ---\n", buffer->gap_end);
        }
        
        char ch = buffer->data[i];
        if (isprint(ch)) {
            printf("[%3zu]='%c' ", i, ch);
        } else if (ch == '\n') {
            printf("[%3zu]='\\n' ", i);
        } else {
            printf("[%3zu]=0x%02x ", i, (unsigned char)ch);
        }
        
        if ((i - start + 1) % 5 == 0) printf("\n");
    }
    printf("\n\n");
}

int main(void) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "first line\nsecond line\nthird", 28);
    
    printf("=== Initial state ===\n");
    printf("Text: 'first line\\nsecond line\\nthird'\n");
    hex_dump(&buffer, 0, 35);
    
    printf("=== After move_gap(15) ===\n");
    lle_buffer_move_gap(&buffer, 15);
    hex_dump(&buffer, 0, 30);
    hex_dump(&buffer, 1005, 25);
    
    lle_buffer_cleanup(&buffer);
    return 0;
}
