/**
 * @file history_forensics.c
 * @brief LLE History System - Forensic Tracking Implementation (Spec 09 Phase 4
 * Day 11)
 *
 * Implements forensic-grade metadata tracking for command history:
 * - Process and session tracking (PID, session ID)
 * - User context (UID, GID)
 * - Terminal information
 * - High-precision timing (nanosecond resolution)
 * - Usage analytics (access count, last access time)
 * - Execution duration tracking
 *
 * Forensic data enables:
 * - Command provenance and accountability
 * - Performance analysis and optimization
 * - Usage pattern analytics
 * - Security auditing
 * - Workflow reconstruction
 *
 * @date 2025-11-01
 * @author LLE Implementation Team
 */

#include "lle/error_handling.h"
#include "lle/history.h"
#include "lle/memory_management.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Platform-specific includes for terminal name */
#if defined(__linux__)
#include <linux/tty.h>
#include <sys/ioctl.h>
#endif

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================
 */

/**
 * Get terminal device name
 *
 * @param buffer Output buffer for terminal name
 * @param size Buffer size
 * @return true on success, false on failure
 */
static bool get_terminal_name(char *buffer, size_t size) {
    if (!buffer || size == 0) {
        return false;
    }

    /* Try ttyname() first */
    const char *tty = ttyname(STDIN_FILENO);
    if (tty) {
        strncpy(buffer, tty, size - 1);
        buffer[size - 1] = '\0';
        return true;
    }

    /* Fallback: check if connected to terminal */
    if (isatty(STDIN_FILENO)) {
        strncpy(buffer, "unknown_tty", size - 1);
        buffer[size - 1] = '\0';
        return true;
    }

    /* Not a terminal */
    strncpy(buffer, "not_a_tty", size - 1);
    buffer[size - 1] = '\0';
    return false;
}

/**
 * Duplicate string using memory pool
 */
static char *pool_strdup(const char *str) {
    if (!str)
        return NULL;

    size_t len = strlen(str) + 1;
    char *dup = lle_pool_alloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

/* ============================================================================
 * PUBLIC API - FORENSIC CONTEXT
 * ============================================================================
 */

/**
 * Get high-precision timestamp in nanoseconds
 *
 * Uses CLOCK_MONOTONIC for consistent timing measurements.
 *
 * @return Timestamp in nanoseconds since arbitrary epoch
 */
uint64_t lle_forensic_get_timestamp_ns(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        /* Fallback to lower precision */
        return (uint64_t)time(NULL) * 1000000000ULL;
    }

    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * Capture current forensic context
 *
 * Captures process, user, terminal, and timing information for forensic
 * tracking.
 *
 * @param context Output structure for forensic data
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_forensic_capture_context(lle_forensic_context_t *context) {
    if (!context) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Initialize structure */
    memset(context, 0, sizeof(lle_forensic_context_t));

    /* Capture process information */
    context->process_id = getpid();
    context->session_id = getsid(0);
    context->user_id = getuid();
    context->group_id = getgid();

    /* Capture terminal name */
    char tty_buffer[256];
    if (get_terminal_name(tty_buffer, sizeof(tty_buffer))) {
        context->terminal_name = pool_strdup(tty_buffer);
    } else {
        context->terminal_name = NULL;
    }

    /* Capture working directory */
    char cwd_buffer[4096];
    if (getcwd(cwd_buffer, sizeof(cwd_buffer)) != NULL) {
        context->working_directory = pool_strdup(cwd_buffer);
    } else {
        context->working_directory = NULL;
    }

    /* Capture high-precision timestamp */
    context->timestamp_ns = lle_forensic_get_timestamp_ns();

    return LLE_SUCCESS;
}

/**
 * Apply forensic context to history entry
 *
 * Populates forensic metadata fields in a history entry from context.
 *
 * @param entry History entry to populate
 * @param context Forensic context to apply
 * @return LLE_SUCCESS or error code
 */
lle_result_t
lle_forensic_apply_to_entry(lle_history_entry_t *entry,
                            const lle_forensic_context_t *context) {
    if (!entry || !context) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Apply process information */
    entry->process_id = context->process_id;
    entry->session_id = context->session_id;
    entry->user_id = context->user_id;
    entry->group_id = context->group_id;

    /* Apply terminal name */
    if (context->terminal_name) {
        entry->terminal_name = pool_strdup(context->terminal_name);
    }

    /* Apply working directory (if not already set) */
    if (!entry->working_directory && context->working_directory) {
        entry->working_directory = pool_strdup(context->working_directory);
    }

    /* Initialize timing fields */
    entry->start_time_ns = context->timestamp_ns;
    entry->end_time_ns = 0; /* Will be set by mark_end */
    entry->duration_ms = 0;

    /* Initialize usage tracking */
    entry->usage_count = 0;
    entry->last_access_time =
        context->timestamp_ns / 1000000000ULL; /* Convert to seconds */

    return LLE_SUCCESS;
}

/**
 * Free forensic context resources
 *
 * Frees allocated strings in forensic context.
 *
 * @param context Forensic context to free
 */
void lle_forensic_free_context(lle_forensic_context_t *context) {
    if (!context) {
        return;
    }

    if (context->terminal_name) {
        lle_pool_free(context->terminal_name);
        context->terminal_name = NULL;
    }

    if (context->working_directory) {
        lle_pool_free(context->working_directory);
        context->working_directory = NULL;
    }
}

/* ============================================================================
 * PUBLIC API - TIMING AND DURATION
 * ============================================================================
 */

/**
 * Record command execution start time
 *
 * Captures high-precision start time for duration calculation.
 *
 * @param entry History entry
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_forensic_mark_start(lle_history_entry_t *entry) {
    if (!entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    entry->start_time_ns = lle_forensic_get_timestamp_ns();
    entry->end_time_ns = 0;
    entry->duration_ms = 0;

    return LLE_SUCCESS;
}

/**
 * Record command execution end time and calculate duration
 *
 * Captures end time and calculates execution duration in milliseconds.
 *
 * @param entry History entry
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_forensic_mark_end(lle_history_entry_t *entry) {
    if (!entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Capture end time */
    entry->end_time_ns = lle_forensic_get_timestamp_ns();

    /* Calculate duration in milliseconds */
    if (entry->end_time_ns > entry->start_time_ns) {
        uint64_t duration_ns = entry->end_time_ns - entry->start_time_ns;
        entry->duration_ms = (uint32_t)(duration_ns / 1000000ULL);
    } else {
        /* Clock issue or zero duration */
        entry->duration_ms = 0;
    }

    return LLE_SUCCESS;
}

/* ============================================================================
 * PUBLIC API - USAGE ANALYTICS
 * ============================================================================
 */

/**
 * Increment usage count for history entry
 *
 * Tracks how many times a command has been reused via expansion or recall.
 *
 * @param entry History entry
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_forensic_increment_usage(lle_history_entry_t *entry) {
    if (!entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    entry->usage_count++;

    /* Also update last access time */
    entry->last_access_time = time(NULL);

    return LLE_SUCCESS;
}

/**
 * Update last access time for history entry
 *
 * Records timestamp when entry was last accessed (searched, expanded, etc.).
 *
 * @param entry History entry
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_forensic_update_access_time(lle_history_entry_t *entry) {
    if (!entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    entry->last_access_time = time(NULL);

    return LLE_SUCCESS;
}
