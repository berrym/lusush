/**
 * @file history_core.c
 * @brief LLE History System - Core Engine Implementation
 * 
 * Specification: Spec 09 - History System
 * Phase: Phase 1 Day 1 - Core Structures and Lifecycle
 * 
 * Implements the central history management engine with basic entry
 * management, storage, and retrieval functionality.
 */

#include "lle/history.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* Note: lle_pool_alloc() and lle_pool_free() are provided by memory_management.h
 * They wrap the Lusush global pool. The memory_pool parameter in our API is for
 * future per-pool allocation support, but currently uses the global pool.
 */

/* ============================================================================
 * CONFIGURATION MANAGEMENT
 * ============================================================================ */

/**
 * Create default configuration
 */
lle_result_t lle_history_config_create_default(
    lle_history_config_t **config,
    lle_memory_pool_t *memory_pool
) {
    (void)memory_pool;  /* Unused - we use global pool via lle_pool_alloc */
    
    if (!config) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate configuration */
    lle_history_config_t *cfg = lle_pool_alloc(sizeof(lle_history_config_t));
    if (!cfg) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Set defaults */
    memset(cfg, 0, sizeof(lle_history_config_t));
    cfg->max_entries = LLE_HISTORY_DEFAULT_CAPACITY;
    cfg->max_command_length = LLE_HISTORY_MAX_COMMAND_LENGTH;
    cfg->initial_capacity = LLE_HISTORY_INITIAL_CAPACITY;
    
    /* File settings */
    const char *home = getenv("HOME");
    if (home) {
        size_t path_len = strlen(home) + strlen(LLE_HISTORY_DEFAULT_FILE) + 2;
        cfg->history_file_path = lle_pool_alloc(path_len);
        if (cfg->history_file_path) {
            snprintf(cfg->history_file_path, path_len, "%s/%s", 
                    home, LLE_HISTORY_DEFAULT_FILE);
        }
    }
    
    cfg->auto_save = false;              /* Phase 3 - disable auto-save for now */
    cfg->load_on_init = false;           /* Phase 3 - disable auto-load for now */
    
    /* Behavior settings */
    cfg->ignore_duplicates = false;      /* Phase 4 - deduplication */
    cfg->ignore_space_prefix = true;     /* Standard shell behavior */
    cfg->save_timestamps = true;
    cfg->save_working_dir = true;
    cfg->save_exit_codes = true;
    cfg->use_indexing = true;            /* Phase 2 - hashtable indexing */
    
    *config = cfg;
    return LLE_SUCCESS;
}

/**
 * Destroy configuration
 */
lle_result_t lle_history_config_destroy(
    lle_history_config_t *config,
    lle_memory_pool_t *memory_pool
) {
    (void)memory_pool;  /* Unused - we use global pool via lle_pool_free */
    
    if (!config) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free history file path if allocated */
    if (config->history_file_path) {
        lle_pool_free( config->history_file_path);
    }
    
    /* Free configuration structure */
    lle_pool_free( config);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * ENTRY MANAGEMENT
 * ============================================================================ */

/**
 * Create history entry
 */
lle_result_t lle_history_entry_create(
    lle_history_entry_t **entry,
    const char *command,
    lle_memory_pool_t *memory_pool
) {
    (void)memory_pool;  /* Unused - we use global pool via lle_pool_alloc */
    
    if (!entry || !command) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Validate command length */
    size_t cmd_len = strlen(command);
    if (cmd_len == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    if (cmd_len > LLE_HISTORY_MAX_COMMAND_LENGTH) {
        return LLE_ERROR_BUFFER_OVERFLOW;
    }
    
    /* Allocate entry */
    lle_history_entry_t *e = lle_pool_alloc(sizeof(lle_history_entry_t));
    if (!e) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize entry */
    memset(e, 0, sizeof(lle_history_entry_t));
    
    /* Copy command */
    e->command = lle_pool_alloc(cmd_len + 1);
    if (!e->command) {
        lle_pool_free( e);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(e->command, command, cmd_len + 1);
    e->command_length = cmd_len;
    
    /* Get current timestamp */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    e->timestamp = (uint64_t)tv.tv_sec;
    
    /* Get current working directory */
    char cwd_buffer[LLE_HISTORY_MAX_PATH_LENGTH];
    if (lle_history_get_cwd(cwd_buffer, sizeof(cwd_buffer)) == LLE_SUCCESS) {
        size_t cwd_len = strlen(cwd_buffer);
        e->working_directory = lle_pool_alloc(cwd_len + 1);
        if (e->working_directory) {
            memcpy(e->working_directory, cwd_buffer, cwd_len + 1);
        }
    }
    
    /* Initialize state */
    e->state = LLE_HISTORY_STATE_ACTIVE;
    e->exit_code = -1;  /* Unknown */
    
    /* Phase 4 fields - initialize to defaults */
    e->is_multiline = false;
    e->original_multiline = NULL;
    e->duration_ms = 0;
    e->edit_count = 0;
    
    /* List pointers */
    e->next = NULL;
    e->prev = NULL;
    
    *entry = e;
    return LLE_SUCCESS;
}

/**
 * Destroy history entry
 */
lle_result_t lle_history_entry_destroy(
    lle_history_entry_t *entry,
    lle_memory_pool_t *memory_pool
) {
    (void)memory_pool;  /* Unused - we use global pool via lle_pool_free */
    
    if (!entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free command */
    if (entry->command) {
        lle_pool_free( entry->command);
    }
    
    /* Free working directory */
    if (entry->working_directory) {
        lle_pool_free( entry->working_directory);
    }
    
    /* Phase 4: Free multiline data if present */
    if (entry->original_multiline) {
        lle_pool_free( entry->original_multiline);
    }
    
    /* Free entry structure */
    lle_pool_free( entry);
    
    return LLE_SUCCESS;
}

/**
 * Validate entry
 */
lle_result_t lle_history_validate_entry(const lle_history_entry_t *entry) {
    if (!entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Validate command */
    if (!entry->command || entry->command_length == 0) {
        return LLE_ERROR_STATE_CORRUPTION;
    }
    
    /* Validate command length matches */
    if (strlen(entry->command) != entry->command_length) {
        return LLE_ERROR_STATE_CORRUPTION;
    }
    
    /* Validate state */
    if (entry->state > LLE_HISTORY_STATE_CORRUPTED) {
        return LLE_ERROR_STATE_CORRUPTION;
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * CORE ENGINE LIFECYCLE
 * ============================================================================ */

/**
 * Create and initialize history core
 */
lle_result_t lle_history_core_create(
    lle_history_core_t **core,
    lle_memory_pool_t *memory_pool,
    const lle_history_config_t *config
) {
    if (!core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    /* Allocate core structure */
    lle_history_core_t *c = lle_pool_alloc(sizeof(lle_history_core_t));
    if (!c) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(c, 0, sizeof(lle_history_core_t));
    
    /* Store memory pool reference */
    c->memory_pool = memory_pool;
    
    /* Create or copy configuration */
    if (config) {
        /* Copy provided configuration */
        c->config = lle_pool_alloc(sizeof(lle_history_config_t));
        if (!c->config) {
            lle_pool_free( c);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        memcpy(c->config, config, sizeof(lle_history_config_t));
        
        /* Deep copy history_file_path if present */
        if (config->history_file_path) {
            size_t path_len = strlen(config->history_file_path) + 1;
            c->config->history_file_path = lle_pool_alloc(path_len);
            if (c->config->history_file_path) {
                memcpy(c->config->history_file_path, config->history_file_path, path_len);
            }
        }
    } else {
        /* Create default configuration */
        result = lle_history_config_create_default(&c->config, memory_pool);
        if (result != LLE_SUCCESS) {
            lle_pool_free( c);
            return result;
        }
    }
    
    /* Allocate initial entry array */
    size_t initial_cap = c->config->initial_capacity;
    c->entries = lle_pool_alloc(sizeof(lle_history_entry_t*) * initial_cap);
    if (!c->entries) {
        lle_history_config_destroy(c->config, memory_pool);
        lle_pool_free( c);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(c->entries, 0, sizeof(lle_history_entry_t*) * initial_cap);
    
    c->entry_capacity = initial_cap;
    c->entry_count = 0;
    c->next_entry_id = 1;  /* Start IDs at 1 */
    
    /* Initialize linked list pointers */
    c->first_entry = NULL;
    c->last_entry = NULL;
    
    /* Phase 1 Day 2: Create hashtable index if enabled */
    if (c->config->use_indexing) {
        result = lle_history_index_create(&c->entry_lookup, initial_cap);
        if (result != LLE_SUCCESS) {
            lle_pool_free(c->entries);
            lle_history_config_destroy(c->config, memory_pool);
            lle_pool_free(c);
            return result;
        }
    } else {
        c->entry_lookup = NULL;
    }
    
    /* Initialize statistics */
    memset(&c->stats, 0, sizeof(lle_history_stats_t));
    
    /* Initialize thread safety */
    if (pthread_rwlock_init(&c->lock, NULL) != 0) {
        lle_pool_free( c->entries);
        lle_history_config_destroy(c->config, memory_pool);
        lle_pool_free( c);
        return LLE_ERROR_INITIALIZATION_FAILED;
    }
    
    c->initialized = true;
    
    *core = c;
    return LLE_SUCCESS;
}

/**
 * Destroy history core
 */
lle_result_t lle_history_core_destroy(lle_history_core_t *core) {
    if (!core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Acquire write lock */
    pthread_rwlock_wrlock(&core->lock);
    
    /* Mark as not initialized */
    core->initialized = false;
    
    /* Free all entries */
    for (size_t i = 0; i < core->entry_count; i++) {
        if (core->entries[i]) {
            lle_history_entry_destroy(core->entries[i], core->memory_pool);
        }
    }
    
    /* Free entries array */
    if (core->entries) {
        lle_pool_free( core->entries);
    }
    
    /* Phase 1 Day 2: Destroy hashtable index if present */
    if (core->entry_lookup) {
        lle_history_index_destroy(core->entry_lookup);
        core->entry_lookup = NULL;
    }
    
    /* Destroy configuration */
    if (core->config) {
        lle_history_config_destroy(core->config, core->memory_pool);
    }
    
    /* Release lock and destroy */
    pthread_rwlock_unlock(&core->lock);
    pthread_rwlock_destroy(&core->lock);
    
    /* Free core structure */
    lle_pool_free( core);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * ENTRY OPERATIONS
 * ============================================================================ */

/**
 * Expand entry array capacity
 */
lle_result_t lle_history_expand_capacity(lle_history_core_t *core) {
    if (!core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Calculate new capacity (double it) */
    size_t new_capacity = core->entry_capacity * 2;
    
    /* Check maximum capacity */
    if (new_capacity > core->config->max_entries) {
        new_capacity = core->config->max_entries;
    }
    
    /* Check if already at max */
    if (core->entry_capacity >= core->config->max_entries) {
        return LLE_ERROR_BUFFER_OVERFLOW;
    }
    
    /* Allocate new array */
    lle_history_entry_t **new_entries = lle_pool_alloc(sizeof(lle_history_entry_t*) * new_capacity);
    if (!new_entries) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy existing entries */
    memcpy(new_entries, core->entries, 
           sizeof(lle_history_entry_t*) * core->entry_count);
    
    /* Zero new space */
    memset(new_entries + core->entry_count, 0,
           sizeof(lle_history_entry_t*) * (new_capacity - core->entry_count));
    
    /* Free old array */
    lle_pool_free( core->entries);
    
    /* Update core */
    core->entries = new_entries;
    core->entry_capacity = new_capacity;
    
    return LLE_SUCCESS;
}

/**
 * Add entry to history
 */
lle_result_t lle_history_add_entry(
    lle_history_core_t *core,
    const char *command,
    int exit_code,
    uint64_t *entry_id
) {
    if (!core || !command) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!core->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    /* Check for space prefix (ignore if configured) */
    if (core->config->ignore_space_prefix && command[0] == ' ') {
        return LLE_SUCCESS;  /* Silently ignore */
    }
    
    /* Acquire write lock */
    pthread_rwlock_wrlock(&core->lock);
    
    lle_result_t result = LLE_SUCCESS;
    
    /* Start performance measurement */
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    
    /* Check if array is full */
    if (core->entry_count >= core->entry_capacity) {
        result = lle_history_expand_capacity(core);
        if (result != LLE_SUCCESS) {
            pthread_rwlock_unlock(&core->lock);
            return result;
        }
    }
    
    /* Create entry */
    lle_history_entry_t *entry = NULL;
    result = lle_history_entry_create(&entry, command, core->memory_pool);
    if (result != LLE_SUCCESS) {
        pthread_rwlock_unlock(&core->lock);
        return result;
    }
    
    /* Assign entry ID */
    entry->entry_id = core->next_entry_id++;
    entry->exit_code = exit_code;
    
    /* Add to array */
    core->entries[core->entry_count] = entry;
    
    /* Update linked list */
    if (core->last_entry) {
        core->last_entry->next = entry;
        entry->prev = core->last_entry;
    } else {
        core->first_entry = entry;
    }
    core->last_entry = entry;
    
    core->entry_count++;
    
    /* Phase 1 Day 2: Add to hashtable index if enabled */
    if (core->entry_lookup) {
        result = lle_history_index_insert(core->entry_lookup, entry->entry_id, entry);
        if (result != LLE_SUCCESS) {
            /* Rollback: remove from array and linked list */
            core->entry_count--;
            core->entries[core->entry_count] = NULL;
            if (entry->prev) {
                entry->prev->next = NULL;
                core->last_entry = entry->prev;
            } else {
                core->first_entry = NULL;
                core->last_entry = NULL;
            }
            lle_history_entry_destroy(entry, core->memory_pool);
            pthread_rwlock_unlock(&core->lock);
            return result;
        }
    }
    
    /* Update statistics */
    core->stats.total_entries++;
    core->stats.active_entries++;
    core->stats.add_count++;
    
    /* End performance measurement */
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    uint64_t elapsed_us = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
                         (end_time.tv_usec - start_time.tv_usec);
    core->stats.total_add_time_us += elapsed_us;
    
    /* Return entry ID if requested */
    if (entry_id) {
        *entry_id = entry->entry_id;
    }
    
    pthread_rwlock_unlock(&core->lock);
    
    return LLE_SUCCESS;
}

/**
 * Get entry by index
 */
lle_result_t lle_history_get_entry_by_index(
    lle_history_core_t *core,
    size_t index,
    lle_history_entry_t **entry
) {
    if (!core || !entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!core->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    /* Acquire read lock */
    pthread_rwlock_rdlock(&core->lock);
    
    /* Check bounds */
    if (index >= core->entry_count) {
        pthread_rwlock_unlock(&core->lock);
        return LLE_ERROR_NOT_FOUND;
    }
    
    /* Start performance measurement */
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    
    /* Return entry */
    *entry = core->entries[index];
    
    /* Update statistics */
    core->stats.retrieve_count++;
    
    /* End performance measurement */
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    uint64_t elapsed_us = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
                         (end_time.tv_usec - start_time.tv_usec);
    core->stats.total_retrieve_time_us += elapsed_us;
    
    pthread_rwlock_unlock(&core->lock);
    
    return LLE_SUCCESS;
}

/**
 * Get entry by ID (Phase 1: Linear search, Phase 2: Hashtable)
 */
lle_result_t lle_history_get_entry_by_id(
    lle_history_core_t *core,
    uint64_t entry_id,
    lle_history_entry_t **entry
) {
    if (!core || !entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!core->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    /* Acquire read lock */
    pthread_rwlock_rdlock(&core->lock);
    
    /* Start performance measurement */
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    
    /* Phase 1 Day 2: Use hashtable lookup if available, otherwise linear search */
    lle_history_entry_t *found = NULL;
    lle_result_t lookup_result;
    
    if (core->entry_lookup) {
        /* O(1) hashtable lookup */
        lookup_result = lle_history_index_lookup(core->entry_lookup, entry_id, &found);
        (void)lookup_result;  /* Lookup returns success even if not found (found will be NULL) */
    } else {
        /* O(n) linear search fallback */
        for (size_t i = 0; i < core->entry_count; i++) {
            if (core->entries[i]->entry_id == entry_id) {
                found = core->entries[i];
                break;
            }
        }
    }
    
    /* Update statistics */
    core->stats.retrieve_count++;
    
    /* End performance measurement */
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    uint64_t elapsed_us = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
                         (end_time.tv_usec - start_time.tv_usec);
    core->stats.total_retrieve_time_us += elapsed_us;
    
    pthread_rwlock_unlock(&core->lock);
    
    if (!found) {
        return LLE_ERROR_NOT_FOUND;
    }
    
    *entry = found;
    return LLE_SUCCESS;
}

/**
 * Get entry count
 */
lle_result_t lle_history_get_entry_count(
    lle_history_core_t *core,
    size_t *count
) {
    if (!core || !count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!core->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    /* Acquire read lock */
    pthread_rwlock_rdlock(&core->lock);
    *count = core->entry_count;
    pthread_rwlock_unlock(&core->lock);
    
    return LLE_SUCCESS;
}

/**
 * Clear all entries
 */
lle_result_t lle_history_clear(lle_history_core_t *core) {
    if (!core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!core->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    /* Acquire write lock */
    pthread_rwlock_wrlock(&core->lock);
    
    /* Destroy all entries */
    for (size_t i = 0; i < core->entry_count; i++) {
        if (core->entries[i]) {
            lle_history_entry_destroy(core->entries[i], core->memory_pool);
            core->entries[i] = NULL;
        }
    }
    
    /* Reset counts */
    core->entry_count = 0;
    core->first_entry = NULL;
    core->last_entry = NULL;
    
    /* Phase 1 Day 2: Clear hashtable index if present */
    if (core->entry_lookup) {
        lle_history_index_clear(core->entry_lookup);
    }
    
    /* Update statistics */
    core->stats.active_entries = 0;
    
    pthread_rwlock_unlock(&core->lock);
    
    return LLE_SUCCESS;
}

/**
 * Get statistics
 */
lle_result_t lle_history_get_stats(
    lle_history_core_t *core,
    const lle_history_stats_t **stats
) {
    if (!core || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!core->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    /* Acquire read lock */
    pthread_rwlock_rdlock(&core->lock);
    *stats = &core->stats;
    pthread_rwlock_unlock(&core->lock);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Get current working directory
 */
lle_result_t lle_history_get_cwd(char *buffer, size_t size) {
    if (!buffer || size == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (getcwd(buffer, size) == NULL) {
        return LLE_ERROR_ASSERTION_FAILED;
    }
    
    return LLE_SUCCESS;
}
