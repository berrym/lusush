/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 2 */
/* Structure Analyzer: Shell construct detection and keyword extraction */

#ifndef LLE_STRUCTURE_ANALYZER_H
#define LLE_STRUCTURE_ANALYZER_H

#include "lle/command_structure.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_structure_analyzer lle_structure_analyzer_t;

/**
 * Configuration for structure analysis
 */
typedef struct lle_analyzer_config {
    /* Maximum nesting depth to analyze */
    size_t max_nesting_depth;

    /* Whether to detect incomplete constructs */
    bool detect_incomplete;

    /* Whether to perform syntax validation */
    bool validate_syntax;

    /* Whether to track indentation */
    bool track_indentation;

    /* Maximum command length to analyze (safety limit) */
    size_t max_command_length;

    /* Reserved for future use */
    void *reserved[4];
} lle_analyzer_config_t;

/**
 * Analysis context for tracking state during parsing
 */
typedef struct lle_analysis_context {
    /* Current line being analyzed */
    size_t current_line;

    /* Current character offset in command */
    size_t current_offset;

    /* Current nesting depth */
    size_t current_depth;

    /* Whether currently inside a quoted string */
    bool in_quoted_string;

    /* Quote character if in_quoted_string is true */
    char quote_char;

    /* Whether last character was an escape */
    bool last_was_escape;

    /* Whether currently inside a comment */
    bool in_comment;

    /* Current indentation level */
    size_t indent_level;

    /* Reserved for future use */
    void *reserved[4];
} lle_analysis_context_t;

/**
 * Keyword detection result
 */
typedef struct lle_keyword_match {
    /* Type of keyword matched */
    lle_keyword_type_t type;

    /* Start offset in command text */
    size_t start_offset;

    /* Length of keyword */
    size_t length;

    /* Line number where keyword appears */
    size_t line_number;

    /* Whether keyword is at start of command/line */
    bool is_command_start;

    /* Reserved for future use */
    void *reserved[2];
} lle_keyword_match_t;

/**
 * Create a structure analyzer instance
 *
 * @param analyzer Output parameter for created analyzer
 * @param memory_pool Memory pool for allocations
 * @param config Configuration for analyzer (NULL for defaults)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_structure_analyzer_create(lle_structure_analyzer_t **analyzer,
                                           lle_memory_pool_t *memory_pool,
                                           const lle_analyzer_config_t *config);

/**
 * Destroy a structure analyzer instance
 *
 * @param analyzer Analyzer to destroy
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_structure_analyzer_destroy(lle_structure_analyzer_t *analyzer);

/**
 * Analyze a command text and build its structure
 *
 * @param analyzer Structure analyzer instance
 * @param command_text Command text to analyze
 * @param command_length Length of command text
 * @param structure Output parameter for command structure
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_structure_analyzer_analyze(lle_structure_analyzer_t *analyzer,
                               const char *command_text, size_t command_length,
                               lle_command_structure_t **structure);

/**
 * Detect all keywords in a command text
 *
 * This is a lower-level function that just extracts keywords without
 * building the full structure tree.
 *
 * @param analyzer Structure analyzer instance
 * @param command_text Command text to analyze
 * @param command_length Length of command text
 * @param keywords Output array of keyword matches (allocated by function)
 * @param keyword_count Output parameter for number of keywords found
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_structure_analyzer_detect_keywords(
    lle_structure_analyzer_t *analyzer, const char *command_text,
    size_t command_length, lle_keyword_match_t **keywords,
    size_t *keyword_count);

/**
 * Determine the primary construct type of a command
 *
 * @param analyzer Structure analyzer instance
 * @param command_text Command text to analyze
 * @param command_length Length of command text
 * @param construct_type Output parameter for construct type
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_structure_analyzer_detect_type(
    lle_structure_analyzer_t *analyzer, const char *command_text,
    size_t command_length, lle_construct_type_t *construct_type);

/**
 * Check if a command is complete (all constructs properly closed)
 *
 * @param analyzer Structure analyzer instance
 * @param command_text Command text to analyze
 * @param command_length Length of command text
 * @param is_complete Output parameter for completeness status
 * @param missing_keyword Output parameter for missing closing keyword type (if
 * incomplete)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_structure_analyzer_check_complete(lle_structure_analyzer_t *analyzer,
                                      const char *command_text,
                                      size_t command_length, bool *is_complete,
                                      lle_keyword_type_t *missing_keyword);

/**
 * Calculate indentation information for a multiline command
 *
 * @param analyzer Structure analyzer instance
 * @param command_text Command text to analyze
 * @param command_length Length of command text
 * @param indent_info Output parameter for indentation information
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_structure_analyzer_calculate_indentation(
    lle_structure_analyzer_t *analyzer, const char *command_text,
    size_t command_length, lle_indentation_info_t **indent_info);

/**
 * Get default analyzer configuration
 *
 * @param config Configuration structure to fill with defaults
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_structure_analyzer_get_default_config(lle_analyzer_config_t *config);

/**
 * Reset analyzer state for reuse
 *
 * @param analyzer Structure analyzer instance
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_structure_analyzer_reset(lle_structure_analyzer_t *analyzer);

#ifdef __cplusplus
}
#endif

#endif /* LLE_STRUCTURE_ANALYZER_H */
