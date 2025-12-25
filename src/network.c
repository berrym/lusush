/**
 * network.c - Network Integration and SSH Host Completion for lusush
 *
 * This module implements network-aware features including SSH host completion,
 * remote context detection, and cloud provider integration for modern DevOps
 * workflows and remote development environments.
 *
 * Copyright (C) 2021-2025 Michael Berry
 * Licensed under GPL-3.0+
 */

#include "network.h"

#include "config.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Global network state
ssh_host_cache_t *g_ssh_host_cache = NULL;
remote_context_t g_remote_context = {0};
network_config_t g_network_config = {0};

// Internal constants
#define SSH_CONFIG_CACHE_TIMEOUT 300 // 5 minutes
#define MAX_CONFIG_FILES 5
#define FUZZY_MATCH_THRESHOLD 60

/**
 * Initialize the network subsystem
 */
int network_init(void) {
    // Initialize global configuration with defaults
    set_network_config_defaults(&g_network_config);

    // Load configuration from ~/.lusushrc
    network_load_config();

    // Initialize SSH host cache
    g_ssh_host_cache = create_ssh_host_cache(MAX_SSH_HOSTS);
    if (!g_ssh_host_cache) {
        return -1;
    }

    // Detect remote context
    detect_remote_context(&g_remote_context);

    // Refresh SSH host cache
    refresh_ssh_host_cache();

    return 0;
}

/**
 * Cleanup network subsystem resources
 */
void network_cleanup(void) {
    if (g_ssh_host_cache) {
        destroy_ssh_host_cache(g_ssh_host_cache);
        g_ssh_host_cache = NULL;
    }

    // Clear sensitive data
    memset(&g_remote_context, 0, sizeof(g_remote_context));
    memset(&g_network_config, 0, sizeof(g_network_config));
}

/**
 * Load network configuration from config system
 */
int network_load_config(void) {
    // Load from configuration system if available
    extern config_values_t config;

    // Map config fields to network config
    g_network_config.ssh_completion_enabled = config.ssh_completion_enabled;
    g_network_config.cloud_discovery_enabled = config.cloud_discovery_enabled;
    g_network_config.cache_ssh_hosts = config.cache_ssh_hosts;
    g_network_config.cache_timeout_minutes = config.cache_timeout_minutes;
    g_network_config.show_remote_context = config.show_remote_context;
    g_network_config.auto_detect_cloud = config.auto_detect_cloud;
    g_network_config.max_completion_hosts = config.max_completion_hosts;

    return 0;
}

/**
 * Set default network configuration values
 */
void set_network_config_defaults(network_config_t *config) {
    if (!config) {
        return;
    }

    config->ssh_completion_enabled = true;
    config->cloud_discovery_enabled = false;
    config->cache_ssh_hosts = true;
    config->cache_timeout_minutes = 5;
    config->show_remote_context = true;
    config->auto_detect_cloud = true;
    config->max_completion_hosts = 50;

    // Set default SSH config paths
    snprintf(config->ssh_config_paths[0], 256, "%s/.ssh/config",
             getenv("HOME") ?: "");
    snprintf(config->ssh_config_paths[1], 256, "/etc/ssh/ssh_config");
    config->ssh_config_paths[2][0] = '\0';
    config->ssh_config_paths[3][0] = '\0';
    config->ssh_config_paths[4][0] = '\0';
}

/**
 * Complete SSH hosts for network commands
 */
void complete_ssh_hosts(const char *text, lusush_completions_t *lc) {
    if (!text || !lc || !g_network_config.ssh_completion_enabled) {
        return;
    }

    ssh_host_cache_t *cache = get_ssh_host_cache();
    if (!cache || cache->count == 0) {
        return;
    }

    size_t text_len = strlen(text);
    int added_count = 0;

    // Complete SSH hosts with fuzzy matching
    for (size_t i = 0; i < cache->count &&
                       added_count < g_network_config.max_completion_hosts;
         i++) {
        ssh_host_t *host = &cache->hosts[i];

        // Try exact hostname match first
        if (strncmp(host->hostname, text, text_len) == 0) {
            char completion[MAX_HOSTNAME_LEN + 64];
            format_ssh_host_completion(host, completion, sizeof(completion));
            lusush_add_completion(lc, completion);
            added_count++;
            continue;
        }

        // Try alias match
        if (host->alias[0] && strncmp(host->alias, text, text_len) == 0) {
            char completion[MAX_HOSTNAME_LEN + 64];
            snprintf(completion, sizeof(completion), "%s", host->alias);
            lusush_add_completion(lc, completion);
            added_count++;
            continue;
        }

        // Try fuzzy matching
        int score = hostname_priority_score(host->hostname, text);
        if (score >= FUZZY_MATCH_THRESHOLD) {
            char completion[MAX_HOSTNAME_LEN + 64];
            format_ssh_host_completion(host, completion, sizeof(completion));
            lusush_add_completion(lc, completion);
            added_count++;
        }
    }
}

/**
 * Parse SSH configuration file
 */
int parse_ssh_config(const char *config_path, ssh_host_cache_t *cache) {
    if (!config_path || !cache) {
        return -1;
    }

    FILE *file = fopen(config_path, "r");
    if (!file) {
        return -1; // File doesn't exist or can't be read
    }

    char line[MAX_CONFIG_LINE_LEN];
    ssh_host_t current_host = {0};
    bool in_host_block = false;
    int hosts_added = 0;

    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';
        trim_whitespace(line);

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        if (is_ssh_host_line(line)) {
            // Save previous host if valid
            if (in_host_block && current_host.hostname[0]) {
                current_host.from_config = true;
                current_host.priority = 90; // High priority for config hosts
                if (add_ssh_host(cache, &current_host) == 0) {
                    hosts_added++;
                }
            }

            // Start new host block
            memset(&current_host, 0, sizeof(current_host));
            extract_ssh_config_value(line, current_host.alias,
                                     sizeof(current_host.alias));
            strncpy(current_host.hostname, current_host.alias,
                    sizeof(current_host.hostname) - 1);
            current_host.hostname[sizeof(current_host.hostname) - 1] = '\0';
            in_host_block = true;

        } else if (in_host_block) {
            parse_ssh_config_line(line, &current_host);
        }
    }

    // Save last host if valid
    if (in_host_block && current_host.hostname[0]) {
        current_host.from_config = true;
        current_host.priority = 90;
        if (add_ssh_host(cache, &current_host) == 0) {
            hosts_added++;
        }
    }

    fclose(file);
    return hosts_added;
}

/**
 * Parse SSH known_hosts file
 */
int parse_ssh_known_hosts(const char *known_hosts_path,
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
        trim_whitespace(line);

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        ssh_host_t host = {0};
        if (parse_known_hosts_line(line, &host) == 0) {
            host.from_known_hosts = true;
            host.priority = 70; // Lower priority than config hosts
            if (add_ssh_host(cache, &host) == 0) {
                hosts_added++;
            }
        }
    }

    fclose(file);
    return hosts_added;
}

/**
 * Get the SSH host cache, refreshing if needed
 */
ssh_host_cache_t *get_ssh_host_cache(void) {
    if (!g_ssh_host_cache) {
        return NULL;
    }

    // Check if cache needs refresh
    time_t now = time(NULL);
    if (g_ssh_host_cache->needs_refresh ||
        (now - g_ssh_host_cache->last_updated) >
            (g_network_config.cache_timeout_minutes * 60)) {
        refresh_ssh_host_cache();
    }

    return g_ssh_host_cache;
}

/**
 * Refresh the SSH host cache from config files
 */
void refresh_ssh_host_cache(void) {
    if (!g_ssh_host_cache) {
        return;
    }

    // Clear existing cache
    g_ssh_host_cache->count = 0;

    // Parse SSH config files
    for (int i = 0; i < MAX_CONFIG_FILES; i++) {
        if (g_network_config.ssh_config_paths[i][0] == '\0') {
            break;
        }
        parse_ssh_config(g_network_config.ssh_config_paths[i],
                         g_ssh_host_cache);
    }

    // Parse known_hosts
    char known_hosts_path[512];
    snprintf(known_hosts_path, sizeof(known_hosts_path), "%s/.ssh/known_hosts",
             getenv("HOME") ?: "");
    parse_ssh_known_hosts(known_hosts_path, g_ssh_host_cache);

    // Sort and deduplicate
    sort_ssh_hosts_by_priority(g_ssh_host_cache);
    deduplicate_ssh_hosts(g_ssh_host_cache);

    // Update cache metadata
    g_ssh_host_cache->last_updated = time(NULL);
    g_ssh_host_cache->needs_refresh = false;
}

/**
 * Parse individual SSH config line
 */
int parse_ssh_config_line(const char *line, ssh_host_t *current_host) {
    if (!line || !current_host) {
        return -1;
    }

    if (is_ssh_hostname_line(line)) {
        extract_ssh_config_value(line, current_host->hostname,
                                 sizeof(current_host->hostname));
    } else if (is_ssh_user_line(line)) {
        extract_ssh_config_value(line, current_host->user,
                                 sizeof(current_host->user));
    } else if (is_ssh_port_line(line)) {
        extract_ssh_config_value(line, current_host->port,
                                 sizeof(current_host->port));
    }

    return 0;
}

/**
 * Check if line is SSH Host directive
 */
bool is_ssh_host_line(const char *line) {
    return line &&
           (strncmp(line, "Host ", 5) == 0 || strncmp(line, "host ", 5) == 0);
}

/**
 * Check if line is SSH HostName directive
 */
bool is_ssh_hostname_line(const char *line) {
    return line && (strncmp(line, "HostName ", 9) == 0 ||
                    strncmp(line, "hostname ", 9) == 0);
}

/**
 * Check if line is SSH User directive
 */
bool is_ssh_user_line(const char *line) {
    return line &&
           (strncmp(line, "User ", 5) == 0 || strncmp(line, "user ", 5) == 0);
}

/**
 * Check if line is SSH Port directive
 */
bool is_ssh_port_line(const char *line) {
    return line &&
           (strncmp(line, "Port ", 5) == 0 || strncmp(line, "port ", 5) == 0);
}

/**
 * Extract configuration value from SSH config line
 */
void extract_ssh_config_value(const char *line, char *value, size_t max_len) {
    if (!line || !value || max_len == 0) {
        return;
    }

    // Find first space
    const char *space = strchr(line, ' ');
    if (!space) {
        value[0] = '\0';
        return;
    }

    // Skip whitespace after directive
    space++;
    while (*space && isspace(*space)) {
        space++;
    }

    // Copy value, handling quoted strings
    size_t len = 0;
    bool in_quotes = false;

    if (*space == '"') {
        in_quotes = true;
        space++;
    }

    while (*space && len < max_len - 1) {
        if (in_quotes && *space == '"') {
            break;
        } else if (!in_quotes && isspace(*space)) {
            break;
        }
        value[len++] = *space++;
    }

    value[len] = '\0';
    trim_whitespace(value);
}

/**
 * Parse known_hosts line
 */
int parse_known_hosts_line(const char *line, ssh_host_t *host) {
    if (!line || !host) {
        return -1;
    }

    // Skip hashed entries for now (they start with |)
    if (is_hashed_known_hosts_entry(line)) {
        return -1;
    }

    // Extract hostname from known_hosts entry
    extract_hostname_from_known_hosts(line, host->hostname,
                                      sizeof(host->hostname));

    // Set alias same as hostname for known_hosts entries
    strncpy(host->alias, host->hostname, sizeof(host->alias) - 1);
    host->alias[sizeof(host->alias) - 1] = '\0';

    return host->hostname[0] ? 0 : -1;
}

/**
 * Check if known_hosts entry is hashed
 */
bool is_hashed_known_hosts_entry(const char *line) {
    return line && line[0] == '|';
}

/**
 * Extract hostname from known_hosts entry
 */
void extract_hostname_from_known_hosts(const char *entry, char *hostname,
                                       size_t max_len) {
    if (!entry || !hostname || max_len == 0) {
        return;
    }

    // known_hosts format: hostname[,hostname]* keytype key [comment]
    // We want the first hostname before any comma or space

    size_t len = 0;
    while (entry[len] && !isspace(entry[len]) && entry[len] != ',' &&
           len < max_len - 1) {
        hostname[len] = entry[len];
        len++;
    }
    hostname[len] = '\0';

    // Remove port specification if present (hostname:port)
    char *colon = strchr(hostname, ':');
    if (colon) {
        *colon = '\0';
    }

    // Remove brackets from IPv6 addresses
    if (hostname[0] == '[') {
        char *end_bracket = strchr(hostname, ']');
        if (end_bracket) {
            memmove(hostname, hostname + 1, end_bracket - hostname - 1);
            hostname[end_bracket - hostname - 1] = '\0';
        }
    }
}

/**
 * Create SSH host cache
 */
ssh_host_cache_t *create_ssh_host_cache(size_t initial_capacity) {
    ssh_host_cache_t *cache = malloc(sizeof(ssh_host_cache_t));
    if (!cache) {
        return NULL;
    }

    cache->hosts = malloc(sizeof(ssh_host_t) * initial_capacity);
    if (!cache->hosts) {
        free(cache);
        return NULL;
    }

    cache->count = 0;
    cache->capacity = initial_capacity;
    cache->last_updated = 0;
    cache->needs_refresh = true;

    return cache;
}

/**
 * Destroy SSH host cache
 */
void destroy_ssh_host_cache(ssh_host_cache_t *cache) {
    if (!cache) {
        return;
    }

    free(cache->hosts);
    free(cache);
}

/**
 * Add SSH host to cache
 */
int add_ssh_host(ssh_host_cache_t *cache, const ssh_host_t *host) {
    if (!cache || !host || !host->hostname[0]) {
        return -1;
    }

    // Check if host already exists
    if (find_ssh_host(cache, host->hostname)) {
        return 0; // Already exists, not an error
    }

    // Resize if needed
    if (cache->count >= cache->capacity) {
        size_t new_capacity = cache->capacity * 2;
        ssh_host_t *new_hosts =
            realloc(cache->hosts, sizeof(ssh_host_t) * new_capacity);
        if (!new_hosts) {
            return -1;
        }
        cache->hosts = new_hosts;
        cache->capacity = new_capacity;
    }

    // Add new host
    cache->hosts[cache->count] = *host;
    cache->count++;

    return 0;
}

/**
 * Find SSH host in cache
 */
ssh_host_t *find_ssh_host(ssh_host_cache_t *cache, const char *hostname) {
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

/**
 * Sort SSH hosts by priority
 */
void sort_ssh_hosts_by_priority(ssh_host_cache_t *cache) {
    if (!cache || cache->count <= 1) {
        return;
    }

    // Simple bubble sort by priority (descending)
    for (size_t i = 0; i < cache->count - 1; i++) {
        for (size_t j = 0; j < cache->count - i - 1; j++) {
            if (cache->hosts[j].priority < cache->hosts[j + 1].priority) {
                ssh_host_t temp = cache->hosts[j];
                cache->hosts[j] = cache->hosts[j + 1];
                cache->hosts[j + 1] = temp;
            }
        }
    }
}

/**
 * Remove duplicate SSH hosts
 */
void deduplicate_ssh_hosts(ssh_host_cache_t *cache) {
    if (!cache || cache->count <= 1) {
        return;
    }

    size_t write_idx = 0;

    for (size_t read_idx = 0; read_idx < cache->count; read_idx++) {
        bool is_duplicate = false;

        // Check if this host already exists earlier in the array
        for (size_t check_idx = 0; check_idx < write_idx; check_idx++) {
            if (strcmp(cache->hosts[read_idx].hostname,
                       cache->hosts[check_idx].hostname) == 0) {
                is_duplicate = true;
                break;
            }
        }

        if (!is_duplicate) {
            if (write_idx != read_idx) {
                cache->hosts[write_idx] = cache->hosts[read_idx];
            }
            write_idx++;
        }
    }

    cache->count = write_idx;
}

/**
 * Detect remote context
 */
int detect_remote_context(remote_context_t *context) {
    if (!context) {
        return -1;
    }

    memset(context, 0, sizeof(remote_context_t));

    // Detect SSH session
    context->is_remote_session = is_ssh_session();

    // Detect cloud instance
    context->is_cloud_instance = is_cloud_instance();

    // Get remote hostname and username
    char *remote_host = get_remote_hostname();
    if (remote_host) {
        strncpy(context->remote_host, remote_host,
                sizeof(context->remote_host) - 1);
        free(remote_host);
    }

    char *remote_user = get_remote_username();
    if (remote_user) {
        strncpy(context->remote_user, remote_user,
                sizeof(context->remote_user) - 1);
        free(remote_user);
    }

    // Detect cloud provider
    char *cloud_provider = detect_cloud_provider();
    if (cloud_provider) {
        strncpy(context->cloud_provider, cloud_provider,
                sizeof(context->cloud_provider) - 1);
        free(cloud_provider);
    }

    // Test connectivity
    context->has_internet = has_internet_connectivity();
    context->vpn_active = is_vpn_active();

    return 0;
}

/**
 * Check if running in SSH session
 */
bool is_ssh_session(void) {
    return (getenv("SSH_CLIENT") != NULL || getenv("SSH_TTY") != NULL ||
            getenv("SSH_CONNECTION") != NULL);
}

/**
 * Check if running on cloud instance
 */
bool is_cloud_instance(void) {
    return (is_aws_instance() || is_gcp_instance() || is_azure_instance());
}

/**
 * Get remote hostname
 */
char *get_remote_hostname(void) {
    char *ssh_client = getenv("SSH_CLIENT");
    if (ssh_client) {
        // SSH_CLIENT format: "client_ip client_port server_port"
        char *hostname = malloc(256);
        if (hostname) {
            strncpy(hostname, ssh_client, 255);
            hostname[255] = '\0';

            // Extract just the IP address
            char *space = strchr(hostname, ' ');
            if (space) {
                *space = '\0';
            }

            return hostname;
        }
    }

    return NULL;
}

/**
 * Get remote username
 */
char *get_remote_username(void) {
    char *user = getenv("USER");
    if (user) {
        char *username = malloc(strlen(user) + 1);
        if (username) {
            strcpy(username, user);
            return username;
        }
    }

    return NULL;
}

/**
 * Detect cloud provider
 */
char *detect_cloud_provider(void) {
    if (is_aws_instance()) {
        return strdup("AWS");
    } else if (is_gcp_instance()) {
        return strdup("GCP");
    } else if (is_azure_instance()) {
        return strdup("Azure");
    }

    return NULL;
}

/**
 * Check if running on AWS instance
 */
bool is_aws_instance(void) {
    // Check for AWS metadata service
    return test_host_connectivity("169.254.169.254", 80, 1000);
}

/**
 * Check if running on GCP instance
 */
bool is_gcp_instance(void) {
    // Check for GCP metadata service
    return test_host_connectivity("metadata.google.internal", 80, 1000);
}

/**
 * Check if running on Azure instance
 */
bool is_azure_instance(void) {
    // Check for Azure metadata service
    return test_host_connectivity("169.254.169.254", 80, 1000) &&
           access("/sys/class/dmi/id/sys_vendor", R_OK) == 0;
}

/**
 * Test internet connectivity
 */
bool has_internet_connectivity(void) {
    // Simple connectivity test to major DNS servers
    return (test_host_connectivity("8.8.8.8", 53, 2000) ||
            test_host_connectivity("1.1.1.1", 53, 2000));
}

/**
 * Check if VPN is active
 */
bool is_vpn_active(void) {
    // Check for common VPN interfaces
    char interfaces[10][64];
    int count = get_network_interfaces(interfaces, 10);

    for (int i = 0; i < count; i++) {
        if (strstr(interfaces[i], "tun") || strstr(interfaces[i], "tap") ||
            strstr(interfaces[i], "vpn") || strstr(interfaces[i], "ppp")) {
            return true;
        }
    }

    return false;
}

/**
 * Get network interfaces (stub implementation)
 */
int get_network_interfaces(char interfaces[][64], int max_interfaces) {
    // This would typically read from /proc/net/dev or use system calls
    // For now, return empty list
    (void)interfaces;
    (void)max_interfaces;
    return 0;
}

/**
 * Test host connectivity (stub implementation)
 */
bool test_host_connectivity(const char *hostname, int port, int timeout_ms) {
    // This would typically create a socket and attempt connection
    // For now, always return false to avoid blocking
    (void)hostname;
    (void)port;
    (void)timeout_ms;
    return false;
}

/**
 * Check if command is network-related
 */
bool is_network_command(const char *command) {
    if (!command) {
        return false;
    }

    static const char *network_commands[] = {
        "ssh", "scp", "rsync", "sftp", "curl", "wget", "ping", "telnet", NULL};

    for (int i = 0; network_commands[i]; i++) {
        if (strcmp(command, network_commands[i]) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Complete network command arguments
 */
void complete_network_command_args(const char *command, const char *text,
                                   lusush_completions_t *lc) {
    if (!command || !text || !lc) {
        return;
    }

    if (strcmp(command, "ssh") == 0) {
        complete_ssh_command(text, lc);
    } else if (strcmp(command, "scp") == 0) {
        complete_scp_command(text, lc);
    } else if (strcmp(command, "rsync") == 0) {
        complete_rsync_command(text, lc);
    }
}

/**
 * Complete network command arguments with full command context
 * This preserves the original command when adding completions
 */
void complete_network_command_args_with_context(const char *command,
                                                const char *text,
                                                lusush_completions_t *lc,
                                                const char *buf,
                                                int start_pos) {
    if (!command || !text || !lc || !buf) {
        return;
    }

    // Get the host completions using the existing functions
    if (strcmp(command, "ssh") == 0) {
        complete_ssh_command(text, lc);
    } else if (strcmp(command, "scp") == 0) {
        complete_scp_command(text, lc);
    } else if (strcmp(command, "rsync") == 0) {
        complete_rsync_command(text, lc);
    }

    // Transform the completions to include the full command context
    // We need to modify the completions in place to preserve command
    for (size_t i = 0; i < lc->len; i++) {
        char *original_completion = lc->cvec[i];

        // Build complete command line: prefix + completion + suffix
        size_t prefix_len = start_pos;
        size_t completion_len = strlen(original_completion);
        size_t suffix_len = strlen(buf + start_pos + strlen(text));
        size_t total_len = prefix_len + completion_len + suffix_len + 1;

        char *complete_command = malloc(total_len);
        if (complete_command) {
            // Copy prefix (everything before current word)
            strncpy(complete_command, buf, prefix_len);
            complete_command[prefix_len] = '\0';

            // Append the completion
            strcat(complete_command, original_completion);

            // Append suffix (everything after current word)
            strcat(complete_command, buf + start_pos + strlen(text));

            // Replace the original completion
            free(lc->cvec[i]);
            lc->cvec[i] = complete_command;
        }
    }
}

/**
 * Complete SSH command
 */
void complete_ssh_command(const char *text, lusush_completions_t *lc) {
    // Look for user@host pattern
    char *at_symbol = strchr(text, '@');
    if (at_symbol) {
        // Complete hostname part after @
        complete_ssh_hosts(at_symbol + 1, lc);
    } else {
        // Complete full hostnames or user@host combinations
        complete_ssh_hosts(text, lc);
    }
}

/**
 * Complete SCP command
 */
void complete_scp_command(const char *text, lusush_completions_t *lc) {
    // SCP can have local files or remote host:path
    char *colon = strchr(text, ':');
    if (colon) {
        // After colon, this would be path completion on remote host
        // For now, just complete hosts before colon
        return;
    }

    char *at_symbol = strchr(text, '@');
    if (at_symbol) {
        complete_ssh_hosts(at_symbol + 1, lc);
    } else {
        complete_ssh_hosts(text, lc);
    }
}

/**
 * Complete rsync command
 */
void complete_rsync_command(const char *text, lusush_completions_t *lc) {
    // Similar to SCP
    complete_scp_command(text, lc);
}

/**
 * Trim whitespace from string
 */
void trim_whitespace(char *str) {
    if (!str) {
        return;
    }

    // Trim leading whitespace
    char *start = str;
    while (*start && isspace(*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        *end = '\0';
        end--;
    }
}

/**
 * Validate hostname format
 */
bool is_valid_hostname(const char *hostname) {
    if (!hostname || strlen(hostname) == 0 ||
        strlen(hostname) > MAX_HOSTNAME_LEN) {
        return false;
    }

    // Basic hostname validation
    for (const char *p = hostname; *p; p++) {
        if (!isalnum(*p) && *p != '.' && *p != '-' && *p != '_') {
            return false;
        }
    }

    return true;
}

/**
 * Validate port format
 */
bool is_valid_port(const char *port) {
    if (!port || strlen(port) == 0) {
        return false;
    }

    for (const char *p = port; *p; p++) {
        if (!isdigit(*p)) {
            return false;
        }
    }

    int port_num = atoi(port);
    return (port_num > 0 && port_num <= 65535);
}

/**
 * Calculate hostname priority score for fuzzy matching
 */
int hostname_priority_score(const char *hostname, const char *pattern) {
    if (!hostname || !pattern) {
        return 0;
    }

    int len_h = strlen(hostname);
    int len_p = strlen(pattern);

    if (len_p == 0) {
        return 100;
    }

    if (len_h == 0) {
        return 0;
    }

    // Exact prefix match gets highest score
    if (strncmp(hostname, pattern, len_p) == 0) {
        return 95;
    }

    // Case-insensitive prefix match
    bool case_match = true;
    for (int i = 0; i < len_p; i++) {
        if (tolower(hostname[i]) != tolower(pattern[i])) {
            case_match = false;
            break;
        }
    }
    if (case_match) {
        return 90;
    }

    // Subsequence matching
    int matches = 0;
    int pattern_idx = 0;

    for (int i = 0; i < len_h && pattern_idx < len_p; i++) {
        if (tolower(hostname[i]) == tolower(pattern[pattern_idx])) {
            matches++;
            pattern_idx++;
        }
    }

    if (pattern_idx < len_p) {
        return 0; // Not all pattern characters found
    }

    return (matches * 80) / len_p;
}

/**
 * Format SSH host completion string
 */
void format_ssh_host_completion(const ssh_host_t *host, char *completion,
                                size_t max_len) {
    if (!host || !completion || max_len == 0) {
        return;
    }

    // Use alias if available, otherwise hostname
    const char *name = host->alias[0] ? host->alias : host->hostname;

    if (host->user[0]) {
        // Format user@host - calculate available space for each part
        // Reserve 1 for '@' and 1 for null terminator
        size_t available = max_len > 2 ? max_len - 2 : 0;
        size_t user_len = strlen(host->user);
        // Give each part half the space, but use actual length if shorter
        size_t user_max = user_len < available / 2 ? user_len : available / 2;
        size_t name_max = available - user_max;
        snprintf(completion, max_len, "%.*s@%.*s", (int)user_max, host->user,
                 (int)name_max, name);
    } else {
        size_t name_len = strlen(name);
        size_t name_max = name_len < max_len - 1 ? name_len : max_len - 1;
        snprintf(completion, max_len, "%.*s", (int)name_max, name);
    }
}

/**
 * Get network configuration
 */
network_config_t *get_network_config(void) { return &g_network_config; }

/**
 * Print SSH host cache statistics
 */
void print_ssh_host_cache_stats(const ssh_host_cache_t *cache) {
    if (!cache) {
        printf("SSH host cache: Not initialized\n");
        return;
    }

    printf("SSH host cache statistics:\n");
    printf("  Total hosts: %zu\n", cache->count);
    printf("  Cache capacity: %zu\n", cache->capacity);
    printf("  Last updated: %s", ctime(&cache->last_updated));
    printf("  Needs refresh: %s\n", cache->needs_refresh ? "Yes" : "No");

    int config_hosts = 0, known_hosts = 0;
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->hosts[i].from_config) {
            config_hosts++;
        }
        if (cache->hosts[i].from_known_hosts) {
            known_hosts++;
        }
    }

    printf("  From SSH config: %d\n", config_hosts);
    printf("  From known_hosts: %d\n", known_hosts);
}

/**
 * Print remote context information
 */
void print_remote_context_info(const remote_context_t *context) {
    if (!context) {
        printf("Remote context: Not available\n");
        return;
    }

    printf("Remote context information:\n");
    printf("  Remote session: %s\n", context->is_remote_session ? "Yes" : "No");
    printf("  Cloud instance: %s\n", context->is_cloud_instance ? "Yes" : "No");
    printf("  Remote host: %s\n",
           context->remote_host[0] ? context->remote_host : "N/A");
    printf("  Remote user: %s\n",
           context->remote_user[0] ? context->remote_user : "N/A");
    printf("  Cloud provider: %s\n",
           context->cloud_provider[0] ? context->cloud_provider : "N/A");
    printf("  Internet connectivity: %s\n",
           context->has_internet ? "Yes" : "No");
    printf("  VPN active: %s\n", context->vpn_active ? "Yes" : "No");
}

/**
 * Print network configuration
 */
void print_network_config(const network_config_t *config) {
    if (!config) {
        printf("Network configuration: Not available\n");
        return;
    }

    printf("Network configuration:\n");
    printf("  SSH completion enabled: %s\n",
           config->ssh_completion_enabled ? "Yes" : "No");
    printf("  Cloud discovery enabled: %s\n",
           config->cloud_discovery_enabled ? "Yes" : "No");
    printf("  Cache SSH hosts: %s\n", config->cache_ssh_hosts ? "Yes" : "No");
    printf("  Cache timeout: %d minutes\n", config->cache_timeout_minutes);
    printf("  Show remote context: %s\n",
           config->show_remote_context ? "Yes" : "No");
    printf("  Auto-detect cloud: %s\n",
           config->auto_detect_cloud ? "Yes" : "No");
    printf("  Max completion hosts: %d\n", config->max_completion_hosts);

    printf("  SSH config paths:\n");
    for (int i = 0; i < MAX_CONFIG_FILES && config->ssh_config_paths[i][0];
         i++) {
        printf("    %s\n", config->ssh_config_paths[i]);
    }
}

/**
 * Run network diagnostics
 */
int run_network_diagnostics(void) {
    printf("=== LUSUSH Network Diagnostics ===\n\n");

    print_network_config(&g_network_config);
    printf("\n");

    print_remote_context_info(&g_remote_context);
    printf("\n");

    print_ssh_host_cache_stats(g_ssh_host_cache);
    printf("\n");

    printf("Network integration status: Operational\n");
    printf("Phase 3 Target 3: Network Integration - IMPLEMENTED\n");

    return 0;
}
