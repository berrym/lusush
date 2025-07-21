#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "test_framework.h"
#include "../../src/line_editor/unicode.h"

/**
 * @file test_lle_027_utf8_text_handling.c
 * @brief Tests for LLE-027: UTF-8 Text Handling
 *
 * Comprehensive test suite for UTF-8 text analysis and navigation functionality.
 * Tests ASCII text, various Unicode characters, invalid sequences, and edge cases.
 */

// Test data constants
static const char *ASCII_TEXT = "Hello World";
static const char *UTF8_SIMPLE = "Héllo Wørld";
static const char *UTF8_COMPLEX = "🌟 Hello 世界 🚀";
static const char *UTF8_EMOJI = "👨‍👩‍👧‍👦🏳️‍🌈";
static const char *INVALID_UTF8 = "Hello\xFF\xFEWorld";
static const char *PARTIAL_UTF8 = "Hello\xC2";

LLE_TEST(utf8_analyze_ascii) {
    printf("Testing UTF-8 analysis on ASCII text... ");
    
    lle_utf8_info_t info = lle_utf8_analyze(ASCII_TEXT);
    
    LLE_ASSERT(info.valid_utf8);
    LLE_ASSERT_EQ(info.byte_length, 11);
    LLE_ASSERT_EQ(info.char_length, 11);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_analyze_simple_unicode) {
    printf("Testing UTF-8 analysis on simple Unicode text... ");
    
    lle_utf8_info_t info = lle_utf8_analyze(UTF8_SIMPLE);
    
    LLE_ASSERT(info.valid_utf8);
    LLE_ASSERT_EQ(info.byte_length, 13); // "Héllo Wørld" = 13 bytes
    LLE_ASSERT_EQ(info.char_length, 11); // 11 characters
    
    printf("PASSED\n");
}

LLE_TEST(utf8_analyze_complex_unicode) {
    printf("Testing UTF-8 analysis on complex Unicode with emojis... ");
    
    lle_utf8_info_t info = lle_utf8_analyze(UTF8_COMPLEX);
    
    LLE_ASSERT(info.valid_utf8);
    // "🌟 Hello 世界 🚀" has more bytes than characters due to multibyte chars
    LLE_ASSERT(info.byte_length > info.char_length);
    LLE_ASSERT_EQ(info.char_length, 12); // 12 visible characters
    
    printf("PASSED\n");
}

LLE_TEST(utf8_analyze_invalid_sequence) {
    printf("Testing UTF-8 analysis on invalid sequence... ");
    
    lle_utf8_info_t info = lle_utf8_analyze(INVALID_UTF8);
    
    LLE_ASSERT(!info.valid_utf8);
    LLE_ASSERT_EQ(info.char_length, 0); // Invalid sequences have 0 char_length
    
    printf("PASSED\n");
}

LLE_TEST(utf8_analyze_null_input) {
    printf("Testing UTF-8 analysis on NULL input... ");
    
    lle_utf8_info_t info = lle_utf8_analyze(NULL);
    
    LLE_ASSERT(!info.valid_utf8);
    LLE_ASSERT_EQ(info.byte_length, 0);
    LLE_ASSERT_EQ(info.char_length, 0);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_analyze_empty_string) {
    printf("Testing UTF-8 analysis on empty string... ");
    
    lle_utf8_info_t info = lle_utf8_analyze("");
    
    LLE_ASSERT(info.valid_utf8);
    LLE_ASSERT_EQ(info.byte_length, 0);
    LLE_ASSERT_EQ(info.char_length, 0);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_char_at_ascii) {
    printf("Testing UTF-8 char_at on ASCII text... ");
    
    // Test various positions in ASCII text
    LLE_ASSERT_EQ(lle_utf8_char_at(ASCII_TEXT, 0), 0);   // "H"
    LLE_ASSERT_EQ(lle_utf8_char_at(ASCII_TEXT, 1), 1);   // "e"
    LLE_ASSERT_EQ(lle_utf8_char_at(ASCII_TEXT, 5), 5);   // " "
    LLE_ASSERT_EQ(lle_utf8_char_at(ASCII_TEXT, 10), 10); // "d"
    
    // Test out of bounds
    LLE_ASSERT_EQ(lle_utf8_char_at(ASCII_TEXT, 11), SIZE_MAX);
    LLE_ASSERT_EQ(lle_utf8_char_at(ASCII_TEXT, 100), SIZE_MAX);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_char_at_unicode) {
    printf("Testing UTF-8 char_at on Unicode text... ");
    
    // "Héllo Wørld" - H=0, é=1(2 bytes), l=3, l=4, o=5, space=6, W=7, ø=8(2 bytes), r=10, l=11, d=12
    LLE_ASSERT_EQ(lle_utf8_char_at(UTF8_SIMPLE, 0), 0);  // "H"
    LLE_ASSERT_EQ(lle_utf8_char_at(UTF8_SIMPLE, 1), 1);  // "é" (starts at byte 1)
    LLE_ASSERT_EQ(lle_utf8_char_at(UTF8_SIMPLE, 2), 3);  // "l" (starts at byte 3)
    LLE_ASSERT_EQ(lle_utf8_char_at(UTF8_SIMPLE, 7), 8);  // "ø" (starts at byte 8)
    
    printf("PASSED\n");
}

LLE_TEST(utf8_char_at_edge_cases) {
    printf("Testing UTF-8 char_at edge cases... ");
    
    // NULL input
    LLE_ASSERT_EQ(lle_utf8_char_at(NULL, 0), SIZE_MAX);
    
    // SIZE_MAX index
    LLE_ASSERT_EQ(lle_utf8_char_at(ASCII_TEXT, SIZE_MAX), SIZE_MAX);
    
    // Empty string
    LLE_ASSERT_EQ(lle_utf8_char_at("", 0), SIZE_MAX);
    LLE_ASSERT_EQ(lle_utf8_char_at("", 1), SIZE_MAX);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_next_char_ascii) {
    printf("Testing UTF-8 next_char on ASCII text... ");
    
    LLE_ASSERT_EQ(lle_utf8_next_char(ASCII_TEXT, 0), 1);   // H -> e
    LLE_ASSERT_EQ(lle_utf8_next_char(ASCII_TEXT, 1), 2);   // e -> l
    LLE_ASSERT_EQ(lle_utf8_next_char(ASCII_TEXT, 9), 10);  // l -> d
    LLE_ASSERT_EQ(lle_utf8_next_char(ASCII_TEXT, 10), 11); // d -> end
    LLE_ASSERT_EQ(lle_utf8_next_char(ASCII_TEXT, 11), 11); // end -> end
    
    printf("PASSED\n");
}

LLE_TEST(utf8_next_char_unicode) {
    printf("Testing UTF-8 next_char on Unicode text... ");
    
    // "Héllo" - H(0) -> é(1-2) -> l(3) -> l(4) -> o(5)
    LLE_ASSERT_EQ(lle_utf8_next_char(UTF8_SIMPLE, 0), 1);  // H -> é
    LLE_ASSERT_EQ(lle_utf8_next_char(UTF8_SIMPLE, 1), 3);  // é (2 bytes) -> l
    LLE_ASSERT_EQ(lle_utf8_next_char(UTF8_SIMPLE, 3), 4);  // l -> l
    
    printf("PASSED\n");
}

LLE_TEST(utf8_next_char_edge_cases) {
    printf("Testing UTF-8 next_char edge cases... ");
    
    // NULL input
    LLE_ASSERT_EQ(lle_utf8_next_char(NULL, 0), 0);
    
    // Empty string
    LLE_ASSERT_EQ(lle_utf8_next_char("", 0), 0);
    
    // Invalid UTF-8 should advance by 1 byte
    LLE_ASSERT_EQ(lle_utf8_next_char(INVALID_UTF8, 5), 6); // Skip invalid byte
    
    printf("PASSED\n");
}

LLE_TEST(utf8_prev_char_ascii) {
    printf("Testing UTF-8 prev_char on ASCII text... ");
    
    LLE_ASSERT_EQ(lle_utf8_prev_char(ASCII_TEXT, 11), 10); // end -> d
    LLE_ASSERT_EQ(lle_utf8_prev_char(ASCII_TEXT, 10), 9);  // d -> l
    LLE_ASSERT_EQ(lle_utf8_prev_char(ASCII_TEXT, 1), 0);   // e -> H
    LLE_ASSERT_EQ(lle_utf8_prev_char(ASCII_TEXT, 0), 0);   // H -> H (beginning)
    
    printf("PASSED\n");
}

LLE_TEST(utf8_prev_char_unicode) {
    printf("Testing UTF-8 prev_char on Unicode text... ");
    
    // "Héllo" navigation backwards
    LLE_ASSERT_EQ(lle_utf8_prev_char(UTF8_SIMPLE, 5), 4);  // o -> l
    LLE_ASSERT_EQ(lle_utf8_prev_char(UTF8_SIMPLE, 4), 3);  // l -> l  
    LLE_ASSERT_EQ(lle_utf8_prev_char(UTF8_SIMPLE, 3), 1);  // l -> é
    LLE_ASSERT_EQ(lle_utf8_prev_char(UTF8_SIMPLE, 1), 0);  // é -> H
    
    printf("PASSED\n");
}

LLE_TEST(utf8_prev_char_edge_cases) {
    printf("Testing UTF-8 prev_char edge cases... ");
    
    // NULL input
    LLE_ASSERT_EQ(lle_utf8_prev_char(NULL, 5), 0);
    
    // Position 0
    LLE_ASSERT_EQ(lle_utf8_prev_char(ASCII_TEXT, 0), 0);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_char_bytes_validation) {
    printf("Testing UTF-8 char_bytes validation... ");
    
    // ASCII characters
    LLE_ASSERT_EQ(lle_utf8_char_bytes("A", 0), 1);
    LLE_ASSERT_EQ(lle_utf8_char_bytes("Z", 0), 1);
    
    // 2-byte UTF-8 (é = C3 A9)
    LLE_ASSERT_EQ(lle_utf8_char_bytes("é", 0), 2);
    
    // 3-byte UTF-8 (€ = E2 82 AC)
    LLE_ASSERT_EQ(lle_utf8_char_bytes("€", 0), 3);
    
    // NULL input
    LLE_ASSERT_EQ(lle_utf8_char_bytes(NULL, 0), 0);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_is_continuation_byte) {
    printf("Testing UTF-8 continuation byte detection... ");
    
    // Regular ASCII bytes (not continuation)
    LLE_ASSERT(!lle_utf8_is_continuation(0x41)); // 'A'
    LLE_ASSERT(!lle_utf8_is_continuation(0x7F)); // DEL
    
    // UTF-8 start bytes (not continuation)
    LLE_ASSERT(!lle_utf8_is_continuation(0xC2)); // 2-byte start
    LLE_ASSERT(!lle_utf8_is_continuation(0xE2)); // 3-byte start
    LLE_ASSERT(!lle_utf8_is_continuation(0xF0)); // 4-byte start
    
    // UTF-8 continuation bytes (10xxxxxx pattern)
    LLE_ASSERT(lle_utf8_is_continuation(0x80)); // 10000000
    LLE_ASSERT(lle_utf8_is_continuation(0xBF)); // 10111111
    LLE_ASSERT(lle_utf8_is_continuation(0xA9)); // 10101001
    
    printf("PASSED\n");
}

LLE_TEST(utf8_expected_length_validation) {
    printf("Testing UTF-8 expected length calculation... ");
    
    // ASCII (0xxxxxxx)
    LLE_ASSERT_EQ(lle_utf8_expected_length(0x41), 1); // 'A'
    LLE_ASSERT_EQ(lle_utf8_expected_length(0x7F), 1); // DEL
    
    // 2-byte sequence (110xxxxx)
    LLE_ASSERT_EQ(lle_utf8_expected_length(0xC2), 2);
    LLE_ASSERT_EQ(lle_utf8_expected_length(0xDF), 2);
    
    // 3-byte sequence (1110xxxx)
    LLE_ASSERT_EQ(lle_utf8_expected_length(0xE0), 3);
    LLE_ASSERT_EQ(lle_utf8_expected_length(0xEF), 3);
    
    // 4-byte sequence (11110xxx)
    LLE_ASSERT_EQ(lle_utf8_expected_length(0xF0), 4);
    LLE_ASSERT_EQ(lle_utf8_expected_length(0xF7), 4);
    
    // Invalid bytes
    LLE_ASSERT_EQ(lle_utf8_expected_length(0x80), 0); // continuation byte
    LLE_ASSERT_EQ(lle_utf8_expected_length(0xFF), 0); // invalid
    
    printf("PASSED\n");
}

LLE_TEST(utf8_count_chars_functionality) {
    printf("Testing UTF-8 character counting... ");
    
    // ASCII text
    LLE_ASSERT_EQ(lle_utf8_count_chars(ASCII_TEXT, 11), 11);
    LLE_ASSERT_EQ(lle_utf8_count_chars(ASCII_TEXT, 5), 5);
    
    // Unicode text
    LLE_ASSERT_EQ(lle_utf8_count_chars(UTF8_SIMPLE, 13), 11); // Full string
    LLE_ASSERT_EQ(lle_utf8_count_chars(UTF8_SIMPLE, 3), 2);   // "Hé"
    
    // Edge cases
    LLE_ASSERT_EQ(lle_utf8_count_chars(NULL, 10), 0);
    LLE_ASSERT_EQ(lle_utf8_count_chars(ASCII_TEXT, 0), 0);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_bytes_for_chars_functionality) {
    printf("Testing UTF-8 bytes for characters calculation... ");
    
    // ASCII text
    LLE_ASSERT_EQ(lle_utf8_bytes_for_chars(ASCII_TEXT, 5), 5);
    LLE_ASSERT_EQ(lle_utf8_bytes_for_chars(ASCII_TEXT, 11), 11);
    
    // Unicode text - "Héllo" needs more bytes than characters
    LLE_ASSERT_EQ(lle_utf8_bytes_for_chars(UTF8_SIMPLE, 2), 3); // "Hé" = 3 bytes
    LLE_ASSERT_EQ(lle_utf8_bytes_for_chars(UTF8_SIMPLE, 5), 6); // "Héllo" = 6 bytes
    
    // Edge cases
    LLE_ASSERT_EQ(lle_utf8_bytes_for_chars(NULL, 5), 0);
    LLE_ASSERT_EQ(lle_utf8_bytes_for_chars(ASCII_TEXT, 0), 0);
    
    printf("PASSED\n");
}

LLE_TEST(utf8_roundtrip_navigation) {
    printf("Testing UTF-8 roundtrip navigation consistency... ");
    
    const char *test_text = UTF8_SIMPLE;
    lle_utf8_info_t info = lle_utf8_analyze(test_text);
    
    // Forward navigation through all characters
    size_t pos = 0;
    for (size_t i = 0; i < info.char_length && pos < info.byte_length; i++) {
        size_t char_pos = lle_utf8_char_at(test_text, i);
        LLE_ASSERT_EQ(char_pos, pos);
        pos = lle_utf8_next_char(test_text, pos);
    }
    
    // Backward navigation
    pos = info.byte_length;
    for (size_t i = info.char_length; i > 0; i--) {
        pos = lle_utf8_prev_char(test_text, pos);
        size_t expected_pos = lle_utf8_char_at(test_text, i - 1);
        LLE_ASSERT_EQ(pos, expected_pos);
    }
    
    printf("PASSED\n");
}

LLE_TEST(utf8_emoji_handling) {
    printf("Testing UTF-8 emoji handling... ");
    
    const char *emoji_text = "👋🌟";
    lle_utf8_info_t info = lle_utf8_analyze(emoji_text);
    
    LLE_ASSERT(info.valid_utf8);
    LLE_ASSERT_EQ(info.char_length, 2); // Two emoji characters
    LLE_ASSERT(info.byte_length > 2);   // More bytes than characters
    
    // Navigation should work correctly
    size_t first_emoji_end = lle_utf8_next_char(emoji_text, 0);
    size_t second_emoji_start = first_emoji_end;
    LLE_ASSERT_EQ(lle_utf8_char_at(emoji_text, 1), second_emoji_start);
    
    printf("PASSED\n");
}

int main(void) {
    printf("Running LLE-027 UTF-8 Text Handling Tests\n");
    printf("==========================================\n\n");
    
    // UTF-8 analysis tests
    test_utf8_analyze_ascii();
    test_utf8_analyze_simple_unicode();
    test_utf8_analyze_complex_unicode();
    test_utf8_analyze_invalid_sequence();
    test_utf8_analyze_null_input();
    test_utf8_analyze_empty_string();
    
    // Character position tests
    test_utf8_char_at_ascii();
    test_utf8_char_at_unicode();
    test_utf8_char_at_edge_cases();
    
    // Navigation tests
    test_utf8_next_char_ascii();
    test_utf8_next_char_unicode();
    test_utf8_next_char_edge_cases();
    test_utf8_prev_char_ascii();
    test_utf8_prev_char_unicode();
    test_utf8_prev_char_edge_cases();
    
    // Validation tests
    test_utf8_char_bytes_validation();
    test_utf8_is_continuation_byte();
    test_utf8_expected_length_validation();
    
    // Utility function tests
    test_utf8_count_chars_functionality();
    test_utf8_bytes_for_chars_functionality();
    
    // Integration tests
    test_utf8_roundtrip_navigation();
    test_utf8_emoji_handling();
    
    printf("\n==========================================\n");
    printf("All LLE-027 UTF-8 Text Handling tests PASSED!\n");
    printf("Implemented functions:\n");
    printf("- lle_utf8_analyze() - comprehensive UTF-8 string analysis\n");
    printf("- lle_utf8_char_at() - character index to byte position conversion\n");
    printf("- lle_utf8_next_char() - forward character navigation\n");
    printf("- lle_utf8_prev_char() - backward character navigation\n");
    printf("- lle_utf8_char_bytes() - character byte length validation\n");
    printf("- lle_utf8_is_continuation() - continuation byte detection\n");
    printf("- lle_utf8_expected_length() - character length from first byte\n");
    printf("- lle_utf8_count_chars() - character counting with byte limits\n");
    printf("- lle_utf8_bytes_for_chars() - byte calculation for character counts\n");
    printf("\nTotal tests: 22\n");
    printf("Coverage: ASCII, Unicode, emojis, invalid sequences, edge cases\n");
    
    return 0;
}