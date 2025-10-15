// src/lle/foundation/fuzzy_matching/fuzzy_matching.c
//
// LLE Fuzzy Matching Library Implementation
//
// All algorithms extracted from production-tested autocorrect.c

#define _POSIX_C_SOURCE 200809L
#include "fuzzy_matching.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

// === Helper Functions ===

static int min3(int a, int b, int c) {
    int min_ab = (a < b) ? a : b;
    return (min_ab < c) ? min_ab : c;
}

static double jaro_similarity(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    if (len1 == 0 && len2 == 0) {
        return 1.0;
    }
    if (len1 == 0 || len2 == 0) {
        return 0.0;
    }

    int match_window = ((len1 > len2) ? len1 : len2) / 2 - 1;
    if (match_window < 0) {
        match_window = 0;
    }

    bool *s1_matches = calloc(len1, sizeof(bool));
    bool *s2_matches = calloc(len2, sizeof(bool));

    if (!s1_matches || !s2_matches) {
        free(s1_matches);
        free(s2_matches);
        return 0.0;
    }

    int matches = 0;

    // Find matches
    for (int i = 0; i < len1; i++) {
        int start = (i - match_window > 0) ? i - match_window : 0;
        int end = (i + match_window < len2) ? i + match_window : len2 - 1;

        for (int j = start; j <= end; j++) {
            if (s2_matches[j] || tolower(s1[i]) != tolower(s2[j])) {
                continue;
            }
            s1_matches[i] = s2_matches[j] = true;
            matches++;
            break;
        }
    }

    if (matches == 0) {
        free(s1_matches);
        free(s2_matches);
        return 0.0;
    }

    // Count transpositions
    int transpositions = 0;
    int k = 0;
    for (int i = 0; i < len1; i++) {
        if (!s1_matches[i]) {
            continue;
        }
        while (!s2_matches[k]) {
            k++;
        }
        if (tolower(s1[i]) != tolower(s2[k])) {
            transpositions++;
        }
        k++;
    }

    free(s1_matches);
    free(s2_matches);

    double jaro = ((double)matches / len1 + (double)matches / len2 +
                   (double)(matches - transpositions / 2) / matches) /
                  3.0;

    return jaro;
}

// === Core Similarity Algorithms ===

int lle_fuzzy_levenshtein_distance(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        return -1;
    }

    int len1 = strlen(s1);
    int len2 = strlen(s2);

    // Create matrix
    int **matrix = malloc((len1 + 1) * sizeof(int *));
    if (!matrix) {
        return len1 + len2; // Worst case on allocation failure
    }

    for (int i = 0; i <= len1; i++) {
        matrix[i] = malloc((len2 + 1) * sizeof(int));
        if (!matrix[i]) {
            // Cleanup and return worst case
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            return len1 + len2;
        }
    }

    // Initialize matrix
    for (int i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }
    for (int j = 0; j <= len2; j++) {
        matrix[0][j] = j;
    }

    // Fill matrix using dynamic programming
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (tolower(s1[i - 1]) == tolower(s2[j - 1])) ? 0 : 1;
            
            int deletion = matrix[i - 1][j] + 1;
            int insertion = matrix[i][j - 1] + 1;
            int substitution = matrix[i - 1][j - 1] + cost;
            
            matrix[i][j] = min3(deletion, insertion, substitution);
        }
    }

    int result = matrix[len1][len2];

    // Cleanup
    for (int i = 0; i <= len1; i++) {
        free(matrix[i]);
    }
    free(matrix);

    return result;
}

int lle_fuzzy_jaro_winkler(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        return 0;
    }

    // Calculate base Jaro similarity
    double jaro = jaro_similarity(s1, s2);

    // Add prefix bonus (Winkler extension)
    int prefix_len = lle_fuzzy_common_prefix(s1, s2, false);
    if (prefix_len > 4) {
        prefix_len = 4; // Max prefix bonus is 4 characters
    }

    double jaro_winkler = jaro + (0.1 * prefix_len * (1.0 - jaro));

    return (int)(jaro_winkler * 100.0);
}

int lle_fuzzy_common_prefix(const char *s1, const char *s2, bool case_sensitive) {
    if (!s1 || !s2) {
        return 0;
    }

    int len = 0;
    while (s1[len] && s2[len]) {
        char c1 = case_sensitive ? s1[len] : tolower(s1[len]);
        char c2 = case_sensitive ? s2[len] : tolower(s2[len]);
        if (c1 != c2) {
            break;
        }
        len++;
    }
    return len;
}

int lle_fuzzy_subsequence_score(const char *pattern, const char *text,
                                bool case_sensitive) {
    if (!pattern || !text) {
        return 0;
    }

    int pattern_len = strlen(pattern);
    int text_len = strlen(text);

    if (pattern_len == 0) {
        return 100;
    }
    if (text_len == 0) {
        return 0;
    }

    int matches = 0;
    int text_idx = 0;

    for (int p = 0; p < pattern_len && text_idx < text_len; p++) {
        char pc = case_sensitive ? pattern[p] : tolower(pattern[p]);

        for (; text_idx < text_len; text_idx++) {
            char tc = case_sensitive ? text[text_idx] : tolower(text[text_idx]);
            if (pc == tc) {
                matches++;
                text_idx++;
                break;
            }
        }
    }

    return (matches * 100) / pattern_len;
}

int lle_fuzzy_similarity_score(const char *s1, const char *s2,
                               const lle_fuzzy_config_t *config) {
    if (!s1 || !s2 || !config) {
        return 0;
    }

    // Perfect match check
    if (config->case_sensitive) {
        if (strcmp(s1, s2) == 0) {
            return 100;
        }
    } else {
        if (strcasecmp(s1, s2) == 0) {
            return 100;
        }
    }

    int len1 = strlen(s1);
    int len2 = strlen(s2);

    if (len1 == 0 || len2 == 0) {
        return 0;
    }

    // 1. Levenshtein distance score
    int edit_distance = lle_fuzzy_levenshtein_distance(s1, s2);
    int max_len = (len1 > len2) ? len1 : len2;
    int levenshtein_score = ((max_len - edit_distance) * 100) / max_len;
    if (levenshtein_score < 0) {
        levenshtein_score = 0;
    }

    // 2. Jaro-Winkler score
    int jaro_score = lle_fuzzy_jaro_winkler(s1, s2);

    // 3. Common prefix score
    int prefix_len = lle_fuzzy_common_prefix(s1, s2, config->case_sensitive);
    int prefix_score = (prefix_len * 100) / ((len1 + len2) / 2);
    if (prefix_score > 100) {
        prefix_score = 100;
    }

    // 4. Subsequence matching score
    int subseq_score = lle_fuzzy_subsequence_score(s1, s2, config->case_sensitive);

    // Weighted combination (weights from config)
    int final_score = (levenshtein_score * config->levenshtein_weight +
                       jaro_score * config->jaro_winkler_weight +
                       prefix_score * config->prefix_weight +
                       subseq_score * config->subsequence_weight) / 100;

    if (final_score > 100) {
        final_score = 100;
    }
    if (final_score < 0) {
        final_score = 0;
    }

    return final_score;
}

// === Configuration Presets ===

void lle_fuzzy_config_default(lle_fuzzy_config_t *config) {
    if (!config) {
        return;
    }

    config->levenshtein_weight = 40;
    config->jaro_winkler_weight = 30;
    config->prefix_weight = 20;
    config->subsequence_weight = 10;
    config->case_sensitive = false;
    config->enable_caching = false;
    config->cache_size = 0;
    config->min_similarity_score = 50;
}

void lle_fuzzy_config_completion(lle_fuzzy_config_t *config) {
    if (!config) {
        return;
    }

    config->levenshtein_weight = 25;
    config->jaro_winkler_weight = 25;
    config->prefix_weight = 40;  // Higher for completion
    config->subsequence_weight = 10;
    config->case_sensitive = false;
    config->enable_caching = true;
    config->cache_size = 1000;
    config->min_similarity_score = 50;
}

void lle_fuzzy_config_history(lle_fuzzy_config_t *config) {
    if (!config) {
        return;
    }

    config->levenshtein_weight = 20;
    config->jaro_winkler_weight = 50;  // Higher for history
    config->prefix_weight = 20;
    config->subsequence_weight = 10;
    config->case_sensitive = false;
    config->enable_caching = false;
    config->cache_size = 0;
    config->min_similarity_score = 50;
}

void lle_fuzzy_config_autocorrect(lle_fuzzy_config_t *config) {
    if (!config) {
        return;
    }

    // Same as default - balanced for autocorrect
    lle_fuzzy_config_default(config);
}
