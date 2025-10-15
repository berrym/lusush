// Debug punctuation word movement
#include "../editor/editor.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    lle_editor_t editor;
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    lle_editor_insert_string(&editor, "hello, world!", 13);
    lle_editor_set_cursor_pos(&editor, 0);
    
    printf("Text: 'hello, world!'\n");
    printf("Positions: 0=h, 5=',', 6=' ', 7='w', 12='!'\n\n");
    
    printf("Starting at position 0\n");
    
    lle_editor_move_word_forward(&editor);
    size_t pos = lle_editor_get_cursor_pos(&editor);
    printf("After 1st move_word_forward: %zu (expected 5)\n", pos);
    printf("  Match: %s\n\n", pos == 5 ? "YES" : "NO");
    
    lle_editor_move_word_forward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    printf("After 2nd move_word_forward: %zu (expected 7)\n", pos);
    printf("  Match: %s\n", pos == 7 ? "YES" : "NO");
    
    lle_editor_cleanup(&editor);
    return 0;
}
