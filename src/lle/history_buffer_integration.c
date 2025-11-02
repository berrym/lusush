/**
 * history_buffer_integration.c - History-Buffer Integration Implementation
 *
 * Phase 1: Core Infrastructure
 * - System lifecycle (create, destroy, config)
 * - Callback registration
 * - State management
 *
 * Critical Gap Spec: 22_history_buffer_integration_complete.md
 * Date: 2025-11-02
 */

#include "lle/history_buffer_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * INTERNAL STRUCTURES (will be moved to separate files in later phases)
 * ============================================================================ */

/* Stub structures for Phase 1 - will be implemented in later phases */
struct lle_reconstruction_engine {
    lle_memory_pool_t *pool;
    bool initialized;
};

struct lle_edit_session_manager {
    lle_memory_pool_t *pool;
    uint64_t next_session_id;
    bool initialized;
};

struct lle_multiline_parser {
    lle_memory_pool_t *pool;
    bool initialized;
};

struct lle_structure_analyzer {
    lle_memory_pool_t *pool;
    bool initialized;
};

struct lle_formatting_engine {
    lle_memory_pool_t *pool;
    bool initialized;
};

struct lle_callback_registry {
    lle_history_edit_callbacks_t callbacks;
    bool has_callbacks;
};

struct lle_edit_cache {
    lle_memory_pool_t *pool;
    uint32_t max_entries;
    uint64_t hits;
    uint64_t misses;
    bool initialized;
};

/* ============================================================================
 * DEFAULT CONFIGURATION
 * ============================================================================ */

static const lle_integration_config_t DEFAULT_CONFIG = {
    .enable_multiline_reconstruction = true,
    .preserve_original_formatting = true,
    .enable_structure_analysis = true,
    .enable_edit_caching = true,
    .max_cache_entries = 100,
    .max_reconstruction_depth = 10,
    .reconstruction_timeout_ms = 1000
};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Initialize stub component (placeholder for Phase 2+)
 */
static lle_result_t init_stub_component(void **component, lle_memory_pool_t *pool, size_t size) {
    if (!component || !pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    void *comp = lle_pool_alloc(size);
    if (!comp) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(comp, 0, size);
    *component = comp;
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * SYSTEM LIFECYCLE FUNCTIONS
 * ============================================================================ */

lle_result_t lle_history_buffer_integration_create(
    lle_history_buffer_integration_t **integration,
    lle_history_core_t *history_core,
    lle_memory_pool_t *memory_pool,
    lle_event_system_t *event_system)
{
    lle_result_t result = LLE_SUCCESS;
    lle_history_buffer_integration_t *integ = NULL;
    
    /* Validate parameters */
    if (!integration || !history_core || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate integration system */
    integ = lle_pool_alloc(sizeof(lle_history_buffer_integration_t));
    if (!integ) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(integ, 0, sizeof(lle_history_buffer_integration_t));
    
    /* Set core references */
    integ->history_core = history_core;
    integ->memory_pool = memory_pool;
    integ->event_system = event_system;
    
    /* Allocate configuration */
    integ->config = lle_pool_alloc(sizeof(lle_integration_config_t));
    if (!integ->config) {
        lle_pool_free(integ);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(integ->config, &DEFAULT_CONFIG, sizeof(lle_integration_config_t));
    
    /* Allocate state */
    integ->current_state = lle_pool_alloc(sizeof(lle_integration_state_t));
    if (!integ->current_state) {
        lle_pool_free(integ->config);
        lle_pool_free(integ);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(integ->current_state, 0, sizeof(lle_integration_state_t));
    integ->current_state->state = LLE_INTEGRATION_UNINITIALIZED;
    
    /* Initialize callback registry */
    result = init_stub_component((void**)&integ->callback_registry, 
                                 memory_pool, 
                                 sizeof(struct lle_callback_registry));
    if (result != LLE_SUCCESS) {
        lle_pool_free(integ->current_state);
        lle_pool_free(integ->config);
        lle_pool_free(integ);
        return result;
    }
    
    /* Initialize stub components (will be fully implemented in later phases) */
    result = init_stub_component((void**)&integ->reconstruction, 
                                 memory_pool, 
                                 sizeof(struct lle_reconstruction_engine));
    if (result != LLE_SUCCESS) goto cleanup;
    
    result = init_stub_component((void**)&integ->session_manager, 
                                 memory_pool, 
                                 sizeof(struct lle_edit_session_manager));
    if (result != LLE_SUCCESS) goto cleanup;
    
    result = init_stub_component((void**)&integ->multiline_parser, 
                                 memory_pool, 
                                 sizeof(struct lle_multiline_parser));
    if (result != LLE_SUCCESS) goto cleanup;
    
    result = init_stub_component((void**)&integ->structure_analyzer, 
                                 memory_pool, 
                                 sizeof(struct lle_structure_analyzer));
    if (result != LLE_SUCCESS) goto cleanup;
    
    result = init_stub_component((void**)&integ->formatter, 
                                 memory_pool, 
                                 sizeof(struct lle_formatting_engine));
    if (result != LLE_SUCCESS) goto cleanup;
    
    result = init_stub_component((void**)&integ->edit_cache, 
                                 memory_pool, 
                                 sizeof(struct lle_edit_cache));
    if (result != LLE_SUCCESS) goto cleanup;
    
    /* Initialize performance monitor if not provided */
    if (!integ->perf_monitor) {
        /* Performance monitor should be created by caller, but we'll handle gracefully */
        integ->perf_monitor = NULL;
    }
    
    /* Initialize thread synchronization */
    if (pthread_rwlock_init(&integ->integration_lock, NULL) != 0) {
        result = LLE_ERROR_INITIALIZATION_FAILED;
        goto cleanup;
    }
    
    /* Set initial state */
    integ->system_active = true;
    integ->session_counter = 0;
    integ->current_state->state = LLE_INTEGRATION_READY;
    
    *integration = integ;
    return LLE_SUCCESS;
    
cleanup:
    if (integ) {
        if (integ->edit_cache) lle_pool_free(integ->edit_cache);
        if (integ->formatter) lle_pool_free(integ->formatter);
        if (integ->structure_analyzer) lle_pool_free(integ->structure_analyzer);
        if (integ->multiline_parser) lle_pool_free(integ->multiline_parser);
        if (integ->session_manager) lle_pool_free(integ->session_manager);
        if (integ->reconstruction) lle_pool_free(integ->reconstruction);
        if (integ->callback_registry) lle_pool_free(integ->callback_registry);
        if (integ->current_state) lle_pool_free(integ->current_state);
        if (integ->config) lle_pool_free(integ->config);
        lle_pool_free(integ);
    }
    return result;
}

lle_result_t lle_history_buffer_integration_destroy(
    lle_history_buffer_integration_t *integration)
{
    if (!integration) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Acquire write lock */
    pthread_rwlock_wrlock(&integration->integration_lock);
    
    /* Mark as shutting down */
    integration->system_active = false;
    if (integration->current_state) {
        integration->current_state->state = LLE_INTEGRATION_SHUTDOWN;
    }
    
    /* Check for active sessions - should wait or fail? */
    if (integration->current_state && integration->current_state->active_sessions > 0) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_STATE;  /* Cannot destroy with active sessions */
    }
    
    /* Free all components */
    if (integration->edit_cache) lle_pool_free(integration->edit_cache);
    if (integration->formatter) lle_pool_free(integration->formatter);
    if (integration->structure_analyzer) lle_pool_free(integration->structure_analyzer);
    if (integration->multiline_parser) lle_pool_free(integration->multiline_parser);
    if (integration->session_manager) lle_pool_free(integration->session_manager);
    if (integration->reconstruction) lle_pool_free(integration->reconstruction);
    if (integration->callback_registry) lle_pool_free(integration->callback_registry);
    if (integration->current_state) lle_pool_free(integration->current_state);
    if (integration->config) lle_pool_free(integration->config);
    
    /* Unlock and destroy lock */
    pthread_rwlock_unlock(&integration->integration_lock);
    pthread_rwlock_destroy(&integration->integration_lock);
    
    /* Free integration structure itself */
    lle_pool_free(integration);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * CONFIGURATION FUNCTIONS
 * ============================================================================ */

lle_result_t lle_history_buffer_integration_set_config(
    lle_history_buffer_integration_t *integration,
    const lle_integration_config_t *config)
{
    if (!integration || !config) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Acquire write lock */
    pthread_rwlock_wrlock(&integration->integration_lock);
    
    if (!integration->system_active) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Validate configuration */
    if (config->max_cache_entries == 0 || config->max_cache_entries > 10000) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (config->max_reconstruction_depth == 0 || config->max_reconstruction_depth > 100) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (config->reconstruction_timeout_ms == 0 || config->reconstruction_timeout_ms > 60000) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Apply configuration */
    memcpy(integration->config, config, sizeof(lle_integration_config_t));
    
    /* Update cache max entries if cache exists */
    if (integration->edit_cache) {
        integration->edit_cache->max_entries = config->max_cache_entries;
    }
    
    pthread_rwlock_unlock(&integration->integration_lock);
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_integration_get_config(
    lle_history_buffer_integration_t *integration,
    lle_integration_config_t *config)
{
    if (!integration || !config) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Acquire read lock */
    pthread_rwlock_rdlock(&integration->integration_lock);
    
    if (!integration->system_active) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Copy configuration */
    memcpy(config, integration->config, sizeof(lle_integration_config_t));
    
    pthread_rwlock_unlock(&integration->integration_lock);
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_integration_get_state(
    lle_history_buffer_integration_t *integration,
    lle_integration_state_t *state)
{
    if (!integration || !state) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Acquire read lock */
    pthread_rwlock_rdlock(&integration->integration_lock);
    
    /* Copy state */
    memcpy(state, integration->current_state, sizeof(lle_integration_state_t));
    
    pthread_rwlock_unlock(&integration->integration_lock);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * CALLBACK REGISTRATION
 * ============================================================================ */

lle_result_t lle_history_buffer_integration_register_callbacks(
    lle_history_buffer_integration_t *integration,
    const lle_history_edit_callbacks_t *callbacks)
{
    if (!integration) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Acquire write lock */
    pthread_rwlock_wrlock(&integration->integration_lock);
    
    if (!integration->system_active) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (!integration->callback_registry) {
        pthread_rwlock_unlock(&integration->integration_lock);
        return LLE_ERROR_INVALID_STATE;
    }
    
    if (callbacks) {
        /* Register callbacks */
        memcpy(&integration->callback_registry->callbacks, 
               callbacks, 
               sizeof(lle_history_edit_callbacks_t));
        integration->callback_registry->has_callbacks = true;
    } else {
        /* Clear callbacks */
        memset(&integration->callback_registry->callbacks, 
               0, 
               sizeof(lle_history_edit_callbacks_t));
        integration->callback_registry->has_callbacks = false;
    }
    
    pthread_rwlock_unlock(&integration->integration_lock);
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_integration_unregister_callbacks(
    lle_history_buffer_integration_t *integration)
{
    return lle_history_buffer_integration_register_callbacks(integration, NULL);
}

/* ============================================================================
 * PHASE 2 & 3 IMPLEMENTATIONS
 * ============================================================================ */

/*
 * Phase 2 (Multiline Reconstruction Engine) and Phase 3 (Interactive Editing)
 * implementations will be added here as they are completed.
 *
 * Phase 1 is complete - all exposed functions are fully implemented.
 * Next: Implement Phase 2 reconstruction engine, then Phase 3 editing functions.
 */
