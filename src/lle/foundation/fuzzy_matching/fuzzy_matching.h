// src/lle/foundation/fuzzy_matching/fuzzy_matching.h
//
// LLE Fuzzy Matching Library
//
// Shared fuzzy matching algorithms extracted from autocorrect.c
// Provides similarity scoring for:
// - Autocorrect system
// - LLE Completion System (Spec 12)
// - LLE History Search (Spec 09)
//
// All algorithms are production-tested from autocorrect.c

#ifndef LLE_FOUNDATION_FUZZY_MATCHING_H
#define LLE_FOUNDATION_FUZZY_MATCHING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Scoring configuration for weighted algorithm combination
typedef struct {
    // Algorithm weights (should sum to 100 for percentage)
    uint8_t levenshtein_weight;    // Default: 40
    uint8_t jaro_winkler_weight;   // Default: 30
    uint8_t prefix_weight;         // Default: 20
    uint8_t subsequence_weight;    // Default: 10
    
    // Matching options
    bool case_sensitive;           // Case-sensitive matching
    
    // Performance options
    bool enable_caching;           // Enable result caching (future)
    size_t cache_size;             // Maximum cache entries (future)
    
    // Threshold
    int min_similarity_score;      // Minimum score to consider (0-100)
} lle_fuzzy_config_t;

// === Core Similarity Algorithms ===

/**
 * Calculate Levenshtein distance (edit distance) between two strings
 * 
 * Returns the minimum number of single-character edits (insertions,
 * deletions, substitutions) required to transform s1 into s2.
 * Case-insensitive comparison.
 * 
 * @param s1 First string
 * @param s2 Second string
 * @return Edit distance (0 = identical, higher = more different)
 */
int lle_fuzzy_levenshtein_distance(const char *s1, const char *s2);

/**
 * Calculate Jaro-Winkler similarity score
 * 
 * Jaro-Winkler is particularly good for comparing strings of similar length.
 * Includes prefix bonus for strings that start similarly.
 * Case-insensitive comparison.
 * 
 * @param s1 First string
 * @param s2 Second string
 * @return Similarity score 0-100 (100 = perfect match)
 */
int lle_fuzzy_jaro_winkler(const char *s1, const char *s2);

/**
 * Calculate common prefix length between two strings
 * 
 * Fast prefix matching useful for autocompletion.
 * 
 * @param s1 First string
 * @param s2 Second string
 * @param case_sensitive Whether to do case-sensitive comparison
 * @return Length of common prefix
 */
int lle_fuzzy_common_prefix(const char *s1, const char *s2, bool case_sensitive);

/**
 * Calculate subsequence match quality score
 * 
 * A subsequence means all characters of pattern appear in text in order,
 * but not necessarily consecutively. Useful for fuzzy completion.
 * 
 * Example: "gti" is a subsequence of "git" (but wrong order)
 *          "gt" is a subsequence of "git" (correct order)
 * 
 * @param pattern Pattern string to find
 * @param text Text to search in
 * @param case_sensitive Whether to do case-sensitive comparison
 * @return Score 0-100 (100 = all pattern chars found in order)
 */
int lle_fuzzy_subsequence_score(const char *pattern, const char *text,
                                bool case_sensitive);

/**
 * Calculate overall similarity score using weighted combination
 * 
 * This is the PROVEN weighted scoring from autocorrect.c that combines
 * all algorithms for best results.
 * 
 * @param s1 First string
 * @param s2 Second string
 * @param config Scoring configuration (algorithm weights)
 * @return Overall similarity score 0-100 (100 = perfect match)
 */
int lle_fuzzy_similarity_score(const char *s1, const char *s2,
                               const lle_fuzzy_config_t *config);

// === Configuration Presets ===

/**
 * Get default configuration
 * Weights: 40% Levenshtein, 30% Jaro-Winkler, 20% prefix, 10% subsequence
 */
void lle_fuzzy_config_default(lle_fuzzy_config_t *config);

/**
 * Get configuration optimized for autocompletion
 * Weights: 25% Levenshtein, 25% Jaro-Winkler, 40% prefix, 10% subsequence
 * (Higher prefix weight for completion)
 */
void lle_fuzzy_config_completion(lle_fuzzy_config_t *config);

/**
 * Get configuration optimized for history search
 * Weights: 20% Levenshtein, 50% Jaro-Winkler, 20% prefix, 10% subsequence
 * (Higher Jaro-Winkler for similar-length commands)
 */
void lle_fuzzy_config_history(lle_fuzzy_config_t *config);

/**
 * Get configuration optimized for autocorrect
 * Weights: 40% Levenshtein, 30% Jaro-Winkler, 20% prefix, 10% subsequence
 * (Balanced for command typos)
 */
void lle_fuzzy_config_autocorrect(lle_fuzzy_config_t *config);

#endif // LLE_FOUNDATION_FUZZY_MATCHING_H
