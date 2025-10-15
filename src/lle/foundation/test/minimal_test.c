// Minimal test case
#include "../editor/editor.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    fprintf(stderr, "Starting test...\n");
    fflush(stderr);
    
    lle_editor_t editor;
    fprintf(stderr, "About to init editor...\n");
    fflush(stderr);
    
    int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
    fprintf(stderr, "Init returned: %d\n", result);
    fflush(stderr);
    
    if (result != 0) {
        fprintf(stderr, "Init failed!\n");
        fflush(stderr);
        return 1;
    }
    
    fprintf(stderr, "Inserting string...\n");
    fflush(stderr);
    
    result = lle_editor_insert_string(&editor, "hello world test", 16);
    fprintf(stderr, "Insert returned: %d\n", result);
    fflush(stderr);
    
    size_t pos = lle_editor_get_cursor_pos(&editor);
    fprintf(stderr, "Cursor position: %zu\n", pos);
    fflush(stderr);
    
    fprintf(stderr, "Moving word backward...\n");
    fflush(stderr);
    
    result = lle_editor_move_word_backward(&editor);
    fprintf(stderr, "Move returned: %d\n", result);
    fflush(stderr);
    
    pos = lle_editor_get_cursor_pos(&editor);
    fprintf(stderr, "New cursor position: %zu (expected 12)\n", pos);
    fflush(stderr);
    
    if (pos != 12) {
        fprintf(stderr, "MISMATCH! Got %zu, expected 12\n", pos);
        fflush(stderr);
    } else {
        fprintf(stderr, "SUCCESS!\n");
        fflush(stderr);
    }
    
    lle_editor_cleanup(&editor);
    fprintf(stderr, "Cleanup done\n");
    fflush(stderr);
    
    return 0;
}
