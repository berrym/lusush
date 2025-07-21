#ifndef LLE_TEST_FRAMEWORK_H
#define LLE_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Lusush Line Editor - Test Framework
 *
 * Simple testing framework for LLE components.
 */

// Test macros
#define LLE_TEST(name) void test_##name(void)

#define LLE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            abort(); \
        } \
    } while(0)

#define LLE_ASSERT_EQ(a, b) LLE_ASSERT((a) == (b))
#define LLE_ASSERT_STR_EQ(a, b) LLE_ASSERT(strcmp((a), (b)) == 0)
#define LLE_ASSERT_NOT_NULL(ptr) LLE_ASSERT((ptr) != NULL)
#define LLE_ASSERT_NULL(ptr) LLE_ASSERT((ptr) == NULL)

// Test runner
void lle_run_all_tests(void);

#endif // LLE_TEST_FRAMEWORK_H
