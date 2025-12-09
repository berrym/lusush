/**
 * @file history_search.c
 * @brief LLE History System - Search Engine Implementation (Spec 09 Phase 3 Day
 * 8)
 *
 * Implements basic search functionality for the LLE history system:
 * - Exact match search
 * - Prefix search
 * - Substring search
 * - Basic fuzzy search (Levenshtein distance)
 * - Result ranking and scoring
 *
 * Performance Targets (Spec 09):
 * - Prefix search: <500μs for 10K entries
 * - Substring search: <5ms for 10K entries
 * - Fuzzy search: <10ms for 10K entries
 *
 * Architecture:
 * - Linear scan algorithms (simple, correct)
 * - Score-based ranking (recency, position, frequency)
 * - Memory pool allocation for results
 * - Integration with history_core for entry access
 *
 * @date 2025-11-01
 * @author LLE Implementation Team
 */

#include "fuzzy_match.h"
#include "lle/error_handling.h"
#include "lle/history.h"
#include "lle/performance.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h> /* for printf */
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* for strncasecmp - must be after other includes */
#include <time.h>

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================
 */

/* Forward declare POSIX function that may not be visible in strict C99 mode */
extern int strncasecmp(const char *s1, const char *s2, size_t n);

#define DEFAULT_MAX_SEARCH_RESULTS 100
#define FUZZY_MAX_DISTANCE 3 /* Maximum Levenshtein distance for fuzzy match   \
                              */

/* Scoring weights */
#define SCORE_EXACT_MATCH 1000
#define SCORE_PREFIX_MATCH 500
#define SCORE_SUBSTRING_MATCH 100
#define SCORE_FUZZY_MATCH 50
#define SCORE_RECENCY_WEIGHT 10  /* Points per position from end */
#define SCORE_POSITION_WEIGHT 5  /* Bonus for match at start */
#define SCORE_FREQUENCY_WEIGHT 2 /* Bonus for frequently used commands */

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================
 */

/* Note: lle_search_type_t and lle_search_result_t are defined in history.h
 * We only define the internal structure for lle_history_search_results_t here
 * (the header has an opaque typedef for this type) */

/**
 * Search results container (internal structure)
 */
struct lle_history_search_results {
    lle_search_result_t *results;  /* Array of results */
    size_t count;                  /* Number of results */
    size_t capacity;               /* Allocated capacity */
    char *query;                   /* Search query (owned copy) */
    lle_search_type_t search_type; /* Search type used */
    uint64_t search_time_us;       /* Search duration in microseconds */
    bool sorted;                   /* Whether results are sorted by score */
};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================
 */

/**
 * Duplicate a string using pool allocation
 */
static char *pool_strdup(const char *str) {
    if (!str)
        return NULL;
    size_t len = strlen(str);
    char *dup = lle_pool_alloc(len + 1);
    if (dup) {
        memcpy(dup, str, len + 1);
    }
    return dup;
}

/* Note: Levenshtein distance now provided by libfuzzy (fuzzy_match.h) */

/**
 * Calculate relevance score for a search result
 *
 * @param command Command string
 * @param query Search query
 * @param match_position Position of match in command
 * @param entry_index Index in history (for recency)
 * @param total_entries Total entries in history
 * @param match_type Type of match
 * @return Relevance score (higher = better)
 */
static int calculate_score(const char *command, const char *query,
                           size_t match_position, size_t entry_index,
                           size_t total_entries, lle_search_type_t match_type) {
    int score = 0;

    /* Base score by match type */
    switch (match_type) {
    case LLE_SEARCH_TYPE_EXACT:
        score += SCORE_EXACT_MATCH;
        break;
    case LLE_SEARCH_TYPE_PREFIX:
        score += SCORE_PREFIX_MATCH;
        break;
    case LLE_SEARCH_TYPE_SUBSTRING:
        score += SCORE_SUBSTRING_MATCH;
        break;
    case LLE_SEARCH_TYPE_FUZZY:
        score += SCORE_FUZZY_MATCH;
        break;
    }

    /* Recency bonus: more recent commands score higher */
    /* entry_index near total_entries = recent */
    size_t recency = total_entries - entry_index;
    score += (int)(recency * SCORE_RECENCY_WEIGHT / 100);

    /* Position bonus: matches earlier in command score higher */
    if (match_position == 0) {
        score += SCORE_POSITION_WEIGHT;
    }

    /* Length bonus: prefer shorter commands (more specific) */
    size_t cmd_len = strlen(command);
    size_t query_len = strlen(query);
    if (cmd_len > 0 && query_len > 0) {
        int length_ratio = (int)((query_len * 100) / cmd_len);
        score += length_ratio; /* Higher ratio = query is larger portion of
                                  command */
    }

    return score;
}

/**
 * Compare function for qsort - sort results by score (descending)
 */
static int compare_results_by_score(const void *a, const void *b) {
    const lle_search_result_t *r1 = (const lle_search_result_t *)a;
    const lle_search_result_t *r2 = (const lle_search_result_t *)b;

    /* Sort by score (descending) */
    if (r1->score > r2->score)
        return -1;
    if (r1->score < r2->score)
        return 1;

    /* Tie-breaker: more recent entry wins */
    if (r1->entry_index > r2->entry_index)
        return -1;
    if (r1->entry_index < r2->entry_index)
        return 1;

    return 0;
}

/**
 * Case-insensitive substring search
 *
 * @param haystack String to search in
 * @param needle String to search for
 * @return Position of match, or NULL if not found
 */
static const char *stristr(const char *haystack, const char *needle) {
    if (!haystack || !needle)
        return NULL;
    if (*needle == '\0')
        return haystack;

    size_t needle_len = strlen(needle);

    for (const char *p = haystack; *p != '\0'; p++) {
        if (strncasecmp(p, needle, needle_len) == 0) {
            return p;
        }
    }

    return NULL;
}

/**
 * Case-insensitive prefix match
 *
 * @param str String to check
 * @param prefix Prefix to match
 * @return true if str starts with prefix (case-insensitive)
 */
static bool str_starts_with_i(const char *str, const char *prefix) {
    if (!str || !prefix)
        return false;
    return strncasecmp(str, prefix, strlen(prefix)) == 0;
}

/* ============================================================================
 * PUBLIC API - SEARCH RESULTS MANAGEMENT
 * ============================================================================
 */

/**
 * Create search results container
 */
lle_history_search_results_t *
lle_history_search_results_create(size_t max_results) {
    if (max_results == 0) {
        max_results = DEFAULT_MAX_SEARCH_RESULTS;
    }

    lle_history_search_results_t *results =
        (lle_history_search_results_t *)lle_pool_alloc(
            sizeof(lle_history_search_results_t));

    if (!results) {
        /* Failed to allocate search results */
        return NULL;
    }

    results->results = (lle_search_result_t *)lle_pool_alloc(
        max_results * sizeof(lle_search_result_t));

    if (!results->results) {
        lle_pool_free(results);
        /* Failed to allocate search results array */
        return NULL;
    }

    results->count = 0;
    results->capacity = max_results;
    results->query = NULL;
    results->search_type = LLE_SEARCH_TYPE_EXACT;
    results->search_time_us = 0;
    results->sorted = false;

    return results;
}

/**
 * Destroy search results and free memory
 */
void lle_history_search_results_destroy(lle_history_search_results_t *results) {
    if (!results)
        return;

    if (results->query) {
        lle_pool_free(results->query);
    }

    if (results->results) {
        lle_pool_free(results->results);
    }

    lle_pool_free(results);
}

/**
 * Add result to search results (if not full)
 */
static bool add_search_result(lle_history_search_results_t *results,
                              uint64_t entry_id, size_t entry_index,
                              const char *command, uint64_t timestamp,
                              int score, size_t match_position,
                              lle_search_type_t match_type) {
    if (!results || results->count >= results->capacity) {
        return false;
    }

    lle_search_result_t *result = &results->results[results->count];
    result->entry_id = entry_id;
    result->entry_index = entry_index;
    result->command = command; /* Reference, not copy */
    result->timestamp = timestamp;
    result->score = score;
    result->match_position = match_position;
    result->match_type = match_type;

    results->count++;
    results->sorted = false; /* Adding invalidates sort */

    return true;
}

/**
 * Sort search results by score (descending)
 */
void lle_history_search_results_sort(lle_history_search_results_t *results) {
    if (!results || results->count == 0 || results->sorted) {
        return;
    }

    qsort(results->results, results->count, sizeof(lle_search_result_t),
          compare_results_by_score);

    results->sorted = true;
}

/* ============================================================================
 * PUBLIC API - SEARCH OPERATIONS
 * ============================================================================
 */

/**
 * Search history for exact command match
 *
 * @param history_core History core engine
 * @param query Search query
 * @param max_results Maximum results to return (0 = default)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t *
lle_history_search_exact(lle_history_core_t *history_core, const char *query,
                         size_t max_results) {
    if (!history_core || !query) {
        return NULL;
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Create results container */
    lle_history_search_results_t *results =
        lle_history_search_results_create(max_results);
    if (!results) {
        return NULL;
    }

    /* Store query */
    results->query = pool_strdup(query);
    results->search_type = LLE_SEARCH_TYPE_EXACT;

    /* Get total entry count */
    size_t total_entries = 0;
    if (lle_history_get_entry_count(history_core, &total_entries) !=
        LLE_SUCCESS) {
        lle_history_search_results_destroy(results);
        return NULL;
    }

    /* Search backward through history (most recent first) */
    for (size_t i = total_entries; i > 0; i--) {
        size_t index = i - 1;

        /* Get entry */
        lle_history_entry_t *entry = NULL;
        if (lle_history_get_entry_by_index(history_core, index, &entry) !=
                LLE_SUCCESS ||
            !entry) {
            continue;
        }
        if (!entry || !entry->command) {
            continue;
        }

        /* Check for exact match */
        if (strcmp(entry->command, query) == 0) {
            int score = calculate_score(
                entry->command, query, 0, /* Exact match = position 0 */
                index, total_entries, LLE_SEARCH_TYPE_EXACT);

            add_search_result(results, entry->entry_id, index, entry->command,
                              entry->timestamp, score, 0,
                              LLE_SEARCH_TYPE_EXACT);

            /* Check if we've hit max results */
            if (results->count >= results->capacity) {
                break;
            }
        }
    }

    /* Sort results by score */
    lle_history_search_results_sort(results);

    /* Record search time */
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    results->search_time_us =
        (uint64_t)((end_time.tv_sec - start_time.tv_sec) * 1000000 +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000);

    return results;
}

/**
 * Search history for commands starting with prefix
 *
 * @param history_core History core engine
 * @param prefix Prefix to search for
 * @param max_results Maximum results to return (0 = default)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t *
lle_history_search_prefix(lle_history_core_t *history_core, const char *prefix,
                          size_t max_results) {
    if (!history_core || !prefix) {
        return NULL;
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Create results container */
    lle_history_search_results_t *results =
        lle_history_search_results_create(max_results);
    if (!results) {
        return NULL;
    }

    /* Store query */
    results->query = pool_strdup(prefix);
    results->search_type = LLE_SEARCH_TYPE_PREFIX;

    /* Get total entry count */
    size_t total_entries = 0;
    if (lle_history_get_entry_count(history_core, &total_entries) !=
        LLE_SUCCESS) {
        lle_history_search_results_destroy(results);
        return NULL;
    }

    /* Search backward through history (most recent first) */
    for (size_t i = total_entries; i > 0; i--) {
        size_t index = i - 1;

        /* Get entry */
        lle_history_entry_t *entry = NULL;
        if (lle_history_get_entry_by_index(history_core, index, &entry) !=
                LLE_SUCCESS ||
            !entry) {
            continue;
        }
        if (!entry->command) {
            continue;
        }

        /* Check for prefix match (case-insensitive) */
        if (str_starts_with_i(entry->command, prefix)) {
            int score = calculate_score(
                entry->command, prefix, 0, /* Prefix match = position 0 */
                index, total_entries, LLE_SEARCH_TYPE_PREFIX);

            add_search_result(results, entry->entry_id, index, entry->command,
                              entry->timestamp, score, 0,
                              LLE_SEARCH_TYPE_PREFIX);

            /* Check if we've hit max results */
            if (results->count >= results->capacity) {
                break;
            }
        }
    }

    /* Sort results by score */
    lle_history_search_results_sort(results);

    /* Record search time */
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    results->search_time_us =
        (uint64_t)((end_time.tv_sec - start_time.tv_sec) * 1000000 +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000);

    return results;
}

/**
 * Search history for commands containing substring
 *
 * @param history_core History core engine
 * @param substring Substring to search for
 * @param max_results Maximum results to return (0 = default)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t *
lle_history_search_substring(lle_history_core_t *history_core,
                             const char *substring, size_t max_results) {
    if (!history_core || !substring) {
        return NULL;
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Create results container */
    lle_history_search_results_t *results =
        lle_history_search_results_create(max_results);
    if (!results) {
        return NULL;
    }

    /* Store query */
    results->query = pool_strdup(substring);
    results->search_type = LLE_SEARCH_TYPE_SUBSTRING;

    /* Get total entry count */
    size_t total_entries = 0;
    if (lle_history_get_entry_count(history_core, &total_entries) !=
        LLE_SUCCESS) {
        lle_history_search_results_destroy(results);
        return NULL;
    }

    /* Search backward through history (most recent first) */
    for (size_t i = total_entries; i > 0; i--) {
        size_t index = i - 1;

        /* Get entry */
        lle_history_entry_t *entry = NULL;
        if (lle_history_get_entry_by_index(history_core, index, &entry) !=
                LLE_SUCCESS ||
            !entry) {
            continue;
        }
        if (!entry->command) {
            continue;
        }

        /* Check for substring match (case-insensitive) */
        const char *match_pos = stristr(entry->command, substring);
        if (match_pos) {
            size_t position = (size_t)(match_pos - entry->command);

            int score =
                calculate_score(entry->command, substring, position, index,
                                total_entries, LLE_SEARCH_TYPE_SUBSTRING);

            add_search_result(results, entry->entry_id, index, entry->command,
                              entry->timestamp, score, position,
                              LLE_SEARCH_TYPE_SUBSTRING);

            /* Check if we've hit max results */
            if (results->count >= results->capacity) {
                break;
            }
        }
    }

    /* Sort results by score */
    lle_history_search_results_sort(results);

    /* Record search time */
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    results->search_time_us =
        (uint64_t)((end_time.tv_sec - start_time.tv_sec) * 1000000 +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000);

    return results;
}

/**
 * Search history for commands with fuzzy matching (Levenshtein distance)
 *
 * @param history_core History core engine
 * @param query Query string
 * @param max_results Maximum results to return (0 = default)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t *
lle_history_search_fuzzy(lle_history_core_t *history_core, const char *query,
                         size_t max_results) {
    if (!history_core || !query) {
        return NULL;
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Create results container */
    lle_history_search_results_t *results =
        lle_history_search_results_create(max_results);
    if (!results) {
        return NULL;
    }

    /* Store query */
    results->query = pool_strdup(query);
    results->search_type = LLE_SEARCH_TYPE_FUZZY;

    /* Get total entry count */
    size_t total_entries = 0;
    if (lle_history_get_entry_count(history_core, &total_entries) !=
        LLE_SUCCESS) {
        lle_history_search_results_destroy(results);
        return NULL;
    }

    /* Search backward through history (most recent first) */
    for (size_t i = total_entries; i > 0; i--) {
        size_t index = i - 1;

        /* Get entry */
        lle_history_entry_t *entry = NULL;
        if (lle_history_get_entry_by_index(history_core, index, &entry) !=
                LLE_SUCCESS ||
            !entry) {
            continue;
        }
        if (!entry->command) {
            continue;
        }

        /* Calculate Levenshtein distance using libfuzzy (Unicode-aware) */
        fuzzy_match_options_t opts = FUZZY_MATCH_DEFAULT;
        opts.case_sensitive = false;
        int distance = fuzzy_levenshtein_distance(entry->command, query, &opts);

        /* Accept if within fuzzy threshold */
        if (distance >= 0 && distance <= FUZZY_MAX_DISTANCE) {
            /* Lower distance = higher score */
            int score = calculate_score(entry->command, query, 0, index,
                                        total_entries, LLE_SEARCH_TYPE_FUZZY);

            /* Adjust score based on distance (closer = better) */
            score -= distance * 50;

            add_search_result(results, entry->entry_id, index, entry->command,
                              entry->timestamp, score, 0,
                              LLE_SEARCH_TYPE_FUZZY);

            /* Check if we've hit max results */
            if (results->count >= results->capacity) {
                break;
            }
        }
    }

    /* Sort results by score */
    lle_history_search_results_sort(results);

    /* Record search time */
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    results->search_time_us =
        (uint64_t)((end_time.tv_sec - start_time.tv_sec) * 1000000 +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000);

    return results;
}

/* ============================================================================
 * PUBLIC API - SEARCH UTILITIES
 * ============================================================================
 */

/**
 * Get number of results in search results
 */
size_t lle_history_search_results_get_count(
    const lle_history_search_results_t *results) {
    return results ? results->count : 0;
}

/**
 * Get specific result from search results
 */
const lle_search_result_t *
lle_history_search_results_get(const lle_history_search_results_t *results,
                               size_t index) {
    if (!results || index >= results->count) {
        return NULL;
    }

    return &results->results[index];
}

/**
 * Get search duration in microseconds
 */
uint64_t lle_history_search_results_get_time_us(
    const lle_history_search_results_t *results) {
    return results ? results->search_time_us : 0;
}

/**
 * Print search results (for debugging)
 */
void lle_history_search_results_print(
    const lle_history_search_results_t *results) {
    if (!results) {
        printf("Search results: NULL\n");
        return;
    }

    printf("Search Results:\n");
    printf("  Query: '%s'\n", results->query ? results->query : "(null)");
    printf("  Type: %s\n",
           results->search_type == LLE_SEARCH_TYPE_EXACT       ? "exact"
           : results->search_type == LLE_SEARCH_TYPE_PREFIX    ? "prefix"
           : results->search_type == LLE_SEARCH_TYPE_SUBSTRING ? "substring"
                                                               : "fuzzy");
    printf("  Count: %zu / %zu\n", results->count, results->capacity);
    printf("  Time: %" PRIu64 " μs\n", results->search_time_us);
    printf("  Results:\n");

    for (size_t i = 0; i < results->count; i++) {
        const lle_search_result_t *r = &results->results[i];
        printf("    [%zu] score=%d pos=%zu cmd=\"%s\"\n", i, r->score,
               r->match_position, r->command);
    }
}
