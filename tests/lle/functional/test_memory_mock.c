/**
 * Mock Memory Pool for LLE Testing
 * 
 * Provides simple malloc/free based implementations of memory pool
 * functions so buffer tests can run standalone without full lusush.
 */

#include <stdlib.h>
#include <stdio.h>

/* Mock memory pool type */
typedef struct lusush_memory_pool_system_t {
    int dummy;
} lusush_memory_pool_system_t;

typedef lusush_memory_pool_system_t lusush_memory_pool_t;

/* Global memory pool (unused in mock) */
static lusush_memory_pool_system_t global_pool_instance = {0};
lusush_memory_pool_t *global_memory_pool = &global_pool_instance;

/* Mock memory pool functions - just use system malloc/free */
void* lle_pool_alloc(size_t size) {
    return malloc(size);
}

void lle_pool_free(void *ptr) {
    free(ptr);
}

void* lusush_pool_alloc(lusush_memory_pool_t *pool, size_t size) {
    (void)pool; /* Unused */
    return malloc(size);
}

void lusush_pool_free(lusush_memory_pool_t *pool, void *ptr) {
    (void)pool; /* Unused */
    free(ptr);
}

/* Mock memory pool config */
typedef struct lusush_memory_pool_config_t {
    size_t initial_size;
} lusush_memory_pool_config_t;

lusush_memory_pool_config_t* lusush_pool_get_default_config(void) {
    static lusush_memory_pool_config_t config = {4096};
    return &config;
}

int lusush_pool_init(lusush_memory_pool_t *pool, lusush_memory_pool_config_t *config) {
    (void)pool;
    (void)config;
    return 0;
}
