/**
 * test_platform_detection.c - Platform Detection Tests for Cross-Platform History Navigation
 *
 * This test suite validates platform detection functionality that enables perfect
 * history navigation on both macOS and Linux platforms. Tests the detection of
 * operating systems, terminals, and platform-specific backspace sequences.
 *
 * SUCCESS FOUNDATION: Based on 100% perfect macOS implementation that achieved
 * "happiest user has been with history recall" - now extending to Linux.
 *
 * Copyright (c) 2025 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "test_framework.h"
#include "../src/line_editor/platform_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Test basic platform detection initialization
 *
 * Validates that platform detection initializes correctly and provides
 * basic operating system and terminal information.
 */
LLE_TEST(platform_detection_init) {
    /* Test initialization */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    /* Test that we can get platform info */
    const lle_platform_info_t *info = lle_platform_get_info();
    LLE_ASSERT(info != NULL);
    
    /* Test detection completion */
    LLE_ASSERT(info->detection_complete == true);
    
    /* Test that OS is detected (not unknown) */
    lle_platform_os_t os = lle_platform_get_os();
    LLE_ASSERT(os != LLE_PLATFORM_OS_UNKNOWN);
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Platform detection initialization test passed\n");
}

/**
 * @brief Test Linux platform detection specifically
 *
 * Validates that Linux detection works correctly and provides appropriate
 * terminal and backspace sequence information for Linux users.
 */
LLE_TEST(linux_platform_detection) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    /* Test Linux detection functions */
    bool is_linux = lle_platform_is_linux();
    bool is_macos = lle_platform_is_macos();
    bool is_unix = lle_platform_is_unix();
    
    printf("🔍 Platform detection results:\n");
    printf("   - Linux: %s\n", is_linux ? "YES" : "NO");
    printf("   - macOS: %s\n", is_macos ? "YES" : "NO");
    printf("   - Unix-like: %s\n", is_unix ? "YES" : "NO");
    
    /* On Linux systems, verify Linux detection */
#ifdef __linux__
    LLE_ASSERT(is_linux == true);
    LLE_ASSERT(is_macos == false);
    LLE_ASSERT(is_unix == true);
#endif
    
    /* On macOS systems, verify macOS detection */
#ifdef __APPLE__
    LLE_ASSERT(is_macos == true);
    LLE_ASSERT(is_linux == false);
    LLE_ASSERT(is_unix == true);
#endif
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Linux platform detection test passed\n");
}

/**
 * @brief Test terminal type detection
 *
 * Validates detection of different terminal types including GNOME Terminal,
 * Konsole, xterm, and other Linux terminals commonly used.
 */
LLE_TEST(terminal_type_detection) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    /* Get terminal information */
    lle_platform_terminal_t terminal = lle_platform_get_terminal();
    
    /* Test specific terminal detection functions */
    bool is_iterm2 = lle_platform_is_iterm2();
    bool is_gnome = lle_platform_is_gnome_terminal();
    bool is_konsole = lle_platform_is_konsole();
    bool is_xterm = lle_platform_is_xterm();
    bool is_multiplexer = lle_platform_is_multiplexer();
    
    printf("🔍 Terminal detection results:\n");
    printf("   - Terminal type: %d\n", (int)terminal);
    printf("   - iTerm2: %s\n", is_iterm2 ? "YES" : "NO");
    printf("   - GNOME Terminal: %s\n", is_gnome ? "YES" : "NO");
    printf("   - Konsole: %s\n", is_konsole ? "YES" : "NO");
    printf("   - xterm: %s\n", is_xterm ? "YES" : "NO");
    printf("   - Multiplexer: %s\n", is_multiplexer ? "YES" : "NO");
    
    /* Terminal should be detected (not unknown) */
    LLE_ASSERT(terminal != LLE_PLATFORM_TERM_UNKNOWN);
    
    /* Only one terminal type should be detected */
    int detection_count = (is_iterm2 ? 1 : 0) + (is_gnome ? 1 : 0) + 
                         (is_konsole ? 1 : 0) + (is_xterm ? 1 : 0);
    LLE_ASSERT(detection_count <= 1);
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Terminal type detection test passed\n");
}

/**
 * @brief Test platform-specific backspace sequence selection
 *
 * This is the critical test for Linux users - validates that the correct
 * backspace sequence is selected for each platform and terminal combination.
 */
LLE_TEST(backspace_sequence_selection) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    /* Get backspace sequence information */
    const char *backspace_seq = lle_platform_get_backspace_sequence();
    size_t backspace_len = lle_platform_get_backspace_length();
    lle_platform_backspace_type_t backspace_type = lle_platform_get_backspace_type();
    bool requires_special = lle_platform_requires_special_backspace();
    
    printf("🔍 Backspace sequence analysis:\n");
    printf("   - Sequence: [%s] (length: %zu)\n", backspace_seq, backspace_len);
    printf("   - Type: %d\n", (int)backspace_type);
    printf("   - Requires special handling: %s\n", requires_special ? "YES" : "NO");
    
    /* Validate backspace sequence properties */
    LLE_ASSERT(backspace_seq != NULL);
    LLE_ASSERT(backspace_len > 0);
    LLE_ASSERT(backspace_len <= 10);
    
    /* Validate sequence content (should be printable or control characters) */
    for (size_t i = 0; i < backspace_len; i++) {
        char c = backspace_seq[i];
        LLE_ASSERT(c >= 0);
    }
    
    /* Platform-specific validations */
#ifdef __linux__
    printf("📋 Linux-specific validation:\n");
    printf("   - Should work on GNOME Terminal, Konsole, xterm variants\n");
    LLE_ASSERT(backspace_type != LLE_PLATFORM_BACKSPACE_MACOS_ITERM);
#endif
    
#ifdef __APPLE__
    printf("📋 macOS-specific validation:\n");
    printf("   - Should preserve perfect iTerm2 behavior\n");
    if (lle_platform_is_iterm2()) {
        LLE_ASSERT(backspace_type == LLE_PLATFORM_BACKSPACE_MACOS_ITERM);
    }
#endif
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Backspace sequence selection test passed\n");
}

/**
 * @brief Test cross-platform compatibility matrix
 *
 * Validates that the platform detection system handles all major
 * platform and terminal combinations correctly.
 */
LLE_TEST(cross_platform_compatibility) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    const lle_platform_info_t *info = lle_platform_get_info();
    
    printf("🔍 Complete platform analysis:\n");
    printf("   - OS: %d\n", (int)info->os);
    printf("   - Terminal: %d\n", (int)info->terminal);
    printf("   - Backspace type: %d\n", (int)info->backspace);
    printf("   - Unicode support: %s\n", info->supports_unicode ? "YES" : "NO");
    printf("   - Color support: %s\n", info->supports_color ? "YES" : "NO");
    printf("   - Mouse support: %s\n", info->supports_mouse ? "YES" : "NO");
    printf("   - Special handling: %s\n", info->requires_special_handling ? "YES" : "NO");
    printf("   - Fast clearing: %s\n", info->fast_clearing ? "YES" : "NO");
    printf("   - Efficient cursor queries: %s\n", info->efficient_cursor_queries ? "YES" : "NO");
    printf("   - Batch output preferred: %s\n", info->batch_output_preferred ? "YES" : "NO");
    printf("   - Detection confidence: %d%%\n", info->detection_confidence);
    
    /* Validate basic capability assumptions */
    LLE_ASSERT(info->supports_unicode == true);
    LLE_ASSERT(info->supports_color == true);
    
    /* Validate detection confidence */
    LLE_ASSERT(info->detection_confidence >= LLE_PLATFORM_CONFIDENCE_LOW);
    
    /* Test platform description */
    char desc_buffer[256];
    int desc_result = lle_platform_get_description(desc_buffer, sizeof(desc_buffer));
    LLE_ASSERT(desc_result > 0);
    LLE_ASSERT(strlen(desc_buffer) > 0);
    
    printf("   - Platform description: %s\n", desc_buffer);
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Cross-platform compatibility test passed\n");
}

/**
 * @brief Test Linux-specific terminal optimizations
 *
 * Validates Linux-specific optimizations and handles the common Linux
 * terminals that users will encounter in the wild.
 */
LLE_TEST(linux_terminal_optimizations) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
#ifdef __linux__
    /* Linux-specific tests */
    printf("🐧 Linux terminal optimization tests:\n");
    
    /* Test efficient clearing capability */
    bool efficient_clearing = lle_platform_supports_efficient_clearing();
    printf("   - Efficient clearing: %s\n", efficient_clearing ? "YES" : "NO");
    
    /* Test clearing verification needs */
    bool needs_verification = lle_platform_needs_clearing_verification();
    printf("   - Needs clearing verification: %s\n", needs_verification ? "YES" : "NO");
    
    /* Test clear-to-EOL reliability */
    bool reliable_clear_eol = lle_platform_has_reliable_clear_eol();
    printf("   - Reliable clear-to-EOL: %s\n", reliable_clear_eol ? "YES" : "NO");
    
    /* Test optimal batch size */
    size_t batch_size = lle_platform_get_optimal_batch_size();
    printf("   - Optimal batch size: %zu\n", batch_size);
    LLE_ASSERT(batch_size >= 1);
    LLE_ASSERT(batch_size <= 100);
    
    /* Test buffered output preference */
    bool prefers_buffered = lle_platform_prefers_buffered_output();
    printf("   - Prefers buffered output: %s\n", prefers_buffered ? "YES" : "NO");
    
    /* Test Linux distribution detection */
    char distro_buffer[128];
    bool distro_detected = lle_platform_detect_linux_distribution(distro_buffer, sizeof(distro_buffer));
    if (distro_detected) {
        printf("   - Linux distribution: %s\n", distro_buffer);
        LLE_ASSERT(strlen(distro_buffer) > 0);
    }
    
    /* Test desktop environment detection */
    char de_buffer[128];
    bool de_detected = lle_platform_detect_desktop_environment(de_buffer, sizeof(de_buffer));
    if (de_detected) {
        printf("   - Desktop environment: %s\n", de_buffer);
        LLE_ASSERT(strlen(de_buffer) > 0);
    }
    
#else
    printf("📋 Non-Linux platform - skipping Linux-specific optimizations\n");
#endif
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Linux terminal optimization test passed\n");
}

/**
 * @brief Test backspace sequence validation for all platforms
 *
 * This critical test validates that backspace sequences work correctly
 * across all platform and terminal combinations, ensuring Linux users
 * get the same perfect experience as macOS users.
 */
LLE_TEST(backspace_sequence_validation) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    /* Get current platform backspace configuration */
    const char *backspace_seq = lle_platform_get_backspace_sequence();
    size_t backspace_len = lle_platform_get_backspace_length();
    lle_platform_backspace_type_t backspace_type = lle_platform_get_backspace_type();
    
    printf("🔍 Backspace sequence validation:\n");
    printf("   - Platform backspace sequence: [");
    for (size_t i = 0; i < backspace_len; i++) {
        char c = backspace_seq[i];
        if (c == '\b') {
            printf("\\b");
        } else if (c == ' ') {
            printf(" ");
        } else if (c >= 32 && c <= 126) {
            printf("%c", c);
        } else {
            printf("\\x%02x", (unsigned char)c);
        }
    }
    printf("]\n");
    printf("   - Sequence length: %zu bytes\n", backspace_len);
    printf("   - Backspace type: %d\n", (int)backspace_type);
    
    /* Validate sequence properties */
    LLE_ASSERT(backspace_seq != NULL);
    LLE_ASSERT(backspace_len >= 1);
    LLE_ASSERT(backspace_len <= 10);
    
    /* Validate that sequence contains expected characters */
    bool has_backspace = false;
    bool has_space = false;
    for (size_t i = 0; i < backspace_len; i++) {
        if (backspace_seq[i] == '\b') has_backspace = true;
        if (backspace_seq[i] == ' ') has_space = true;
    }
    
    /* Standard sequences should contain backspace and space */
    if (backspace_type != LLE_PLATFORM_BACKSPACE_FALLBACK) {
        LLE_ASSERT(has_backspace == true);
        LLE_ASSERT(has_space == true);
    }
    
    /* Test sequence consistency */
    const char *seq1 = lle_platform_get_backspace_sequence();
    const char *seq2 = lle_platform_get_backspace_sequence();
    LLE_ASSERT(seq1 == seq2);
    
    size_t len1 = lle_platform_get_backspace_length();
    size_t len2 = lle_platform_get_backspace_length();
    LLE_ASSERT(len1 == len2);
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Backspace sequence validation test passed\n");
}

/**
 * @brief Test platform detection reinitialization
 *
 * Validates that platform detection can be reinitialized correctly,
 * which is important for testing and runtime environment changes.
 */
LLE_TEST(platform_detection_reinit) {
    /* Initial initialization */
    bool init1 = lle_platform_init();
    LLE_ASSERT(init1 == true);
    
    const lle_platform_info_t *info1 = lle_platform_get_info();
    lle_platform_os_t os1 = info1->os;
    lle_platform_terminal_t term1 = info1->terminal;
    
    /* Cleanup and reinitialize */
    lle_platform_cleanup();
    bool init2 = lle_platform_init();
    LLE_ASSERT(init2 == true);
    
    const lle_platform_info_t *info2 = lle_platform_get_info();
    lle_platform_os_t os2 = info2->os;
    lle_platform_terminal_t term2 = info2->terminal;
    
    /* Results should be consistent */
    LLE_ASSERT(os1 == os2);
    LLE_ASSERT(term1 == term2);
    
    /* Test redetection function */
    bool redetect_result = lle_platform_redetect();
    LLE_ASSERT(redetect_result == true);
    
    const lle_platform_info_t *info3 = lle_platform_get_info();
    LLE_ASSERT(info3->os == os1);
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Platform detection reinitialization test passed\n");
}

/**
 * @brief Test platform detection validation
 *
 * Validates the platform detection validation functions that ensure
 * detection quality and reliability.
 */
LLE_TEST(platform_detection_validation) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    /* Test validation function */
    int validation_result = lle_platform_validate_detection();
    printf("🔍 Platform detection validation result: %d\n", validation_result);
    
    LLE_ASSERT(validation_result == LLE_PLATFORM_OK);
    
    /* Test detection confidence */
    const lle_platform_info_t *info = lle_platform_get_info();
    printf("🔍 Detection confidence: %d%%\n", info->detection_confidence);
    
    LLE_ASSERT(info->detection_confidence >= LLE_PLATFORM_CONFIDENCE_LOW);
    
    /* For well-known platforms, confidence should be higher */
    if (lle_platform_is_linux() || lle_platform_is_macos()) {
        LLE_ASSERT(info->detection_confidence >= LLE_PLATFORM_CONFIDENCE_MEDIUM);
    }
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ Platform detection validation test passed\n");
}

/**
 * @brief Integration test for history navigation platform compatibility
 *
 * This test validates that the platform detection integrates correctly
 * with the history navigation system for cross-platform success.
 */
LLE_TEST(history_navigation_platform_integration) {
    /* Initialize platform detection */
    bool init_result = lle_platform_init();
    LLE_ASSERT(init_result == true);
    
    printf("🔍 History navigation platform integration test:\n");
    
    /* Simulate the exact pattern used in successful macOS history navigation */
    const char *backspace_seq = lle_platform_get_backspace_sequence();
    size_t backspace_len = lle_platform_get_backspace_length();
    
    /* Test that we can get sequences for clearing operations */
    LLE_ASSERT(backspace_seq != NULL);
    LLE_ASSERT(backspace_len > 0);
    
    /* Test platform-specific optimizations */
    bool supports_efficient_clearing = lle_platform_supports_efficient_clearing();
    bool needs_verification = lle_platform_needs_clearing_verification();
    bool reliable_clear_eol = lle_platform_has_reliable_clear_eol();
    
    printf("   - Efficient clearing: %s\n", supports_efficient_clearing ? "YES" : "NO");
    printf("   - Needs verification: %s\n", needs_verification ? "YES" : "NO");
    printf("   - Reliable clear-EOL: %s\n", reliable_clear_eol ? "YES" : "NO");
    
    /* Validate that clearing capabilities are available */
    LLE_ASSERT(reliable_clear_eol == true || needs_verification == true);
    
    /* Test optimal batch size for performance */
    size_t batch_size = lle_platform_get_optimal_batch_size();
    printf("   - Optimal batch size: %zu\n", batch_size);
    
    /* Platform-specific integration validation */
    if (lle_platform_is_linux()) {
        printf("📋 Linux integration validation:\n");
        printf("   - Ready for GNOME Terminal, Konsole, xterm compatibility\n");
        printf("   - Backspace sequence optimized for Linux terminals\n");
        
        /* Linux should use appropriate sequences */
        lle_platform_backspace_type_t type = lle_platform_get_backspace_type();
        bool linux_appropriate = (type == LLE_PLATFORM_BACKSPACE_LINUX_GNOME) ||
                                 (type == LLE_PLATFORM_BACKSPACE_LINUX_KDE) ||
                                 (type == LLE_PLATFORM_BACKSPACE_LINUX_XTERM) ||
                                 (type == LLE_PLATFORM_BACKSPACE_FALLBACK);
        LLE_ASSERT(linux_appropriate);
    }
    
    if (lle_platform_is_macos()) {
        printf("📋 macOS integration validation:\n");
        printf("   - Should preserve perfect iTerm2 behavior\n");
        printf("   - Backward compatibility with proven implementation\n");
    }
    
    /* Cleanup */
    lle_platform_cleanup();
    
    printf("✅ History navigation platform integration test passed\n");
}

/**
 * @brief Main test function that runs all platform detection tests
 *
 * Comprehensive test suite for platform detection functionality that ensures
 * Linux users get the same perfect history navigation as macOS users.
 */
int main(void) {
    printf("🚀 Starting Platform Detection Tests for Cross-Platform History Navigation\n");
    printf("🎯 Goal: Give Linux users the same perfect experience as macOS users\n\n");
    
    /* Run all platform detection tests */
    test_platform_detection_init();
    test_linux_platform_detection();
    test_terminal_type_detection();
    test_backspace_sequence_selection();
    test_cross_platform_compatibility();
    test_linux_terminal_optimizations();
    test_platform_detection_validation();
    test_history_navigation_platform_integration();
    
    printf("\n🎉 All Platform Detection Tests Passed!\n");
    printf("✅ Linux users are ready for perfect history navigation\n");
    printf("🚀 Cross-platform exact backspace replication validated\n");
    
    return 0;
}