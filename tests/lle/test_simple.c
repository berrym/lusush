#include "lle/history.h"
#include "lle/error_handling.h"
#include <stdio.h>

int main(void) {
    lle_history_core_t *core = NULL;
    lle_result_t result;
    
    printf("Creating history core...\n");
    result = lle_history_core_create(&core, NULL, NULL);
    printf("Result: %d\n", result);
    
    if (result == LLE_SUCCESS) {
        printf("SUCCESS! Core created.\n");
        lle_history_core_destroy(core);
    } else {
        printf("FAILED with error code: %d\n", result);
    }
    
    return 0;
}
