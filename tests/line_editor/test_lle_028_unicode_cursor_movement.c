#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "test_framework.h"
#include "../../src/line_editor/text_buffer.h"

/**
 * @file test_lle_028_unicode_cursor_movement.c
 * @brief Tests for LLE-028: Unicode Cursor Movement
 *
 * Comprehensive test suite for Unicode-aware cursor movement functionality.
 * Tests cursor movement by characters (not bytes), Unicode word boundaries,
 * and proper position calculations with multibyte characters.
 */

// Test data constants
static const char *ASCII_TEXT = "Hello World";
static const char *UTF8_SIMPLE = "Héllo Wørld";
static const char *UTF8_COMPLEX = "🌟 Hello 世界 🚀";
static const char *UTF8_MIXED = "Hello é 世界 test";
static const char *UTF8_CJK = "这是中文测试";
static const char *UTF8_WORDS = "word1 wørd2 单词3 test";

LLE_TEST(unicode_cursor_left_right_ascii) {
    printf("Testing Unicode cursor left/right on ASCII text... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, ASCII_TEXT);
    lle_text_set_cursor(buffer, 5); // Position at " " (space)
    
    // Move right - should advance by 1 character (1 byte for ASCII)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 6); // At "W"
    
    // Move left - should go back by 1 character
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 5); // Back at " "
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_left_right_unicode) {
    printf("Testing Unicode cursor left/right on Unicode text... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // "Héllo" - H(0) é(1-2) l(3) l(4) o(5)
    lle_text_insert_string(buffer, UTF8_SIMPLE);
    lle_text_set_cursor(buffer, 3); // Position at first "l"
    
    // Move left - should go to start of "é" (position 1)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 1); // At start of "é"
    
    // Move right - should go to position 3 (after "é")
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 3); // Back at "l"
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_complex_navigation) {
    printf("Testing Unicode cursor navigation through complex text... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // "🌟 Hello 世界 🚀" - emoji(4) space(1) Hello(5) space(1) 世(3) 界(3) space(1) emoji(4)
    lle_text_insert_string(buffer, UTF8_COMPLEX);
    lle_text_set_cursor(buffer, 0); // Start
    
    // Navigate through each character
    size_t expected_positions[] = {0, 4, 5, 6, 7, 8, 9, 10, 11, 14, 17, 18};
    
    for (size_t i = 1; i < 12; i++) {
        LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
        LLE_ASSERT_EQ(buffer->cursor_pos, expected_positions[i]);
    }
    
    // Navigate back
    for (size_t i = 10; i > 0; i--) {
        LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));
        LLE_ASSERT_EQ(buffer->cursor_pos, expected_positions[i]);
    }
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_home_end) {
    printf("Testing Unicode cursor HOME/END movement... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, UTF8_SIMPLE);
    lle_text_set_cursor(buffer, 5); // Middle of text
    
    // Move to end
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_END));
    LLE_ASSERT_EQ(buffer->cursor_pos, buffer->length);
    
    // Move to home
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_HOME));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_word_boundaries_ascii) {
    printf("Testing Unicode word boundaries on ASCII text... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, "word1 word2 word3");
    lle_text_set_cursor(buffer, 8); // Middle of "word2"
    
    // Move to previous word start
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 6); // Start of "word2"
    
    // Move to next word start
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 12); // Start of "word3"
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_word_boundaries_mixed) {
    printf("Testing Unicode word boundaries on mixed text... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // "word1 wørd2 单词3 test"
    lle_text_insert_string(buffer, UTF8_WORDS);
    lle_text_set_cursor(buffer, 10); // Middle of "wørd2"
    
    // Move to previous word start
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 6); // Start of "wørd2"
    
    // Move to next word start (should skip "单词3")
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT(buffer->cursor_pos > 12); // Should be at "test" or after CJK
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_word_boundaries_cjk) {
    printf("Testing Unicode word boundaries on CJK text... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // "这是中文测试" - each CJK character should be treated as word boundary
    lle_text_insert_string(buffer, UTF8_CJK);
    lle_text_set_cursor(buffer, 3); // At second character
    
    // Move word left - should go to start
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    
    // Move word right - should advance through CJK characters
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT));
    LLE_ASSERT(buffer->cursor_pos > 0);
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_char_position_conversion) {
    printf("Testing Unicode character position conversion... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, UTF8_SIMPLE);
    
    // Test various cursor positions
    lle_text_set_cursor(buffer, 0);
    LLE_ASSERT_EQ(lle_text_get_cursor_char_pos(buffer), 0);
    
    lle_text_set_cursor(buffer, 1); // Start of "é"
    LLE_ASSERT_EQ(lle_text_get_cursor_char_pos(buffer), 1);
    
    lle_text_set_cursor(buffer, 3); // After "é"
    LLE_ASSERT_EQ(lle_text_get_cursor_char_pos(buffer), 2);
    
    // Test setting by character position
    LLE_ASSERT(lle_text_set_cursor_char_pos(buffer, 1));
    LLE_ASSERT_EQ(buffer->cursor_pos, 1); // Start of "é"
    
    LLE_ASSERT(lle_text_set_cursor_char_pos(buffer, 2));
    LLE_ASSERT_EQ(buffer->cursor_pos, 3); // After "é"
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_display_width) {
    printf("Testing Unicode cursor display width calculation... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, UTF8_SIMPLE);
    
    // Test display width at various positions
    lle_text_set_cursor(buffer, 0);
    LLE_ASSERT_EQ(lle_text_get_cursor_display_width(buffer), 0);
    
    lle_text_set_cursor(buffer, 1); // At "é"
    LLE_ASSERT_EQ(lle_text_get_cursor_display_width(buffer), 1);
    
    lle_text_set_cursor(buffer, 3); // After "é"
    LLE_ASSERT_EQ(lle_text_get_cursor_display_width(buffer), 2);
    
    lle_text_set_cursor(buffer, buffer->length); // End
    LLE_ASSERT_EQ(lle_text_get_cursor_display_width(buffer), 11);
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_boundary_conditions) {
    printf("Testing Unicode cursor boundary conditions... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, UTF8_SIMPLE);
    
    // Test movement at boundaries
    lle_text_set_cursor(buffer, 0);
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT)); // Idempotent: already at start
    
    lle_text_set_cursor(buffer, buffer->length);
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT)); // Idempotent: already at end
    
    // Test invalid character positions
    LLE_ASSERT(!lle_text_set_cursor_char_pos(buffer, 100)); // Out of bounds
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_empty_buffer) {
    printf("Testing Unicode cursor on empty buffer... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Empty buffer tests
    LLE_ASSERT_EQ(lle_text_get_cursor_char_pos(buffer), 0);
    LLE_ASSERT_EQ(lle_text_get_cursor_display_width(buffer), 0);
    LLE_ASSERT(lle_text_set_cursor_char_pos(buffer, 0));
    LLE_ASSERT(!lle_text_set_cursor_char_pos(buffer, 1)); // Invalid for empty buffer
    
    // Idempotent movement behavior - succeed when already at target position
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));    // Already at position 0
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));   // Already at position 0 (end of empty)
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_LEFT));  // Already at beginning
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_WORD_RIGHT)); // Already at end
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_emoji_navigation) {
    printf("Testing Unicode cursor navigation through emojis... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, "👋🌟👨‍👩‍👧‍👦");
    lle_text_set_cursor(buffer, 0);
    
    // Navigate through emojis - each should be treated as one character
    size_t initial_pos = buffer->cursor_pos;
    
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
    size_t pos1 = buffer->cursor_pos;
    LLE_ASSERT(pos1 > initial_pos); // Moved forward
    
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_RIGHT));
    size_t pos2 = buffer->cursor_pos;
    LLE_ASSERT(pos2 > pos1); // Moved forward again
    
    // Move back
    LLE_ASSERT(lle_text_move_cursor(buffer, LLE_MOVE_LEFT));
    LLE_ASSERT_EQ(buffer->cursor_pos, pos1);
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_position_consistency) {
    printf("Testing Unicode cursor position consistency... ");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    LLE_ASSERT_NOT_NULL(buffer);
    
    lle_text_insert_string(buffer, UTF8_COMPLEX);
    
    // Test that byte position and character position conversions are consistent
    for (size_t char_pos = 0; char_pos <= 12; char_pos++) {
        if (lle_text_set_cursor_char_pos(buffer, char_pos)) {
            size_t converted_char_pos = lle_text_get_cursor_char_pos(buffer);
            LLE_ASSERT_EQ(converted_char_pos, char_pos);
        }
    }
    
    lle_text_buffer_destroy(buffer);
    printf("PASSED\n");
}

LLE_TEST(unicode_cursor_null_input_safety) {
    printf("Testing Unicode cursor NULL input safety... ");
    
    // Test all functions with NULL input
    LLE_ASSERT_EQ(lle_text_get_cursor_char_pos(NULL), SIZE_MAX);
    LLE_ASSERT_EQ(lle_text_get_cursor_display_width(NULL), 0);
    LLE_ASSERT(!lle_text_set_cursor_char_pos(NULL, 0));
    
    printf("PASSED\n");
}

int main(void) {
    printf("Running LLE-028 Unicode Cursor Movement Tests\n");
    printf("=============================================\n\n");
    
    // Basic cursor movement tests
    test_unicode_cursor_left_right_ascii();
    test_unicode_cursor_left_right_unicode();
    test_unicode_cursor_complex_navigation();
    test_unicode_cursor_home_end();
    
    // Word boundary tests
    test_unicode_word_boundaries_ascii();
    test_unicode_word_boundaries_mixed();
    test_unicode_word_boundaries_cjk();
    
    // Position conversion tests
    test_unicode_cursor_char_position_conversion();
    test_unicode_cursor_display_width();
    
    // Edge case tests
    test_unicode_cursor_boundary_conditions();
    test_unicode_cursor_empty_buffer();
    test_unicode_cursor_emoji_navigation();
    test_unicode_cursor_position_consistency();
    test_unicode_cursor_null_input_safety();
    
    printf("\n=============================================\n");
    printf("All LLE-028 Unicode Cursor Movement tests PASSED!\n");
    printf("Updated functionality:\n");
    printf("- lle_text_move_cursor() - now Unicode character-aware for LEFT/RIGHT\n");
    printf("- Unicode word boundary detection for WORD_LEFT/WORD_RIGHT\n");
    printf("- lle_text_get_cursor_char_pos() - byte to character position conversion\n");
    printf("- lle_text_set_cursor_char_pos() - character to byte position conversion\n");
    printf("- lle_text_get_cursor_display_width() - Unicode display width calculation\n");
    printf("\nTotal tests: 13\n");
    printf("Coverage: ASCII, Unicode, CJK, emojis, word boundaries, position conversion\n");
    printf("Movement types: LEFT/RIGHT (character-aware), WORD_LEFT/WORD_RIGHT (Unicode boundaries)\n");
    
    return 0;
}