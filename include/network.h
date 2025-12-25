/**
 * network.h - Network Integration and SSH Host Completion for lusush
 *
 * This module provides network-aware features for modern development workflows,
 * including SSH host completion, remote server awareness, and cloud provider
 * integration for DevOps environments.
 *
 * Features:
 *
 * 1. SSH Host Completion:
 *    - Parse ~/.ssh/config and /etc/ssh/ssh_config for host definitions
 *    - Extract Host entries with aliases and patterns
 *    - Complete SSH hostnames with fuzzy matching
 *    - Support for HostName aliases and Port specifications
 *
 * 2. SSH Known Hosts Integration:
 *    - Parse ~/.ssh/known_hosts for previously connected hosts
 *    - Extract hostnames from known_hosts entries
 *    - Support for hashed known_hosts (when readable)
 *    - Merge with SSH config hosts for comprehensive completion
 *
 * 3. Remote Context Awareness:
 *    - Detect when running in SSH session (SSH_CLIENT, SSH_TTY)
 *    - Identify remote server environment and adapt behavior
 *    - Display remote context in prompts and status
 *    - Network connectivity detection and status
 *
 * 4. Cloud Provider Integration:
 *    - AWS EC2 instance discovery and completion
 *    - Google Cloud Platform host completion
 *    - Azure VM discovery and completion
 *    - Generic cloud metadata detection
 *
 * 5. Network Utilities:
 *    - Built-in network connectivity testing
 *    - SSH connection status and management
 *    - VPN and tunnel detection
 *    - Network interface awareness
 *
 * 6. Configuration Integration:
 *    - Network settings in ~/.lusushrc
 *    - SSH completion preferences and caching
 *    - Cloud provider credentials and regions
 *    - Remote session behavior customization
 *
 * Usage:
 *   The network system is automatically initialized and integrates with
 *   the existing completion system. SSH hosts are completed when typing
 *   commands like 'ssh', 'scp', 'rsync', etc.
 *
 * Example completions:
 *   ssh <TAB>        -> Lists all SSH hosts from config and known_hosts
 *   scp file user@<TAB> -> Completes hostnames for SCP operations
 *   rsync -av dir/ host:<TAB> -> Completes remote hostnames
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "readline_integration.h"

#include <stdbool.h>
#include <stddef.h>
#include <strings.h>
#include <time.h>

// Maximum lengths for network identifiers
#define MAX_HOSTNAME_LEN 253
#define MAX_USERNAME_LEN 64
#define MAX_PORT_LEN 8
#define MAX_CONFIG_LINE_LEN 1024
#define MAX_SSH_HOSTS 1000

// SSH host entry structure
typedef struct ssh_host {
    char hostname[MAX_HOSTNAME_LEN + 1]; // Host or HostName
    char alias[MAX_HOSTNAME_LEN + 1];    // Host alias from config
    char user[MAX_USERNAME_LEN + 1];     // User from config
    char port[MAX_PORT_LEN + 1];         // Port from config
    bool from_config;                    // True if from SSH config
    bool from_known_hosts;               // True if from known_hosts
    int priority;                        // Completion priority (0-100)
} ssh_host_t;

// SSH host cache for performance
typedef struct ssh_host_cache {
    ssh_host_t *hosts;   // Array of SSH hosts
    size_t count;        // Number of hosts
    size_t capacity;     // Allocated capacity
    time_t last_updated; // Cache timestamp
    bool needs_refresh;  // Refresh flag
} ssh_host_cache_t;

// Remote context information
typedef struct remote_context {
    bool is_remote_session;                 // Running in SSH session
    bool is_cloud_instance;                 // Running on cloud provider
    char remote_host[MAX_HOSTNAME_LEN + 1]; // Remote hostname
    char remote_user[MAX_USERNAME_LEN + 1]; // Remote username
    char cloud_provider[32];                // Cloud provider name
    char cloud_region[64];                  // Cloud region/zone
    bool has_internet;                      // Internet connectivity
    bool vpn_active;                        // VPN connection detected
} remote_context_t;

// Network configuration settings
typedef struct network_config {
    bool ssh_completion_enabled;   // Enable SSH host completion
    bool cloud_discovery_enabled;  // Enable cloud host discovery
    bool cache_ssh_hosts;          // Cache SSH hosts for performance
    int cache_timeout_minutes;     // Cache refresh timeout
    bool show_remote_context;      // Show remote info in prompt
    bool auto_detect_cloud;        // Auto-detect cloud environment
    char ssh_config_paths[5][256]; // SSH config file paths
    int max_completion_hosts;      // Maximum hosts in completion
} network_config_t;

// Function declarations

// Initialization and cleanup
int network_init(void);
void network_cleanup(void);
int network_load_config(void);

// SSH host completion
void complete_ssh_hosts(const char *text, lusush_completions_t *lc);
int parse_ssh_config(const char *config_path, ssh_host_cache_t *cache);
int parse_ssh_known_hosts(const char *known_hosts_path,
                          ssh_host_cache_t *cache);
ssh_host_cache_t *get_ssh_host_cache(void);
void refresh_ssh_host_cache(void);

// SSH config parsing helpers
int parse_ssh_config_line(const char *line, ssh_host_t *current_host);
bool is_ssh_host_line(const char *line);
bool is_ssh_hostname_line(const char *line);
bool is_ssh_user_line(const char *line);
bool is_ssh_port_line(const char *line);
void extract_ssh_config_value(const char *line, char *value, size_t max_len);

// Known hosts parsing helpers
int parse_known_hosts_line(const char *line, ssh_host_t *host);
bool is_hashed_known_hosts_entry(const char *line);
void extract_hostname_from_known_hosts(const char *entry, char *hostname,
                                       size_t max_len);

// SSH host cache management
ssh_host_cache_t *create_ssh_host_cache(size_t initial_capacity);
void destroy_ssh_host_cache(ssh_host_cache_t *cache);
int add_ssh_host(ssh_host_cache_t *cache, const ssh_host_t *host);
ssh_host_t *find_ssh_host(ssh_host_cache_t *cache, const char *hostname);
void sort_ssh_hosts_by_priority(ssh_host_cache_t *cache);
void deduplicate_ssh_hosts(ssh_host_cache_t *cache);

// Remote context detection
int detect_remote_context(remote_context_t *context);
bool is_ssh_session(void);
bool is_cloud_instance(void);
char *get_remote_hostname(void);
char *get_remote_username(void);
char *detect_cloud_provider(void);
char *get_cloud_region(void);

// Network connectivity
bool has_internet_connectivity(void);
bool is_vpn_active(void);
int get_network_interfaces(char interfaces[][64], int max_interfaces);
bool test_host_connectivity(const char *hostname, int port, int timeout_ms);

// Cloud provider integration
int discover_aws_instances(ssh_host_cache_t *cache);
int discover_gcp_instances(ssh_host_cache_t *cache);
int discover_azure_instances(ssh_host_cache_t *cache);
bool is_aws_instance(void);
bool is_gcp_instance(void);
bool is_azure_instance(void);

// Network command completion
bool is_network_command(const char *command);
void complete_network_command_args(const char *command, const char *text,
                                   lusush_completions_t *lc);
void complete_network_command_args_with_context(const char *command,
                                                const char *text,
                                                lusush_completions_t *lc,
                                                const char *buf, int start_pos);
void complete_ssh_command(const char *text, lusush_completions_t *lc);
void complete_scp_command(const char *text, lusush_completions_t *lc);
void complete_rsync_command(const char *text, lusush_completions_t *lc);

// Utility functions
void trim_whitespace(char *str);
bool is_valid_hostname(const char *hostname);
bool is_valid_port(const char *port);
int hostname_priority_score(const char *hostname, const char *pattern);
void format_ssh_host_completion(const ssh_host_t *host, char *completion,
                                size_t max_len);

// Configuration and preferences
network_config_t *get_network_config(void);
void set_network_config_defaults(network_config_t *config);
int load_network_config_from_file(const char *config_file,
                                  network_config_t *config);
void apply_network_config(const network_config_t *config);

// Debug and diagnostics
void print_ssh_host_cache_stats(const ssh_host_cache_t *cache);
void print_remote_context_info(const remote_context_t *context);
void print_network_config(const network_config_t *config);
int run_network_diagnostics(void);

// Global state access
extern ssh_host_cache_t *g_ssh_host_cache;
extern remote_context_t g_remote_context;
extern network_config_t g_network_config;

#endif /* NETWORK_H */
