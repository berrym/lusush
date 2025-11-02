/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 3 */
/* Edit Session Manager Implementation */

#include "lle/edit_session_manager.h"
#include "lle/history_buffer_integration.h"
#include "lle/history.h"
#include "lle/memory_management.h"
#include <string.h>
#include <time.h>

/* Default configuration values */
#define DEFAULT_MAX_SESSIONS 10
#define DEFAULT_SESSION_TIMEOUT_MS 300000  /* 5 minutes */
#define DEFAULT_MAX_OPERATIONS 1000

/* Edit session manager implementation */
struct lle_edit_session_manager {
    lle_memory_pool_t *memory_pool;
    lle_history_core_t *history_core;
    lle_session_manager_config_t config;
    
    /* Active sessions */
    lle_edit_session_t *current_session;
    lle_edit_session_t **sessions;
    size_t session_count;
    
    /* Session ID counter */
    uint64_t next_session_id;
    
    bool active;
};

/* Forward declarations for internal functions */
static lle_edit_session_t* create_edit_session(lle_edit_session_manager_t *manager,
                                                size_t entry_index);
static void free_edit_session(lle_edit_session_t *session);
static lle_result_t get_entry_text(lle_history_core_t *history,
                                   size_t index,
                                   char **text,
                                   size_t *length);
static int64_t timespec_diff_ms(const struct timespec *start,
                                const struct timespec *end);

lle_result_t lle_edit_session_manager_get_default_config(
    lle_session_manager_config_t *config)
{
    if (!config) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    config->max_sessions = DEFAULT_MAX_SESSIONS;
    config->session_timeout_ms = DEFAULT_SESSION_TIMEOUT_MS;
    config->track_operations = true;
    config->max_operations = DEFAULT_MAX_OPERATIONS;
    memset(config->reserved, 0, sizeof(config->reserved));
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_create(
    lle_edit_session_manager_t **manager,
    lle_memory_pool_t *memory_pool,
    lle_history_core_t *history_core,
    const lle_session_manager_config_t *config)
{
    if (!manager || !memory_pool || !history_core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_edit_session_manager_t *new_manager = 
        lle_pool_alloc(sizeof(lle_edit_session_manager_t));
    if (!new_manager) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(new_manager, 0, sizeof(lle_edit_session_manager_t));
    
    new_manager->memory_pool = memory_pool;
    new_manager->history_core = history_core;
    
    if (config) {
        memcpy(&new_manager->config, config, sizeof(lle_session_manager_config_t));
    } else {
        lle_edit_session_manager_get_default_config(&new_manager->config);
    }
    
    /* Allocate sessions array */
    new_manager->sessions = lle_pool_alloc(
        new_manager->config.max_sessions * sizeof(lle_edit_session_t *));
    if (!new_manager->sessions) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(new_manager->sessions, 0,
           new_manager->config.max_sessions * sizeof(lle_edit_session_t *));
    
    new_manager->session_count = 0;
    new_manager->next_session_id = 1;
    new_manager->current_session = NULL;
    new_manager->active = true;
    
    *manager = new_manager;
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_destroy(
    lle_edit_session_manager_t *manager)
{
    if (!manager) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free all sessions */
    if (manager->sessions) {
        for (size_t i = 0; i < manager->config.max_sessions; i++) {
            if (manager->sessions[i]) {
                free_edit_session(manager->sessions[i]);
            }
        }
    }
    
    manager->active = false;
    /* Memory pool owns all allocations, no explicit frees needed */
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_start_session(
    lle_edit_session_manager_t *manager,
    size_t entry_index,
    lle_edit_session_t **session)
{
    if (!manager || !manager->active || !session) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if already at max sessions */
    if (manager->session_count >= manager->config.max_sessions) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Verify entry exists */
    size_t entry_count = 0;
    lle_result_t result = lle_history_get_entry_count(manager->history_core, &entry_count);
    if (result != LLE_SUCCESS || entry_index >= entry_count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Create new session */
    lle_edit_session_t *new_session = create_edit_session(manager, entry_index);
    if (!new_session) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Add to sessions array */
    for (size_t i = 0; i < manager->config.max_sessions; i++) {
        if (!manager->sessions[i]) {
            manager->sessions[i] = new_session;
            manager->session_count++;
            break;
        }
    }
    
    /* Set as current session */
    manager->current_session = new_session;
    
    *session = new_session;
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_get_current_session(
    lle_edit_session_manager_t *manager,
    lle_edit_session_t **session)
{
    if (!manager || !manager->active || !session) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    *session = manager->current_session;
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_record_operation(
    lle_edit_session_manager_t *manager,
    lle_edit_session_t *session,
    const lle_edit_operation_t *operation)
{
    if (!manager || !manager->active || !session || !operation) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!manager->config.track_operations) {
        return LLE_SUCCESS;  /* Operation tracking disabled */
    }
    
    /* Check operation limit */
    if (session->operation_count >= manager->config.max_operations) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Allocate operation record */
    lle_edit_operation_t *op = lle_pool_alloc(sizeof(lle_edit_operation_t));
    if (!op) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(op, operation, sizeof(lle_edit_operation_t));
    op->next = NULL;
    
    /* Copy text if present */
    if (operation->text && operation->text_length > 0) {
        op->text = lle_pool_alloc(operation->text_length + 1);
        if (!op->text) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        memcpy(op->text, operation->text, operation->text_length);
        op->text[operation->text_length] = '\0';
    }
    
    /* Get timestamp */
    clock_gettime(CLOCK_MONOTONIC, &op->timestamp);
    
    /* Add to session operation list */
    if (!session->first_operation) {
        session->first_operation = op;
        session->last_operation = op;
    } else {
        session->last_operation->next = op;
        session->last_operation = op;
    }
    
    session->operation_count++;
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_update_text(
    lle_edit_session_manager_t *manager,
    lle_edit_session_t *session,
    const char *new_text,
    size_t new_length)
{
    if (!manager || !manager->active || !session || !new_text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate new text buffer */
    char *text_copy = lle_pool_alloc(new_length + 1);
    if (!text_copy) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(text_copy, new_text, new_length);
    text_copy[new_length] = '\0';
    
    /* Update session */
    session->current_text = text_copy;
    session->current_length = new_length;
    session->has_modifications = true;
    session->state = LLE_EDIT_SESSION_MODIFIED;
    
    /* Update timestamp */
    clock_gettime(CLOCK_MONOTONIC, &session->last_modified);
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_complete_session(
    lle_edit_session_manager_t *manager,
    lle_edit_session_t *session)
{
    if (!manager || !manager->active || !session) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (session->state != LLE_EDIT_SESSION_ACTIVE &&
        session->state != LLE_EDIT_SESSION_MODIFIED) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    session->state = LLE_EDIT_SESSION_COMPLETED;
    
    /* If modifications were made, update the history entry */
    if (session->has_modifications && session->current_text) {
        /* Get the history entry */
        lle_history_entry_t *entry = NULL;
        lle_result_t result = lle_history_get_entry_by_index(
            manager->history_core,
            session->entry_index,
            &entry);
        
        if (result == LLE_SUCCESS && entry) {
            /* Update entry command text */
            /* Note: This would normally call lle_history_update_entry() */
            /* For now, we just mark the session as complete */
            (void)entry;  /* Suppress unused warning */
        }
    }
    
    /* Remove from sessions array */
    for (size_t i = 0; i < manager->config.max_sessions; i++) {
        if (manager->sessions[i] == session) {
            manager->sessions[i] = NULL;
            manager->session_count--;
            break;
        }
    }
    
    /* Clear current session if this was it */
    if (manager->current_session == session) {
        manager->current_session = NULL;
    }
    
    /* Free session resources */
    free_edit_session(session);
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_cancel_session(
    lle_edit_session_manager_t *manager,
    lle_edit_session_t *session)
{
    if (!manager || !manager->active || !session) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    session->state = LLE_EDIT_SESSION_CANCELING;
    
    /* Remove from sessions array */
    for (size_t i = 0; i < manager->config.max_sessions; i++) {
        if (manager->sessions[i] == session) {
            manager->sessions[i] = NULL;
            manager->session_count--;
            break;
        }
    }
    
    /* Clear current session if this was it */
    if (manager->current_session == session) {
        manager->current_session = NULL;
    }
    
    /* Free session resources */
    free_edit_session(session);
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_session_manager_check_timeout(
    lle_edit_session_manager_t *manager,
    lle_edit_session_t *session,
    bool *timed_out)
{
    if (!manager || !manager->active || !session || !timed_out) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    *timed_out = false;
    
    if (manager->config.session_timeout_ms == 0) {
        return LLE_SUCCESS;  /* No timeout configured */
    }
    
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    int64_t elapsed_ms = timespec_diff_ms(&session->last_modified, &now);
    
    if (elapsed_ms >= (int64_t)manager->config.session_timeout_ms) {
        *timed_out = true;
        session->state = LLE_EDIT_SESSION_ERROR;
    }
    
    return LLE_SUCCESS;
}

/* Internal helper functions */

static lle_edit_session_t* create_edit_session(lle_edit_session_manager_t *manager,
                                                size_t entry_index)
{
    lle_edit_session_t *session = lle_pool_alloc(sizeof(lle_edit_session_t));
    if (!session) {
        return NULL;
    }
    
    memset(session, 0, sizeof(lle_edit_session_t));
    
    session->session_id = manager->next_session_id++;
    session->state = LLE_EDIT_SESSION_ACTIVE;
    session->entry_index = entry_index;
    
    /* Get original entry text */
    char *text = NULL;
    size_t length = 0;
    if (get_entry_text(manager->history_core, entry_index, &text, &length) == LLE_SUCCESS) {
        session->original_text = text;
        session->original_length = length;
        
        /* Initialize current text as copy of original */
        session->current_text = text;
        session->current_length = length;
    }
    
    /* Initialize timestamps */
    clock_gettime(CLOCK_MONOTONIC, &session->start_time);
    session->last_modified = session->start_time;
    
    session->has_modifications = false;
    session->multiline_mode = false;
    session->first_operation = NULL;
    session->last_operation = NULL;
    session->operation_count = 0;
    session->structure = NULL;
    
    return session;
}

static void free_edit_session(lle_edit_session_t *session)
{
    /* Memory pool owns all allocations, no explicit frees needed */
    /* This function exists for API completeness and future enhancements */
    (void)session;
}

static lle_result_t get_entry_text(lle_history_core_t *history,
                                   size_t index,
                                   char **text,
                                   size_t *length)
{
    lle_history_entry_t *entry = NULL;
    lle_result_t result = lle_history_get_entry_by_index(history, index, &entry);
    
    if (result != LLE_SUCCESS || !entry || !entry->command) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t cmd_len = strlen(entry->command);
    char *text_copy = lle_pool_alloc(cmd_len + 1);
    if (!text_copy) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(text_copy, entry->command, cmd_len);
    text_copy[cmd_len] = '\0';
    
    *text = text_copy;
    *length = cmd_len;
    
    return LLE_SUCCESS;
}

static int64_t timespec_diff_ms(const struct timespec *start,
                                const struct timespec *end)
{
    int64_t sec_diff = end->tv_sec - start->tv_sec;
    int64_t nsec_diff = end->tv_nsec - start->tv_nsec;
    
    return (sec_diff * 1000) + (nsec_diff / 1000000);
}
