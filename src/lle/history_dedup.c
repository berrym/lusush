/**
 * history_dedup.c - Intelligent deduplication engine for LLE history system
 *
 * Part of Spec 09: History System (Phase 4 Day 12)
 *
 * Provides intelligent duplicate detection and merging with configurable
 * strategies. Supports frequency tracking, recency-based preservation,
 * and minimal performance impact through efficient algorithms.
 *
 * DEDUPLICATION STRATEGIES:
 * - IGNORE: Ignore all duplicates (simple dedup)
 * - KEEP_RECENT: Keep most recent, discard older
 * - KEEP_FREQUENT: Keep entry with highest usage count
 * - MERGE_METADATA: Merge forensic metadata, keep recent command
 * - KEEP_ALL: No deduplication (track frequency only)
 */

#include "lle/history.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/performance.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

/**
 * Deduplication engine state
 */
struct lle_history_dedup_engine {
    lle_history_dedup_strategy_t strategy;    /* Active strategy */
    lle_history_core_t *history_core;         /* Reference to history core */
    
    /* Statistics */
    uint64_t duplicates_detected;             /* Total duplicates found */
    uint64_t duplicates_merged;               /* Total duplicates merged */
    uint64_t duplicates_ignored;              /* Total duplicates ignored */
    
    /* Performance tracking */
    lle_performance_monitor_t *perf_monitor;  /* Performance monitor */
    
    /* Configuration */
    bool case_sensitive;                      /* Case-sensitive comparison */
    bool trim_whitespace;                     /* Trim whitespace before compare */
    bool merge_forensics;                     /* Merge forensic metadata */
};

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * Normalize command for comparison
 *
 * @param dedup Dedup engine
 * @param command Command string
 * @param normalized Buffer for normalized command
 * @param normalized_size Size of normalized buffer
 * @return LLE_SUCCESS on success, error code on failure
 */
static lle_result_t normalize_command(
    const lle_history_dedup_engine_t *dedup,
    const char *command,
    char *normalized,
    size_t normalized_size)
{
    if (!dedup || !command || !normalized || normalized_size == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    const char *src = command;
    char *dst = normalized;
    size_t remaining = normalized_size - 1;  /* Reserve space for null terminator */
    
    /* Trim leading whitespace if configured */
    if (dedup->trim_whitespace) {
        while (*src && (*src == ' ' || *src == '\t')) {
            src++;
        }
    }
    
    /* Copy and optionally convert case */
    while (*src && remaining > 0) {
        char c = *src++;
        
        /* Convert to lowercase if case-insensitive */
        if (!dedup->case_sensitive && c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
        
        *dst++ = c;
        remaining--;
    }
    
    /* Trim trailing whitespace if configured */
    if (dedup->trim_whitespace) {
        while (dst > normalized && (dst[-1] == ' ' || dst[-1] == '\t')) {
            dst--;
        }
    }
    
    *dst = '\0';
    
    if (*src && remaining == 0) {
        return LLE_ERROR_BUFFER_OVERFLOW;
    }
    
    return LLE_SUCCESS;
}

/**
 * Compare two commands for equality (respecting dedup config)
 *
 * @param dedup Dedup engine
 * @param cmd1 First command
 * @param cmd2 Second command
 * @return true if commands are equal, false otherwise
 */
static bool commands_equal(
    const lle_history_dedup_engine_t *dedup,
    const char *cmd1,
    const char *cmd2)
{
    if (!dedup || !cmd1 || !cmd2) {
        return false;
    }
    
    /* Fast path: pointer equality */
    if (cmd1 == cmd2) {
        return true;
    }
    
    /* Need to normalize for comparison */
    char norm1[LLE_HISTORY_MAX_COMMAND_LENGTH];
    char norm2[LLE_HISTORY_MAX_COMMAND_LENGTH];
    
    if (normalize_command(dedup, cmd1, norm1, sizeof(norm1)) != LLE_SUCCESS) {
        return false;
    }
    
    if (normalize_command(dedup, cmd2, norm2, sizeof(norm2)) != LLE_SUCCESS) {
        return false;
    }
    
    return strcmp(norm1, norm2) == 0;
}

/**
 * Merge forensic metadata from old entry to new entry
 *
 * @param new_entry Entry to update (keep)
 * @param old_entry Entry being merged (will be discarded)
 * @return LLE_SUCCESS on success, error code on failure
 */
static lle_result_t merge_forensic_metadata(
    lle_history_entry_t *new_entry,
    const lle_history_entry_t *old_entry)
{
    if (!new_entry || !old_entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Merge usage counts */
    new_entry->usage_count += old_entry->usage_count;
    
    /* Keep earliest start time (when command was first used) */
    if (old_entry->start_time_ns > 0 && 
        (new_entry->start_time_ns == 0 || old_entry->start_time_ns < new_entry->start_time_ns)) {
        new_entry->start_time_ns = old_entry->start_time_ns;
    }
    
    /* Keep most recent access time */
    if (old_entry->last_access_time > new_entry->last_access_time) {
        new_entry->last_access_time = old_entry->last_access_time;
    }
    
    /* Accumulate total execution time for duration tracking */
    if (old_entry->end_time_ns > 0 && old_entry->start_time_ns > 0) {
        uint64_t old_duration_ns = old_entry->end_time_ns - old_entry->start_time_ns;
        uint64_t new_duration_ns = 0;
        
        if (new_entry->end_time_ns > 0 && new_entry->start_time_ns > 0) {
            new_duration_ns = new_entry->end_time_ns - new_entry->start_time_ns;
        }
        
        /* Store combined duration in duration_ms field (convert to ms) */
        uint64_t total_duration_ms = (old_duration_ns + new_duration_ns) / 1000000;
        if (total_duration_ms > UINT32_MAX) {
            total_duration_ms = UINT32_MAX;
        }
        new_entry->duration_ms = (uint32_t)total_duration_ms;
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * Create deduplication engine
 */
lle_result_t lle_history_dedup_create(
    lle_history_dedup_engine_t **dedup,
    lle_history_core_t *history_core,
    lle_history_dedup_strategy_t strategy)
{
    if (!dedup || !history_core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_history_dedup_engine_t *engine = lle_pool_alloc(sizeof(lle_history_dedup_engine_t));
    if (!engine) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize engine */
    engine->strategy = strategy;
    engine->history_core = history_core;
    engine->duplicates_detected = 0;
    engine->duplicates_merged = 0;
    engine->duplicates_ignored = 0;
    engine->perf_monitor = NULL;
    
    /* Default configuration */
    engine->case_sensitive = true;
    engine->trim_whitespace = true;
    engine->merge_forensics = true;
    
    *dedup = engine;
    return LLE_SUCCESS;
}

/**
 * Destroy deduplication engine
 */
lle_result_t lle_history_dedup_destroy(lle_history_dedup_engine_t *dedup)
{
    if (!dedup) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_pool_free(dedup);
    return LLE_SUCCESS;
}

/**
 * Check if entry is duplicate of existing entry
 */
lle_result_t lle_history_dedup_check(
    lle_history_dedup_engine_t *dedup,
    const lle_history_entry_t *new_entry,
    lle_history_entry_t **duplicate_entry)
{
    if (!dedup || !new_entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (duplicate_entry) {
        *duplicate_entry = NULL;
    }
    
    /* Get history entries (we need to scan backwards from most recent) */
    lle_history_core_t *core = dedup->history_core;
    if (!core) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* For now, we'll do a linear scan of recent entries
     * In production, this would use a hash table for O(1) lookup */
    size_t entry_count = 0;
    lle_result_t result = lle_history_get_entry_count(core, &entry_count);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Scan backwards through recent entries (check last 100 for duplicates) */
    size_t check_limit = (entry_count > 100) ? 100 : entry_count;
    
    for (size_t i = entry_count; i > entry_count - check_limit && i > 0; i--) {
        lle_history_entry_t *entry = NULL;
        result = lle_history_get_entry_by_index(core, i - 1, &entry);
        
        if (result != LLE_SUCCESS || !entry) {
            continue;
        }
        
        /* Skip deleted/archived entries */
        if (entry->state != LLE_HISTORY_STATE_ACTIVE) {
            continue;
        }
        
        /* Compare commands */
        if (commands_equal(dedup, new_entry->command, entry->command)) {
            dedup->duplicates_detected++;
            
            if (duplicate_entry) {
                *duplicate_entry = entry;
            }
            
            return LLE_SUCCESS;  /* Duplicate found */
        }
    }
    
    return LLE_ERROR_NOT_FOUND;  /* Not a duplicate */
}

/**
 * Merge duplicate entry with existing entry
 */
lle_result_t lle_history_dedup_merge(
    lle_history_dedup_engine_t *dedup,
    lle_history_entry_t *keep_entry,
    lle_history_entry_t *discard_entry)
{
    if (!dedup || !keep_entry || !discard_entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Verify they're actually duplicates */
    if (!commands_equal(dedup, keep_entry->command, discard_entry->command)) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Merge forensic metadata if enabled */
    if (dedup->merge_forensics) {
        lle_result_t result = merge_forensic_metadata(keep_entry, discard_entry);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    /* Mark discard entry as deleted */
    discard_entry->state = LLE_HISTORY_STATE_DELETED;
    
    dedup->duplicates_merged++;
    return LLE_SUCCESS;
}

/**
 * Apply deduplication strategy to new entry
 */
lle_result_t lle_history_dedup_apply(
    lle_history_dedup_engine_t *dedup,
    lle_history_entry_t *new_entry,
    bool *entry_rejected)
{
    if (!dedup || !new_entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (entry_rejected) {
        *entry_rejected = false;
    }
    
    /* KEEP_ALL strategy: no deduplication */
    if (dedup->strategy == LLE_DEDUP_KEEP_ALL) {
        return LLE_SUCCESS;
    }
    
    /* Check for duplicates */
    lle_history_entry_t *duplicate = NULL;
    lle_result_t result = lle_history_dedup_check(dedup, new_entry, &duplicate);
    
    if (result == LLE_ERROR_NOT_FOUND) {
        /* Not a duplicate - accept entry */
        return LLE_SUCCESS;
    }
    
    if (result != LLE_SUCCESS) {
        /* Error during check */
        return result;
    }
    
    /* Found duplicate - apply strategy */
    switch (dedup->strategy) {
        case LLE_DEDUP_IGNORE:
            /* Reject new entry, keep old one */
            dedup->duplicates_ignored++;
            if (entry_rejected) {
                *entry_rejected = true;
            }
            return LLE_SUCCESS;
            
        case LLE_DEDUP_KEEP_RECENT:
            /* Keep new entry (more recent), mark old as deleted */
            if (duplicate) {
                result = lle_history_dedup_merge(dedup, new_entry, duplicate);
                if (result != LLE_SUCCESS) {
                    return result;
                }
            }
            return LLE_SUCCESS;
            
        case LLE_DEDUP_KEEP_FREQUENT:
            /* Keep entry with higher usage count */
            if (duplicate && duplicate->usage_count >= new_entry->usage_count) {
                /* Old entry is more frequent - reject new one */
                if (dedup->merge_forensics) {
                    merge_forensic_metadata(duplicate, new_entry);
                }
                dedup->duplicates_ignored++;
                if (entry_rejected) {
                    *entry_rejected = true;
                }
            } else if (duplicate) {
                /* New entry is more frequent - keep it, discard old */
                result = lle_history_dedup_merge(dedup, new_entry, duplicate);
                if (result != LLE_SUCCESS) {
                    return result;
                }
            }
            return LLE_SUCCESS;
            
        case LLE_DEDUP_MERGE_METADATA:
            /* Merge metadata into existing entry, reject new entry */
            if (duplicate) {
                if (dedup->merge_forensics) {
                    merge_forensic_metadata(duplicate, new_entry);
                }
                dedup->duplicates_ignored++;
                if (entry_rejected) {
                    *entry_rejected = true;
                }
            }
            return LLE_SUCCESS;
            
        case LLE_DEDUP_KEEP_ALL:
            /* Already handled above */
            return LLE_SUCCESS;
    }
    
    return LLE_ERROR_INVALID_PARAMETER;  /* Invalid strategy */
}

/**
 * Cleanup old duplicate entries
 */
lle_result_t lle_history_dedup_cleanup(
    lle_history_dedup_engine_t *dedup,
    size_t *entries_removed)
{
    if (!dedup) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (entries_removed) {
        *entries_removed = 0;
    }
    
    lle_history_core_t *core = dedup->history_core;
    if (!core) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Get all entries and remove those marked as deleted */
    size_t entry_count = 0;
    lle_result_t result = lle_history_get_entry_count(core, &entry_count);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    size_t removed = 0;
    
    /* Scan all entries and physically remove deleted ones
     * In production, this would be more efficient with a cleanup list */
    for (size_t i = 0; i < entry_count; i++) {
        lle_history_entry_t *entry = NULL;
        result = lle_history_get_entry_by_index(core, i, &entry);
        
        if (result != LLE_SUCCESS || !entry) {
            continue;
        }
        
        if (entry->state == LLE_HISTORY_STATE_DELETED) {
            /* This entry should be removed - for now just count it
             * Full implementation would call lle_history_remove_entry() */
            removed++;
        }
    }
    
    if (entries_removed) {
        *entries_removed = removed;
    }
    
    return LLE_SUCCESS;
}

/**
 * Set deduplication strategy
 */
lle_result_t lle_history_dedup_set_strategy(
    lle_history_dedup_engine_t *dedup,
    lle_history_dedup_strategy_t strategy)
{
    if (!dedup) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Validate strategy */
    if (strategy < LLE_DEDUP_IGNORE || strategy > LLE_DEDUP_KEEP_ALL) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    dedup->strategy = strategy;
    return LLE_SUCCESS;
}

/**
 * Get deduplication statistics
 */
lle_result_t lle_history_dedup_get_stats(
    const lle_history_dedup_engine_t *dedup,
    lle_history_dedup_stats_t *stats)
{
    if (!dedup || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    stats->duplicates_detected = dedup->duplicates_detected;
    stats->duplicates_merged = dedup->duplicates_merged;
    stats->duplicates_ignored = dedup->duplicates_ignored;
    stats->current_strategy = dedup->strategy;
    
    return LLE_SUCCESS;
}

/**
 * Set deduplication configuration options
 */
lle_result_t lle_history_dedup_configure(
    lle_history_dedup_engine_t *dedup,
    bool case_sensitive,
    bool trim_whitespace,
    bool merge_forensics)
{
    if (!dedup) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    dedup->case_sensitive = case_sensitive;
    dedup->trim_whitespace = trim_whitespace;
    dedup->merge_forensics = merge_forensics;
    
    return LLE_SUCCESS;
}
