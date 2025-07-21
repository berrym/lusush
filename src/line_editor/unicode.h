#ifndef LLE_UNICODE_H
#define LLE_UNICODE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @file unicode.h
 * @brief Lusush Line Editor - UTF-8 Unicode Support
 *
 * Provides UTF-8 text analysis and navigation functionality for the Lusush Line Editor.
 * Handles proper Unicode character boundary detection, validation, and navigation
 * while maintaining the critical distinction between byte positions and character positions.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

/**
 * @brief UTF-8 character byte length limits
 */
#define LLE_UTF8_MAX_CHAR_BYTES 4
#define LLE_UTF8_MIN_CHAR_BYTES 1

/**
 * @brief UTF-8 byte sequence patterns for validation
 */
#define LLE_UTF8_BYTE_MASK        0x80
#define LLE_UTF8_BYTE_PATTERN     0x00
#define LLE_UTF8_CONTINUATION_MASK    0xC0
#define LLE_UTF8_CONTINUATION_PATTERN 0x80

/**
 * @brief UTF-8 analysis information structure
 *
 * Contains comprehensive information about a UTF-8 string including
 * byte length, character count, and validation status. This structure
 * is essential for proper cursor positioning in UTF-8 text.
 */
typedef struct {
    size_t byte_length;    /**< Length in bytes */
    size_t char_length;    /**< Length in Unicode characters */
    bool valid_utf8;       /**< Valid UTF-8 encoding */
} lle_utf8_info_t;

/**
 * @brief Analyze UTF-8 string and return comprehensive information
 *
 * Performs complete analysis of a UTF-8 string, counting both bytes
 * and Unicode characters while validating proper encoding. This is
 * the primary function for understanding UTF-8 text structure.
 *
 * @param text UTF-8 string to analyze (must be null-terminated)
 * @return lle_utf8_info_t structure with analysis results
 *
 * @note For invalid UTF-8, char_length will be 0 and valid_utf8 will be false
 * @note Handles NULL input gracefully by returning zero-initialized structure
 */
lle_utf8_info_t lle_utf8_analyze(const char *text);

/**
 * @brief Find byte position of character at given character index
 *
 * Converts from character-based indexing to byte-based indexing,
 * which is essential for proper cursor positioning in UTF-8 text.
 * Character indices are 0-based.
 *
 * @param text UTF-8 string to search
 * @param char_index 0-based character index to find
 * @return Byte position of character, or SIZE_MAX if index is out of bounds
 *
 * @note Returns SIZE_MAX for invalid input or out-of-bounds character index
 * @note char_index of 0 returns 0 (beginning of string)
 */
size_t lle_utf8_char_at(const char *text, size_t char_index);

/**
 * @brief Move to next Unicode character from current byte position
 *
 * Advances from the current byte position to the start of the next
 * UTF-8 character. Handles multibyte characters correctly by skipping
 * all continuation bytes.
 *
 * @param text UTF-8 string being navigated
 * @param byte_pos Current byte position in string
 * @return Byte position of next character, or original position if at end
 *
 * @note Returns original byte_pos if already at end of string
 * @note Handles invalid UTF-8 by advancing one byte at a time
 */
size_t lle_utf8_next_char(const char *text, size_t byte_pos);

/**
 * @brief Move to previous Unicode character from current byte position
 *
 * Moves backward from the current byte position to the start of the
 * previous UTF-8 character. Handles multibyte characters correctly
 * by finding the character boundary.
 *
 * @param text UTF-8 string being navigated
 * @param byte_pos Current byte position in string
 * @return Byte position of previous character, or 0 if at beginning
 *
 * @note Returns 0 if byte_pos is 0 (already at beginning)
 * @note Handles invalid UTF-8 by moving back one byte at a time
 */
size_t lle_utf8_prev_char(const char *text, size_t byte_pos);

/**
 * @brief Validate if byte sequence forms valid UTF-8 character
 *
 * Checks if the bytes starting at the given position form a valid
 * UTF-8 character according to the Unicode standard. Used internally
 * for validation and navigation functions.
 *
 * @param text UTF-8 string to validate
 * @param byte_pos Starting byte position
 * @return Number of bytes in the character (1-4), or 0 if invalid
 *
 * @note Returns 0 for invalid UTF-8 sequences
 * @note Does not check string bounds - caller must ensure valid position
 */
size_t lle_utf8_char_bytes(const char *text, size_t byte_pos);

/**
 * @brief Check if byte is UTF-8 continuation byte
 *
 * Determines if a byte is a UTF-8 continuation byte (10xxxxxx pattern).
 * Used for navigation and validation functions.
 *
 * @param byte Byte to check
 * @return true if byte is UTF-8 continuation byte, false otherwise
 */
bool lle_utf8_is_continuation(uint8_t byte);

/**
 * @brief Get character length from first byte of UTF-8 character
 *
 * Determines the expected length of a UTF-8 character by examining
 * the bit pattern of the first byte. Used for validation and parsing.
 *
 * @param first_byte First byte of UTF-8 character
 * @return Expected character length (1-4), or 0 if invalid first byte
 */
size_t lle_utf8_expected_length(uint8_t first_byte);

/**
 * @brief Count characters in UTF-8 string up to byte limit
 *
 * Counts Unicode characters in a UTF-8 string, stopping at either
 * the null terminator or the specified byte limit. Useful for
 * display width calculations.
 *
 * @param text UTF-8 string to count
 * @param max_bytes Maximum bytes to examine
 * @return Number of Unicode characters found
 *
 * @note Stops counting at first invalid UTF-8 sequence
 * @note Returns 0 for NULL input
 */
size_t lle_utf8_count_chars(const char *text, size_t max_bytes);

/**
 * @brief Find byte length of UTF-8 string up to character limit
 *
 * Determines how many bytes are needed to encode the specified number
 * of Unicode characters. Useful for buffer allocation and text truncation.
 *
 * @param text UTF-8 string to measure
 * @param max_chars Maximum characters to include
 * @return Number of bytes needed for max_chars characters
 *
 * @note Returns total string length if max_chars exceeds actual character count
 * @note Returns 0 for NULL input or invalid UTF-8
 */
size_t lle_utf8_bytes_for_chars(const char *text, size_t max_chars);

#endif // LLE_UNICODE_H