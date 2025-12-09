/*
 * terminal_input_processor.c - Input Event Processing (Spec 02 Subsystem 5)
 *
 * Processes input events from Unix terminal interface and validates them
 * before they are used to modify internal state.
 *
 * Key Responsibilities:
 * - Read input events from Unix interface
 * - Validate event data
 * - Track event sequence and timing
 * - Provide performance metrics
 *
 * Spec 02: Terminal Abstraction - Subsystem 5
 */

#include "lle/terminal_abstraction.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INPUT PROCESSOR OPERATIONS
 * ============================================================================
 */

/*
 * Initialize input processor
 */
lle_result_t lle_input_processor_init(lle_input_processor_t **processor,
                                      lle_terminal_capabilities_t *caps,
                                      lle_unix_interface_t *unix_iface) {
    if (!processor || !caps || !unix_iface) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_input_processor_t *proc = calloc(1, sizeof(lle_input_processor_t));
    if (!proc) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    proc->capabilities = caps;
    proc->unix_interface = unix_iface;

    /* Initialize processing state */
    proc->events_processed = 0;
    proc->next_sequence_number = 1;
    proc->total_processing_time_us = 0;

    *processor = proc;
    return LLE_SUCCESS;
}

/*
 * Destroy input processor
 */
void lle_input_processor_destroy(lle_input_processor_t *processor) {
    if (!processor) {
        return;
    }

    free(processor);
}

/*
 * Validate input event data
 */
static bool validate_event(lle_input_event_t *event) {
    if (!event) {
        return false;
    }

    /* Validate based on event type */
    switch (event->type) {
    case LLE_INPUT_TYPE_CHARACTER:
        /* Validate UTF-8 byte count */
        if (event->data.character.byte_count == 0 ||
            event->data.character.byte_count > 8) {
            return false;
        }
        /* Validate codepoint range */
        if (event->data.character.codepoint > 0x10FFFF) {
            return false;
        }
        break;

    case LLE_INPUT_TYPE_SPECIAL_KEY:
        /* Validate key code - allow LLE_KEY_UNKNOWN if it has a valid keycode
         * (for Ctrl+letter) */
        if (event->data.special_key.key == LLE_KEY_UNKNOWN) {
            /* LLE_KEY_UNKNOWN is valid if keycode is set (e.g., Ctrl+A has
             * keycode='A') */
            if (event->data.special_key.keycode == 0) {
                return false; /* Invalid: UNKNOWN key with no keycode */
            }
        }
        break;

    case LLE_INPUT_TYPE_WINDOW_RESIZE:
        /* Validate geometry */
        if (event->data.resize.new_width == 0 ||
            event->data.resize.new_height == 0) {
            return false;
        }
        break;

    case LLE_INPUT_TYPE_SIGNAL:
    case LLE_INPUT_TYPE_TIMEOUT:
    case LLE_INPUT_TYPE_ERROR:
    case LLE_INPUT_TYPE_EOF:
        /* These events are always valid */
        break;

    default:
        return false;
    }

    return true;
}

/*
 * Process input event (validate and track)
 */
lle_result_t lle_input_processor_process_event(lle_input_processor_t *processor,
                                               lle_input_event_t *event) {
    if (!processor || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    uint64_t processing_start = lle_get_current_time_microseconds();

    /* Validate event */
    if (!validate_event(event)) {
        return LLE_ERROR_INVALID_INPUT_EVENT;
    }

    /* Assign sequence number */
    event->sequence_number = processor->next_sequence_number++;

    /* Update processing statistics */
    processor->events_processed++;
    uint64_t processing_time =
        lle_get_current_time_microseconds() - processing_start;
    processor->total_processing_time_us += processing_time;

    return LLE_SUCCESS;
}

/*
 * Read next input event from Unix interface
 */
lle_result_t
lle_input_processor_read_next_event(lle_input_processor_t *processor,
                                    lle_input_event_t **event,
                                    uint32_t timeout_ms) {
    if (!processor || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Allocate event structure */
    lle_input_event_t *new_event = calloc(1, sizeof(lle_input_event_t));
    if (!new_event) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Read event from Unix interface */
    lle_result_t result = lle_unix_interface_read_event(
        processor->unix_interface, new_event, timeout_ms);
    if (result != LLE_SUCCESS) {
        free(new_event);
        return result;
    }

    /* Process (validate and track) the event */
    result = lle_input_processor_process_event(processor, new_event);
    if (result != LLE_SUCCESS) {
        free(new_event);
        return result;
    }

    *event = new_event;
    return LLE_SUCCESS;
}
