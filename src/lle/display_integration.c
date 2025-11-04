/**
 * @file display_integration.c
 * @brief LLE Display Integration - Implementation (Layer 1)
 * 
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION PLAN: docs/lle_implementation/SPEC_08_IMPLEMENTATION_PLAN.md
 * 
 * This file implements the Spec 08 display integration layer that connects
 * LLE's buffer system with Lusush's layered display architecture.
 * 
 * ZERO-TOLERANCE COMPLIANCE:
 * - Complete implementations only
 * - 100% spec-compliant implementations  
 * - Complete error handling for all functions
 */

#include "lle/display_integration.h"
#include "lle/lle_readline.h"
#include "display/command_layer.h"
#include "display/prompt_layer.h"
#include "display/layer_events.h"
#include "display_integration.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ========================================================================== */
/*                         GLOBAL STATE                                       */
/* ========================================================================== */

/**
 * @brief Global display integration singleton instance
 * 
 * This is initialized once during LLE startup and provides the bridge
 * between LLE and Lusush display systems.
 */
static lle_display_integration_t *global_display_integration = NULL;

/* ========================================================================== */
/*                    CORE INTEGRATION FUNCTIONS                              */
/* ========================================================================== */

/**
 * @brief Get the global display integration instance
 * 
 * @return Pointer to global display integration, or NULL if not initialized
 */
lle_display_integration_t *lle_display_integration_get_global(void)
{
    return global_display_integration;
}

/**
 * @brief Initialize the display integration system
 * 
 * Creates and initializes the global display integration instance that connects
 * LLE's editing system with Lusush's display layers.
 * 
 * @param integration Output pointer for created integration instance
 * @param editor LLE editor instance (opaque)
 * @param lusush_display Lusush display controller
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_display_integration_init(
    lle_display_integration_t **integration,
    void *editor,
    display_controller_t *lusush_display,
    lle_memory_pool_t *memory_pool)
{
    if (!integration || !lusush_display) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Allocate main integration structure */
    lle_display_integration_t *integ = calloc(1, sizeof(lle_display_integration_t));
    if (!integ) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Store Lusush display controller reference */
    integ->lusush_display = lusush_display;
    integ->memory_pool = memory_pool;
    integ->integration_active = true;
    integ->api_version = 1;
    integ->frame_counter = 0;

    /* Initialize integration lock */
    if (pthread_rwlock_init(&integ->integration_lock, NULL) != 0) {
        free(integ);
        return LLE_ERROR_SYSTEM_CALL;
    }

    /* Initialize display bridge */
    lle_result_t result = lle_display_bridge_init(
        &integ->display_bridge,
        editor,
        lusush_display,
        memory_pool
    );

    if (result != LLE_SUCCESS) {
        pthread_rwlock_destroy(&integ->integration_lock);
        free(integ);
        return result;
    }

    /* Initialize render controller */
    result = lle_render_controller_init(
        &integ->render_controller,
        integ->display_bridge,
        memory_pool
    );

    if (result != LLE_SUCCESS) {
        lle_display_bridge_cleanup(integ->display_bridge);
        pthread_rwlock_destroy(&integ->integration_lock);
        free(integ);
        return result;
    }

    /* Allocate display metrics */
    integ->perf_metrics = calloc(1, sizeof(lle_display_metrics_t));
    if (!integ->perf_metrics) {
        lle_render_controller_cleanup(integ->render_controller);
        lle_display_bridge_cleanup(integ->display_bridge);
        pthread_rwlock_destroy(&integ->integration_lock);
        free(integ);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Allocate display config */
    integ->config = calloc(1, sizeof(lle_display_config_t));
    if (!integ->config) {
        free(integ->perf_metrics);
        lle_render_controller_cleanup(integ->render_controller);
        lle_display_bridge_cleanup(integ->display_bridge);
        pthread_rwlock_destroy(&integ->integration_lock);
        free(integ);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Set default config */
    integ->config->enable_syntax_highlighting = true;
    integ->config->enable_caching = false;  /* Disabled for now */
    integ->config->enable_performance_monitoring = false;

    /* Allocate display state */
    integ->current_state = calloc(1, sizeof(lle_display_state_t));
    if (!integ->current_state) {
        free(integ->config);
        free(integ->perf_metrics);
        lle_render_controller_cleanup(integ->render_controller);
        lle_display_bridge_cleanup(integ->display_bridge);
        pthread_rwlock_destroy(&integ->integration_lock);
        free(integ);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Set as global singleton */
    global_display_integration = integ;
    *integration = integ;

    return LLE_SUCCESS;
}

/**
 * @brief Cleanup the display integration system
 * 
 * @param integration Display integration instance to cleanup
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_display_integration_cleanup(lle_display_integration_t *integration)
{
    if (!integration) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Clear global singleton if this is it */
    if (global_display_integration == integration) {
        global_display_integration = NULL;
    }

    /* Cleanup render controller */
    if (integration->render_controller) {
        lle_render_controller_cleanup(integration->render_controller);
    }

    /* Cleanup display bridge */
    if (integration->display_bridge) {
        lle_display_bridge_cleanup(integration->display_bridge);
    }

    /* Free allocated structures */
    free(integration->current_state);
    free(integration->config);
    free(integration->perf_metrics);

    /* Destroy lock */
    pthread_rwlock_destroy(&integration->integration_lock);

    /* Free main structure */
    free(integration);

    return LLE_SUCCESS;
}

/* ========================================================================== */
/*                      DISPLAY BRIDGE FUNCTIONS                              */
/* ========================================================================== */

/* NOTE: Display bridge init, cleanup, and send_output are implemented in
 * display_bridge.c to avoid duplication. */

/* ========================================================================== */
/*                      RENDER CONTROLLER FUNCTIONS                           */
/* ========================================================================== */

/**
 * @brief Initialize render controller
 * 
 * @param controller Output pointer for created controller
 * @param bridge Display bridge instance
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_render_controller_init(
    lle_render_controller_t **controller,
    lle_display_bridge_t *bridge,
    lle_memory_pool_t *memory_pool)
{
    if (!controller || !bridge) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_render_controller_t *ctrl = calloc(1, sizeof(lle_render_controller_t));
    if (!ctrl) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    ctrl->bridge = bridge;
    ctrl->memory_pool = memory_pool;

    /* Allocate dirty tracker */
    ctrl->dirty_tracker = calloc(1, sizeof(lle_dirty_tracker_t));
    if (!ctrl->dirty_tracker) {
        free(ctrl);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    ctrl->dirty_tracker->full_redraw_needed = true;

    /* Allocate render metrics */
    ctrl->metrics = calloc(1, sizeof(lle_render_metrics_t));
    if (!ctrl->metrics) {
        free(ctrl->dirty_tracker);
        free(ctrl);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Allocate render config */
    ctrl->config = calloc(1, sizeof(lle_render_config_t));
    if (!ctrl->config) {
        free(ctrl->metrics);
        free(ctrl->dirty_tracker);
        free(ctrl);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    ctrl->config->syntax_highlighting_enabled = true;
    ctrl->config->caching_enabled = false;
    ctrl->config->dirty_tracking_enabled = true;

    *controller = ctrl;
    return LLE_SUCCESS;
}

/**
 * @brief Cleanup render controller
 * 
 * @param controller Render controller to cleanup
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_render_controller_cleanup(lle_render_controller_t *controller)
{
    if (!controller) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    free(controller->config);
    free(controller->metrics);
    free(controller->dirty_tracker);
    free(controller);

    return LLE_SUCCESS;
}

/**
 * @brief Render buffer content to display format
 * 
 * This function takes a buffer and cursor position and produces rendered
 * output ready for the display system. The output is plain text - syntax
 * highlighting is applied by command_layer when we send to display_bridge.
 * 
 * @param controller Render controller instance
 * @param buffer Buffer to render
 * @param cursor Cursor position
 * @param output Output pointer for rendered output
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_render_buffer_content(
    lle_render_controller_t *controller,
    lle_buffer_t *buffer,
    lle_cursor_position_t *cursor,
    lle_render_output_t **output)
{
    (void)cursor;  /* Cursor is in buffer structure */
    
    if (!controller || !buffer || !output) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Allocate render output structure */
    lle_render_output_t *render_out = calloc(1, sizeof(lle_render_output_t));
    if (!render_out) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Get buffer content */
    const char *buffer_content = buffer->data ? buffer->data : "";
    size_t content_len = strlen(buffer_content);

    /* Allocate content buffer (add extra space for safety) */
    render_out->content_capacity = content_len + 256;
    render_out->content = malloc(render_out->content_capacity);
    if (!render_out->content) {
        free(render_out);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Copy buffer content to render output
     * Note: Syntax highlighting is applied later by command_layer
     */
    memcpy(render_out->content, buffer_content, content_len);
    render_out->content[content_len] = '\0';
    render_out->content_length = content_len;

    /* Set timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    render_out->timestamp = (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;

    /* Update metrics */
    if (controller->metrics) {
        controller->metrics->total_renders++;
        if (controller->dirty_tracker && controller->dirty_tracker->full_redraw_needed) {
            controller->metrics->full_renders++;
        } else {
            controller->metrics->partial_renders++;
        }
    }

    *output = render_out;
    return LLE_SUCCESS;
}

/**
 * @brief Free render output
 * 
 * @param output Render output to free
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_render_output_free(lle_render_output_t *output)
{
    if (!output) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    free(output->content);
    free(output->attributes);
    free(output);

    return LLE_SUCCESS;
}

/* ========================================================================== */
/*                      DIRTY TRACKING FUNCTIONS                              */
/* ========================================================================== */

/* NOTE: Dirty tracker functions are implemented in dirty_tracker.c */
