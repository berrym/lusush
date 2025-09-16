# Advanced Scripting Guide for Lusush

> **Master the art of shell scripting with Lusush's professional development environment**

Lusush transforms shell scripting from basic automation to professional software development. This guide covers advanced scripting techniques, function systems, debugging workflows, and real-world patterns that make Lusush the premier choice for serious shell developers.

> **⚠️ Development Status Note**: Lusush is under active development with continuously evolving scripting capabilities. While the core features documented here (debugging, functions, local variables) are stable and production-ready, some advanced integration features may be in various stages of implementation. Configuration options and command syntax may change as the shell matures. Always test scripts thoroughly and use `debug help` and `config show` to verify available features in your version.

## 🎯 Why Advanced Scripting Matters

Traditional shells force you to write scripts blindly, debug with echo statements, and guess at variable scoping. Lusush provides **professional development tools** that make complex scripting reliable, maintainable, and debuggable.

### The Lusush Advantage
- **Interactive debugging** with breakpoints and variable inspection
- **Proper function scoping** with local variables
- **Professional error handling** with stack traces
- **Performance profiling** for optimization
- **Real-time script analysis** for quality assurance

## 🏗️ Function System Mastery

### Basic Function Definitions

```bash
# Simple function with proper syntax
function greet() {
    local name=$1
    echo "Hello, $name!"
}

# Alternative syntax (POSIX compatible)
deploy_app() {
    local environment=$1
    local version=${2:-"latest"}
    
    echo "Deploying $version to $environment"
}
```

### Local Variable Scoping

One of Lusush's most powerful features is **proper local variable scoping**—something missing or limited in other shells.

```bash
function advanced_calculator() {
    local operation=$1
    local num1=$2
    local num2=$3
    
    # These variables are truly local to this function
    local result
    local precision=2
    local temp_file="/tmp/calc_$$"
    
    case "$operation" in
        add)
            result=$((num1 + num2))
            ;;
        multiply)
            result=$((num1 * num2))
            ;;
        divide)
            if [[ $num2 -eq 0 ]]; then
                echo "Error: Division by zero" >&2
                return 1
            fi
            result=$(echo "scale=$precision; $num1 / $num2" | bc)
            ;;
        *)
            echo "Unknown operation: $operation" >&2
            return 1
            ;;
    esac
    
    echo "$result"
    return 0
}

# Usage with debugging
debug break add calculator.sh 15 'num2 == 0'
result=$(advanced_calculator divide 10 0)
```

### Function Libraries and Modules

Create reusable function libraries:

```bash
# file: lib/database.sh
function db_connect() {
    local host=$1
    local database=$2
    local user=$3
    
    # Connection logic with local variables
    local connection_string="postgresql://$user@$host/$database"
    local max_retries=3
    local retry_count=0
    
    while [[ $retry_count -lt $max_retries ]]; do
        if pg_isready -h "$host" >/dev/null 2>&1; then
            export DB_CONNECTION="$connection_string"
            return 0
        fi
        
        ((retry_count++))
        echo "Retry $retry_count/$max_retries..." >&2
        sleep 2
    done
    
    echo "Failed to connect to database" >&2
    return 1
}

function db_query() {
    local query=$1
    local output_format=${2:-"table"}
    
    # Local variables for query execution
    local temp_result="/tmp/query_result_$$"
    local exit_code
    
    psql "$DB_CONNECTION" -c "$query" --quiet > "$temp_result"
    exit_code=$?
    
    if [[ $exit_code -eq 0 ]]; then
        cat "$temp_result"
        rm -f "$temp_result"
        return 0
    else
        echo "Query failed with exit code: $exit_code" >&2
        rm -f "$temp_result"
        return $exit_code
    fi
}

# file: main.sh
source lib/database.sh

function main() {
    local environment=$1
    
    # Debug the connection process
    debug break add lib/database.sh 12 'retry_count > 1'
    
    if db_connect "prod-db.company.com" "app_db" "app_user"; then
        db_query "SELECT COUNT(*) FROM users WHERE active = true"
    else
        echo "Database connection failed" >&2
        exit 1
    fi
}

main production
```

### Advanced Function Patterns

#### Error Handling with Stack Traces

```bash
function handle_error() {
    local exit_code=$1
    local line_number=$2
    local function_name=$3
    
    echo "ERROR: Exit code $exit_code at line $line_number in function $function_name" >&2
    
    # Show call stack using Lusush's debug system
    debug stack
    
    # Cleanup and exit
    cleanup_resources
    exit $exit_code
}

function risky_operation() {
    local file=$1
    
    # Set error trap
    trap 'handle_error $? $LINENO ${FUNCNAME[0]}' ERR
    
    # Risky operations
    cp "$file" /important/location/
    process_file "/important/location/$(basename "$file")"
    
    # Clear trap on success
    trap - ERR
}
```

#### Configuration-Driven Functions

```bash
function deploy_service() {
    local config_file=$1
    
    # Local configuration variables
    local service_name
    local target_hosts
    local health_check_url
    local timeout
    
    # Parse configuration with proper scoping
    service_name=$(config_get "$config_file" "service.name")
    target_hosts=$(config_get "$config_file" "deployment.hosts")
    health_check_url=$(config_get "$config_file" "service.health_check")
    timeout=$(config_get "$config_file" "deployment.timeout" "300")
    
    echo "Deploying $service_name to: $target_hosts"
    
    # Debug deployment process
    debug break add deploy.sh 25 'timeout > 600'
    
    for host in $target_hosts; do
        deploy_to_host "$host" "$service_name" "$health_check_url" "$timeout"
    done
}

function config_get() {
    local file=$1
    local key=$2
    local default_value=${3:-""}
    
    # Local parsing variables
    local section
    local current_section=""
    local value
    
    # Parse INI-style configuration
    while IFS='=' read -r config_key config_value; do
        # Handle sections
        if [[ $config_key =~ ^\[(.+)\]$ ]]; then
            current_section="${BASH_REMATCH[1]}"
            continue
        fi
        
        # Handle key-value pairs
        if [[ "$current_section.$config_key" == "$key" ]]; then
            echo "$config_value"
            return 0
        fi
    done < "$file"
    
    echo "$default_value"
}
```

## 🐛 Integrated Debugging Workflows

### Setting Up Debug Sessions

```bash
# Enable comprehensive debugging
debug on 3                              # Full debugging with traces
debug profile on                        # Performance profiling

# Set strategic breakpoints
debug break add deploy.sh 45 'error_count > 0'
debug break add lib/utils.sh 23
debug break add main.sh 67 'retries >= max_retries'

# Execute your script
./deploy.sh production v2.1.0
```

### Interactive Debugging Commands

When a breakpoint hits, you have full debugging capabilities:

```bash
# Variable inspection
vars                    # Show all variables in current scope
print $service_name     # Print specific variable
print ${config_hosts[@]} # Print array contents

# Execution control
step                    # Step into next statement (enters functions)
next                    # Step over next statement (skips function internals)
continue                # Continue to next breakpoint

# Context inspection
stack                   # Show full call stack
where                   # Show current location and context
```

### Conditional Breakpoints for Complex Logic

```bash
function process_batch() {
    local batch_size=$1
    local total_items=$2
    
    local processed=0
    local errors=0
    local batch_count=0
    
    # Set conditional breakpoint for error conditions
    debug break add batch.sh 15 'errors > 5'
    debug break add batch.sh 28 'processed == 0 && batch_count > 1'
    
    while [[ $processed -lt $total_items ]]; do
        local current_batch_size=$((total_items - processed))
        if [[ $current_batch_size -gt $batch_size ]]; then
            current_batch_size=$batch_size
        fi
        
        ((batch_count++))
        echo "Processing batch $batch_count (size: $current_batch_size)"
        
        # Process items (breakpoint here for error monitoring)
        if ! process_items $processed $current_batch_size; then
            ((errors++))
            echo "Batch $batch_count failed"
        fi
        
        processed=$((processed + current_batch_size))
    done
    
    echo "Processed $processed items with $errors errors"
}
```

### Performance Profiling Integration

```bash
function performance_critical_function() {
    local data_file=$1
    
    # Start profiling this function
    debug profile on
    
    # CPU-intensive operations
    local line_count=$(wc -l < "$data_file")
    local processed=0
    
    while IFS= read -r line; do
        # Complex processing
        process_line "$line"
        ((processed++))
        
        # Progress reporting (profile this section)
        if ((processed % 1000 == 0)); then
            echo "Processed $processed/$line_count lines"
        fi
    done < "$data_file"
    
    # Generate performance report
    debug profile report
}
```

## 🏢 Real-World Enterprise Patterns

### Configuration Management System

```bash
# file: lib/config_manager.sh

function config_init() {
    local config_dir=${1:-"/etc/myapp"}
    local environment=${2:-"development"}
    
    # Global configuration state (properly scoped)
    local config_file="$config_dir/$environment.conf"
    local schema_file="$config_dir/schema.json"
    
    # Validate configuration exists
    if [[ ! -f "$config_file" ]]; then
        echo "Configuration file not found: $config_file" >&2
        return 1
    fi
    
    # Load and validate configuration
    export CONFIG_FILE="$config_file"
    export CONFIG_ENVIRONMENT="$environment"
    
    config_validate || return 1
    
    echo "Configuration loaded for environment: $environment"
}

function config_validate() {
    local required_keys="database.host api.port logging.level"
    local missing_keys=""
    
    for key in $required_keys; do
        if ! config_has_key "$key"; then
            missing_keys="$missing_keys $key"
        fi
    done
    
    if [ -n "$missing_keys" ]; then
        echo "Missing required configuration keys:" >&2
        for key in $missing_keys; do
            echo "  - $key" >&2
        done
        return 1
    fi
    
    return 0
}

function config_has_key() {
    local key=$1
    local value
    
    value=$(config_get_value "$key")
    [[ -n "$value" ]]
}

function config_get_value() {
    local key=$1
    local section="${key%.*}"
    local property="${key##*.}"
    
    # Use Lusush's built-in config system when available
    if command -v config >/dev/null 2>&1; then
        config get "$key" 2>/dev/null
    else
        # Fallback to manual parsing
        awk -F'=' -v section="[$section]" -v prop="$property" '
            $0 == section { in_section = 1; next }
            /^\[/ { in_section = 0; next }
            in_section && $1 == prop { print $2; exit }
        ' "$CONFIG_FILE"
    fi
}
```

### Deployment Orchestration

```bash
# file: deploy.sh

function deploy_application() {
    local environment=$1
    local version=$2
    local config_file="configs/${environment}.conf"
    
    # Deployment state management
    local deployment_id="deploy_$(date +%Y%m%d_%H%M%S)"
    local log_file="logs/${deployment_id}.log"
    local rollback_data="/tmp/rollback_${deployment_id}.json"
    
    echo "Starting deployment $deployment_id" | tee -a "$log_file"
    
    # Initialize configuration and debugging
    config_init "$(dirname "$config_file")" "$environment"
    debug break add deploy.sh 50 'deployment_failed == true'
    
    # Pre-deployment validation
    if ! validate_deployment_environment "$environment"; then
        echo "Environment validation failed" >&2
        return 1
    fi
    
    # Create rollback checkpoint
    create_rollback_checkpoint "$rollback_data"
    
    # Execute deployment phases with error handling
    local deployment_failed=false
    
    deploy_phase "database" migrate_database || deployment_failed=true
    deploy_phase "services" deploy_services "$version" || deployment_failed=true
    deploy_phase "configuration" update_configuration || deployment_failed=true
    deploy_phase "health_check" verify_deployment || deployment_failed=true
    
    # Handle deployment result
    if [[ "$deployment_failed" == "true" ]]; then
        echo "Deployment failed, initiating rollback..." | tee -a "$log_file"
        rollback_deployment "$rollback_data"
        return 1
    else
        echo "Deployment $deployment_id completed successfully" | tee -a "$log_file"
        cleanup_rollback_data "$rollback_data"
        return 0
    fi
}

function deploy_phase() {
    local phase_name=$1
    local phase_function=$2
    shift 2
    local phase_args=("$@")
    
    echo "Executing phase: $phase_name"
    
    # Phase-specific debugging
    debug break add deploy.sh 85 'phase_name == "services"'
    
    if "$phase_function" "${phase_args[@]}"; then
        echo "Phase $phase_name completed successfully"
        return 0
    else
        echo "Phase $phase_name failed" >&2
        return 1
    fi
}

function migrate_database() {
    local migration_dir="migrations"
    local applied_migrations=()
    local pending_migrations=()
    
    # Get list of applied migrations
    mapfile -t applied_migrations < <(db_query "SELECT version FROM schema_migrations ORDER BY version")
    
    # Find pending migrations
    for migration_file in "$migration_dir"/*.sql; do
        local version
        version=$(basename "$migration_file" .sql)
        
        if ! in_array "$version" "${applied_migrations[@]}"; then
            pending_migrations+=("$migration_file")
        fi
    done
    
    # Apply pending migrations with progress tracking
    local total=${#pending_migrations[@]}
    local current=0
    
    for migration in $pending_migrations; do
        ((current++))
        echo "Applying migration $current/$total: $(basename "$migration")"
        
        if ! db_query_file "$migration"; then
            echo "Migration failed: $migration" >&2
            return 1
        fi
        
        # Record successful migration
        local version
        version=$(basename "$migration" .sql)
        db_query "INSERT INTO schema_migrations (version) VALUES ('$version')"
    done
    
    echo "Database migration completed ($total migrations applied)"
}
```

### Monitoring and Alerting Integration

```bash
function monitor_deployment() {
    local deployment_id=$1
    local service_urls=()
    local alert_channels=()
    
    # Load monitoring configuration
    mapfile -t service_urls < <(config_get_value "monitoring.health_check_urls" | tr ',' '\n')
    mapfile -t alert_channels < <(config_get_value "alerts.channels" | tr ',' '\n')
    
    local failed_services=()
    local start_time=$(date +%s)
    local timeout=300  # 5 minutes
    
    echo "Starting health monitoring for deployment $deployment_id"
    
    # Continuous monitoring loop with debugging
    debug break add monitor.sh 25 'failed_count > 2'
    
    while true; do
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        
        if [[ $elapsed -gt $timeout ]]; then
            echo "Monitoring timeout reached"
            break
        fi
        
        local failed_count=0
        failed_services=()
        
        for url in $service_urls; do
            if ! check_service_health "$url"; then
                failed_services="$failed_services $url"
                ((failed_count++))
            fi
        done
        
        if [[ $failed_count -eq 0 ]]; then
            echo "All services healthy - monitoring complete"
            return 0
        fi
        
        echo "Health check failed for $failed_count services, retrying..."
        sleep 10
    done
    
    # Send alerts for failed services
    if [[ ${#failed_services[@]} -gt 0 ]]; then
        send_deployment_alert "$deployment_id" "${failed_services[@]}"
        return 1
    fi
}

function check_service_health() {
    local url=$1
    local max_retries=3
    local retry_delay=2
    
    for ((i = 1; i <= max_retries; i++)); do
        local response_code
        response_code=$(curl -s -o /dev/null -w "%{http_code}" "$url" --max-time 10)
        
        if [[ "$response_code" == "200" ]]; then
            return 0
        fi
        
        if [[ $i -lt $max_retries ]]; then
            sleep $retry_delay
        fi
    done
    
    echo "Health check failed for $url (HTTP $response_code)" >&2
    return 1
}
```

## 🔧 Configuration Integration

### Using Lusush's Configuration System

> **Note**: Configuration integration features may vary by version. Use `config show` to see available options.

```bash
# Configure script behavior at runtime
config set debug_mode true
config set history_size 5000
config set theme_name dark

function configurable_operation() {
    local operation=$1
    
    # Get configuration values with defaults
    local debug_enabled
    local max_retries
    local timeout
    
    debug_enabled=$(config get debug_mode || echo "false")
    max_retries=3  # Default value for now
    timeout=60     # Default value for now
    
    # Use configuration to control behavior
    if [ "$debug_enabled" = "true" ]; then
        debug on 2
    fi
    
    local attempt=0
    while [[ $attempt -lt $max_retries ]]; do
        ((attempt++))
        
        echo "Attempt $attempt/$max_retries for operation: $operation"
        
        if timeout "$timeout" perform_operation "$operation"; then
            echo "Operation succeeded on attempt $attempt"
            return 0
        fi
        
        echo "Attempt $attempt failed, retrying..."
        sleep $((attempt * 2))  # Exponential backoff
    done
    
    echo "Operation failed after $max_retries attempts" >&2
    return 1
}
```

## 📊 Performance Optimization

### Profiling and Optimization

```bash
function optimize_data_processing() {
    local input_file=$1
    local output_file=$2
    
    echo "Starting performance analysis"
    debug profile on
    
    # Benchmark different approaches
    local start_time end_time
    
    # Method 1: Line-by-line processing
    start_time=$(date +%s%N)
    process_file_line_by_line "$input_file" "$output_file.method1"
    end_time=$(date +%s%N)
    local method1_time=$(( (end_time - start_time) / 1000000 ))
    
    # Method 2: Batch processing
    start_time=$(date +%s%N)
    process_file_in_batches "$input_file" "$output_file.method2" 1000
    end_time=$(date +%s%N)
    local method2_time=$(( (end_time - start_time) / 1000000 ))
    
    # Method 3: Parallel processing
    start_time=$(date +%s%N)
    process_file_parallel "$input_file" "$output_file.method3"
    end_time=$(date +%s%N)
    local method3_time=$(( (end_time - start_time) / 1000000 ))
    
    # Performance analysis
    echo "Performance Results:"
    echo "  Line-by-line: ${method1_time}ms"
    echo "  Batch processing: ${method2_time}ms"
    echo "  Parallel processing: ${method3_time}ms"
    
    # Choose optimal method
    local best_time=$method1_time
    local best_method="method1"
    local best_file="$output_file.method1"
    
    if [[ $method2_time -lt $best_time ]]; then
        best_time=$method2_time
        best_method="method2"
        best_file="$output_file.method2"
    fi
    
    if [[ $method3_time -lt $best_time ]]; then
        best_time=$method3_time
        best_method="method3"
        best_file="$output_file.method3"
    fi
    
    echo "Optimal method: $best_method (${best_time}ms)"
    cp "$best_file" "$output_file"
    
    # Generate detailed performance report
    debug profile report
}
```

## 🎨 Advanced Multiline Constructs

### Complex Control Structures

```bash
function advanced_deployment_logic() {
    local environment=$1
    
    # Complex nested structures with proper continuation prompts
    case "$environment" in
        production|prod)
            echo "Production deployment - extra safety checks"
            
            # Nested if within case
            if [[ -f "maintenance.lock" ]]; then
                echo "Maintenance mode detected"
                
                # Nested while within if within case
                while [[ -f "maintenance.lock" ]]; do
                    echo "Waiting for maintenance to complete..."
                    sleep 30
                    
                    # Nested for within while within if within case
                    for service in api web worker; do
                        check_service_status "$service" || {
                            echo "Service $service not responding"
                            return 1
                        }
                    done
                done
            fi
            
            # Deploy to production with extra validation
            deploy_with_validation "production"
            ;;
            
        staging|stage)
            echo "Staging deployment"
            
            # Simple deployment for staging
            for component in database api frontend; do
                if ! deploy_component "$component" "staging"; then
                    echo "Failed to deploy $component" >&2
                    rollback_staging_deployment
                    return 1
                fi
            done
            ;;
            
        development|dev)
            echo "Development deployment - fast and loose"
            quick_deploy "development"
            ;;
            
        *)
            echo "Unknown environment: $environment" >&2
            echo "Valid options: production, staging, development"
            return 1
            ;;
    esac
    
    # Post-deployment verification
    echo "Verifying deployment..."
    
    local services_to_check=("api" "web" "worker")
    local all_healthy=true
    
    for service in $services_to_check; do
        if ! verify_service_health "$service"; then
            echo "Service $service failed health check" >&2
            all_healthy=false
        fi
    done
    
    if [[ "$all_healthy" == "true" ]]; then
        echo "Deployment completed successfully"
        return 0
    else
        echo "Some services failed health checks" >&2
        return 1
    fi
}
```

## 📚 Best Practices and Patterns

### Error Handling Best Practices

```bash
# Global error handling setup
set -euo pipefail  # Exit on error, undefined vars, pipe failures

function setup_error_handling() {
    # Set up comprehensive error trapping
    trap 'handle_script_error $? $LINENO $BASH_COMMAND' ERR
    trap 'handle_script_exit' EXIT
    trap 'handle_script_interrupt' INT TERM
}

function handle_script_error() {
    local exit_code=$1
    local line_number=$2
    local failed_command=$3
    
    echo "SCRIPT ERROR:" >&2
    echo "  Exit Code: $exit_code" >&2
    echo "  Line Number: $line_number" >&2
    echo "  Failed Command: $failed_command" >&2
    
    # Use Lusush debugging for detailed analysis
    debug stack
    
    # Cleanup and exit
    cleanup_on_error
    exit $exit_code
}

function retry_with_backoff() {
    local max_attempts=$1
    local base_delay=$2
    shift 2
    local command=("$@")
    
    local attempt=1
    local delay=$base_delay
    
    while [[ $attempt -le $max_attempts ]]; do
        echo "Attempt $attempt/$max_attempts: ${command[*]}"
        
        if "${command[@]}"; then
            echo "Command succeeded on attempt $attempt"
            return 0
        fi
        
        if [[ $attempt -eq $max_attempts ]]; then
            echo "Command failed after $max_attempts attempts" >&2
            return 1
        fi
        
        echo "Attempt $attempt failed, waiting ${delay}s before retry..."
        sleep "$delay"
        
        # Exponential backoff
        delay=$((delay * 2))
        ((attempt++))
    done
}
```

### Logging and Monitoring Integration

```bash
function setup_logging() {
    local log_level=${1:-"INFO"}
    local log_file=${2:-"script.log"}
    
    # Configure logging based on environment
    export LOG_LEVEL="$log_level"
    export LOG_FILE="$log_file"
    
    # Create log directory if needed
    mkdir -p "$(dirname "$log_file")"
    
    echo "$(date '+%Y-%m-%d %H:%M:%S') [INFO] Logging initialized" >> "$log_file"
}

function log() {
    local level=$1
    local message=$2
    local timestamp
    
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    # Log to file
    echo "$timestamp [$level] $message" >> "$LOG_FILE"
    
    # Also log to console based on level
    case "$level" in
        ERROR)
            echo "$timestamp [$level] $message" >&2
            ;;
        WARN|INFO)
            echo "$timestamp [$level] $message"
            ;;
        DEBUG)
            if [[ "$LOG_LEVEL" == "DEBUG" ]]; then
                echo "$timestamp [$level] $message"
            fi
            ;;
    esac
}
```

## 🚀 Advanced Examples Repository

### Complete Application Deployment Script

See the complete example in the repository:

```bash
# Clone example repository
git clone https://github.com/berrym/lusush-examples.git
cd lusush-examples/advanced-scripting

# Study the examples:
ls -la
# - deployment-orchestration.sh
# - database-migration-system.sh
# - monitoring-and-alerting.sh
# - configuration-management.sh
# - performance-optimization.sh
```

### Testing Your Advanced Scripts

```bash
# Test with debugging enabled
debug on 3
debug break add your_script.sh 25 'error_count > 0'
./your_script.sh

# Performance testing
debug profile on
time ./your_script.sh
debug profile report

# Configuration testing
config set script.timeout 30
config set script.retries 5
./your_script.sh
```

## 🎓 Mastery Checklist

Mark your progress as you master each concept:

### Basic Mastery
- [ ] Define functions with proper local variable scoping
- [ ] Use continuation prompts for complex constructs
- [ ] Set breakpoints and debug interactively
- [ ] Configure script behavior at runtime

### Intermediate Mastery
- [ ] Create reusable function libraries
- [ ] Implement proper error handling with stack traces
- [ ] Use conditional breakpoints for complex logic
- [ ] Profile and optimize script performance

### Advanced Mastery
- [ ] Build configuration-driven deployment systems
- [ ] Implement enterprise monitoring and alerting
- [ ] Create modular, testable script architectures
- [ ] Optimize performance using profiling data

### Expert Mastery
- [ ] Design complete development workflows
- [ ] Integrate with external systems and APIs
- [ ] Build fault-tolerant distributed operations
- [ ] Mentor others in advanced scripting techniques

## 🌟 Next Steps

1. **Practice with real projects** - Apply these techniques to your actual work
2. **Study the examples** - Examine real-world patterns in the examples repository
3. **Contribute back** - Share your advanced patterns with the community
4. **Master the debugging system** - Become proficient with all debugging features
5. **Build your own libraries** - Create reusable components for your domain

## 📖 Additional Resources

- **[Configuration Mastery Guide](CONFIGURATION_MASTERY_GUIDE.md)** - Deep dive into configuration management
- **[Debugging Reference](docs/DEBUGGING.md)** - Complete debugging command reference
- **[Performance Guide](docs/PERFORMANCE.md)** - Optimization techniques and patterns
- **[Examples Repository](https://github.com/berrym/lusush-examples)** - Real-world script examples

## ⚠️ Development & Compatibility Notes

**Script Compatibility**: Core scripting features (functions, local variables, debugging) are stable across Lusush versions. Configuration integration and advanced features may evolve.

**Version Checking**: Use these commands to verify available features:
- `debug help` - See current debugging capabilities
- `config show` - View available configuration options  
- `lusush --version` - Check your version

**Future-Proofing Scripts**: Focus on core Lusush features (debugging, functions) which are stable, and test configuration integration features as they may change between versions.

---

**Master advanced scripting with Lusush and transform your shell scripts into professional software.**

*Happy scripting! 🚀*