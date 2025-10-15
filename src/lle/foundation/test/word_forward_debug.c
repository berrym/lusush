// Debug word forward movement
#include "../editor/editor.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    lle_editor_t editor;
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    lle_editor_insert_string(&editor, "hello world test", 16);
    lle_editor_set_cursor_pos(&editor, 0);
    
    printf("Text: 'hello world test'\n");
    printf("Starting at position: 0\n\n");
    
    // First move
    lle_editor_move_word_forward(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    printf("After 1st forward: %zu (expected 6)\n", pos);
    printf("  Match: %s\n\n", pos == 6 ? "YES" : "NO");
    
    // Second move
    lle_editor_move_word_forward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    printf("After 2nd forward: %zu (expected 12)\n", pos);
    printf("  Match: %s\n\n", pos == 12 ? "YES" : "NO");
    
    // Third move
    lle_editor_move_word_forward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    printf("After 3rd forward: %zu (expected 16)\n", pos);
    printf("  Match: %s\n", pos == 16 ? "YES" : "NO");
    
    lle_editor_cleanup(&editor);
    return 0;
}
