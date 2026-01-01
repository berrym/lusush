/**
 * @file lle_watchdog.c
 * @brief LLE Watchdog Timer Implementation
 *
 * Implements a SIGALRM-based watchdog mechanism for detecting deadlocks
 * and stuck conditions in the readline input loop.
 */

#include "lle/lle_watchdog.h"

#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>

/* ============================================================================
 * PRIVATE STATE
 * ============================================================================
 */

/**
 * Atomic flags for signal-safe communication.
 * These are set by the signal handler and read by the main loop.
 */
static atomic_bool g_watchdog_fired = false;
static atomic_bool g_watchdog_armed = false;

/**
 * Previous SIGALRM handler to restore on cleanup.
 */
static struct sigaction g_old_sigalrm_action;

/**
 * Initialization state.
 */
static bool g_initialized = false;

/**
 * Current timeout setting (for get_timeout).
 */
static unsigned int g_current_timeout = 0;

/**
 * Statistics counters.
 */
static atomic_uint g_stats_pets = 0;
static atomic_uint g_stats_fires = 0;
static atomic_uint g_stats_recoveries = 0;

/* ============================================================================
 * SIGNAL HANDLER
 * ============================================================================
 */

/**
 * SIGALRM handler - must be async-signal-safe.
 *
 * Only uses atomic operations and avoids any non-reentrant functions.
 * This is called from signal context, so must be minimal.
 */
static void watchdog_signal_handler(int sig) {
    (void)sig; /* Unused, but required by signal handler signature */

    /* Set the fired flag atomically */
    atomic_store(&g_watchdog_fired, true);

    /* Disarm the watchdog to prevent repeated firing */
    atomic_store(&g_watchdog_armed, false);

    /* Increment fire counter (atomic) */
    atomic_fetch_add(&g_stats_fires, 1);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

lle_result_t lle_watchdog_init(void) {
    if (g_initialized) {
        return LLE_SUCCESS; /* Already initialized */
    }

    struct sigaction sa;
    sa.sa_handler = watchdog_signal_handler;
    sa.sa_flags = 0; /* No SA_RESTART - we want to interrupt syscalls */
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, &g_old_sigalrm_action) < 0) {
        return LLE_ERROR_SYSTEM_CALL;
    }

    g_initialized = true;
    atomic_store(&g_watchdog_fired, false);
    atomic_store(&g_watchdog_armed, false);
    g_current_timeout = 0;

    return LLE_SUCCESS;
}

void lle_watchdog_cleanup(void) {
    if (!g_initialized) {
        return;
    }

    /* Cancel any pending alarm */
    alarm(0);

    /* Restore previous handler */
    sigaction(SIGALRM, &g_old_sigalrm_action, NULL);

    g_initialized = false;
    atomic_store(&g_watchdog_armed, false);
    atomic_store(&g_watchdog_fired, false);
    g_current_timeout = 0;
}

void lle_watchdog_pet(unsigned int timeout_seconds) {
    if (!g_initialized) {
        return;
    }

    if (timeout_seconds == 0) {
        timeout_seconds = LLE_WATCHDOG_TIMEOUT_DEFAULT;
    }

    /* Clear fired flag and arm the watchdog */
    atomic_store(&g_watchdog_fired, false);
    atomic_store(&g_watchdog_armed, true);
    g_current_timeout = timeout_seconds;

    /* Set the alarm */
    alarm(timeout_seconds);

    /* Increment pet counter */
    atomic_fetch_add(&g_stats_pets, 1);
}

void lle_watchdog_stop(void) {
    if (!g_initialized) {
        return;
    }

    /* Cancel any pending alarm */
    alarm(0);

    /* Disarm and clear flags */
    atomic_store(&g_watchdog_armed, false);
    atomic_store(&g_watchdog_fired, false);
    g_current_timeout = 0;
}

bool lle_watchdog_check_and_clear(void) {
    bool was_fired = atomic_exchange(&g_watchdog_fired, false);

    if (was_fired) {
        /* Count successful recovery (we caught the timeout) */
        atomic_fetch_add(&g_stats_recoveries, 1);
    }

    return was_fired;
}

bool lle_watchdog_check(void) { return atomic_load(&g_watchdog_fired); }

bool lle_watchdog_is_armed(void) { return atomic_load(&g_watchdog_armed); }

unsigned int lle_watchdog_get_timeout(void) { return g_current_timeout; }

lle_result_t lle_watchdog_get_stats(lle_watchdog_stats_t *stats) {
    if (!stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    stats->total_pets = atomic_load(&g_stats_pets);
    stats->total_fires = atomic_load(&g_stats_fires);
    stats->total_recoveries = atomic_load(&g_stats_recoveries);

    return LLE_SUCCESS;
}
