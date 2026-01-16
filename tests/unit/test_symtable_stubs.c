/**
 * @file test_symtable_stubs.c
 * @brief Stub implementations for symbol table unit tests
 */

#include <stdbool.h>

/* Global variable stub */
int last_exit_status = 0;

/* Shell mode stub */
bool shell_mode_allows(int feature) {
    (void)feature;
    return true;  /* Allow all features in tests */
}
