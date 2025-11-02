/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 2 */
/* Multiline Parser: Parse and preserve multiline command structure */

#ifndef LLE_MULTILINE_PARSER_H
#define LLE_MULTILINE_PARSER_H

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/command_structure.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_multiline_parser lle_multiline_parser_t;
typedef struct lle_structure_analyzer lle_structure_analyzer_t;

/**
 * Parsed line information
 */
typedef struct lle_parsed_line {
    /* Line content */
    char *content;
    size_t length;
    
    /* Line metadata */
    size_t line_number;
    size_t indent_level;
    
    /* Whether line ends with continuation */
    bool has_continuation;
    
    /* Keyword information */
    lle_keyword_type_t primary_keyword;
    size_t keyword_count;
    
    /* Next line in list */
    struct lle_parsed_line *next;
} lle_parsed_line_t;

/**
 * Multiline parse result
 */
typedef struct lle_multiline_parse_result {
    /* Parsed lines */
    lle_parsed_line_t *first_line;
    lle_parsed_line_t *last_line;
    size_t line_count;
    
    /* Command structure */
    lle_command_structure_t *structure;
    
    /* Parse metadata */
    bool is_complete;
    bool has_syntax_error;
    size_t total_length;
    
    /* Expected closing keyword (if incomplete) */
    lle_keyword_type_t expected_keyword;
    
    /* Reserved for future use */
    void *reserved[2];
} lle_multiline_parse_result_t;

/**
 * Parser configuration
 */
typedef struct lle_parser_config {
    /* Whether to preserve original indentation */
    bool preserve_indentation;
    
    /* Whether to detect continuation characters */
    bool detect_continuations;
    
    /* Whether to validate syntax */
    bool validate_syntax;
    
    /* Maximum lines to parse (safety limit) */
    size_t max_lines;
    
    /* Reserved for future use */
    void *reserved[4];
} lle_parser_config_t;

/**
 * Create a multiline parser instance
 *
 * @param parser Output parameter for created parser
 * @param memory_pool Memory pool for allocations
 * @param analyzer Structure analyzer for command analysis
 * @param config Configuration (NULL for defaults)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_create(
    lle_multiline_parser_t **parser,
    lle_memory_pool_t *memory_pool,
    lle_structure_analyzer_t *analyzer,
    const lle_parser_config_t *config);

/**
 * Destroy a multiline parser instance
 *
 * @param parser Parser to destroy
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_destroy(
    lle_multiline_parser_t *parser);

/**
 * Parse a multiline command
 *
 * @param parser Multiline parser instance
 * @param command_text Command text to parse
 * @param command_length Length of command text
 * @param result Output parameter for parse result
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_parse(
    lle_multiline_parser_t *parser,
    const char *command_text,
    size_t command_length,
    lle_multiline_parse_result_t **result);

/**
 * Split command into individual lines
 *
 * @param parser Multiline parser instance
 * @param command_text Command text to split
 * @param command_length Length of command text
 * @param lines Output parameter for line array (allocated by function)
 * @param line_count Output parameter for number of lines
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_split_lines(
    lle_multiline_parser_t *parser,
    const char *command_text,
    size_t command_length,
    lle_parsed_line_t ***lines,
    size_t *line_count);

/**
 * Check if a line has a continuation character
 *
 * @param parser Multiline parser instance
 * @param line_text Line text to check
 * @param line_length Length of line text
 * @param has_continuation Output parameter for continuation status
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_check_continuation(
    lle_multiline_parser_t *parser,
    const char *line_text,
    size_t line_length,
    bool *has_continuation);

/**
 * Free a parse result
 *
 * @param parser Multiline parser instance
 * @param result Parse result to free
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_free_result(
    lle_multiline_parser_t *parser,
    lle_multiline_parse_result_t *result);

/**
 * Get default parser configuration
 *
 * @param config Configuration structure to fill with defaults
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_get_default_config(
    lle_parser_config_t *config);

/**
 * Reset parser state for reuse
 *
 * @param parser Multiline parser instance
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_multiline_parser_reset(
    lle_multiline_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif /* LLE_MULTILINE_PARSER_H */
