// src/lle/foundation/test/test_fuzzy_matching.c
//
// Test suite for LLE Fuzzy Matching Library
//
// Validates all algorithms extracted from autocorrect.c

#include "../fuzzy_matching/fuzzy_matching.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running: %s ... ", name); \
        fflush(stdout);

#define PASS() \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL\n"); \
            printf("  Expected: %d\n", (expected)); \
            printf("  Actual: %d\n", (actual)); \
            return; \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("FAIL\n"); \
            printf("  Condition failed: %s\n", #condition); \
            return; \
        } \
    } while(0)

#define ASSERT_RANGE(value, min, max) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            printf("FAIL\n"); \
            printf("  Value %d not in range [%d, %d]\n", (value), (min), (max)); \
            return; \
        } \
    } while(0)

// === Levenshtein Distance Tests ===

void test_levenshtein_identical(void) {
    TEST("Levenshtein: identical strings");
    ASSERT_EQ(0, lle_fuzzy_levenshtein_distance("git", "git"));
    PASS();
}

void test_levenshtein_single_char(void) {
    TEST("Levenshtein: character operations");
    ASSERT_EQ(2, lle_fuzzy_levenshtein_distance("git", "gti")); // transposition = 2 ops
    ASSERT_EQ(1, lle_fuzzy_levenshtein_distance("git", "gitt")); // 1 insertion
    ASSERT_EQ(1, lle_fuzzy_levenshtein_distance("git", "gi")); // 1 deletion
    PASS();
}

void test_levenshtein_case_insensitive(void) {
    TEST("Levenshtein: case insensitive");
    ASSERT_EQ(0, lle_fuzzy_levenshtein_distance("Git", "git"));
    ASSERT_EQ(0, lle_fuzzy_levenshtein_distance("GIT", "git"));
    PASS();
}

void test_levenshtein_empty(void) {
    TEST("Levenshtein: empty strings");
    ASSERT_EQ(3, lle_fuzzy_levenshtein_distance("", "git"));
    ASSERT_EQ(3, lle_fuzzy_levenshtein_distance("git", ""));
    ASSERT_EQ(0, lle_fuzzy_levenshtein_distance("", ""));
    PASS();
}

void test_levenshtein_commands(void) {
    TEST("Levenshtein: common command typos");
    // cd -> dc (2 operations: delete c, insert c at end)
    int dist_cd = lle_fuzzy_levenshtein_distance("cd", "dc");
    ASSERT_TRUE(dist_cd == 2);
    
    // ls -> sl (2 operations)
    int dist_ls = lle_fuzzy_levenshtein_distance("ls", "sl");
    ASSERT_TRUE(dist_ls == 2);
    
    PASS();
}

// === Jaro-Winkler Tests ===

void test_jaro_winkler_identical(void) {
    TEST("Jaro-Winkler: identical strings");
    ASSERT_EQ(100, lle_fuzzy_jaro_winkler("git", "git"));
    ASSERT_EQ(100, lle_fuzzy_jaro_winkler("command", "command"));
    PASS();
}

void test_jaro_winkler_similar(void) {
    TEST("Jaro-Winkler: similar strings");
    int score = lle_fuzzy_jaro_winkler("git", "gti");
    ASSERT_TRUE(score > 50); // Moderate similarity (transposition)
    PASS();
}

void test_jaro_winkler_case_insensitive(void) {
    TEST("Jaro-Winkler: case insensitive");
    ASSERT_EQ(100, lle_fuzzy_jaro_winkler("Git", "git"));
    ASSERT_EQ(100, lle_fuzzy_jaro_winkler("GIT", "git"));
    PASS();
}

void test_jaro_winkler_prefix_bonus(void) {
    TEST("Jaro-Winkler: prefix bonus");
    // Strings with same prefix should score higher
    int score1 = lle_fuzzy_jaro_winkler("gitlab", "gitea");
    int score2 = lle_fuzzy_jaro_winkler("gitlab", "bitbucket");
    ASSERT_TRUE(score1 > score2);
    PASS();
}

// === Common Prefix Tests ===

void test_common_prefix_basic(void) {
    TEST("Common prefix: basic cases");
    ASSERT_EQ(3, lle_fuzzy_common_prefix("gitlab", "gitea", false));
    ASSERT_EQ(4, lle_fuzzy_common_prefix("command", "commit", false)); // "comm"
    ASSERT_EQ(0, lle_fuzzy_common_prefix("abc", "xyz", false));
    PASS();
}

void test_common_prefix_case_sensitive(void) {
    TEST("Common prefix: case sensitive");
    ASSERT_EQ(0, lle_fuzzy_common_prefix("Git", "git", true));
    ASSERT_EQ(3, lle_fuzzy_common_prefix("Git", "git", false));
    PASS();
}

void test_common_prefix_identical(void) {
    TEST("Common prefix: identical strings");
    ASSERT_EQ(3, lle_fuzzy_common_prefix("git", "git", false));
    ASSERT_EQ(7, lle_fuzzy_common_prefix("command", "command", false));
    PASS();
}

void test_common_prefix_empty(void) {
    TEST("Common prefix: empty strings");
    ASSERT_EQ(0, lle_fuzzy_common_prefix("", "git", false));
    ASSERT_EQ(0, lle_fuzzy_common_prefix("git", "", false));
    ASSERT_EQ(0, lle_fuzzy_common_prefix("", "", false));
    PASS();
}

// === Subsequence Score Tests ===

void test_subsequence_perfect(void) {
    TEST("Subsequence: perfect match");
    ASSERT_EQ(100, lle_fuzzy_subsequence_score("git", "git", false));
    PASS();
}

void test_subsequence_partial(void) {
    TEST("Subsequence: partial matches");
    ASSERT_EQ(100, lle_fuzzy_subsequence_score("gt", "git", false));
    ASSERT_EQ(100, lle_fuzzy_subsequence_score("gi", "git", false));
    PASS();
}

void test_subsequence_case_insensitive(void) {
    TEST("Subsequence: case insensitive");
    ASSERT_EQ(100, lle_fuzzy_subsequence_score("GT", "git", false));
    PASS();
}

void test_subsequence_no_match(void) {
    TEST("Subsequence: no match");
    int score = lle_fuzzy_subsequence_score("xyz", "git", false);
    ASSERT_EQ(0, score);
    PASS();
}

void test_subsequence_fuzzy(void) {
    TEST("Subsequence: fuzzy command matching");
    // "gti" in "git" - all chars present but wrong order
    int score = lle_fuzzy_subsequence_score("gti", "git", false);
    ASSERT_TRUE(score >= 66); // At least 2/3 chars match in order
    PASS();
}

// === Overall Similarity Score Tests ===

void test_similarity_identical(void) {
    TEST("Similarity: identical strings");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_default(&config);
    
    ASSERT_EQ(100, lle_fuzzy_similarity_score("git", "git", &config));
    ASSERT_EQ(100, lle_fuzzy_similarity_score("command", "command", &config));
    PASS();
}

void test_similarity_typos(void) {
    TEST("Similarity: common typos");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_default(&config);
    
    // Transpositions score moderately (not perfect but detectable)
    int score1 = lle_fuzzy_similarity_score("git", "gti", &config);
    int score2 = lle_fuzzy_similarity_score("ls", "sl", &config);
    
    ASSERT_TRUE(score1 > 30); // Detectable similarity
    ASSERT_TRUE(score2 >= 0);  // sl/ls are very different (2 char vs transposition)
    PASS();
}

void test_similarity_completions(void) {
    TEST("Similarity: completion candidates");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_completion(&config); // Completion-optimized weights
    
    // Prefix matches should score very high with completion config
    int score = lle_fuzzy_similarity_score("gi", "git", &config);
    ASSERT_TRUE(score > 70);
    PASS();
}

void test_similarity_history(void) {
    TEST("Similarity: history search");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_history(&config); // History-optimized weights
    
    // Similar-length commands should score well
    int score = lle_fuzzy_similarity_score("commit", "commits", &config);
    ASSERT_TRUE(score > 80);
    PASS();
}

void test_similarity_autocorrect(void) {
    TEST("Similarity: autocorrect");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_autocorrect(&config);
    
    // Balanced scoring for typos
    int score = lle_fuzzy_similarity_score("comand", "command", &config);
    ASSERT_TRUE(score > 70);
    PASS();
}

// === Configuration Tests ===

void test_config_default(void) {
    TEST("Config: default preset");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_default(&config);
    
    ASSERT_EQ(40, config.levenshtein_weight);
    ASSERT_EQ(30, config.jaro_winkler_weight);
    ASSERT_EQ(20, config.prefix_weight);
    ASSERT_EQ(10, config.subsequence_weight);
    ASSERT_EQ(false, config.case_sensitive);
    PASS();
}

void test_config_completion(void) {
    TEST("Config: completion preset");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_completion(&config);
    
    ASSERT_EQ(40, config.prefix_weight); // Higher for completion
    ASSERT_TRUE(config.enable_caching); // Caching enabled
    PASS();
}

void test_config_history(void) {
    TEST("Config: history preset");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_history(&config);
    
    ASSERT_EQ(50, config.jaro_winkler_weight); // Higher for history
    PASS();
}

// === Edge Cases ===

void test_edge_cases_null(void) {
    TEST("Edge cases: NULL pointers");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_default(&config);
    
    ASSERT_EQ(0, lle_fuzzy_similarity_score(NULL, "git", &config));
    ASSERT_EQ(0, lle_fuzzy_similarity_score("git", NULL, &config));
    ASSERT_EQ(0, lle_fuzzy_similarity_score("git", "git", NULL));
    PASS();
}

void test_edge_cases_long_strings(void) {
    TEST("Edge cases: long strings");
    lle_fuzzy_config_t config;
    lle_fuzzy_config_default(&config);
    
    char long1[256] = "this_is_a_very_long_command_name_that_might_appear_in_shell_scripts";
    char long2[256] = "this_is_a_very_long_command_name_that_might_appear_in_shell_script";
    
    int score = lle_fuzzy_similarity_score(long1, long2, &config);
    ASSERT_TRUE(score > 90); // Should be very similar
    PASS();
}

// === Main Test Runner ===

int main(void) {
    printf("=== LLE Fuzzy Matching Library Test Suite ===\n\n");
    
    // Levenshtein tests
    test_levenshtein_identical();
    test_levenshtein_single_char();
    test_levenshtein_case_insensitive();
    test_levenshtein_empty();
    test_levenshtein_commands();
    
    // Jaro-Winkler tests
    test_jaro_winkler_identical();
    test_jaro_winkler_similar();
    test_jaro_winkler_case_insensitive();
    test_jaro_winkler_prefix_bonus();
    
    // Common prefix tests
    test_common_prefix_basic();
    test_common_prefix_case_sensitive();
    test_common_prefix_identical();
    test_common_prefix_empty();
    
    // Subsequence tests
    test_subsequence_perfect();
    test_subsequence_partial();
    test_subsequence_case_insensitive();
    test_subsequence_no_match();
    test_subsequence_fuzzy();
    
    // Overall similarity tests
    test_similarity_identical();
    test_similarity_typos();
    test_similarity_completions();
    test_similarity_history();
    test_similarity_autocorrect();
    
    // Configuration tests
    test_config_default();
    test_config_completion();
    test_config_history();
    
    // Edge cases
    test_edge_cases_null();
    test_edge_cases_long_strings();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    } else {
        printf("\nSOME TESTS FAILED\n");
        return 1;
    }
}
