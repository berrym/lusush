#ifndef LLE_UTF8_INDEX_H
#define LLE_UTF8_INDEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "error_handling.h"

/**
 * UTF-8 Index Structure
 * 
 * Provides fast bidirectional mapping between:
 * - Byte offsets (storage)
 * - Codepoint indices (logical characters)
 * - Grapheme cluster indices (user-visible characters)
 * - Display columns (visual position)
 * 
 * Based on Spec 03, Section 4.1-4.2
 */
typedef struct lle_utf8_index {
    // Fast position mapping arrays
    size_t *byte_to_codepoint;      // [byte_offset] -> codepoint index
    size_t *codepoint_to_byte;      // [codepoint_idx] -> byte offset
    size_t *grapheme_to_codepoint;  // [grapheme_idx] -> codepoint index (start)
    size_t *codepoint_to_grapheme;  // [codepoint_idx] -> grapheme index
    size_t *grapheme_to_display;    // [grapheme_idx] -> display column
    size_t *display_to_grapheme;    // [display_col] -> grapheme index
    
    // Index metadata
    size_t byte_count;              // Total bytes indexed
    size_t codepoint_count;         // Total codepoints indexed
    size_t grapheme_count;          // Total grapheme clusters indexed
    size_t display_width;           // Total display columns
    
    // Index validity and versioning
    bool index_valid;               // Index is up-to-date
    uint32_t buffer_version;        // Associated buffer version
    uint64_t last_update_time;      // Timestamp of last rebuild
    
    // Performance tracking
    size_t rebuild_count;           // Number of times rebuilt
    uint64_t total_rebuild_time_ns; // Total time spent rebuilding
    
} lle_utf8_index_t;

/**
 * Initialize UTF-8 index structure
 * 
 * @param index Pointer to index structure to initialize
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_init(lle_utf8_index_t *index);

/**
 * Clean up UTF-8 index structure
 * 
 * @param index Pointer to index structure to clean up
 */
void lle_utf8_index_cleanup(lle_utf8_index_t *index);

/**
 * Rebuild UTF-8 index from text
 * 
 * Analyzes text and builds all mapping arrays.
 * This is the core function - see Spec 03 lines 612-763 for algorithm.
 * 
 * @param index Index structure to rebuild
 * @param text Text to index (UTF-8 encoded)
 * @param text_length Length of text in bytes
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index,
                                    const char *text,
                                    size_t text_length);

/**
 * Convert byte offset to codepoint index
 * 
 * @param index UTF-8 index
 * @param byte_offset Byte offset in buffer
 * @param codepoint_index Output: codepoint index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_byte_to_codepoint(const lle_utf8_index_t *index,
                                               size_t byte_offset,
                                               size_t *codepoint_index);

/**
 * Convert codepoint index to byte offset
 * 
 * @param index UTF-8 index
 * @param codepoint_index Codepoint index
 * @param byte_offset Output: byte offset
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_codepoint_to_byte(const lle_utf8_index_t *index,
                                               size_t codepoint_index,
                                               size_t *byte_offset);

/**
 * Convert codepoint index to grapheme cluster index
 * 
 * @param index UTF-8 index
 * @param codepoint_index Codepoint index
 * @param grapheme_index Output: grapheme cluster index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_codepoint_to_grapheme(const lle_utf8_index_t *index,
                                                   size_t codepoint_index,
                                                   size_t *grapheme_index);

/**
 * Convert grapheme cluster index to codepoint index (start of cluster)
 * 
 * @param index UTF-8 index
 * @param grapheme_index Grapheme cluster index
 * @param codepoint_index Output: codepoint index (first codepoint in cluster)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_grapheme_to_codepoint(const lle_utf8_index_t *index,
                                                   size_t grapheme_index,
                                                   size_t *codepoint_index);

/**
 * Convert grapheme cluster index to display column
 * 
 * @param index UTF-8 index
 * @param grapheme_index Grapheme cluster index
 * @param display_column Output: display column
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_grapheme_to_display(const lle_utf8_index_t *index,
                                                 size_t grapheme_index,
                                                 size_t *display_column);

/**
 * Convert display column to grapheme cluster index
 * 
 * @param index UTF-8 index
 * @param display_column Display column
 * @param grapheme_index Output: grapheme cluster index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_display_to_grapheme(const lle_utf8_index_t *index,
                                                 size_t display_column,
                                                 size_t *grapheme_index);

/**
 * Invalidate index (mark as needing rebuild)
 * 
 * @param index Index to invalidate
 */
void lle_utf8_index_invalidate(lle_utf8_index_t *index);

/**
 * Check if index is valid
 * 
 * @param index Index to check
 * @return true if valid, false if needs rebuild
 */
bool lle_utf8_index_is_valid(const lle_utf8_index_t *index);

#endif // LLE_UTF8_INDEX_H
