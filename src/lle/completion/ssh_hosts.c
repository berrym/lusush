/**
 * @file ssh_hosts.c
 * @brief SSH Host Cache and Parsing for LLE Completion
 *
 * Provides SSH host completion by parsing:
 * - ~/.ssh/config for Host entries
 * - ~/.ssh/known_hosts for previously connected hosts
 *
 * This module is used by the LLE completion system to provide
 * SSH hostname completions for ssh, scp, rsync commands.
 */

#include "lle/completion/ssh_hosts.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* Forward declaration for portability (see ht_fnv1a.c) */
int strncasecmp(const char *s1, const char *s2, size_t n);

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define SSH_CONFIG_CACHE_TIMEOUT 300 /* 5 minutes */
#define MAX_SSH_HOSTS 1000
#define MAX_CONFIG_LINE_LEN 1024
#define MAX_HOSTNAME_LEN 253
#define MAX_USERNAME_LEN 64
#define MAX_PORT_LEN 8

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static ssh_host_cache_t *g_ssh_host_cache = NULL;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

static void trim_whitespace(char *str);
static bool is_ssh_host_line(const char *line);
static bool is_ssh_hostname_line(const char *line);
static bool is_ssh_user_line(const char *line);
static bool is_ssh_port_line(const char *line);
static void extract_ssh_config_value(const char *line, char *value,
                                     size_t max_len);

/* ============================================================================
 * CACHE MANAGEMENT
 * ============================================================================
 */

ssh_host_cache_t *ssh_host_cache_create(size_t initial_capacity) {
    ssh_host_cache_t *cache = calloc(1, sizeof(ssh_host_cache_t));
    if (!cache) {
        return NULL;
    }

    if (initial_capacity == 0) {
        initial_capacity = 64;
    }

    cache->hosts = calloc(initial_capacity, sizeof(ssh_host_t));
    if (!cache->hosts) {
        free(cache);
        return NULL;
    }

    cache->capacity = initial_capacity;
    cache->count = 0;
    cache->last_updated = 0;
    cache->needs_refresh = true;

    return cache;
}

void ssh_host_cache_destroy(ssh_host_cache_t *cache) {
    if (!cache) {
        return;
    }

    free(cache->hosts);
    free(cache);
}

int ssh_host_cache_add(ssh_host_cache_t *cache, const ssh_host_t *host) {
    if (!cache || !host) {
        return -1;
    }

    /* Grow if needed */
    if (cache->count >= cache->capacity) {
        size_t new_capacity = cache->capacity * 2;
        ssh_host_t *new_hosts =
            realloc(cache->hosts, new_capacity * sizeof(ssh_host_t));
        if (!new_hosts) {
            return -1;
        }
        cache->hosts = new_hosts;
        cache->capacity = new_capacity;
    }

    /* Copy host entry */
    cache->hosts[cache->count] = *host;
    cache->count++;

    return 0;
}

ssh_host_t *ssh_host_cache_find(ssh_host_cache_t *cache, const char *hostname) {
    if (!cache || !hostname) {
        return NULL;
    }

    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->hosts[i].hostname, hostname) == 0 ||
            strcmp(cache->hosts[i].alias, hostname) == 0) {
            return &cache->hosts[i];
        }
    }

    return NULL;
}

/* ============================================================================
 * SSH CONFIG PARSING
 * ============================================================================
 */

int ssh_parse_config(const char *config_path, ssh_host_cache_t *cache) {
    if (!config_path || !cache) {
        return -1;
    }

    FILE *file = fopen(config_path, "r");
    if (!file) {
        return -1;
    }

    char line[MAX_CONFIG_LINE_LEN];
    ssh_host_t current_host = {0};
    bool in_host_block = false;
    int hosts_added = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        trim_whitespace(line);

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        if (is_ssh_host_line(line)) {
            /* Save previous host if valid */
            if (in_host_block && current_host.hostname[0]) {
                current_host.from_config = true;
                current_host.priority = 80;
                if (ssh_host_cache_add(cache, &current_host) == 0) {
                    hosts_added++;
                }
            }

            /* Start new host block */
            memset(&current_host, 0, sizeof(current_host));
            extract_ssh_config_value(line, current_host.alias,
                                     MAX_HOSTNAME_LEN);

            /* If alias doesn't contain wildcards, use as hostname too */
            if (!strchr(current_host.alias, '*') &&
                !strchr(current_host.alias, '?')) {
                strncpy(current_host.hostname, current_host.alias,
                        MAX_HOSTNAME_LEN - 1);
                current_host.hostname[MAX_HOSTNAME_LEN - 1] = '\0';
            }

            in_host_block = true;
        } else if (in_host_block) {
            if (is_ssh_hostname_line(line)) {
                extract_ssh_config_value(line, current_host.hostname,
                                         MAX_HOSTNAME_LEN);
            } else if (is_ssh_user_line(line)) {
                extract_ssh_config_value(line, current_host.user,
                                         MAX_USERNAME_LEN);
            } else if (is_ssh_port_line(line)) {
                extract_ssh_config_value(line, current_host.port, MAX_PORT_LEN);
            }
        }
    }

    /* Save last host */
    if (in_host_block && current_host.hostname[0]) {
        current_host.from_config = true;
        current_host.priority = 80;
        if (ssh_host_cache_add(cache, &current_host) == 0) {
            hosts_added++;
        }
    }

    fclose(file);
    return hosts_added;
}

int ssh_parse_known_hosts(const char *known_hosts_path,
                          ssh_host_cache_t *cache) {
    if (!known_hosts_path || !cache) {
        return -1;
    }

    FILE *file = fopen(known_hosts_path, "r");
    if (!file) {
        return -1;
    }

    char line[MAX_CONFIG_LINE_LEN];
    int hosts_added = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        /* Skip empty lines, comments, and hashed entries */
        if (line[0] == '\0' || line[0] == '#' || line[0] == '|') {
            continue;
        }

        /* Extract hostname (first field before space) */
        char *space = strchr(line, ' ');
        if (!space) {
            continue;
        }

        size_t hostname_len = space - line;
        if (hostname_len >= MAX_HOSTNAME_LEN) {
            hostname_len = MAX_HOSTNAME_LEN - 1;
        }

        ssh_host_t host = {0};
        strncpy(host.hostname, line, hostname_len);
        host.hostname[hostname_len] = '\0';

        /* Handle [host]:port format */
        if (host.hostname[0] == '[') {
            char *bracket = strchr(host.hostname, ']');
            if (bracket) {
                memmove(host.hostname, host.hostname + 1,
                        bracket - host.hostname - 1);
                host.hostname[bracket - host.hostname - 1] = '\0';
            }
        }

        /* Remove any comma-separated aliases, keep first */
        char *comma = strchr(host.hostname, ',');
        if (comma) {
            *comma = '\0';
        }

        /* Skip if already in cache */
        if (ssh_host_cache_find(cache, host.hostname)) {
            continue;
        }

        host.from_known_hosts = true;
        host.priority = 50;

        if (ssh_host_cache_add(cache, &host) == 0) {
            hosts_added++;
        }
    }

    fclose(file);
    return hosts_added;
}

/* ============================================================================
 * GLOBAL CACHE ACCESS
 * ============================================================================
 */

int ssh_hosts_init(void) {
    if (g_ssh_host_cache) {
        return 0; /* Already initialized */
    }

    g_ssh_host_cache = ssh_host_cache_create(MAX_SSH_HOSTS);
    if (!g_ssh_host_cache) {
        return -1;
    }

    ssh_hosts_refresh();
    return 0;
}

void ssh_hosts_cleanup(void) {
    if (g_ssh_host_cache) {
        ssh_host_cache_destroy(g_ssh_host_cache);
        g_ssh_host_cache = NULL;
    }
}

void ssh_hosts_refresh(void) {
    if (!g_ssh_host_cache) {
        return;
    }

    /* Clear existing entries */
    g_ssh_host_cache->count = 0;

    /* Get home directory */
    const char *home = getenv("HOME");
    if (!home) {
        return;
    }

    char path[1024];

    /* Parse user SSH config */
    snprintf(path, sizeof(path), "%s/.ssh/config", home);
    ssh_parse_config(path, g_ssh_host_cache);

    /* Parse system SSH config */
    ssh_parse_config("/etc/ssh/ssh_config", g_ssh_host_cache);

    /* Parse known_hosts */
    snprintf(path, sizeof(path), "%s/.ssh/known_hosts", home);
    ssh_parse_known_hosts(path, g_ssh_host_cache);

    g_ssh_host_cache->last_updated = time(NULL);
    g_ssh_host_cache->needs_refresh = false;
}

ssh_host_cache_t *get_ssh_host_cache(void) {
    if (!g_ssh_host_cache) {
        ssh_hosts_init();
    }

    /* Check if refresh needed */
    if (g_ssh_host_cache && g_ssh_host_cache->needs_refresh) {
        ssh_hosts_refresh();
    }

    /* Check cache timeout */
    if (g_ssh_host_cache) {
        time_t now = time(NULL);
        if (now - g_ssh_host_cache->last_updated > SSH_CONFIG_CACHE_TIMEOUT) {
            ssh_hosts_refresh();
        }
    }

    return g_ssh_host_cache;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

static void trim_whitespace(char *str) {
    if (!str) {
        return;
    }

    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }
}

static bool is_ssh_host_line(const char *line) {
    return strncasecmp(line, "Host ", 5) == 0 ||
           strncasecmp(line, "Host\t", 5) == 0;
}

static bool is_ssh_hostname_line(const char *line) {
    return strncasecmp(line, "HostName ", 9) == 0 ||
           strncasecmp(line, "HostName\t", 9) == 0;
}

static bool is_ssh_user_line(const char *line) {
    return strncasecmp(line, "User ", 5) == 0 ||
           strncasecmp(line, "User\t", 5) == 0;
}

static bool is_ssh_port_line(const char *line) {
    return strncasecmp(line, "Port ", 5) == 0 ||
           strncasecmp(line, "Port\t", 5) == 0;
}

static void extract_ssh_config_value(const char *line, char *value,
                                     size_t max_len) {
    if (!line || !value || max_len == 0) {
        return;
    }

    /* Skip keyword */
    const char *p = line;
    while (*p && !isspace((unsigned char)*p)) {
        p++;
    }

    /* Skip whitespace */
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }

    /* Copy value */
    size_t len = strlen(p);
    if (len >= max_len) {
        len = max_len - 1;
    }
    strncpy(value, p, len);
    value[len] = '\0';

    /* Remove trailing whitespace */
    trim_whitespace(value);
}
