/*
 * utf8_support.h - Complete UTF-8 Unicode Support for LLE
 *
 * Provides comprehensive UTF-8 encoding/decoding, validation, and
 * manipulation functions for the Line Editing Engine.
 */

#ifndef LLE_UTF8_SUPPORT_H
#define LLE_UTF8_SUPPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Get the length of a UTF-8 sequence from its first byte
 *
 * Parameters:
 *   first_byte - The first byte of a UTF-8 sequence
 *
 * Returns:
 *   Length of the sequence in bytes (1-4), or 0 if invalid
 */
int lle_utf8_sequence_length(unsigned char first_byte);

/*
 * Validate a complete UTF-8 sequence
 *
 * Parameters:
 *   ptr    - Pointer to the UTF-8 sequence
 *   length - Expected length of the sequence
 *
 * Returns:
 *   true if the sequence is valid, false otherwise
 */
bool lle_utf8_is_valid_sequence(const char *ptr, int length);

/*
 * Validate an entire UTF-8 string
 *
 * Parameters:
 *   text   - UTF-8 text to validate
 *   length - Length of text in bytes
 *
 * Returns:
 *   true if all text is valid UTF-8, false otherwise
 */
bool lle_utf8_is_valid(const char *text, size_t length);

/*
 * Decode a UTF-8 sequence to a Unicode codepoint
 *
 * Parameters:
 *   ptr       - Pointer to UTF-8 sequence
 *   max_len   - Maximum bytes to read
 *   codepoint - Output parameter for decoded codepoint
 *
 * Returns:
 *   Number of bytes consumed (1-4), or negative on error
 */
int lle_utf8_decode_codepoint(const char *ptr, size_t max_len,
                              uint32_t *codepoint);

/*
 * Encode a Unicode codepoint to UTF-8
 *
 * Parameters:
 *   codepoint - Unicode codepoint to encode
 *   buffer    - Output buffer (must be at least 4 bytes)
 *
 * Returns:
 *   Number of bytes written (1-4), or 0 on error
 */
int lle_utf8_encode_codepoint(uint32_t codepoint, char *buffer);

/*
 * Count the number of Unicode codepoints in a UTF-8 string
 *
 * Parameters:
 *   text   - UTF-8 text to analyze
 *   length - Length of text in bytes
 *
 * Returns:
 *   Number of codepoints in the string
 */
size_t lle_utf8_count_codepoints(const char *text, size_t length);

/*
 * Convert byte offset to codepoint index
 *
 * Parameters:
 *   text        - UTF-8 text
 *   byte_offset - Byte offset in text
 *   cp_index    - Output parameter for codepoint index
 *
 * Returns:
 *   0 on success, negative on error
 */
int lle_utf8_byte_to_codepoint_index(const char *text, size_t byte_offset,
                                     size_t *cp_index);

/*
 * Convert codepoint index to byte offset
 *
 * Parameters:
 *   text       - UTF-8 text
 *   cp_index   - Codepoint index
 *   byte_offset - Output parameter for byte offset
 *
 * Returns:
 *   0 on success, negative on error
 */
int lle_utf8_codepoint_to_byte_offset(const char *text, size_t cp_index,
                                      size_t *byte_offset);

/*
 * Convert codepoint index to grapheme cluster index
 *
 * Parameters:
 *   text       - UTF-8 text
 *   cp_index   - Codepoint index
 *   grapheme_index - Output parameter for grapheme cluster index
 *
 * Returns:
 *   0 on success, negative on error
 */
int lle_utf8_codepoint_to_grapheme_index(const char *text, size_t cp_index,
                                         size_t *grapheme_index);

/*
 * Get the display width of a Unicode codepoint
 *
 * This function returns the visual width for terminal display,
 * accounting for:
 * - Zero-width characters (combining marks, format characters)
 * - Wide characters (CJK, emoji)
 * - Normal characters
 *
 * Parameters:
 *   codepoint - Unicode codepoint
 *
 * Returns:
 *   Display width (0, 1, or 2)
 */
int lle_utf8_codepoint_width(uint32_t codepoint);

/*
 * Get the display width of a UTF-8 string
 *
 * Parameters:
 *   text   - UTF-8 text
 *   length - Length in bytes
 *
 * Returns:
 *   Total display width
 */
size_t lle_utf8_string_width(const char *text, size_t length);

#endif /* LLE_UTF8_SUPPORT_H */
