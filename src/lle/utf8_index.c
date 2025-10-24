/**
 * @file utf8_index.c
 * @brief UTF-8 Index System Implementation
 * 
 * Spec Reference: Spec 03 Section 4 - UTF-8 Unicode Support
 * 
 * Provides fast O(1) position lookups for UTF-8 buffers by maintaining
 * mapping arrays between byte offsets, codepoint indices, and grapheme
 * cluster indices.
 */

#include "../../include/lle/buffer_management.h"
#include "../../include/lle/error_handling.h"
#include "../../include/lle/memory_management.h"
#include "../../include/lle/performance.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* External UTF-8 helper functions from utf8_support.c */
extern int lle_utf8_sequence_length(unsigned char first_byte);
extern bool lle_utf8_is_valid_sequence(const char *ptr, int length);
extern size_t lle_utf8_count_codepoints(const char *text, size_t length);
extern size_t lle_utf8_count_graphemes(const char *text, size_t length);
extern bool lle_is_grapheme_boundary(const char *ptr, const char *start, const char *end);

/**
 * @brief Get current time in microseconds
 * 
 * @return Current time in microseconds
 */
static uint64_t lle_get_current_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/**
 * @brief Initialize UTF-8 index
 * 
 * @param index Pointer to receive initialized UTF-8 index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_init(lle_utf8_index_t **index) {
    if (!index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate index structure */
    lle_utf8_index_t *idx = calloc(1, sizeof(lle_utf8_index_t));
    if (!idx) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize fields to zero (already done by calloc) */
    idx->index_valid = false;
    idx->buffer_version = 0;
    idx->last_update_time = 0;
    idx->cache_hit_count = 0;
    idx->cache_miss_count = 0;
    
    *index = idx;
    return LLE_SUCCESS;
}

/**
 * @brief Destroy UTF-8 index
 * 
 * @param index UTF-8 index to destroy
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_destroy(lle_utf8_index_t *index) {
    if (!index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free all mapping arrays */
    free(index->byte_to_codepoint);
    free(index->codepoint_to_byte);
    free(index->grapheme_to_codepoint);
    free(index->codepoint_to_grapheme);
    
    /* Free index structure */
    free(index);
    
    return LLE_SUCCESS;
}

/**
 * @brief Rebuild UTF-8 index from text
 * 
 * Spec Reference: Spec 03, Section 4.2
 * 
 * @param index UTF-8 index to rebuild
 * @param text Text to index
 * @param text_length Length of text in bytes
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index,
                                    const char *text,
                                    size_t text_length) {
    if (!index || !text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    size_t codepoint_count = 0;
    size_t grapheme_count = 0;
    
    /* Step 1: Count codepoints and grapheme clusters */
    const char *ptr = text;
    const char *end = text + text_length;
    
    while (ptr < end) {
        /* Validate UTF-8 sequence */
        int sequence_length = lle_utf8_sequence_length((unsigned char)*ptr);
        if (sequence_length == 0 || ptr + sequence_length > end) {
            return LLE_ERROR_INVALID_ENCODING;
        }
        
        /* Validate complete UTF-8 sequence */
        if (!lle_utf8_is_valid_sequence(ptr, sequence_length)) {
            return LLE_ERROR_INVALID_ENCODING;
        }
        
        codepoint_count++;
        
        /* Check if this starts a new grapheme cluster */
        if (lle_is_grapheme_boundary(ptr, text, end)) {
            grapheme_count++;
        }
        
        ptr += sequence_length;
    }
    
    /* Step 2: Allocate index arrays */
    size_t *new_byte_to_codepoint = calloc(text_length + 1, sizeof(size_t));
    size_t *new_codepoint_to_byte = calloc(codepoint_count + 1, sizeof(size_t));
    size_t *new_grapheme_to_codepoint = calloc(grapheme_count + 1, sizeof(size_t));
    size_t *new_codepoint_to_grapheme = calloc(codepoint_count + 1, sizeof(size_t));
    
    if (!new_byte_to_codepoint || !new_codepoint_to_byte ||
        !new_grapheme_to_codepoint || !new_codepoint_to_grapheme) {
        free(new_byte_to_codepoint);
        free(new_codepoint_to_byte);
        free(new_grapheme_to_codepoint);
        free(new_codepoint_to_grapheme);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Step 3: Build index mappings */
    ptr = text;
    size_t byte_pos = 0;
    size_t codepoint_pos = 0;
    size_t grapheme_pos = 0;
    size_t current_grapheme_start = 0;
    
    while (ptr < end) {
        int sequence_length = lle_utf8_sequence_length((unsigned char)*ptr);
        
        /* Update byte-to-codepoint mapping */
        for (int i = 0; i < sequence_length; i++) {
            new_byte_to_codepoint[byte_pos + i] = codepoint_pos;
        }
        
        /* Update codepoint-to-byte mapping */
        new_codepoint_to_byte[codepoint_pos] = byte_pos;
        
        /* Update grapheme cluster mappings */
        if (lle_is_grapheme_boundary(ptr, text, end)) {
            if (grapheme_pos > 0) {
                /* Complete previous grapheme cluster */
                for (size_t i = current_grapheme_start; i < codepoint_pos; i++) {
                    new_codepoint_to_grapheme[i] = grapheme_pos - 1;
                }
            }
            new_grapheme_to_codepoint[grapheme_pos] = codepoint_pos;
            current_grapheme_start = codepoint_pos;
            grapheme_pos++;
        }
        
        codepoint_pos++;
        byte_pos += sequence_length;
        ptr += sequence_length;
    }
    
    /* Step 4: Complete final grapheme cluster */
    if (codepoint_count > 0 && current_grapheme_start < codepoint_count) {
        for (size_t i = current_grapheme_start; i < codepoint_count; i++) {
            new_codepoint_to_grapheme[i] = grapheme_pos - 1;
        }
    }
    
    /* Step 5: Replace old index arrays with new ones */
    free(index->byte_to_codepoint);
    free(index->codepoint_to_byte);
    free(index->grapheme_to_codepoint);
    free(index->codepoint_to_grapheme);
    
    index->byte_to_codepoint = new_byte_to_codepoint;
    index->codepoint_to_byte = new_codepoint_to_byte;
    index->grapheme_to_codepoint = new_grapheme_to_codepoint;
    index->codepoint_to_grapheme = new_codepoint_to_grapheme;
    
    /* Step 6: Update index metadata */
    index->byte_count = text_length;
    index->codepoint_count = codepoint_count;
    index->grapheme_count = grapheme_count;
    index->index_valid = true;
    index->last_update_time = lle_get_current_time_us();
    
    return LLE_SUCCESS;
}

/**
 * @brief Get codepoint index from byte offset
 * 
 * @param index UTF-8 index
 * @param byte_offset Byte offset in text
 * @param codepoint_index Pointer to receive codepoint index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_byte_to_codepoint(const lle_utf8_index_t *index,
                                              size_t byte_offset,
                                              size_t *codepoint_index) {
    if (!index || !codepoint_index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check index validity */
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Check bounds */
    if (byte_offset > index->byte_count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* O(1) lookup */
    *codepoint_index = index->byte_to_codepoint[byte_offset];
    
    return LLE_SUCCESS;
}

/**
 * @brief Get byte offset from codepoint index
 * 
 * @param index UTF-8 index
 * @param codepoint_index Codepoint index
 * @param byte_offset Pointer to receive byte offset
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_codepoint_to_byte(const lle_utf8_index_t *index,
                                              size_t codepoint_index,
                                              size_t *byte_offset) {
    if (!index || !byte_offset) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check index validity */
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Check bounds */
    if (codepoint_index > index->codepoint_count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* O(1) lookup */
    *byte_offset = index->codepoint_to_byte[codepoint_index];
    
    return LLE_SUCCESS;
}

/**
 * @brief Get grapheme index from codepoint index
 * 
 * @param index UTF-8 index
 * @param codepoint_index Codepoint index
 * @param grapheme_index Pointer to receive grapheme index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_codepoint_to_grapheme(const lle_utf8_index_t *index,
                                                  size_t codepoint_index,
                                                  size_t *grapheme_index) {
    if (!index || !grapheme_index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check index validity */
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Check bounds */
    if (codepoint_index > index->codepoint_count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* O(1) lookup */
    *grapheme_index = index->codepoint_to_grapheme[codepoint_index];
    
    return LLE_SUCCESS;
}

/**
 * @brief Get codepoint index from grapheme index
 * 
 * @param index UTF-8 index
 * @param grapheme_index Grapheme cluster index
 * @param codepoint_index Pointer to receive codepoint index
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_grapheme_to_codepoint(const lle_utf8_index_t *index,
                                                  size_t grapheme_index,
                                                  size_t *codepoint_index) {
    if (!index || !codepoint_index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check index validity */
    if (!index->index_valid) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Check bounds */
    if (grapheme_index >= index->grapheme_count) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* O(1) lookup */
    *codepoint_index = index->grapheme_to_codepoint[grapheme_index];
    
    return LLE_SUCCESS;
}

/**
 * @brief Invalidate UTF-8 index
 * 
 * @param index UTF-8 index to invalidate
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_utf8_index_invalidate(lle_utf8_index_t *index) {
    if (!index) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    index->index_valid = false;
    
    return LLE_SUCCESS;
}
