/**
 * @file test_fuzzy_match.c
 * @brief Unit tests for fuzzy string matching library
 *
 * Tests the fuzzy matching algorithms including:
 * - Levenshtein distance
 * - Damerau-Levenshtein distance (with transpositions)
 * - Jaro and Jaro-Winkler similarity
 * - Subsequence matching
 * - Combined scoring
 * - Unicode support
 * - Batch operations
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#include "fuzzy_match.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test framework macros */
#define TEST(name) static void test_##name(void)
#define RUN_TEST(name)                                                         \
    do {                                                                       \
        printf("  Running: %s...\n", #name);                                   \
        test_##name();                                                         \
        printf("    PASSED\n");                                                \
    } while (0)

#define ASSERT(condition, message)                                             \
    do {                                                                       \
        if (!(condition)) {                                                    \
            printf("    FAILED: %s\n", message);                               \
            printf("      at %s:%d\n", __FILE__, __LINE__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define ASSERT_EQ(actual, expected, message)                                   \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            printf("    FAILED: %s\n", message);                               \
            printf("      Expected: %d, Got: %d\n", (int)(expected),           \
                   (int)(actual));                                             \
            printf("      at %s:%d\n", __FILE__, __LINE__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define ASSERT_RANGE(actual, min_val, max_val, message)                        \
    do {                                                                       \
        if ((actual) < (min_val) || (actual) > (max_val)) {                    \
            printf("    FAILED: %s\n", message);                               \
            printf("      Expected range: [%d, %d], Got: %d\n",                \
                   (int)(min_val), (int)(max_val), (int)(actual));             \
            printf("      at %s:%d\n", __FILE__, __LINE__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE(condition, message) ASSERT(condition, message)
#define ASSERT_FALSE(condition, message) ASSERT(!(condition), message)

/* ============================================================================
 * LEVENSHTEIN DISTANCE TESTS
 * ============================================================================
 */

TEST(levenshtein_identical_strings) {
    int dist = fuzzy_levenshtein_distance("hello", "hello", NULL);
    ASSERT_EQ(dist, 0, "Identical strings should have distance 0");
}

TEST(levenshtein_empty_strings) {
    int dist1 = fuzzy_levenshtein_distance("", "", NULL);
    ASSERT_EQ(dist1, 0, "Two empty strings should have distance 0");

    int dist2 = fuzzy_levenshtein_distance("hello", "", NULL);
    ASSERT_EQ(dist2, 5, "hello to empty should be 5 deletions");

    int dist3 = fuzzy_levenshtein_distance("", "world", NULL);
    ASSERT_EQ(dist3, 5, "Empty to world should be 5 insertions");
}

TEST(levenshtein_single_edit) {
    /* Single insertion */
    int dist1 = fuzzy_levenshtein_distance("cat", "cats", NULL);
    ASSERT_EQ(dist1, 1, "cat to cats should be 1 insertion");

    /* Single deletion */
    int dist2 = fuzzy_levenshtein_distance("cats", "cat", NULL);
    ASSERT_EQ(dist2, 1, "cats to cat should be 1 deletion");

    /* Single substitution */
    int dist3 = fuzzy_levenshtein_distance("cat", "car", NULL);
    ASSERT_EQ(dist3, 1, "cat to car should be 1 substitution");
}

TEST(levenshtein_multiple_edits) {
    int dist1 = fuzzy_levenshtein_distance("kitten", "sitting", NULL);
    ASSERT_EQ(dist1, 3, "kitten to sitting should be 3 edits");

    int dist2 = fuzzy_levenshtein_distance("hello", "world", NULL);
    ASSERT_EQ(dist2, 4, "hello to world should be 4 edits");
}

TEST(levenshtein_case_insensitive) {
    /* Default is case-insensitive */
    int dist1 = fuzzy_levenshtein_distance("Hello", "hello", NULL);
    ASSERT_EQ(dist1, 0, "Case-insensitive: Hello == hello");

    int dist2 = fuzzy_levenshtein_distance("WORLD", "world", NULL);
    ASSERT_EQ(dist2, 0, "Case-insensitive: WORLD == world");
}

TEST(levenshtein_case_sensitive) {
    int dist = fuzzy_levenshtein_distance("Hello", "hello", &FUZZY_MATCH_STRICT);
    ASSERT_EQ(dist, 1, "Case-sensitive: Hello != hello");
}

/* ============================================================================
 * DAMERAU-LEVENSHTEIN DISTANCE TESTS
 * ============================================================================
 */

TEST(damerau_identical_strings) {
    int dist = fuzzy_damerau_levenshtein_distance("hello", "hello", NULL);
    ASSERT_EQ(dist, 0, "Identical strings should have distance 0");
}

TEST(damerau_transposition) {
    /* Classic typo: "teh" -> "the" is 1 transposition */
    int dist1 = fuzzy_damerau_levenshtein_distance("teh", "the", NULL);
    ASSERT_EQ(dist1, 1, "teh to the should be 1 transposition");

    int dist2 = fuzzy_damerau_levenshtein_distance("recieve", "receive", NULL);
    ASSERT_EQ(dist2, 1, "recieve to receive should be 1 transposition");
}

TEST(damerau_vs_levenshtein) {
    /* Transposition should be 1 edit in Damerau, 2 in Levenshtein */
    int damerau = fuzzy_damerau_levenshtein_distance("ab", "ba", NULL);
    int levenshtein = fuzzy_levenshtein_distance("ab", "ba", NULL);

    ASSERT_EQ(damerau, 1, "Damerau: ab to ba should be 1 transposition");
    ASSERT_EQ(levenshtein, 2, "Levenshtein: ab to ba should be 2 edits");
}

TEST(damerau_multiple_transpositions) {
    int dist = fuzzy_damerau_levenshtein_distance("abcd", "badc", NULL);
    ASSERT_EQ(dist, 2, "Two transpositions: abcd to badc");
}

/* ============================================================================
 * JARO SIMILARITY TESTS
 * ============================================================================
 */

TEST(jaro_identical_strings) {
    int score = fuzzy_jaro_score("hello", "hello", NULL);
    ASSERT_EQ(score, 100, "Identical strings should have Jaro score 100");
}

TEST(jaro_completely_different) {
    int score = fuzzy_jaro_score("abc", "xyz", NULL);
    ASSERT_EQ(score, 0, "Completely different strings should have Jaro score 0");
}

TEST(jaro_partial_match) {
    int score = fuzzy_jaro_score("martha", "marhta", NULL);
    ASSERT_RANGE(score, 90, 100, "martha vs marhta should have high Jaro score");
}

TEST(jaro_empty_strings) {
    int score1 = fuzzy_jaro_score("", "", NULL);
    ASSERT_EQ(score1, 100, "Two empty strings should match perfectly");

    int score2 = fuzzy_jaro_score("hello", "", NULL);
    ASSERT_EQ(score2, 0, "Non-empty vs empty should have score 0");
}

/* ============================================================================
 * JARO-WINKLER SIMILARITY TESTS
 * ============================================================================
 */

TEST(jaro_winkler_identical) {
    int score = fuzzy_jaro_winkler_score("hello", "hello", NULL);
    ASSERT_EQ(score, 100, "Identical strings should have JW score 100");
}

TEST(jaro_winkler_prefix_bonus) {
    /* Jaro-Winkler gives bonus for common prefix */
    int jaro = fuzzy_jaro_score("prefix_aaa", "prefix_bbb", NULL);
    int jw = fuzzy_jaro_winkler_score("prefix_aaa", "prefix_bbb", NULL);

    ASSERT(jw >= jaro, "Jaro-Winkler should be >= Jaro for shared prefixes");
}

TEST(jaro_winkler_common_prefixes) {
    int score = fuzzy_jaro_winkler_score("string", "strong", NULL);
    ASSERT_RANGE(score, 80, 100, "string vs strong should have high JW score");
}

/* ============================================================================
 * COMMON PREFIX TESTS
 * ============================================================================
 */

TEST(common_prefix_identical) {
    int len = fuzzy_common_prefix_length("hello", "hello", NULL);
    ASSERT_EQ(len, 5, "Identical strings should have full prefix match");
}

TEST(common_prefix_partial) {
    int len = fuzzy_common_prefix_length("prefix_one", "prefix_two", NULL);
    ASSERT_EQ(len, 7, "prefix_one and prefix_two share 7 chars");
}

TEST(common_prefix_none) {
    int len = fuzzy_common_prefix_length("abc", "xyz", NULL);
    ASSERT_EQ(len, 0, "No common prefix should return 0");
}

TEST(common_prefix_case_insensitive) {
    int len = fuzzy_common_prefix_length("Hello", "hello", NULL);
    ASSERT_EQ(len, 5, "Case-insensitive prefix should match");
}

TEST(common_prefix_case_sensitive) {
    int len = fuzzy_common_prefix_length("Hello", "hello", &FUZZY_MATCH_STRICT);
    ASSERT_EQ(len, 0, "Case-sensitive: Hello and hello have no common prefix");
}

/* ============================================================================
 * SUBSEQUENCE MATCHING TESTS
 * ============================================================================
 */

TEST(subsequence_identical) {
    ASSERT_TRUE(fuzzy_is_subsequence("hello", "hello", NULL),
                "String is subsequence of itself");
}

TEST(subsequence_simple) {
    ASSERT_TRUE(fuzzy_is_subsequence("gco", "git checkout", NULL),
                "gco is subsequence of git checkout");

    ASSERT_TRUE(fuzzy_is_subsequence("abc", "aXbXcX", NULL),
                "abc is subsequence of aXbXcX");
}

TEST(subsequence_not_found) {
    ASSERT_FALSE(fuzzy_is_subsequence("xyz", "hello", NULL),
                 "xyz is not subsequence of hello");

    ASSERT_FALSE(fuzzy_is_subsequence("cba", "abc", NULL),
                 "cba is not subsequence of abc (order matters)");
}

TEST(subsequence_empty) {
    ASSERT_TRUE(fuzzy_is_subsequence("", "hello", NULL),
                "Empty string is subsequence of any string");

    ASSERT_FALSE(fuzzy_is_subsequence("a", "", NULL),
                 "Non-empty is not subsequence of empty");
}

TEST(subsequence_score) {
    int score1 = fuzzy_subsequence_score("gco", "git checkout", NULL);
    ASSERT_RANGE(score1, 1, 100, "Subsequence score should be positive");

    int score2 = fuzzy_subsequence_score("xyz", "hello", NULL);
    ASSERT_EQ(score2, 0, "Non-subsequence should have score 0");
}

/* ============================================================================
 * COMBINED SCORE TESTS
 * ============================================================================
 */

TEST(combined_score_identical) {
    int score = fuzzy_match_score("hello", "hello", NULL);
    ASSERT_EQ(score, 100, "Identical strings should have score 100");
}

TEST(combined_score_similar) {
    int score = fuzzy_match_score("hello", "helo", NULL);
    ASSERT_RANGE(score, 70, 99, "Similar strings should have high score");
}

TEST(combined_score_different) {
    int score = fuzzy_match_score("hello", "world", NULL);
    ASSERT_RANGE(score, 0, 50, "Different strings should have low score");
}

TEST(combined_score_empty) {
    int score1 = fuzzy_match_score("", "", NULL);
    ASSERT_EQ(score1, 100, "Two empty strings should match perfectly");

    int score2 = fuzzy_match_score("hello", "", NULL);
    ASSERT_EQ(score2, 0, "Non-empty vs empty should have score 0");
}

TEST(combined_is_match) {
    ASSERT_TRUE(fuzzy_match_is_match("hello", "hello", 80, NULL),
                "Identical strings should match at threshold 80");

    ASSERT_TRUE(fuzzy_match_is_match("hello", "helo", 70, NULL),
                "Similar strings should match at threshold 70");

    ASSERT_FALSE(fuzzy_match_is_match("hello", "world", 80, NULL),
                 "Different strings should not match at threshold 80");
}

/* ============================================================================
 * UNICODE SUPPORT TESTS
 * ============================================================================
 */

TEST(unicode_identical) {
    int score = fuzzy_match_score("café", "café", NULL);
    ASSERT_EQ(score, 100, "Identical Unicode strings should match");
}

TEST(unicode_case_folding) {
    /* Latin-1 uppercase to lowercase */
    int dist = fuzzy_levenshtein_distance("ÜBER", "über", NULL);
    ASSERT_EQ(dist, 0, "Unicode case folding should work");
}

TEST(unicode_multibyte) {
    int len = fuzzy_string_length("日本語", NULL);
    ASSERT_EQ(len, 3, "Japanese string should have 3 codepoints");
}

TEST(unicode_emoji) {
    int len = fuzzy_string_length("hello😀world", NULL);
    ASSERT_EQ(len, 11, "String with emoji should count codepoints correctly");
}

TEST(unicode_levenshtein) {
    int dist = fuzzy_levenshtein_distance("naïve", "naive", NULL);
    /* With normalization, this might be 0 or 1 depending on implementation */
    ASSERT_RANGE(dist, 0, 1, "naïve vs naive should be close");
}

/* ============================================================================
 * BATCH OPERATION TESTS
 * ============================================================================
 */

TEST(batch_match_best) {
    const char *candidates[] = {"hello", "help", "world", "helicopter", "held"};
    fuzzy_match_result_t results[3];

    int count = fuzzy_match_best("hel", candidates, 5, results, 3, 0, NULL);

    ASSERT(count >= 1, "Should find at least one match");
    ASSERT(results[0].score >= results[count - 1].score,
           "Results should be sorted by score (highest first)");
}

TEST(batch_match_threshold) {
    const char *candidates[] = {"hello", "world", "helo", "xyz"};
    fuzzy_match_result_t results[4];

    int count = fuzzy_match_best("hello", candidates, 4, results, 4, 80, NULL);

    /* Only "hello" and "helo" should pass threshold 80 */
    ASSERT_RANGE(count, 1, 2, "Should find 1-2 matches above threshold 80");
}

TEST(batch_filter) {
    const char *candidates[] = {"git", "gti", "cat", "grep"};
    int indices[4];

    int count = fuzzy_match_filter("git", candidates, 4, indices, 4, 70, NULL);

    ASSERT(count >= 1, "Should filter at least one match");
}

TEST(batch_empty_pattern) {
    const char *candidates[] = {"hello", "world"};
    fuzzy_match_result_t results[2];

    int count = fuzzy_match_best("", candidates, 2, results, 2, 0, NULL);
    /* Empty pattern behavior depends on implementation */
    ASSERT(count >= 0, "Empty pattern should not crash");
}

/* ============================================================================
 * UTILITY FUNCTION TESTS
 * ============================================================================
 */

TEST(distance_to_score) {
    int score1 = fuzzy_distance_to_score(0, 5);
    ASSERT_EQ(score1, 100, "Distance 0 should give score 100");

    int score2 = fuzzy_distance_to_score(5, 5);
    ASSERT_EQ(score2, 0, "Distance equal to length should give score 0");

    int score3 = fuzzy_distance_to_score(1, 5);
    ASSERT_EQ(score3, 80, "Distance 1 of 5 should give score 80");
}

TEST(string_length) {
    int len1 = fuzzy_string_length("hello", NULL);
    ASSERT_EQ(len1, 5, "ASCII string length");

    int len2 = fuzzy_string_length("", NULL);
    ASSERT_EQ(len2, 0, "Empty string length");
}

/* ============================================================================
 * OPTIONS PRESET TESTS
 * ============================================================================
 */

TEST(options_default) {
    ASSERT_FALSE(FUZZY_MATCH_DEFAULT.case_sensitive,
                 "Default should be case-insensitive");
    ASSERT_TRUE(FUZZY_MATCH_DEFAULT.unicode_normalize,
                "Default should normalize Unicode");
    ASSERT_TRUE(FUZZY_MATCH_DEFAULT.use_damerau,
                "Default should use Damerau-Levenshtein");
}

TEST(options_strict) {
    ASSERT_TRUE(FUZZY_MATCH_STRICT.case_sensitive,
                "Strict should be case-sensitive");
    ASSERT_FALSE(FUZZY_MATCH_STRICT.unicode_normalize,
                 "Strict should not normalize Unicode");
    ASSERT_FALSE(FUZZY_MATCH_STRICT.use_damerau,
                 "Strict should use plain Levenshtein");
}

TEST(options_fast) {
    ASSERT_FALSE(FUZZY_MATCH_FAST.case_sensitive,
                 "Fast should be case-insensitive");
    ASSERT_FALSE(FUZZY_MATCH_FAST.unicode_normalize,
                 "Fast should not normalize Unicode");
    ASSERT_FALSE(FUZZY_MATCH_FAST.use_damerau,
                 "Fast should use plain Levenshtein");
}

/* ============================================================================
 * EDGE CASE TESTS
 * ============================================================================
 */

TEST(edge_single_char) {
    int dist = fuzzy_levenshtein_distance("a", "b", NULL);
    ASSERT_EQ(dist, 1, "Single char difference should be 1");

    int score = fuzzy_match_score("a", "a", NULL);
    ASSERT_EQ(score, 100, "Single identical chars should match");
}

TEST(edge_very_different_lengths) {
    int dist = fuzzy_levenshtein_distance("a", "abcdefghij", NULL);
    ASSERT_EQ(dist, 9, "Very different lengths");

    int score = fuzzy_match_score("a", "abcdefghij", NULL);
    ASSERT_RANGE(score, 0, 50, "Very different lengths should have low score");
}

TEST(edge_repeated_chars) {
    int dist = fuzzy_levenshtein_distance("aaa", "aaaa", NULL);
    ASSERT_EQ(dist, 1, "aaa to aaaa should be 1 insertion");

    int score = fuzzy_match_score("aaa", "aaa", NULL);
    ASSERT_EQ(score, 100, "Identical repeated chars should match");
}

TEST(edge_null_options) {
    /* All functions should handle NULL options gracefully */
    int dist = fuzzy_levenshtein_distance("hello", "world", NULL);
    ASSERT(dist >= 0, "NULL options should use defaults");

    int score = fuzzy_match_score("hello", "world", NULL);
    ASSERT(score >= 0, "NULL options should use defaults");

    bool match = fuzzy_match_is_match("hello", "hello", 50, NULL);
    ASSERT(match, "NULL options should use defaults");
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================
 */

int main(void) {
    printf("\n=== Fuzzy Match Unit Tests ===\n\n");

    /* Levenshtein tests */
    printf("Levenshtein Distance:\n");
    RUN_TEST(levenshtein_identical_strings);
    RUN_TEST(levenshtein_empty_strings);
    RUN_TEST(levenshtein_single_edit);
    RUN_TEST(levenshtein_multiple_edits);
    RUN_TEST(levenshtein_case_insensitive);
    RUN_TEST(levenshtein_case_sensitive);

    /* Damerau-Levenshtein tests */
    printf("\nDamerau-Levenshtein Distance:\n");
    RUN_TEST(damerau_identical_strings);
    RUN_TEST(damerau_transposition);
    RUN_TEST(damerau_vs_levenshtein);
    RUN_TEST(damerau_multiple_transpositions);

    /* Jaro tests */
    printf("\nJaro Similarity:\n");
    RUN_TEST(jaro_identical_strings);
    RUN_TEST(jaro_completely_different);
    RUN_TEST(jaro_partial_match);
    RUN_TEST(jaro_empty_strings);

    /* Jaro-Winkler tests */
    printf("\nJaro-Winkler Similarity:\n");
    RUN_TEST(jaro_winkler_identical);
    RUN_TEST(jaro_winkler_prefix_bonus);
    RUN_TEST(jaro_winkler_common_prefixes);

    /* Common prefix tests */
    printf("\nCommon Prefix:\n");
    RUN_TEST(common_prefix_identical);
    RUN_TEST(common_prefix_partial);
    RUN_TEST(common_prefix_none);
    RUN_TEST(common_prefix_case_insensitive);
    RUN_TEST(common_prefix_case_sensitive);

    /* Subsequence tests */
    printf("\nSubsequence Matching:\n");
    RUN_TEST(subsequence_identical);
    RUN_TEST(subsequence_simple);
    RUN_TEST(subsequence_not_found);
    RUN_TEST(subsequence_empty);
    RUN_TEST(subsequence_score);

    /* Combined score tests */
    printf("\nCombined Scoring:\n");
    RUN_TEST(combined_score_identical);
    RUN_TEST(combined_score_similar);
    RUN_TEST(combined_score_different);
    RUN_TEST(combined_score_empty);
    RUN_TEST(combined_is_match);

    /* Unicode tests */
    printf("\nUnicode Support:\n");
    RUN_TEST(unicode_identical);
    RUN_TEST(unicode_case_folding);
    RUN_TEST(unicode_multibyte);
    RUN_TEST(unicode_emoji);
    RUN_TEST(unicode_levenshtein);

    /* Batch operation tests */
    printf("\nBatch Operations:\n");
    RUN_TEST(batch_match_best);
    RUN_TEST(batch_match_threshold);
    RUN_TEST(batch_filter);
    RUN_TEST(batch_empty_pattern);

    /* Utility function tests */
    printf("\nUtility Functions:\n");
    RUN_TEST(distance_to_score);
    RUN_TEST(string_length);

    /* Options preset tests */
    printf("\nOptions Presets:\n");
    RUN_TEST(options_default);
    RUN_TEST(options_strict);
    RUN_TEST(options_fast);

    /* Edge case tests */
    printf("\nEdge Cases:\n");
    RUN_TEST(edge_single_char);
    RUN_TEST(edge_very_different_lengths);
    RUN_TEST(edge_repeated_chars);
    RUN_TEST(edge_null_options);

    printf("\n=== All %d Fuzzy Match Tests Passed ===\n\n", 47);
    return 0;
}
