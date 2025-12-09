/**
 * @file dirty_tracker.c
 * @brief LLE Dirty Region Tracker Implementation (Layer 1)
 *
 * Implements dirty region tracking for efficient partial rendering updates.
 * Tracks which regions of the display need to be redrawn to avoid full
 * screen refreshes on every change.
 *
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION PLAN: docs/lle_implementation/SPEC_08_IMPLEMENTATION_PLAN.md
 *
 * FEATURES:
 * - Track dirty regions by byte offset
 * - Dynamic region array growth
 * - Full redraw flag for complete updates
 * - Efficient region merging
 * - Memory pool integration
 *
 * Week 5 Day 4-5: Dirty Region Tracking
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stdlib.h>
#include <string.h>

/* ========================================================================== */
/*                      DIRTY TRACKER CONSTANTS                               */
/* ========================================================================== */

#define LLE_DIRTY_REGION_INITIAL_CAPACITY 16 /* Initial region array size */
#define LLE_DIRTY_REGION_GROWTH_FACTOR 2 /* Growth factor for reallocation */
#define LLE_DIRTY_MERGE_THRESHOLD 64     /* Merge regions within 64 bytes */

/* ========================================================================== */
/*                      DIRTY TRACKER IMPLEMENTATION                          */
/* ========================================================================== */

/**
 * @brief Initialize dirty region tracker
 *
 * Creates a dirty region tracker for efficient partial updates.
 *
 * @param tracker Output pointer to receive initialized tracker
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_dirty_tracker_init(lle_dirty_tracker_t **tracker,
                                    lle_memory_pool_t *memory_pool) {
    /* Step 1: Validate parameters */
    if (!tracker || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Step 2: Allocate tracker structure */
    lle_dirty_tracker_t *dt = lle_pool_alloc(sizeof(lle_dirty_tracker_t));
    if (!dt) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(dt, 0, sizeof(lle_dirty_tracker_t));

    /* Step 3: Allocate initial dirty regions array */
    dt->dirty_regions =
        lle_pool_alloc(sizeof(size_t) * LLE_DIRTY_REGION_INITIAL_CAPACITY);
    if (!dt->dirty_regions) {
        lle_pool_free(dt);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Step 4: Initialize state */
    dt->region_count = 0;
    dt->region_capacity = LLE_DIRTY_REGION_INITIAL_CAPACITY;
    dt->full_redraw_needed = true; /* Initial render is always full */

    /* Step 5: Return initialized tracker */
    *tracker = dt;
    return LLE_SUCCESS;
}

/**
 * @brief Clean up dirty region tracker
 *
 * @param tracker Tracker to clean up
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_dirty_tracker_cleanup(lle_dirty_tracker_t *tracker) {
    if (!tracker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Free dirty regions array */
    if (tracker->dirty_regions) {
        lle_pool_free(tracker->dirty_regions);
    }

    /* Free tracker structure */
    lle_pool_free(tracker);

    return LLE_SUCCESS;
}

/**
 * @brief Mark entire display as dirty (full redraw)
 *
 * Sets the full redraw flag to force a complete screen refresh.
 *
 * @param tracker Dirty tracker
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_dirty_tracker_mark_full(lle_dirty_tracker_t *tracker) {
    if (!tracker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    tracker->full_redraw_needed = true;
    tracker->region_count = 0; /* Clear partial regions */

    return LLE_SUCCESS;
}

/**
 * @brief Mark a specific region as dirty
 *
 * Adds a dirty region at the specified byte offset. Regions are automatically
 * merged if they are within the merge threshold.
 *
 * @param tracker Dirty tracker
 * @param offset Byte offset of dirty region
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_dirty_tracker_mark_region(lle_dirty_tracker_t *tracker,
                                           size_t offset) {
    if (!tracker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* If full redraw is needed, don't track individual regions */
    if (tracker->full_redraw_needed) {
        return LLE_SUCCESS;
    }

    /* Check if region is close to existing ones (merge opportunity) */
    for (size_t i = 0; i < tracker->region_count; i++) {
        size_t existing = tracker->dirty_regions[i];
        size_t distance =
            (offset > existing) ? (offset - existing) : (existing - offset);

        if (distance < LLE_DIRTY_MERGE_THRESHOLD) {
            /* Region is close enough, no need to add separately */
            return LLE_SUCCESS;
        }
    }

    /* Need to add new region - check if array needs to grow */
    if (tracker->region_count >= tracker->region_capacity) {
        size_t new_capacity =
            tracker->region_capacity * LLE_DIRTY_REGION_GROWTH_FACTOR;
        size_t *new_regions = lle_pool_alloc(sizeof(size_t) * new_capacity);

        if (!new_regions) {
            /* Out of memory - fall back to full redraw */
            tracker->full_redraw_needed = true;
            tracker->region_count = 0;
            return LLE_SUCCESS; /* Not an error, just degraded mode */
        }

        /* Copy existing regions */
        memcpy(new_regions, tracker->dirty_regions,
               sizeof(size_t) * tracker->region_count);

        /* Free old array and update */
        lle_pool_free(tracker->dirty_regions);
        tracker->dirty_regions = new_regions;
        tracker->region_capacity = new_capacity;
    }

    /* Add new dirty region */
    tracker->dirty_regions[tracker->region_count++] = offset;

    return LLE_SUCCESS;
}

/**
 * @brief Mark a range of bytes as dirty
 *
 * Marks all bytes from start_offset to end_offset as needing update.
 *
 * @param tracker Dirty tracker
 * @param start_offset Start of dirty range
 * @param end_offset End of dirty range
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_dirty_tracker_mark_range(lle_dirty_tracker_t *tracker,
                                          size_t start_offset, size_t length) {
    if (!tracker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (length == 0) {
        return LLE_SUCCESS; /* Nothing to mark */
    }

    /* For large ranges, just mark as full redraw */
    if (length > 1024) { /* Arbitrary threshold */
        return lle_dirty_tracker_mark_full(tracker);
    }

    /* Calculate end offset */
    size_t end_offset = start_offset + length;

    /* Mark start and end of range */
    lle_result_t result = lle_dirty_tracker_mark_region(tracker, start_offset);
    if (result != LLE_SUCCESS) {
        return result;
    }

    result = lle_dirty_tracker_mark_region(tracker, end_offset);
    if (result != LLE_SUCCESS) {
        return result;
    }

    return LLE_SUCCESS;
}

/**
 * @brief Check if full redraw is needed
 *
 * @param tracker Dirty tracker
 * @return true if full redraw is needed, false for partial
 */
bool lle_dirty_tracker_needs_full_redraw(const lle_dirty_tracker_t *tracker) {
    if (!tracker) {
        return true; /* Safe default */
    }

    return tracker->full_redraw_needed;
}

/**
 * @brief Get dirty region count
 *
 * @param tracker Dirty tracker
 * @return Number of dirty regions, or 0 on error
 */
size_t lle_dirty_tracker_get_region_count(const lle_dirty_tracker_t *tracker) {
    if (!tracker) {
        return 0;
    }

    return tracker->region_count;
}

/**
 * @brief Get dirty region at index
 *
 * @param tracker Dirty tracker
 * @param index Region index
 * @param offset Output pointer for region offset
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_dirty_tracker_get_region(const lle_dirty_tracker_t *tracker,
                                          size_t index, size_t *offset) {
    if (!tracker || !offset) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (index >= tracker->region_count) {
        return LLE_ERROR_INVALID_RANGE;
    }

    *offset = tracker->dirty_regions[index];
    return LLE_SUCCESS;
}

/**
 * @brief Clear all dirty regions
 *
 * Resets the tracker after a render, indicating nothing is dirty.
 *
 * @param tracker Dirty tracker
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_dirty_tracker_clear(lle_dirty_tracker_t *tracker) {
    if (!tracker) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    tracker->region_count = 0;
    tracker->full_redraw_needed = false;

    return LLE_SUCCESS;
}

/**
 * @brief Check if a specific region is dirty
 *
 * Checks if the given offset is within any tracked dirty region.
 *
 * @param tracker Dirty tracker
 * @param offset Byte offset to check
 * @return true if region is dirty, false otherwise
 */
bool lle_dirty_tracker_is_region_dirty(const lle_dirty_tracker_t *tracker,
                                       size_t offset) {
    if (!tracker) {
        return true; /* Safe default - assume dirty */
    }

    /* If full redraw, everything is dirty */
    if (tracker->full_redraw_needed) {
        return true;
    }

    /* Check if offset is within merge threshold of any dirty region */
    for (size_t i = 0; i < tracker->region_count; i++) {
        size_t region_offset = tracker->dirty_regions[i];
        size_t distance = (offset > region_offset) ? (offset - region_offset)
                                                   : (region_offset - offset);

        if (distance < LLE_DIRTY_MERGE_THRESHOLD) {
            return true;
        }
    }

    return false;
}
