// Detailed kill line debug
#include "../editor/editor.h"
#include "../buffer/buffer.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    lle_editor_t editor;
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    
    printf("Buffer content:\n");
    char content[100];
    lle_editor_get_content(&editor, content, sizeof(content));
    printf("'%s'\n\n", content);
    
    // Position character map
    for (size_t i = 0; i < 28; i++) {
        if (content[i] == '\n') {
            printf("[%2zu]='\\n' ", i);
        } else {
            printf("[%2zu]='%c' ", i, content[i]);
        }
        if ((i + 1) % 5 == 0) printf("\n");
    }
    printf("\n\n");
    
    lle_editor_set_cursor_pos(&editor, 15);
    printf("Set cursor to position 15 (char '%c')\n", content[15]);
    
    // Call line_end directly on the buffer
    lle_buffer_pos_t line_end = lle_buffer_line_end(&editor.buffer, 15);
    printf("lle_buffer_line_end(15) = %zu\n", line_end);
    printf("Should delete from 15 to %zu (%zu characters)\n", line_end, line_end - 15);
    
    // Now call kill_line
    printf("\nCalling lle_editor_kill_line...\n");
    int result = lle_editor_kill_line(&editor);
    printf("Result: %d\n", result);
    
    lle_editor_get_content(&editor, content, sizeof(content));
    printf("\nAfter kill_line:\n");
    printf("'%s'\n", content);
    printf("Expected: 'first line\\nsecond\\nthird'\n");
    
    size_t pos = lle_editor_get_cursor_pos(&editor);
    printf("\nCursor position: %zu (expected 15)\n", pos);
    
    lle_editor_cleanup(&editor);
    return 0;
}
