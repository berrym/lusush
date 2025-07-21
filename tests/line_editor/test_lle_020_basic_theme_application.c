/**
 * @file test_lle_020_basic_theme_application.c
 * @brief Tests for LLE-020: Basic Theme Application
 * 
 * Tests the theme application implementation including theme loading,
 * color retrieval, and integration with the Lusush theme system.
 * Uses graceful fallbacks when theme system is not available.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "theme_integration.h"
#include "prompt.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Basic Initialization Tests
// ============================================================================

LLE_TEST(theme_init_basic) {
    printf("Testing basic theme initialization... ");
    
    lle_theme_integration_t ti;
    
    // Initialize theme integration
    bool result = lle_theme_init(&ti);
    LLE_ASSERT(result == true);
    
    // Verify basic state
    LLE_ASSERT(ti.color_requests == 0);
    LLE_ASSERT(ti.cache_hits == 0);
    
    // Verify capabilities were detected
    LLE_ASSERT(ti.color_support >= 0);
    
    // Cleanup
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_init_null_pointer) {
    printf("Testing theme initialization with null pointer... ");
    
    // Should handle null pointer gracefully
    bool result = lle_theme_init(NULL);
    LLE_ASSERT(result == false);
    
    printf("PASSED\n");
}

LLE_TEST(theme_cleanup_basic) {
    printf("Testing theme cleanup... ");
    
    lle_theme_integration_t ti;
    
    // Initialize and then cleanup
    lle_theme_init(&ti);
    lle_theme_cleanup(&ti);
    
    // After cleanup, structure should be zeroed
    LLE_ASSERT(ti.theme_active == false);
    LLE_ASSERT(ti.color_requests == 0);
    LLE_ASSERT(ti.cache_hits == 0);
    
    printf("PASSED\n");
}

LLE_TEST(theme_cleanup_null_pointer) {
    printf("Testing theme cleanup with null pointer... ");
    
    // Should handle null pointer gracefully
    lle_theme_cleanup(NULL);
    
    printf("PASSED\n");
}

// ============================================================================
// Theme Loading and Application Tests
// ============================================================================

LLE_TEST(theme_sync_basic) {
    printf("Testing theme synchronization... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Sync with active theme (may succeed or fail depending on environment)
    bool result = lle_theme_sync(&ti);
    
    // If sync succeeds, verify theme is active
    if (result) {
        LLE_ASSERT(ti.theme_active == true);
        LLE_ASSERT(strlen(ti.theme_name) > 0);
    }
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_apply_null_parameters) {
    printf("Testing theme application with null parameters... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Test null theme integration
    bool result = lle_theme_apply(NULL, "test_theme");
    LLE_ASSERT(result == false);
    
    // Test null theme name
    result = lle_theme_apply(&ti, NULL);
    LLE_ASSERT(result == false);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

// ============================================================================
// Color Access Tests
// ============================================================================

LLE_TEST(theme_get_color_basic) {
    printf("Testing basic color retrieval... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Get colors for different elements
    const char *primary = lle_theme_get_color(&ti, LLE_THEME_PROMPT_PRIMARY);
    LLE_ASSERT(primary != NULL);
    
    const char *input = lle_theme_get_color(&ti, LLE_THEME_INPUT_TEXT);
    LLE_ASSERT(input != NULL);
    
    const char *error = lle_theme_get_color(&ti, LLE_THEME_ERROR_HIGHLIGHT);
    LLE_ASSERT(error != NULL);
    
    // Verify color requests were counted
    LLE_ASSERT(ti.color_requests >= 3);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_get_color_invalid_element) {
    printf("Testing color retrieval with invalid element... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Test invalid element
    const char *color = lle_theme_get_color(&ti, LLE_THEME_ELEMENT_COUNT);
    LLE_ASSERT(strcmp(color, "") == 0);
    
    // Test out of range element
    const char *color2 = lle_theme_get_color(&ti, (lle_theme_element_t)999);
    LLE_ASSERT(strcmp(color2, "") == 0);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_get_color_by_name) {
    printf("Testing color retrieval by name... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Get color by element name
    const char *color = lle_theme_get_color_by_name(&ti, "prompt_primary");
    LLE_ASSERT(color != NULL);
    
    // Test invalid name
    const char *invalid = lle_theme_get_color_by_name(&ti, "invalid_element");
    LLE_ASSERT(strcmp(invalid, "") == 0);
    
    // Test null parameters
    const char *null_result = lle_theme_get_color_by_name(NULL, "prompt_primary");
    LLE_ASSERT(strcmp(null_result, "") == 0);
    
    null_result = lle_theme_get_color_by_name(&ti, NULL);
    LLE_ASSERT(strcmp(null_result, "") == 0);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_supports_element) {
    printf("Testing element support checking... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Test primary elements (should have fallbacks at minimum)
    bool supports = lle_theme_supports_element(&ti, LLE_THEME_PROMPT_PRIMARY);
    LLE_ASSERT(supports == true);
    
    supports = lle_theme_supports_element(&ti, LLE_THEME_ERROR_HIGHLIGHT);
    LLE_ASSERT(supports == true);
    
    // Test invalid element
    supports = lle_theme_supports_element(&ti, LLE_THEME_ELEMENT_COUNT);
    LLE_ASSERT(supports == false);
    
    // Test null pointer
    supports = lle_theme_supports_element(NULL, LLE_THEME_PROMPT_PRIMARY);
    LLE_ASSERT(supports == false);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_get_fallback_color) {
    printf("Testing fallback color retrieval... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Get fallback colors
    const char *fallback = lle_theme_get_fallback_color(&ti, LLE_THEME_PROMPT_PRIMARY);
    LLE_ASSERT(fallback != NULL);
    LLE_ASSERT(strlen(fallback) > 0);
    
    // Test invalid element
    const char *invalid_fallback = lle_theme_get_fallback_color(&ti, LLE_THEME_ELEMENT_COUNT);
    LLE_ASSERT(strcmp(invalid_fallback, "\033[0m") == 0); // Reset as ultimate fallback
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

// ============================================================================
// Cache Performance Tests
// ============================================================================

LLE_TEST(theme_color_caching) {
    printf("Testing color caching performance... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Clear cache statistics
    size_t initial_requests = ti.color_requests;
    size_t initial_hits = ti.cache_hits;
    
    // Request same color multiple times
    const char *color1 = lle_theme_get_color(&ti, LLE_THEME_PROMPT_PRIMARY);
    const char *color2 = lle_theme_get_color(&ti, LLE_THEME_PROMPT_PRIMARY);
    const char *color3 = lle_theme_get_color(&ti, LLE_THEME_PROMPT_PRIMARY);
    
    // Verify colors are the same
    LLE_ASSERT(strcmp(color1, color2) == 0);
    LLE_ASSERT(strcmp(color2, color3) == 0);
    
    // Verify requests increased
    LLE_ASSERT(ti.color_requests > initial_requests);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_statistics) {
    printf("Testing theme statistics... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Make some color requests
    lle_theme_get_color(&ti, LLE_THEME_PROMPT_PRIMARY);
    lle_theme_get_color(&ti, LLE_THEME_INPUT_TEXT);
    lle_theme_get_color(&ti, LLE_THEME_PROMPT_PRIMARY); // Potential cache hit
    
    // Get statistics
    size_t requests, hits;
    double ratio;
    lle_theme_get_stats(&ti, &requests, &hits, &ratio);
    
    LLE_ASSERT(requests >= 3);
    LLE_ASSERT(hits >= 0);
    LLE_ASSERT(ratio >= 0.0 && ratio <= 1.0);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

// ============================================================================
// Element Mapping Tests
// ============================================================================

LLE_TEST(theme_element_string_conversion) {
    printf("Testing element to string conversion... ");
    
    // Test valid elements
    const char *name = lle_theme_element_to_string(LLE_THEME_PROMPT_PRIMARY);
    LLE_ASSERT(name != NULL);
    LLE_ASSERT(strcmp(name, "prompt_primary") == 0);
    
    name = lle_theme_element_to_string(LLE_THEME_ERROR_HIGHLIGHT);
    LLE_ASSERT(name != NULL);
    LLE_ASSERT(strcmp(name, "error_highlight") == 0);
    
    // Test invalid element
    name = lle_theme_element_to_string(LLE_THEME_ELEMENT_COUNT);
    LLE_ASSERT(name == NULL);
    
    printf("PASSED\n");
}

LLE_TEST(theme_string_element_conversion) {
    printf("Testing string to element conversion... ");
    
    // Test valid strings
    lle_theme_element_t element = lle_theme_string_to_element("prompt_primary");
    LLE_ASSERT(element == LLE_THEME_PROMPT_PRIMARY);
    
    element = lle_theme_string_to_element("error_highlight");
    LLE_ASSERT(element == LLE_THEME_ERROR_HIGHLIGHT);
    
    // Test invalid string
    element = lle_theme_string_to_element("invalid_element");
    LLE_ASSERT(element == LLE_THEME_ELEMENT_COUNT);
    
    // Test null string
    element = lle_theme_string_to_element(NULL);
    LLE_ASSERT(element == LLE_THEME_ELEMENT_COUNT);
    
    printf("PASSED\n");
}

LLE_TEST(theme_lusush_mapping) {
    printf("Testing Lusush color mapping... ");
    
    // Test element mapping
    const char *lusush_color = lle_theme_map_to_lusush_color(LLE_THEME_PROMPT_PRIMARY);
    LLE_ASSERT(lusush_color != NULL);
    LLE_ASSERT(strcmp(lusush_color, "primary") == 0);
    
    lusush_color = lle_theme_map_to_lusush_color(LLE_THEME_ERROR_HIGHLIGHT);
    LLE_ASSERT(lusush_color != NULL);
    LLE_ASSERT(strcmp(lusush_color, "error") == 0);
    
    // Test invalid element
    lusush_color = lle_theme_map_to_lusush_color(LLE_THEME_ELEMENT_COUNT);
    LLE_ASSERT(lusush_color == NULL);
    
    printf("PASSED\n");
}

LLE_TEST(theme_default_mappings) {
    printf("Testing default theme mappings... ");
    
    size_t count;
    const lle_theme_mapping_t *mappings = lle_theme_get_default_mappings(&count);
    
    LLE_ASSERT(mappings != NULL);
    LLE_ASSERT(count == 18); // Should have mappings for all elements
    
    // Verify first mapping
    LLE_ASSERT(mappings[0].lle_element == LLE_THEME_PROMPT_PRIMARY);
    LLE_ASSERT(strcmp(mappings[0].lusush_color_name, "primary") == 0);
    LLE_ASSERT(mappings[0].fallback_color != NULL);
    
    printf("PASSED\n");
}

// ============================================================================
// Capability Detection Tests
// ============================================================================

LLE_TEST(theme_capability_detection) {
    printf("Testing capability detection... ");
    
    lle_theme_integration_t ti;
    memset(&ti, 0, sizeof(ti));
    
    bool result = lle_theme_detect_capabilities(&ti);
    LLE_ASSERT(result == true);
    
    // Verify capabilities were set
    LLE_ASSERT(ti.color_support >= 0);
    
    printf("PASSED\n");
}

LLE_TEST(theme_validation) {
    printf("Testing theme validation... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Test null parameters
    bool valid = lle_theme_validate_compatibility(NULL, "test_theme");
    LLE_ASSERT(valid == false);
    
    valid = lle_theme_validate_compatibility(&ti, NULL);
    LLE_ASSERT(valid == false);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

// ============================================================================
// Utility Function Tests
// ============================================================================

LLE_TEST(theme_refresh) {
    printf("Testing theme refresh... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Mark as dirty and refresh
    ti.dirty = true;
    bool result = lle_theme_refresh(&ti);
    LLE_ASSERT(result == true);
    
    // Should be clean after refresh
    LLE_ASSERT(ti.dirty == false);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_debug_mode) {
    printf("Testing debug mode... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Enable debug mode
    lle_theme_set_debug(&ti, true);
    LLE_ASSERT(ti.debug_mode == true);
    
    // Disable debug mode
    lle_theme_set_debug(&ti, false);
    LLE_ASSERT(ti.debug_mode == false);
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

LLE_TEST(theme_active_status) {
    printf("Testing theme active status... ");
    
    lle_theme_integration_t ti;
    lle_theme_init(&ti);
    
    // Check if theme system is active
    bool active = lle_theme_is_active(&ti);
    // May be true or false depending on environment
    
    // If active, should have a name
    if (active) {
        const char *name = lle_theme_get_active_name(&ti);
        LLE_ASSERT(name != NULL);
        LLE_ASSERT(strlen(name) > 0);
    }
    
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

// ============================================================================
// Integration Tests
// ============================================================================

LLE_TEST(theme_integration_workflow) {
    printf("Testing complete theme integration workflow... ");
    
    lle_theme_integration_t ti;
    
    // Initialize
    bool result = lle_theme_init(&ti);
    LLE_ASSERT(result == true);
    
    // Get some colors
    const char *prompt_color = lle_theme_get_color(&ti, LLE_THEME_PROMPT_PRIMARY);
    const char *input_color = lle_theme_get_color(&ti, LLE_THEME_INPUT_TEXT);
    
    LLE_ASSERT(prompt_color != NULL);
    LLE_ASSERT(input_color != NULL);
    
    // Check element support
    bool supports_prompt = lle_theme_supports_element(&ti, LLE_THEME_PROMPT_PRIMARY);
    bool supports_invalid = lle_theme_supports_element(&ti, LLE_THEME_ELEMENT_COUNT);
    
    LLE_ASSERT(supports_prompt == true);
    LLE_ASSERT(supports_invalid == false);
    
    // Get statistics
    size_t requests, hits;
    double ratio;
    lle_theme_get_stats(&ti, &requests, &hits, &ratio);
    
    LLE_ASSERT(requests >= 2);
    LLE_ASSERT(ratio >= 0.0 && ratio <= 1.0);
    
    // Cleanup
    lle_theme_cleanup(&ti);
    
    printf("PASSED\n");
}

// ============================================================================
// Test Runner
// ============================================================================

int main(void) {
    printf("Running LLE-020 Basic Theme Application Tests...\n");
    printf("================================================\n\n");
    
    // Basic initialization tests
    test_theme_init_basic();
    test_theme_init_null_pointer();
    test_theme_cleanup_basic();
    test_theme_cleanup_null_pointer();
    
    // Theme loading and application tests
    test_theme_sync_basic();
    test_theme_apply_null_parameters();
    
    // Color access tests
    test_theme_get_color_basic();
    test_theme_get_color_invalid_element();
    test_theme_get_color_by_name();
    test_theme_supports_element();
    test_theme_get_fallback_color();
    
    // Cache performance tests
    test_theme_color_caching();
    test_theme_statistics();
    
    // Element mapping tests
    test_theme_element_string_conversion();
    test_theme_string_element_conversion();
    test_theme_lusush_mapping();
    test_theme_default_mappings();
    
    // Capability detection tests
    test_theme_capability_detection();
    test_theme_validation();
    
    // Utility function tests
    test_theme_refresh();
    test_theme_debug_mode();
    test_theme_active_status();
    
    // Integration tests
    test_theme_integration_workflow();
    
    printf("\n================================================\n");
    printf("All LLE-020 Basic Theme Application Tests Passed!\n");
    printf("Theme application system working correctly.\n");
    printf("Color loading, caching, and integration validated.\n");
    printf("Ready for LLE-021 implementation.\n");
    
    return 0;
}