/*
 * unicode_compare.h - Unicode-aware string comparison for LLE
 *
 * Provides Unicode-normalized string comparison using NFC (Canonical
 * Composition) normalization. This ensures that equivalent Unicode sequences
 * compare as equal regardless of their byte representation.
 *
 * Example: "é" (U+00E9 precomposed) == "e" + "́" (U+0065 + U+0301 decomposed)
 *
 * For shell commands, most input is ASCII, but this handles edge cases where
 * users type accented characters, emoji, or other Unicode that may have
 * multiple valid representations.
 */

#ifndef LLE_UNICODE_COMPARE_H
#define LLE_UNICODE_COMPARE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Unicode comparison options
 */
typedef struct lle_unicode_compare_options {
    bool normalize;        /* Apply NFC normalization before comparison */
    bool case_insensitive; /* Case-insensitive comparison */
    bool trim_whitespace;  /* Trim leading/trailing whitespace */
} lle_unicode_compare_options_t;

/*
 * Default comparison options (normalize only)
 */
extern const lle_unicode_compare_options_t LLE_UNICODE_COMPARE_DEFAULT;

/*
 * Strict byte comparison (no normalization, same as strcmp)
 */
extern const lle_unicode_compare_options_t LLE_UNICODE_COMPARE_STRICT;

/*
 * Compare two UTF-8 strings with Unicode normalization
 *
 * This function compares strings using NFC normalization to ensure
 * equivalent Unicode sequences compare as equal. It handles:
 * - Precomposed vs decomposed characters (é vs e + combining acute)
 * - Different byte representations of the same codepoint
 * - Optional case-insensitive comparison
 * - Optional whitespace trimming
 *
 * Parameters:
 *   str1    - First UTF-8 string
 *   str2    - Second UTF-8 string
 *   options - Comparison options (NULL for defaults)
 *
 * Returns:
 *   true if strings are equal after normalization, false otherwise
 */
bool lle_unicode_strings_equal(const char *str1, const char *str2,
                               const lle_unicode_compare_options_t *options);

/*
 * Compare two UTF-8 strings with length limits
 *
 * Like lle_unicode_strings_equal but with explicit length parameters.
 * Useful when strings may not be null-terminated.
 *
 * Parameters:
 *   str1    - First UTF-8 string
 *   len1    - Length of first string in bytes
 *   str2    - Second UTF-8 string
 *   len2    - Length of second string in bytes
 *   options - Comparison options (NULL for defaults)
 *
 * Returns:
 *   true if strings are equal after normalization, false otherwise
 */
bool lle_unicode_strings_equal_n(const char *str1, size_t len1,
                                 const char *str2, size_t len2,
                                 const lle_unicode_compare_options_t *options);

/*
 * Normalize a UTF-8 string to NFC form
 *
 * Converts a UTF-8 string to Normalization Form C (Canonical Composition).
 * This is a two-step process:
 * 1. Canonical decomposition (NFD)
 * 2. Canonical composition
 *
 * Parameters:
 *   input       - Input UTF-8 string
 *   input_len   - Length of input in bytes
 *   output      - Output buffer for normalized string
 *   output_size - Size of output buffer
 *   output_len  - Receives actual length of normalized string
 *
 * Returns:
 *   0 on success, negative on error:
 *   -1: Invalid parameter
 *   -2: Output buffer too small
 *   -3: Invalid UTF-8 input
 */
int lle_unicode_normalize_nfc(const char *input, size_t input_len, char *output,
                              size_t output_size, size_t *output_len);

/*
 * Check if a codepoint is a combining character
 *
 * Parameters:
 *   codepoint - Unicode codepoint to check
 *
 * Returns:
 *   true if the codepoint is a combining character (Mn, Mc, Me categories)
 */
bool lle_unicode_is_combining(uint32_t codepoint);

/*
 * Get the canonical combining class of a codepoint
 *
 * The combining class is used for canonical ordering of combining marks
 * during normalization. Class 0 means the character is a starter (base).
 *
 * Parameters:
 *   codepoint - Unicode codepoint
 *
 * Returns:
 *   Canonical combining class (0-254)
 */
uint8_t lle_unicode_combining_class(uint32_t codepoint);

/*
 * Get the canonical decomposition of a codepoint
 *
 * Returns the canonical decomposition mapping for precomposed characters.
 * For example, U+00E9 (é) decomposes to U+0065 U+0301 (e + combining acute).
 *
 * Parameters:
 *   codepoint   - Unicode codepoint to decompose
 *   decomposed  - Output buffer for decomposed codepoints
 *   max_length  - Maximum number of codepoints to output
 *
 * Returns:
 *   Number of codepoints in decomposition (0 if no decomposition)
 */
int lle_unicode_decompose(uint32_t codepoint, uint32_t *decomposed,
                          int max_length);

/*
 * Compose two codepoints into a precomposed form
 *
 * Attempts to compose a base character with a combining character
 * into a single precomposed codepoint.
 *
 * Parameters:
 *   base     - Base character
 *   combining - Combining character
 *   composed - Receives composed character if successful
 *
 * Returns:
 *   true if composition successful, false if no composition exists
 */
bool lle_unicode_compose(uint32_t base, uint32_t combining, uint32_t *composed);

/*
 * Check if one UTF-8 string is a prefix of another (Unicode-aware)
 *
 * This function performs Unicode-normalized prefix matching using NFC
 * normalization. It correctly handles:
 * - Precomposed vs decomposed characters (café vs cafe + combining acute)
 * - Different byte representations of equivalent Unicode sequences
 * - Grapheme cluster boundaries (won't match partial graphemes)
 *
 * For shell autosuggestions, this ensures that typing "café" will match
 * history entry "café_script.sh" regardless of how either was encoded.
 *
 * Parameters:
 *   prefix     - The prefix string to check (UTF-8)
 *   prefix_len - Length of prefix in bytes
 *   str        - The string to check against (UTF-8)
 *   str_len    - Length of string in bytes
 *   options    - Comparison options (NULL for defaults with normalization)
 *
 * Returns:
 *   true if prefix is a Unicode-normalized prefix of str, false otherwise
 */
bool lle_unicode_is_prefix(const char *prefix, size_t prefix_len,
                           const char *str, size_t str_len,
                           const lle_unicode_compare_options_t *options);

/*
 * Check if one null-terminated UTF-8 string is a prefix of another
 *
 * Convenience wrapper around lle_unicode_is_prefix() for null-terminated
 * strings.
 *
 * Parameters:
 *   prefix  - The prefix string to check (UTF-8, null-terminated)
 *   str     - The string to check against (UTF-8, null-terminated)
 *   options - Comparison options (NULL for defaults with normalization)
 *
 * Returns:
 *   true if prefix is a Unicode-normalized prefix of str, false otherwise
 */
bool lle_unicode_is_prefix_z(const char *prefix, const char *str,
                             const lle_unicode_compare_options_t *options);

#endif /* LLE_UNICODE_COMPARE_H */
