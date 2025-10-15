// Test editor initialization
#include "../editor/editor.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("Testing multiple editor init/cleanup cycles...\n\n");
    
    for (int i = 0; i < 3; i++) {
        printf("Cycle %d:\n", i + 1);
        
        lle_editor_t editor;
        int result = lle_editor_init(&editor, STDIN_FILENO, STDOUT_FILENO);
        
        printf("  Init result: %d (%s)\n", result, lle_editor_error_string(result));
        
        if (result != LLE_EDITOR_OK) {
            printf("  FAILED - cannot continue\n");
            return 1;
        }
        
        // Try to insert text
        result = lle_editor_insert_string(&editor, "test", 4);
        printf("  Insert result: %d\n", result);
        
        size_t size = lle_editor_get_size(&editor);
        printf("  Buffer size: %zu\n", size);
        
        lle_editor_cleanup(&editor);
        printf("  Cleanup done\n\n");
    }
    
    printf("All cycles completed successfully\n");
    return 0;
}
