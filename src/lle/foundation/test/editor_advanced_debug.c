// src/lle/foundation/test/editor_advanced_debug.c
//
// Debug version to identify word movement issues

#include "../editor/editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "Requires TTY\n");
        return 1;
    }
    
    printf("=== Word Backward Movement Debug ===\n");
    
    lle_editor_t editor;
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    // Insert test text: "hello world test"
    lle_editor_insert_string(&editor, "hello world test", 16);
    printf("Text: 'hello world test' (16 chars)\n");
    printf("Initial cursor position: %zu\n", lle_editor_get_cursor_pos(&editor));
    
    // Move backward one word
    lle_editor_move_word_backward(&editor);
    size_t pos1 = lle_editor_get_cursor_pos(&editor);
    printf("After first backward: pos=%zu (expected 12)\n", pos1);
    
    // Get character at position
    char buffer[100];
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("Content: '%s'\n", buffer);
    if (pos1 < 16) {
        printf("Character at pos %zu: '%c'\n", pos1, buffer[pos1]);
    }
    
    // Move backward again
    lle_editor_move_word_backward(&editor);
    size_t pos2 = lle_editor_get_cursor_pos(&editor);
    printf("After second backward: pos=%zu (expected 6)\n", pos2);
    if (pos2 < 16) {
        printf("Character at pos %zu: '%c'\n", pos2, buffer[pos2]);
    }
    
    // Move backward again
    lle_editor_move_word_backward(&editor);
    size_t pos3 = lle_editor_get_cursor_pos(&editor);
    printf("After third backward: pos=%zu (expected 0)\n", pos3);
    
    printf("\n=== Line Start/End Debug ===\n");
    
    lle_editor_cleanup(&editor);
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    lle_editor_insert_string(&editor, "first line\nsecond line\nthird", 28);
    printf("Text: 'first line\\nsecond line\\nthird'\n");
    
    // Position 0: 'f'
    // Position 10: '\n'
    // Position 11: 's' (start of second)
    // Position 22: 'e' (last char of "line")
    // Position 23: '\n'
    
    lle_editor_set_cursor_pos(&editor, 18);
    printf("Set cursor to position 18\n");
    
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("Full content: '%s'\n", buffer);
    printf("Char at 18: '%c'\n", buffer[18]);
    
    lle_editor_move_to_line_start(&editor);
    size_t line_start = lle_editor_get_cursor_pos(&editor);
    printf("Line start: %zu (expected 11)\n", line_start);
    
    lle_editor_set_cursor_pos(&editor, 15);
    lle_editor_move_to_line_end(&editor);
    size_t line_end = lle_editor_get_cursor_pos(&editor);
    printf("Line end from pos 15: %zu (expected 22)\n", line_end);
    
    printf("\n=== Delete Word Before Debug ===\n");
    
    lle_editor_cleanup(&editor);
    lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    
    lle_editor_insert_string(&editor, "hello world test", 16);
    lle_editor_set_cursor_pos(&editor, 11);  // After "hello world"
    
    printf("Before delete: cursor at 11\n");
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("Content: '%s'\n", buffer);
    
    lle_editor_delete_word_before_cursor(&editor);
    
    lle_editor_get_content(&editor, buffer, sizeof(buffer));
    printf("After delete word before:\n");
    printf("  Content: '%s'\n", buffer);
    printf("  Cursor: %zu (expected 6)\n", lle_editor_get_cursor_pos(&editor));
    printf("  Expected: 'hello  test'\n");
    
    lle_editor_cleanup(&editor);
    
    return 0;
}
