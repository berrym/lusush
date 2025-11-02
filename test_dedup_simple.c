#include <stdio.h>
#include <string.h>
#include "lle/history.h"

int main(void) {
    printf("Creating history core with dedup...\n");
    
    lle_history_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_entries = 1000;
    config.ignore_duplicates = true;
    
    lle_history_core_t *core = NULL;
    lle_result_t result = lle_history_core_create(&core, NULL, &config);
    if (result != LLE_SUCCESS) {
        printf("FAIL: Core creation failed\n");
        return 1;
    }
    printf("Core created successfully\n");
    
    printf("Adding first entry...\n");
    uint64_t id1 = 0;
    result = lle_history_add_entry(core, "test command", 0, &id1);
    if (result != LLE_SUCCESS) {
        printf("FAIL: First add failed\n");
        return 1;
    }
    printf("First entry added, id=%lu\n", id1);
    
    printf("Adding duplicate entry...\n");
    uint64_t id2 = 0;
    result = lle_history_add_entry(core, "test command", 0, &id2);
    printf("Second add returned: result=%d, id=%lu\n", result, id2);
    
    printf("Destroying core...\n");
    lle_history_core_destroy(core);
    printf("SUCCESS: Test completed\n");
    
    return 0;
}
