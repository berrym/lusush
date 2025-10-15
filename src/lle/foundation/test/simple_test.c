// Simple test to verify exact behavior
#include "../editor/editor.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    lle_editor_t editor;
    char buffer[100];
    
    printf("Test 1: Word backward from end\n");
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_editor_insert_string(&editor, "hello world test", 16);
    
    size_t pos = lle_editor_get_cursor_pos(&editor);
    printf("  Initial pos: %zu\n", pos);
    
    lle_editor_move_word_backward(&editor);
    pos = lle_editor_get_cursor_pos(&editor);
    printf("  After 1st backward: %zu (expect 12)\n", pos);
    printf("  Match: %s\n\n", pos == 12 ? "YES" : "NO");
    
    lle_editor_cleanup(&editor);
    
    printf("Test 2: Kill whole line\n");
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    
    // Cursor at position 15 in "second line"
    lle_editor_set_cursor_pos(&editor, 15);
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("  Before: '%s'\n", buffer);
    printf("  Cursor at: %zu (char '%c')\n", (size_t)15, buffer[15]);
    
    lle_editor_kill_whole_line(&editor);
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("  After: '%s'\n", buffer);
    printf("  Cursor: %zu\n", lle_editor_get_cursor_pos(&editor));
    
    // What I expect: "first line\n\nthird"
    // Line start of pos 15 is 11 (after first \n)
    // Line end of pos 15 is 22 (at second \n)
    // So we delete [11, 22) which is "second line"
    // Result: "first line\n" + "\nthird" = "first line\n\nthird"
    
    printf("  Expected: 'first line\\n\\nthird'\n");
    printf("  Match: %s\n\n", strcmp(buffer, "first line\n\nthird") == 0 ? "YES" : "NO");
    
    lle_editor_cleanup(&editor);
    
    printf("Test 3: Empty buffer operations\n");
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    // All should work without crash
    lle_editor_move_word_forward(&editor);
    lle_editor_move_word_backward(&editor);
    lle_editor_kill_line(&editor);
    
    size_t size = lle_editor_get_size(&editor);
    printf("  Size after operations: %zu (expect 0)\n", size);
    printf("  Match: %s\n", size == 0 ? "YES" : "NO");
    
    lle_editor_cleanup(&editor);
    
    return 0;
}
