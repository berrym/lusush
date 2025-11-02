/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 3 */
/* History-Buffer Bridge: Bidirectional integration between history and buffer */

#ifndef LLE_HISTORY_BUFFER_BRIDGE_H
#define LLE_HISTORY_BUFFER_BRIDGE_H

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/command_structure.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_history_buffer_bridge lle_history_buffer_bridge_t;
typedef struct lle_history_core lle_history_core_t;
typedef struct lle_multiline_parser lle_multiline_parser_t;
typedef struct lle_reconstruction_engine lle_reconstruction_engine_t;

/* Buffer type - already defined in buffer_management.h */
typedef struct lle_buffer_t lle_buffer_t;

/**
 * Transfer direction
 */
typedef enum lle_transfer_direction {
    LLE_TRANSFER_HISTORY_TO_BUFFER = 0,  /* Load from history into buffer */
    LLE_TRANSFER_BUFFER_TO_HISTORY       /* Save from buffer to history */
} lle_transfer_direction_t;

/**
 * Transfer options
 */
typedef struct lle_transfer_options {
    /* Apply reconstruction when loading to buffer */
    bool apply_reconstruction;
    
    /* Parse multiline structure when loading */
    bool parse_multiline;
    
    /* Preserve indentation */
    bool preserve_indentation;
    
    /* Create new entry vs update existing */
    bool create_new_entry;
    
    /* Reserved for future use */
    void *reserved[4];
} lle_transfer_options_t;

/**
 * Transfer result
 */
typedef struct lle_transfer_result {
    /* Success flag */
    bool success;
    
    /* Entry index (for history operations) */
    size_t entry_index;
    
    /* Buffer position (for buffer operations) */
    size_t buffer_position;
    
    /* Bytes transferred */
    size_t bytes_transferred;
    
    /* Whether multiline structure was detected */
    bool is_multiline;
    
    /* Reserved for future use */
    void *reserved[2];
} lle_transfer_result_t;

/**
 * Create a history-buffer bridge
 *
 * @param bridge Output parameter for created bridge
 * @param memory_pool Memory pool for allocations
 * @param history_core History core for entry access
 * @param parser Multiline parser for structure analysis
 * @param reconstruction Reconstruction engine for formatting
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_create(
    lle_history_buffer_bridge_t **bridge,
    lle_memory_pool_t *memory_pool,
    lle_history_core_t *history_core,
    lle_multiline_parser_t *parser,
    lle_reconstruction_engine_t *reconstruction);

/**
 * Destroy a history-buffer bridge
 *
 * @param bridge Bridge to destroy
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_destroy(
    lle_history_buffer_bridge_t *bridge);

/**
 * Transfer a history entry to a buffer
 *
 * @param bridge History-buffer bridge
 * @param history_index History entry index
 * @param buffer Target buffer
 * @param options Transfer options (NULL for defaults)
 * @param result Output parameter for transfer result
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_load_to_buffer(
    lle_history_buffer_bridge_t *bridge,
    size_t history_index,
    lle_buffer_t *buffer,
    const lle_transfer_options_t *options,
    lle_transfer_result_t *result);

/**
 * Transfer buffer contents to history
 *
 * @param bridge History-buffer bridge
 * @param buffer Source buffer
 * @param options Transfer options (NULL for defaults)
 * @param result Output parameter for transfer result
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_save_from_buffer(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer,
    const lle_transfer_options_t *options,
    lle_transfer_result_t *result);

/**
 * Get buffer contents as string
 *
 * @param bridge History-buffer bridge
 * @param buffer Source buffer
 * @param text Output parameter for text (allocated by function)
 * @param length Output parameter for text length
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_get_buffer_text(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer,
    char **text,
    size_t *length);

/**
 * Set buffer contents from string
 *
 * @param bridge History-buffer bridge
 * @param buffer Target buffer
 * @param text Text to set
 * @param length Text length
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_set_buffer_text(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer,
    const char *text,
    size_t length);

/**
 * Clear buffer contents
 *
 * @param bridge History-buffer bridge
 * @param buffer Buffer to clear
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_clear_buffer(
    lle_history_buffer_bridge_t *bridge,
    lle_buffer_t *buffer);

/**
 * Get default transfer options
 *
 * @param options Options structure to fill with defaults
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_history_buffer_bridge_get_default_options(
    lle_transfer_options_t *options);

#ifdef __cplusplus
}
#endif

#endif /* LLE_HISTORY_BUFFER_BRIDGE_H */
