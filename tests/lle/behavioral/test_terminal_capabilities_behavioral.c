/**
 * LLE Terminal Capability Detection - Behavioral Validation Tests
 * 
 * Week 1: Terminal Capability Detection Testing
 * 
 * TESTING STANDARDS COMPLIANCE:
 * - These tests validate BEHAVIOR, not just internal state
 * - Tests verify what the system DOES, not just memory values
 * - Manual testing REQUIRED for TTY-specific behavior
 * 
 * Test Categories:
 * 1. Automated tests (verify detection logic)
 * 2. TTY tests (must run in real terminal)
 * 3. Performance tests (verify <50ms requirement)
 * 4. Environment simulation tests (verify different terminals)
 */

#include <lle/terminal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/* Test result tracking */
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_results_t;

static test_results_t results = {0, 0, 0};

/* Test helper macros */
#define TEST_START(name) \
    do { \
        printf("TEST: %s ... ", name); \
        fflush(stdout); \
        results.tests_run++; \
    } while (0)

#define TEST_PASS() \
    do { \
        printf("PASS\n"); \
        results.tests_passed++; \
    } while (0)

#define TEST_FAIL(msg) \
    do { \
        printf("FAIL - %s\n", msg); \
        results.tests_failed++; \
    } while (0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while (0)

/* Forward declarations */
static void test_basic_detection(void);
static void test_performance_requirement(void);
static void test_tty_detection(void);
static void test_terminal_geometry(void);
static void test_color_detection_truecolor(void);
static void test_color_detection_256(void);
static void test_color_detection_conservative(void);
static void test_adaptive_enhanced_terminal(void);
static void test_adaptive_ai_environment(void);
static void test_terminal_type_classification(void);
static void test_non_tty_fallback(void);
static void test_capabilities_consistency(void);
static void print_test_summary(void);

/**
 * Main test runner
 */
int main(void)
{
    printf("=== LLE Terminal Capability Detection - Behavioral Tests ===\n");
    printf("Week 1 Implementation Validation\n\n");
    
    /* Check if running in TTY */
    if (!isatty(STDOUT_FILENO)) {
        printf("NOTE: Not running in TTY - some tests will use conservative assumptions\n\n");
    }
    
    /* Run all tests */
    test_basic_detection();
    test_performance_requirement();
    test_tty_detection();
    test_terminal_geometry();
    test_color_detection_truecolor();
    test_color_detection_256();
    test_color_detection_conservative();
    test_adaptive_enhanced_terminal();
    test_adaptive_ai_environment();
    test_terminal_type_classification();
    test_non_tty_fallback();
    test_capabilities_consistency();
    
    /* Print summary */
    print_test_summary();
    
    /* Return appropriate exit code */
    return (results.tests_failed == 0) ? 0 : 1;
}

/**
 * Test 1: Basic detection succeeds
 * 
 * VALIDATES: Function returns success and allocates capabilities
 */
static void test_basic_detection(void)
{
    TEST_START("Basic detection succeeds");
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    ASSERT_TRUE(caps->detection_complete, "Detection should be marked complete");
    
    lle_terminal_capabilities_destroy(caps);
    TEST_PASS();
}

/**
 * Test 2: Performance requirement (<50ms)
 * 
 * VALIDATES: Detection completes in less than 50 milliseconds
 * CRITICAL: This is a MANDATORY requirement from Week 1 spec
 */
static void test_performance_requirement(void)
{
    TEST_START("Performance requirement (<50ms)");
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* CRITICAL TEST: Must complete in <50ms (50,000 microseconds) */
    ASSERT_TRUE(caps->detection_time_us < 50000, 
                "Detection must complete in <50ms");
    
    printf("(took %lu us) ", (unsigned long)caps->detection_time_us);
    
    lle_terminal_capabilities_destroy(caps);
    TEST_PASS();
}

/**
 * Test 3: TTY detection matches system
 * 
 * VALIDATES: is_tty flag matches actual TTY status
 */
static void test_tty_detection(void)
{
    TEST_START("TTY detection matches system");
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* Verify TTY detection matches system isatty() */
    bool system_is_tty = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    ASSERT_TRUE(caps->is_tty == system_is_tty, 
                "is_tty should match system isatty()");
    
    lle_terminal_capabilities_destroy(caps);
    TEST_PASS();
}

/**
 * Test 4: Terminal geometry is reasonable
 * 
 * VALIDATES: Terminal size is detected and within reasonable bounds
 */
static void test_terminal_geometry(void)
{
    TEST_START("Terminal geometry is reasonable");
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* Verify width is reasonable (between 20 and 500 columns) */
    ASSERT_TRUE(caps->terminal_width >= 20 && caps->terminal_width <= 500,
                "Terminal width should be reasonable");
    
    /* Verify height is reasonable (between 10 and 200 rows) */
    ASSERT_TRUE(caps->terminal_height >= 10 && caps->terminal_height <= 200,
                "Terminal height should be reasonable");
    
    printf("(%ux%u) ", caps->terminal_width, caps->terminal_height);
    
    lle_terminal_capabilities_destroy(caps);
    TEST_PASS();
}

/**
 * Test 5: Truecolor detection from COLORTERM
 * 
 * VALIDATES: COLORTERM=truecolor enables truecolor support
 */
static void test_color_detection_truecolor(void)
{
    TEST_START("Truecolor detection from COLORTERM");
    
    /* Save original environment */
    char *original_colorterm = getenv("COLORTERM");
    char *saved_colorterm = original_colorterm ? strdup(original_colorterm) : NULL;
    
    /* Set COLORTERM=truecolor */
    setenv("COLORTERM", "truecolor", 1);
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* If we're in a TTY, verify truecolor was detected */
    if (caps->is_tty) {
        ASSERT_TRUE(caps->supports_truecolor, 
                    "COLORTERM=truecolor should enable truecolor");
        ASSERT_TRUE(caps->color_depth == LLE_COLOR_DEPTH_TRUECOLOR,
                    "Color depth should be truecolor");
    }
    
    lle_terminal_capabilities_destroy(caps);
    
    /* Restore original environment */
    if (saved_colorterm) {
        setenv("COLORTERM", saved_colorterm, 1);
        free(saved_colorterm);
    } else {
        unsetenv("COLORTERM");
    }
    
    TEST_PASS();
}

/**
 * Test 6: 256-color detection from TERM
 * 
 * VALIDATES: TERM=*-256color enables 256-color support
 */
static void test_color_detection_256(void)
{
    TEST_START("256-color detection from TERM");
    
    /* Save original TERM */
    char *original_term = getenv("TERM");
    char *saved_term = original_term ? strdup(original_term) : NULL;
    
    /* Clear COLORTERM to isolate TERM test */
    char *original_colorterm = getenv("COLORTERM");
    char *saved_colorterm = original_colorterm ? strdup(original_colorterm) : NULL;
    unsetenv("COLORTERM");
    
    /* Set TERM=xterm-256color */
    setenv("TERM", "xterm-256color", 1);
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* If in TTY, verify 256-color was detected */
    if (caps->is_tty) {
        ASSERT_TRUE(caps->supports_256_colors,
                    "TERM=xterm-256color should enable 256 colors");
        ASSERT_TRUE(caps->color_depth >= LLE_COLOR_DEPTH_256,
                    "Color depth should be at least 256");
    }
    
    lle_terminal_capabilities_destroy(caps);
    
    /* Restore environment */
    if (saved_term) {
        setenv("TERM", saved_term, 1);
        free(saved_term);
    }
    if (saved_colorterm) {
        setenv("COLORTERM", saved_colorterm, 1);
        free(saved_colorterm);
    }
    
    TEST_PASS();
}

/**
 * Test 7: Conservative fallback for unknown terminals
 * 
 * VALIDATES: Unknown terminals get conservative capabilities
 */
static void test_color_detection_conservative(void)
{
    TEST_START("Conservative fallback for unknown terminals");
    
    /* Save original environment */
    char *original_term = getenv("TERM");
    char *saved_term = original_term ? strdup(original_term) : NULL;
    
    /* Set unknown TERM */
    setenv("TERM", "unknown", 1);
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* Unknown terminal should be classified as UNKNOWN */
    ASSERT_TRUE(caps->terminal_type == LLE_TERMINAL_UNKNOWN,
                "Unknown TERM should be classified as UNKNOWN");
    
    /* If in TTY, unknown terminal should have conservative capabilities */
    if (caps->is_tty) {
        ASSERT_TRUE(caps->color_depth == LLE_COLOR_DEPTH_NONE,
                    "Unknown terminal should have no color by default");
    }
    
    lle_terminal_capabilities_destroy(caps);
    
    /* Restore environment */
    if (saved_term) {
        setenv("TERM", saved_term, 1);
        free(saved_term);
    }
    
    TEST_PASS();
}

/**
 * Test 8: Adaptive enhanced terminal detection (Spec 26)
 * 
 * VALIDATES: Enhanced editor terminals are detected
 */
static void test_adaptive_enhanced_terminal(void)
{
    TEST_START("Adaptive enhanced terminal detection (Spec 26)");
    
    /* Save original environment */
    char *original_term_program = getenv("TERM_PROGRAM");
    char *saved_term_program = original_term_program ? strdup(original_term_program) : NULL;
    
    /* Set TERM_PROGRAM=vscode */
    setenv("TERM_PROGRAM", "vscode", 1);
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* Verify enhanced terminal detected */
    ASSERT_TRUE(caps->is_enhanced_terminal,
                "TERM_PROGRAM=vscode should be detected as enhanced terminal");
    
    lle_terminal_capabilities_destroy(caps);
    
    /* Restore environment */
    if (saved_term_program) {
        setenv("TERM_PROGRAM", saved_term_program, 1);
        free(saved_term_program);
    } else {
        unsetenv("TERM_PROGRAM");
    }
    
    TEST_PASS();
}

/**
 * Test 9: Adaptive AI environment detection (Spec 26)
 * 
 * VALIDATES: AI assistant environments are detected
 */
static void test_adaptive_ai_environment(void)
{
    TEST_START("Adaptive AI environment detection (Spec 26)");
    
    /* Save original environment */
    char *original_claude = getenv("CLAUDE_CODE");
    char *saved_claude = original_claude ? strdup(original_claude) : NULL;
    
    /* Set CLAUDE_CODE environment variable */
    setenv("CLAUDE_CODE", "1", 1);
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* Verify AI environment detected */
    ASSERT_TRUE(caps->is_ai_environment,
                "CLAUDE_CODE should be detected as AI environment");
    
    lle_terminal_capabilities_destroy(caps);
    
    /* Restore environment */
    if (saved_claude) {
        setenv("CLAUDE_CODE", saved_claude, 1);
        free(saved_claude);
    } else {
        unsetenv("CLAUDE_CODE");
    }
    
    TEST_PASS();
}

/**
 * Test 10: Terminal type classification
 * 
 * VALIDATES: Known terminals are classified correctly
 */
static void test_terminal_type_classification(void)
{
    TEST_START("Terminal type classification");
    
    /* Test structure: TERM value -> Expected classification */
    struct {
        const char *term;
        lle_terminal_type_t expected_type;
    } test_cases[] = {
        {"xterm", LLE_TERMINAL_XTERM},
        {"xterm-256color", LLE_TERMINAL_XTERM},
        {"screen", LLE_TERMINAL_SCREEN},
        {"screen-256color", LLE_TERMINAL_SCREEN},
        {"tmux", LLE_TERMINAL_TMUX},
        {"tmux-256color", LLE_TERMINAL_TMUX},
        {"alacritty", LLE_TERMINAL_ALACRITTY},
        {"kitty", LLE_TERMINAL_KITTY},
    };
    
    /* Save original TERM */
    char *original_term = getenv("TERM");
    char *saved_term = original_term ? strdup(original_term) : NULL;
    
    /* Test each case */
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        setenv("TERM", test_cases[i].term, 1);
        
        lle_terminal_capabilities_t *caps = NULL;
        lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
        
        if (result != LLE_TERMINAL_SUCCESS || !caps) {
            if (saved_term) {
                setenv("TERM", saved_term, 1);
                free(saved_term);
            }
            TEST_FAIL("Detection failed for test case");
            return;
        }
        
        /* If not in TTY, terminal type should be UNKNOWN (conservative) */
        if (!caps->is_tty) {
            if (caps->terminal_type != LLE_TERMINAL_UNKNOWN) {
                printf("Non-TTY should be UNKNOWN, got %d ", caps->terminal_type);
                lle_terminal_capabilities_destroy(caps);
                if (saved_term) {
                    setenv("TERM", saved_term, 1);
                    free(saved_term);
                }
                TEST_FAIL("Non-TTY should force UNKNOWN");
                return;
            }
        } else {
            /* In TTY, check expected type */
            if (caps->terminal_type != test_cases[i].expected_type) {
                printf("TERM=%s expected %d, got %d ", 
                       test_cases[i].term, 
                       test_cases[i].expected_type,
                       caps->terminal_type);
                lle_terminal_capabilities_destroy(caps);
                if (saved_term) {
                    setenv("TERM", saved_term, 1);
                    free(saved_term);
                }
                TEST_FAIL("Terminal type mismatch");
                return;
            }
        }
        
        lle_terminal_capabilities_destroy(caps);
    }
    
    /* Restore environment */
    if (saved_term) {
        setenv("TERM", saved_term, 1);
        free(saved_term);
    }
    
    TEST_PASS();
}

/**
 * Test 11: Non-TTY fallback behavior
 * 
 * VALIDATES: Non-TTY environments get safe defaults
 * 
 * NOTE: This test cannot be fully automated if running in a TTY.
 * Manual testing required: pipe output to verify non-TTY behavior.
 */
static void test_non_tty_fallback(void)
{
    TEST_START("Non-TTY fallback behavior");
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* If not a TTY, verify conservative fallbacks are used */
    if (!caps->is_tty) {
        ASSERT_TRUE(caps->terminal_width == 80, 
                    "Non-TTY should default to 80 columns");
        ASSERT_TRUE(caps->terminal_height == 24,
                    "Non-TTY should default to 24 rows");
        ASSERT_TRUE(caps->terminal_type == LLE_TERMINAL_UNKNOWN,
                    "Non-TTY should be classified as UNKNOWN");
    }
    
    lle_terminal_capabilities_destroy(caps);
    TEST_PASS();
}

/**
 * Test 12: Capabilities consistency
 * 
 * VALIDATES: Capability flags are logically consistent
 * Example: If supports_truecolor, then supports_256_colors should also be true
 */
static void test_capabilities_consistency(void)
{
    TEST_START("Capabilities consistency");
    
    lle_terminal_capabilities_t *caps = NULL;
    lle_terminal_result_t result = lle_terminal_detect_capabilities(&caps);
    
    ASSERT_TRUE(result == LLE_TERMINAL_SUCCESS, "Detection should succeed");
    ASSERT_NOT_NULL(caps, "Capabilities should be allocated");
    
    /* Consistency check: Truecolor implies 256-color */
    if (caps->supports_truecolor) {
        ASSERT_TRUE(caps->supports_256_colors,
                    "Truecolor support implies 256-color support");
        ASSERT_TRUE(caps->supports_ansi_colors,
                    "Truecolor support implies ANSI color support");
    }
    
    /* Consistency check: 256-color implies ANSI */
    if (caps->supports_256_colors) {
        ASSERT_TRUE(caps->supports_ansi_colors,
                    "256-color support implies ANSI color support");
    }
    
    /* Consistency check: Color depth matches flags */
    if (caps->color_depth == LLE_COLOR_DEPTH_TRUECOLOR) {
        ASSERT_TRUE(caps->supports_truecolor,
                    "Truecolor depth should have truecolor flag");
    }
    if (caps->color_depth == LLE_COLOR_DEPTH_256) {
        ASSERT_TRUE(caps->supports_256_colors,
                    "256-color depth should have 256-color flag");
    }
    if (caps->color_depth == LLE_COLOR_DEPTH_16) {
        ASSERT_TRUE(caps->supports_ansi_colors,
                    "16-color depth should have ANSI color flag");
    }
    
    lle_terminal_capabilities_destroy(caps);
    TEST_PASS();
}

/**
 * Print test summary
 */
static void print_test_summary(void)
{
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", results.tests_run);
    printf("Tests passed: %d\n", results.tests_passed);
    printf("Tests failed: %d\n", results.tests_failed);
    
    if (results.tests_failed == 0) {
        printf("\n✓ ALL TESTS PASSED\n");
    } else {
        printf("\n✗ SOME TESTS FAILED\n");
    }
    printf("====================\n");
}
