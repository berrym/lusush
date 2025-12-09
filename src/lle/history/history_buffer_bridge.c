/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 3 */
/* History-Buffer Bridge Implementation */

#include "lle/history_buffer_bridge.h"
#include "lle/history.h"
#include "lle/buffer_management.h"
#include "lle/multiline_parser.h"
#include "lle/reconstruction_engine.h"
#include "lle/memory_management.h"
#include <string.h>

/* History-buffer bridge implementation */
struct lle_history_buffer_bridge {
    lle_memory_pool_t *memory_pool;
    lle_history_core_t *history_core;
    lle_multiline_parser_t *parser;
    lle_reconstruction_engine_t *reconstruction;
    bool active;
};

/* Forward declarations for internal functions */
static lle_result_t get_history_entry_text(lle_history_core_t *history,
                                           size_t index,
                                           char **text,
                                           size_t *length);

lle_result_t lle_history_buffer_bridge_get_default_options(
    lle_transfer_options_t *options)
{
    if (!options) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    options->apply_reconstruction = true;
    options->parse_multiline = true;
    options->preserve_indentation = true;
    options->create_new_entry = false;
    memset(options->reserved, 0, sizeof(options->reserved));
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_bridge_create(
    lle_history_buffer_bridge_t **bridge,
    lle_memory_pool_t *memory_pool,
    lle_history_core_t *history_core,
    lle_multiline_parser_t *parser,
    lle_reconstruction_engine_t *reconstruction)
{
    if (!bridge || !history_core || !parser || !reconstruction) { /* memory_pool can be NULL */
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_history_buffer_bridge_t *new_bridge = 
        lle_pool_alloc(sizeof(lle_history_buffer_bridge_t));
    if (!new_bridge) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    new_bridge->memory_pool = memory_pool;
    new_bridge->history_core = history_core;
    new_bridge->parser = parser;
    new_bridge->reconstruction = reconstruction;
    new_bridge->active = true;
    
    *bridge = new_bridge;
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_bridge_destroy(
    lle_history_buffer_bridge_t *bridge)
{
    if (!bridge) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    bridge->active = false;
    /* Memory pool owns all allocations, no explicit frees needed */
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_bridge_get_buffer_text(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer,
    char **text,
    size_t *length)
{
    if (!bridge || !bridge->active || !buffer || !text || !length) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Get buffer text directly from buffer structure */
    const char *buffer_text = buffer->data;
    if (!buffer_text) {
        *text = NULL;
        *length = 0;
        return LLE_SUCCESS;
    }
    
    size_t text_len = buffer->length;
    
    /* Allocate and copy */
    char *text_copy = lle_pool_alloc(text_len + 1);
    if (!text_copy) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(text_copy, buffer_text, text_len);
    text_copy[text_len] = '\0';
    
    *text = text_copy;
    *length = text_len;
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_bridge_set_buffer_text(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer,
    const char *text,
    size_t length)
{
    if (!bridge || !bridge->active || !buffer || !text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear buffer first */
    lle_result_t result = lle_buffer_clear(buffer);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Insert text at beginning */
    if (length > 0) {
        result = lle_buffer_insert_text(buffer, 0, text, length);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_history_buffer_bridge_clear_buffer(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer)
{
    if (!bridge || !bridge->active || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    return lle_buffer_clear(buffer);
}

lle_result_t lle_history_buffer_bridge_load_to_buffer(
    lle_history_buffer_bridge_t *bridge,
    size_t history_index,
    lle_buffer_t *buffer,
    const lle_transfer_options_t *options,
    lle_transfer_result_t *result)
{
    if (!bridge || !bridge->active || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Get default options if not provided */
    lle_transfer_options_t default_options;
    if (!options) {
        lle_history_buffer_bridge_get_default_options(&default_options);
        options = &default_options;
    }
    
    /* Initialize result */
    if (result) {
        memset(result, 0, sizeof(lle_transfer_result_t));
        result->entry_index = history_index;
    }
    
    /* Get history entry text */
    char *entry_text = NULL;
    size_t entry_length = 0;
    lle_result_t res = get_history_entry_text(
        bridge->history_core,
        history_index,
        &entry_text,
        &entry_length);
    
    if (res != LLE_SUCCESS) {
        if (result) result->success = false;
        return res;
    }
    
    /* Apply reconstruction if requested */
    char *final_text = entry_text;
    size_t final_length = entry_length;
    
    if (options->apply_reconstruction && bridge->reconstruction) {
        lle_reconstructed_command_t *reconstructed = NULL;
        res = lle_reconstruction_engine_reconstruct(
            bridge->reconstruction,
            entry_text,
            entry_length,
            &reconstructed);
        
        if (res == LLE_SUCCESS && reconstructed) {
            final_text = reconstructed->text;
            final_length = reconstructed->length;
        }
    }
    
    /* Parse multiline structure if requested */
    bool is_multiline = false;
    if (options->parse_multiline && bridge->parser) {
        lle_multiline_parse_result_t *parse_result = NULL;
        res = lle_multiline_parser_parse(
            bridge->parser,
            final_text,
            final_length,
            &parse_result);
        
        if (res == LLE_SUCCESS && parse_result) {
            is_multiline = (parse_result->line_count > 1);
            if (result) result->is_multiline = is_multiline;
        }
    }
    
    /* Set buffer text */
    res = lle_history_buffer_bridge_set_buffer_text(
        bridge,
        buffer,
        final_text,
        final_length);
    
    if (result) {
        result->success = (res == LLE_SUCCESS);
        result->bytes_transferred = final_length;
        result->buffer_position = 0;
    }
    
    return res;
}

lle_result_t lle_history_buffer_bridge_save_from_buffer(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer,
    const lle_transfer_options_t *options,
    lle_transfer_result_t *result)
{
    if (!bridge || !bridge->active || !buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Get default options if not provided */
    lle_transfer_options_t default_options;
    if (!options) {
        lle_history_buffer_bridge_get_default_options(&default_options);
        options = &default_options;
    }
    
    /* Initialize result */
    if (result) {
        memset(result, 0, sizeof(lle_transfer_result_t));
    }
    
    /* Get buffer text */
    char *buffer_text = NULL;
    size_t buffer_length = 0;
    lle_result_t res = lle_history_buffer_bridge_get_buffer_text(
        bridge,
        buffer,
        &buffer_text,
        &buffer_length);
    
    if (res != LLE_SUCCESS) {
        if (result) result->success = false;
        return res;
    }
    
    /* Add to history */
    if (buffer_text && buffer_length > 0) {
        uint64_t entry_id = 0;
        res = lle_history_add_entry(bridge->history_core, buffer_text, -1, &entry_id);
        
        if (res == LLE_SUCCESS && result) {
            result->success = true;
            result->bytes_transferred = buffer_length;
            
            /* Get the index of the newly added entry */
            size_t entry_count = 0;
            if (lle_history_get_entry_count(bridge->history_core, &entry_count) == LLE_SUCCESS) {
                result->entry_index = entry_count - 1;
            }
        }
    }
    
    return res;
}

/* Internal helper functions */

static lle_result_t get_history_entry_text(lle_history_core_t *history,
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
