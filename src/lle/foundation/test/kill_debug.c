// Debug kill line and delete word at cursor
#include "../editor/editor.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    lle_editor_t editor;
    char buffer[100];
    
    printf("=== Delete word at cursor ===\n");
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_editor_insert_string(&editor, "hello world test", 16);
    lle_editor_set_cursor_pos(&editor, 6);  // At start of "world"
    
    printf("Before: 'hello world test'\n");
    printf("Cursor at position 6 (start of 'world')\n");
    
    lle_editor_delete_word_at_cursor(&editor);
    
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("After delete word at cursor: '%s'\n", buffer);
    printf("Expected: 'hello  test'\n");
    printf("Match: %s\n\n", strcmp(buffer, "hello  test") == 0 ? "YES" : "NO");
    
    size_t pos = lle_editor_get_cursor_pos(&editor);
    printf("Cursor position: %zu (expected 6)\n", pos);
    printf("Match: %s\n\n", pos == 6 ? "YES" : "NO");
    
    lle_editor_cleanup(&editor);
    
    printf("=== Kill line from cursor ===\n");
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    lle_editor_set_cursor_pos(&editor, 15);  // In middle of "second line"
    
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("Before: '%s'\n", buffer);
    printf("Cursor at position 15 (char '%c')\n", buffer[15]);
    
    lle_editor_kill_line(&editor);
    
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("After kill line: '%s'\n", buffer);
    printf("Expected: 'first line\\nsecond\\nthird'\n");
    printf("Match: %s\n\n", strcmp(buffer, "first line\nsecond\nthird") == 0 ? "YES" : "NO");
    
    pos = lle_editor_get_cursor_pos(&editor);
    printf("Cursor position: %zu (expected 15)\n", pos);
    printf("Match: %s\n", pos == 15 ? "YES" : "NO");
    
    lle_editor_cleanup(&editor);
    
    return 0;
}
