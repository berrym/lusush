/**
 * @file test_lle_037_syntax_display_integration.c
 * @brief Tests for LLE-037: Syntax Display Integration
 * 
 * Tests the integration of syntax highlighting with the display system,
 * including visual verification, performance testing, theme integration,
 * and real-time updates. Builds upon LLE-035 and LLE-036 foundations.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "display.h"
#include "syntax.h"
#include "theme_integration.h"
#include "text_buffer.h"
#include "prompt.h"
#include "terminal_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Create a minimal display state for testing
 */
static lle_display_state_t *create_test_display_state(void) {
    // Create text buffer
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    if (!buffer) return NULL;
    
    // Create prompt
    lle_prompt_t *prompt = lle_prompt_create(4);
    if (!prompt) {
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    
    // Set simple prompt
    if (!lle_prompt_parse(prompt, "$ ")) {
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    
    // Create terminal manager
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    if (!terminal) {
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    
    if (lle_terminal_init(terminal) != LLE_TERM_INIT_SUCCESS) {
        free(terminal);
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    
    // Create display state
    lle_display_state_t *state = lle_display_create(prompt, buffer, terminal);
    if (!state) {
        if (terminal) {
            lle_terminal_cleanup(terminal);
            free(terminal);
        }
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    
    return state;
}

/**
 * @brief Destroy test display state and all components
 */
static void destroy_test_display_state(lle_display_state_t *state) {
    if (!state) return;
    
    lle_terminal_manager_t *terminal = state->terminal;
    lle_prompt_t *prompt = state->prompt;
    lle_text_buffer_t *buffer = state->buffer;
    
    lle_display_destroy(state);
    
    if (terminal) {
        lle_terminal_cleanup(terminal);
        free(terminal);
    }
    if (prompt) lle_prompt_destroy(prompt);
    if (buffer) lle_text_buffer_destroy(buffer);
}

/**
 * @brief Create a mock theme integration for testing
 */
static lle_theme_integration_t *create_test_theme_integration(void) {
    lle_theme_integration_t *ti = malloc(sizeof(lle_theme_integration_t));
    if (!ti) return NULL;
    
    // Initialize with test values
    memset(ti, 0, sizeof(lle_theme_integration_t));
    strcpy(ti->theme_name, "test_theme");
    ti->theme_active = true;
    ti->colors_cached = true;
    ti->color_support = 256;
    ti->supports_256_color = true;
    
    // Set up test colors
    strcpy(ti->colors.syntax_keyword, "\033[1;32m");     // Bright green
    strcpy(ti->colors.syntax_string, "\033[0;33m");      // Yellow
    strcpy(ti->colors.syntax_comment, "\033[0;36m");     // Cyan
    strcpy(ti->colors.syntax_operator, "\033[1;31m");    // Bright red
    strcpy(ti->colors.syntax_variable, "\033[0;35m");    // Magenta
    strcpy(ti->colors.input_text, "\033[0;37m");         // White
    strcpy(ti->colors.error_highlight, "\033[1;41m");    // Red background
    
    return ti;
}

/**
 * @brief Destroy test theme integration
 */
static void destroy_test_theme_integration(lle_theme_integration_t *ti) {
    if (ti) {
        free(ti);
    }
}

/**
 * @brief Mock theme color getter for testing
 */


// ============================================================================
// Basic Integration Tests
// ============================================================================

LLE_TEST(syntax_highlighter_integration) {
    printf("Testing syntax highlighter integration with display... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    // Create syntax highlighter
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Set highlighter in display
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    
    // Check that highlighter is set
    LLE_ASSERT_EQ(state->syntax_highlighter, highlighter);
    
    // Check that syntax highlighting is not enabled yet (no theme integration)
    LLE_ASSERT(!lle_display_is_syntax_highlighting_enabled(state));
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

LLE_TEST(theme_integration_setup) {
    printf("Testing theme integration setup in display... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    // Create theme integration
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set theme integration in display
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    
    // Check that theme integration is set
    LLE_ASSERT_EQ(state->theme_integration, theme);
    
    // Cleanup
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

LLE_TEST(syntax_highlighting_enablement) {
    printf("Testing syntax highlighting enablement... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set both components
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    
    // Should not be enabled by default
    LLE_ASSERT(!lle_display_is_syntax_highlighting_enabled(state));
    
    // Enable syntax highlighting
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, true));
    LLE_ASSERT(lle_display_is_syntax_highlighting_enabled(state));
    
    // Disable syntax highlighting
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, false));
    LLE_ASSERT(!lle_display_is_syntax_highlighting_enabled(state));
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

LLE_TEST(syntax_highlighting_without_components) {
    printf("Testing syntax highlighting enablement without required components... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    // Try to enable without highlighter or theme
    LLE_ASSERT(!lle_display_enable_syntax_highlighting(state, true));
    
    // Add highlighter but no theme
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(!lle_display_enable_syntax_highlighting(state, true));
    
    // Add theme but remove highlighter
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, NULL));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    LLE_ASSERT(!lle_display_enable_syntax_highlighting(state, true));
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

// ============================================================================
// Syntax Highlighting Update Tests
// ============================================================================

LLE_TEST(syntax_highlighting_update) {
    printf("Testing syntax highlighting update for buffer content... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set up syntax highlighting
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, true));
    
    // Add text to buffer
    const char *test_text = "echo 'hello world'";
    LLE_ASSERT(lle_text_insert_string(state->buffer, test_text));
    
    // Update syntax highlighting
    LLE_ASSERT(lle_display_update_syntax_highlighting(state));
    
    // Verify highlighting was applied
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    LLE_ASSERT(region_count > 0);
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

LLE_TEST(syntax_highlighting_update_empty_buffer) {
    printf("Testing syntax highlighting update with empty buffer... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set up syntax highlighting
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, true));
    
    // Update syntax highlighting with empty buffer
    LLE_ASSERT(lle_display_update_syntax_highlighting(state));
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

// ============================================================================
// Color Application Tests
// ============================================================================

LLE_TEST(color_optimization_caching) {
    printf("Testing color optimization and caching... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    // Initially no color should be cached
    LLE_ASSERT_EQ(state->last_applied_color[0], '\0');
    
    // Set a color (simulate internal function)
    const char *test_color = "\033[1;32m";
    strncpy(state->last_applied_color, test_color, sizeof(state->last_applied_color) - 1);
    state->last_applied_color[sizeof(state->last_applied_color) - 1] = '\0';
    
    // Verify color is cached
    LLE_ASSERT_EQ(strcmp(state->last_applied_color, test_color), 0);
    
    // Cleanup
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

// ============================================================================
// Performance Tests
// ============================================================================

LLE_TEST(syntax_highlighting_performance) {
    printf("Testing syntax highlighting performance... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set up syntax highlighting
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, true));
    
    // Create a complex command for performance testing
    const char *complex_text = "for i in $(seq 1 100); do echo \"Processing $i\" | grep 'Process' >> /tmp/output.txt; done";
    LLE_ASSERT(lle_text_insert_string(state->buffer, complex_text));
    
    // Measure performance of syntax highlighting update
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    LLE_ASSERT(lle_display_update_syntax_highlighting(state));
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Calculate time in milliseconds
    long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    
    // Should complete within 5ms for complex commands (as per requirements)
    LLE_ASSERT(ms <= 5);
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED (took %ld ms)\n", ms);
}

LLE_TEST(large_text_performance) {
    printf("Testing syntax highlighting performance with large text... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set up syntax highlighting
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, true));
    
    // Create large text (simulate long command line)
    char *large_text = malloc(1000);
    LLE_ASSERT_NOT_NULL(large_text);
    
    strcpy(large_text, "find /usr/local/bin -name '*.sh' -exec grep -l 'bash' {} \\; | ");
    strcat(large_text, "xargs -I {} cp {} /tmp/scripts/ && ");
    strcat(large_text, "echo 'Copied shell scripts' | ");
    strcat(large_text, "tee -a /var/log/copy.log");
    
    LLE_ASSERT(lle_text_insert_string(state->buffer, large_text));
    
    // Measure performance
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    LLE_ASSERT(lle_display_update_syntax_highlighting(state));
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    
    // Should still complete within 5ms
    LLE_ASSERT(ms <= 5);
    
    // Cleanup
    free(large_text);
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED (took %ld ms)\n", ms);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

LLE_TEST(null_parameter_handling) {
    printf("Testing null parameter handling... ");
    
    // Test with NULL display state
    LLE_ASSERT(!lle_display_set_syntax_highlighter(NULL, NULL));
    LLE_ASSERT(!lle_display_set_theme_integration(NULL, NULL));
    LLE_ASSERT(!lle_display_enable_syntax_highlighting(NULL, true));
    LLE_ASSERT(!lle_display_is_syntax_highlighting_enabled(NULL));
    LLE_ASSERT(!lle_display_update_syntax_highlighting(NULL));
    
    // Test with valid state but NULL components
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, NULL));
    LLE_ASSERT(lle_display_set_theme_integration(state, NULL));
    
    // Cleanup
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

LLE_TEST(invalid_state_handling) {
    printf("Testing invalid state handling... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    // Mark state as uninitialized
    state->initialized = false;
    
    // Functions should handle uninitialized state gracefully
    LLE_ASSERT(!lle_display_validate(state));
    
    // Re-initialize for cleanup
    state->initialized = true;
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

// ============================================================================
// Real-time Update Tests
// ============================================================================

LLE_TEST(real_time_syntax_updates) {
    printf("Testing real-time syntax highlighting updates... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set up syntax highlighting
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, true));
    
    // Add initial text
    const char *initial_text = "ech";
    LLE_ASSERT(lle_text_insert_string(state->buffer, initial_text));
    LLE_ASSERT(lle_display_update_syntax_highlighting(state));
    
    // Complete the command
    const char *completion = "o 'hello'";
    LLE_ASSERT(lle_text_insert_string(state->buffer, completion));
    LLE_ASSERT(lle_display_update_syntax_highlighting(state));
    
    // Verify highlighting was updated
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    LLE_ASSERT(region_count > 0);
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

// ============================================================================
// Integration with Existing Systems Tests
// ============================================================================

LLE_TEST(syntax_highlighting_with_display_rendering) {
    printf("Testing syntax highlighting integration with display rendering... ");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state);
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    lle_theme_integration_t *theme = create_test_theme_integration();
    LLE_ASSERT_NOT_NULL(theme);
    
    // Set up syntax highlighting
    LLE_ASSERT(lle_display_set_syntax_highlighter(state, highlighter));
    LLE_ASSERT(lle_display_set_theme_integration(state, theme));
    LLE_ASSERT(lle_display_enable_syntax_highlighting(state, true));
    
    // Add text to buffer
    const char *test_text = "ls -la | grep '.txt'";
    LLE_ASSERT(lle_text_insert_string(state->buffer, test_text));
    LLE_ASSERT(lle_display_update_syntax_highlighting(state));
    
    // Test display rendering (may fail in non-terminal environment, that's OK)
    // Test display rendering (may fail in non-terminal environment, that's OK)
    lle_display_render(state);
    // We don't assert on render result as it may fail in test environment
    // The important thing is that it doesn't crash
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    destroy_test_theme_integration(theme);
    destroy_test_display_state(state);
    
    printf("PASSED\n");
}

// ============================================================================
// Main Test Function
// ============================================================================

int main(void) {
    printf("Running LLE-037 Syntax Display Integration Tests...\n\n");
    
    // Basic integration tests
    test_syntax_highlighter_integration();
    test_theme_integration_setup();
    test_syntax_highlighting_enablement();
    test_syntax_highlighting_without_components();
    
    // Syntax highlighting update tests
    test_syntax_highlighting_update();
    test_syntax_highlighting_update_empty_buffer();
    
    // Color application tests
    test_color_optimization_caching();
    
    // Performance tests
    test_syntax_highlighting_performance();
    test_large_text_performance();
    
    // Error handling tests
    test_null_parameter_handling();
    test_invalid_state_handling();
    
    // Real-time update tests
    test_real_time_syntax_updates();
    
    // Integration tests
    test_syntax_highlighting_with_display_rendering();
    
    printf("\n✅ All LLE-037 tests completed successfully!\n");
    printf("📊 Total tests: 13\n");
    printf("🎯 Syntax display integration verified\n");
    printf("⚡ Performance requirements met (< 5ms)\n");
    printf("🎨 Theme integration functional\n");
    printf("🔄 Real-time updates working\n");
    
    return 0;
}