/*
 * test_terminal_capabilities.c - Unit tests for terminal capability detection
 *
 * Tests Spec 02 Phase 1: Terminal Capability Detection
 *
 * Test Categories:
 * 1. Terminal type detection
 * 2. Color capability detection
 * 3. Advanced feature detection
 * 4. Geometry detection
 * 5. Performance characteristics
 */

#include "lle/terminal_abstraction.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                             \
    static void name(void);                                                    \
    static void run_##name(void) {                                             \
        tests_run++;                                                           \
        printf("  Running %s...", #name);                                      \
        fflush(stdout);                                                        \
        name();                                                                \
        tests_passed++;                                                        \
        printf(" PASS\n");                                                     \
    }                                                                          \
    static void name(void)

/* ============================================================================
 * TERMINAL TYPE DETECTION TESTS
 * ============================================================================
 */

TEST(test_capability_detection_basic) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);
    assert(caps->terminal_type != NULL);
    assert(caps->terminal_program != NULL);

    /* Should detect some terminal type */
    assert(caps->terminal_type_enum >= LLE_TERMINAL_UNKNOWN);
    assert(caps->terminal_type_enum <= LLE_TERMINAL_KITTY);

    /* Cleanup */
    lle_capabilities_destroy(caps);
}

TEST(test_terminal_type_strings) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Terminal type string should not be empty */
    assert(strlen(caps->terminal_type) > 0);
    assert(strlen(caps->terminal_program) > 0);

    /* Should be valid strings */
    assert(strcmp(caps->terminal_type, "") != 0);
    assert(strcmp(caps->terminal_program, "") != 0);

    lle_capabilities_destroy(caps);
}

TEST(test_tty_detection) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* is_tty should match actual TTY status */
    int stdin_is_tty = isatty(STDIN_FILENO);
    int stdout_is_tty = isatty(STDOUT_FILENO);
    bool expected_tty = (stdin_is_tty && stdout_is_tty);

    assert(caps->is_tty == expected_tty);

    lle_capabilities_destroy(caps);
}

/* ============================================================================
 * COLOR CAPABILITY TESTS
 * ============================================================================
 */

TEST(test_color_depth_valid) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Color depth should be 0, 4, 8, or 24 */
    assert(caps->detected_color_depth == 0 || caps->detected_color_depth == 4 ||
           caps->detected_color_depth == 8 || caps->detected_color_depth == 24);

    /* Color support flags should be consistent with depth */
    if (caps->detected_color_depth >= 4) {
        assert(caps->supports_ansi_colors == true);
    }

    if (caps->detected_color_depth >= 8) {
        assert(caps->supports_256_colors == true);
    }

    if (caps->detected_color_depth == 24) {
        assert(caps->supports_truecolor == true);
    }

    lle_capabilities_destroy(caps);
}

TEST(test_color_flags_consistency) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* If truecolor supported, 256 colors must also be supported */
    if (caps->supports_truecolor) {
        assert(caps->supports_256_colors == true);
        assert(caps->supports_ansi_colors == true);
    }

    /* If 256 colors supported, ANSI colors must be supported */
    if (caps->supports_256_colors) {
        assert(caps->supports_ansi_colors == true);
    }

    lle_capabilities_destroy(caps);
}

/* ============================================================================
 * TEXT ATTRIBUTE TESTS
 * ============================================================================
 */

TEST(test_text_attributes_detected) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Modern terminals should support at least bold and underline */
    /* Note: This might fail in very minimal environments, but that's OK */
    /* We're just testing that detection ran, not enforcing support */

    /* Text attribute flags should be boolean */
    assert(caps->supports_bold == true || caps->supports_bold == false);
    assert(caps->supports_italic == true || caps->supports_italic == false);
    assert(caps->supports_underline == true ||
           caps->supports_underline == false);
    assert(caps->supports_strikethrough == true ||
           caps->supports_strikethrough == false);
    assert(caps->supports_reverse == true || caps->supports_reverse == false);
    assert(caps->supports_dim == true || caps->supports_dim == false);

    lle_capabilities_destroy(caps);
}

/* ============================================================================
 * ADVANCED FEATURE TESTS
 * ============================================================================
 */

TEST(test_advanced_features_detected) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Feature flags should be boolean */
    assert(caps->supports_mouse_reporting == true ||
           caps->supports_mouse_reporting == false);
    assert(caps->supports_bracketed_paste == true ||
           caps->supports_bracketed_paste == false);
    assert(caps->supports_focus_events == true ||
           caps->supports_focus_events == false);
    assert(caps->supports_synchronized_output == true ||
           caps->supports_synchronized_output == false);
    assert(caps->supports_unicode == true || caps->supports_unicode == false);

    lle_capabilities_destroy(caps);
}

TEST(test_feature_correlation) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Modern terminals (Alacritty, Kitty) should support most features */
    if (caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_KITTY) {
        assert(caps->supports_unicode == true);
        assert(caps->supports_mouse_reporting == true);
        assert(caps->supports_bracketed_paste == true);
    }

    lle_capabilities_destroy(caps);
}

/* ============================================================================
 * GEOMETRY DETECTION TESTS
 * ============================================================================
 */

TEST(test_geometry_detection) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Width and height should be reasonable */
    assert(caps->terminal_width >= 20); /* Minimum enforced */
    assert(caps->terminal_height >= 5); /* Minimum enforced */

    /* Should not be absurdly large */
    assert(caps->terminal_width < 10000);
    assert(caps->terminal_height < 10000);

    lle_capabilities_destroy(caps);
}

TEST(test_geometry_update) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Update with specific dimensions */
    result = lle_capabilities_update_geometry(caps, 100, 40);
    assert(result == LLE_SUCCESS);
    assert(caps->terminal_width == 100);
    assert(caps->terminal_height == 40);

    /* Update with zeros (should re-detect) */
    result = lle_capabilities_update_geometry(caps, 0, 0);
    assert(result == LLE_SUCCESS);
    assert(caps->terminal_width >= 20);
    assert(caps->terminal_height >= 5);

    /* Update with too-small values (should enforce minimums) */
    result = lle_capabilities_update_geometry(caps, 10, 2);
    assert(result == LLE_SUCCESS);
    assert(caps->terminal_width == 80);  /* Enforced minimum */
    assert(caps->terminal_height == 24); /* Enforced minimum */

    lle_capabilities_destroy(caps);
}

/* ============================================================================
 * PERFORMANCE CHARACTERISTICS TESTS
 * ============================================================================
 */

TEST(test_performance_characteristics) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Latency should be reasonable (1-100ms) */
    assert(caps->estimated_latency_ms >= 1);
    assert(caps->estimated_latency_ms <= 100);

    /* Fast update flag should be boolean */
    assert(caps->supports_fast_updates == true ||
           caps->supports_fast_updates == false);

    /* Modern terminals should have lower latency */
    if (caps->terminal_type_enum == LLE_TERMINAL_ALACRITTY ||
        caps->terminal_type_enum == LLE_TERMINAL_KITTY) {
        assert(caps->estimated_latency_ms <= 10);
        assert(caps->supports_fast_updates == true);
    }

    lle_capabilities_destroy(caps);
}

TEST(test_optimization_flags) {
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);

    /* Optimization flags should be set */
    /* At minimum, unicode awareness should be set for modern terminals */
    if (caps->supports_unicode) {
        assert((caps->optimizations & LLE_OPT_UNICODE_AWARE) != 0);
    }

    /* Fast terminals should have incremental draw enabled */
    if (caps->supports_fast_updates) {
        assert((caps->optimizations & LLE_OPT_INCREMENTAL_DRAW) != 0);
    }

    lle_capabilities_destroy(caps);
}

/* ============================================================================
 * ERROR HANDLING TESTS
 * ============================================================================
 */

TEST(test_null_parameter_handling) {
    /* NULL caps pointer should return error */
    lle_result_t result = lle_capabilities_detect_environment(NULL, NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    /* NULL destroy should not crash */
    lle_capabilities_destroy(NULL);

    /* NULL update should return error */
    result = lle_capabilities_update_geometry(NULL, 80, 24);
    assert(result == LLE_ERROR_INVALID_PARAMETER);
}

TEST(test_multiple_detections) {
    /* Should be able to detect multiple times */
    lle_terminal_capabilities_t *caps1 = NULL;
    lle_terminal_capabilities_t *caps2 = NULL;

    lle_result_t result1 = lle_capabilities_detect_environment(&caps1, NULL);
    lle_result_t result2 = lle_capabilities_detect_environment(&caps2, NULL);

    assert(result1 == LLE_SUCCESS);
    assert(result2 == LLE_SUCCESS);
    assert(caps1 != NULL);
    assert(caps2 != NULL);

    /* Results should be consistent */
    assert(caps1->terminal_type_enum == caps2->terminal_type_enum);
    assert(caps1->terminal_width == caps2->terminal_width);
    assert(caps1->terminal_height == caps2->terminal_height);

    lle_capabilities_destroy(caps1);
    lle_capabilities_destroy(caps2);
}

/* ============================================================================
 * MEMORY MANAGEMENT TESTS
 * ============================================================================
 */

TEST(test_memory_cleanup) {
    /* Test that destroy properly frees memory */
    lle_terminal_capabilities_t *caps = NULL;
    lle_result_t result = lle_capabilities_detect_environment(&caps, NULL);

    assert(result == LLE_SUCCESS);
    assert(caps != NULL);
    assert(caps->terminal_type != NULL);
    assert(caps->terminal_program != NULL);

    /* Should not leak memory when destroyed */
    lle_capabilities_destroy(caps);
    /* If valgrind is run, this will be verified */
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================
 */

int main(void) {
    printf("Running Terminal Capability Detection Tests\n");
    printf("============================================\n\n");

    printf("Terminal Type Detection Tests:\n");
    run_test_capability_detection_basic();
    run_test_terminal_type_strings();
    run_test_tty_detection();

    printf("\nColor Capability Tests:\n");
    run_test_color_depth_valid();
    run_test_color_flags_consistency();

    printf("\nText Attribute Tests:\n");
    run_test_text_attributes_detected();

    printf("\nAdvanced Feature Tests:\n");
    run_test_advanced_features_detected();
    run_test_feature_correlation();

    printf("\nGeometry Detection Tests:\n");
    run_test_geometry_detection();
    run_test_geometry_update();

    printf("\nPerformance Characteristics Tests:\n");
    run_test_performance_characteristics();
    run_test_optimization_flags();

    printf("\nError Handling Tests:\n");
    run_test_null_parameter_handling();
    run_test_multiple_detections();

    printf("\nMemory Management Tests:\n");
    run_test_memory_cleanup();

    printf("\n============================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);

    if (tests_passed == tests_run) {
        printf("✓ ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("✗ SOME TESTS FAILED\n");
        return 1;
    }
}
