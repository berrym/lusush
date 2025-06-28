/**
 * Simple Auto-Correction Test Program
 * Tests the auto-correction algorithms and functionality independently
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include auto-correction header
#include "../../include/autocorrect.h"

void test_similarity_scores() {
    printf("Testing similarity scores...\n");

    // Test identical commands
    int score = autocorrect_similarity_score("echo", "echo", false);
    printf("'echo' vs 'echo': %d (expected: 100)\n", score);
    assert(score == 100);

    // Test similar commands
    score = autocorrect_similarity_score("ech", "echo", false);
    printf("'ech' vs 'echo': %d\n", score);

    score = autocorrect_similarity_score("ehco", "echo", false);
    printf("'ehco' vs 'echo': %d\n", score);

    score = autocorrect_similarity_score("ecoh", "echo", false);
    printf("'ecoh' vs 'echo': %d\n", score);

    // Test builtin suggestions
    score = autocorrect_similarity_score("cd", "pwd", false);
    printf("'cd' vs 'pwd': %d\n", score);

    score = autocorrect_similarity_score("pws", "pwd", false);
    printf("'pws' vs 'pwd': %d\n", score);

    score = autocorrect_similarity_score("pwdd", "pwd", false);
    printf("'pwdd' vs 'pwd': %d\n", score);

    printf("Similarity score tests completed.\n\n");
}

void test_levenshtein_distance() {
    printf("Testing Levenshtein distance...\n");

    int dist = autocorrect_levenshtein_distance("echo", "echo");
    printf("'echo' vs 'echo': %d (expected: 0)\n", dist);
    assert(dist == 0);

    dist = autocorrect_levenshtein_distance("ech", "echo");
    printf("'ech' vs 'echo': %d (expected: 1)\n", dist);

    dist = autocorrect_levenshtein_distance("ehco", "echo");
    printf("'ehco' vs 'echo': %d\n", dist);

    dist = autocorrect_levenshtein_distance("test", "echo");
    printf("'test' vs 'echo': %d\n", dist);

    printf("Levenshtein distance tests completed.\n\n");
}

void test_builtin_suggestions() {
    printf("Testing builtin suggestions...\n");

    // Skip builtin suggestions test to avoid external dependencies
    printf(
        "Builtin suggestions tests skipped (requires full shell context).\n\n");
}

void test_configuration() {
    printf("Testing auto-correction configuration...\n");

    // Test default configuration
    autocorrect_config_t config;
    autocorrect_get_default_config(&config);

    printf("Default config:\n");
    printf("  enabled: %s\n", config.enabled ? "true" : "false");
    printf("  max_suggestions: %d\n", config.max_suggestions);
    printf("  similarity_threshold: %d\n", config.similarity_threshold);
    printf("  interactive_prompts: %s\n",
           config.interactive_prompts ? "true" : "false");
    printf("  learn_from_history: %s\n",
           config.learn_from_history ? "true" : "false");
    printf("  correct_builtins: %s\n",
           config.correct_builtins ? "true" : "false");
    printf("  correct_external: %s\n",
           config.correct_external ? "true" : "false");
    printf("  case_sensitive: %s\n", config.case_sensitive ? "true" : "false");

    // Test validation
    bool valid = autocorrect_validate_config(&config);
    printf("Config validation: %s\n", valid ? "PASS" : "FAIL");
    assert(valid);

    // Test invalid config
    config.max_suggestions = 10; // Too high
    valid = autocorrect_validate_config(&config);
    printf("Invalid config validation: %s (expected: false)\n",
           valid ? "true" : "false");
    assert(!valid);

    printf("Configuration tests completed.\n\n");
}

void test_common_prefix() {
    printf("Testing common prefix length...\n");

    int len = autocorrect_common_prefix_length("echo", "echo", false);
    printf("'echo' vs 'echo': %d (expected: 4)\n", len);
    assert(len == 4);

    len = autocorrect_common_prefix_length("echo", "ech", false);
    printf("'echo' vs 'ech': %d (expected: 3)\n", len);
    assert(len == 3);

    len = autocorrect_common_prefix_length("pwd", "pwdd", false);
    printf("'pwd' vs 'pwdd': %d (expected: 3)\n", len);
    assert(len == 3);

    len = autocorrect_common_prefix_length("test", "echo", false);
    printf("'test' vs 'echo': %d (expected: 0)\n", len);
    assert(len == 0);

    // Test case sensitivity
    len = autocorrect_common_prefix_length("Echo", "echo", true);
    printf("'Echo' vs 'echo' (case-sensitive): %d (expected: 0)\n", len);
    assert(len == 0);

    len = autocorrect_common_prefix_length("Echo", "echo", false);
    printf("'Echo' vs 'echo' (case-insensitive): %d (expected: 4)\n", len);
    assert(len == 4);

    printf("Common prefix tests completed.\n\n");
}

void test_subsequence_score() {
    printf("Testing subsequence score...\n");

    int score = autocorrect_subsequence_score("echo", "echo", false);
    printf("'echo' subsequence in 'echo': %d (expected: 100)\n", score);
    assert(score == 100);

    score = autocorrect_subsequence_score("ech", "echo", false);
    printf("'ech' subsequence in 'echo': %d\n", score);

    score = autocorrect_subsequence_score("eh", "echo", false);
    printf("'eh' subsequence in 'echo': %d\n", score);

    score = autocorrect_subsequence_score("eco", "echo", false);
    printf("'eco' subsequence in 'echo': %d\n", score);

    score = autocorrect_subsequence_score("xyz", "echo", false);
    printf("'xyz' subsequence in 'echo': %d (expected: 0)\n", score);
    assert(score == 0);

    printf("Subsequence score tests completed.\n\n");
}

void test_end_to_end() {
    printf("Testing end-to-end auto-correction...\n");

    // Initialize auto-correction
    int result = autocorrect_init();
    printf("Auto-correction init: %s\n", result == 0 ? "SUCCESS" : "FAILED");
    assert(result == 0);

    // Set up configuration
    autocorrect_config_t config;
    autocorrect_get_default_config(&config);
    config.enabled = true;
    config.interactive_prompts = false; // Don't prompt in test
    config.max_suggestions = 3;
    config.similarity_threshold = 40;

    result = autocorrect_load_config(&config);
    printf("Config load: %s\n", result == 0 ? "SUCCESS" : "FAILED");
    assert(result == 0);

    // Test if enabled
    bool enabled = autocorrect_is_enabled();
    printf("Auto-correction enabled: %s\n", enabled ? "true" : "false");
    assert(enabled);

    // Skip full suggestion test to avoid dependencies
    printf("Full suggestion tests skipped (requires shell context).\n");

    // Test statistics
    int offered, accepted, learned;
    autocorrect_get_stats(&offered, &accepted, &learned);
    printf("Stats - offered: %d, accepted: %d, learned: %d\n", offered,
           accepted, learned);

    // Clean up auto-correction
    autocorrect_cleanup();

    printf("End-to-end tests completed.\n\n");
}

int main() {
    printf("=== Auto-Correction Test Suite ===\n\n");

    test_similarity_scores();
    test_levenshtein_distance();
    test_common_prefix();
    test_subsequence_score();
    test_configuration();
    test_end_to_end();

    printf("=== All Tests Completed Successfully! ===\n");
    return 0;
}
