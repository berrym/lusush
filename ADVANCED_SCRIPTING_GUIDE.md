# Advanced Scripting Guide for Lusush

> **Professional shell scripting with modern development tools**

Lusush provides advanced scripting capabilities that go beyond standard POSIX shells. This guide covers the enhanced function system, debugging workflows, multiline construct handling, and real-world scripting patterns that make Lusush suitable for professional development environments.

## 🎯 What Makes Lusush Advanced

Lusush builds upon solid POSIX foundations while adding modern development features:

- **Enhanced function system** with parameter validation and advanced return values
- **Professional debugging tools** for script development and troubleshooting
- **Complete multiline support** for complex constructs via stdin/pipe
- **Function introspection** for development and maintenance
- **Robust parsing** that handles real-world shell scripts correctly

## 🏗️ Enhanced Function System

### Function Parameter Validation

Lusush supports function parameter definitions with validation and default values:

```bash
# Function with required and optional parameters
function deploy(environment, version="latest") {
    if [ -z "$environment" ]; then
        echo "Error: Environment parameter is required"
        return 1
    fi
    
    echo "Deploying $version to $environment"
    return 0
}

# Function calls with parameter validation
deploy production v2.1.0     # Uses specified version
deploy staging               # Uses default version "latest"
deploy                       # Error: Environment required
```

### Advanced Return Values

Beyond exit codes, Lusush functions can return string values:

```bash
function calculate_total(base, tax_rate="0.08") {
    local tax=$(echo "$base * $tax_rate" | bc -l)
    local total=$(echo "$base + $tax" | bc -l)
    return_value "$total"
}

# Capture function return value
total=$(calculate_total 100.00 0.10)
echo "Total cost: $total"  # Output: Total cost: 110.00

# Use in conditional expressions
if [ "$(calculate_total 50)" = "54.00" ]; then
    echo "Tax calculation correct"
fi
```

### Function Introspection and Debugging

Lusush provides built-in tools for function development:

```bash
# Define a complex function
function process_data(input_file, output_format="json") {
    local temp_file="/tmp/processing_$$"
    local line_count=0
    
    if [ ! -f "$input_file" ]; then
        echo "Error: Input file not found: $input_file"
        return 1
    fi
    
    # Process data based on format
    case "$output_format" in
        "json") 
            process_as_json "$input_file" > "$temp_file"
            ;;
        "csv")
            process_as_csv "$input_file" > "$temp_file"
            ;;
        *)
            echo "Error: Unknown format: $output_format"
            return 2
            ;;
    esac
    
    line_count=$(wc -l < "$temp_file")
    return_value "$line_count"
}

# Function introspection
debug functions                          # List all defined functions
debug function process_data             # Show function details and usage
```

## 🔍 Professional Debugging Workflow

### Debug Command Overview

Lusush provides comprehensive debugging capabilities:

```bash
# Enable debugging
debug on 2                              # Set debug level (0-4)
debug trace on                          # Enable execution tracing

# Breakpoint management
debug break add script.sh 25            # Add breakpoint at line 25
debug break add script.sh 30 'count > 10'  # Conditional breakpoint
debug break list                         # List all breakpoints
debug break remove 1                     # Remove breakpoint by ID

# Execution control
debug step                              # Step into next statement
debug next                              # Step over next statement
debug continue                          # Continue execution

# Variable inspection
debug vars                              # Show all variables
debug print variable_name               # Print specific variable
debug stack                             # Show call stack

# Performance analysis
debug profile on                        # Enable profiling
debug profile report                    # Show performance report
debug profile reset                     # Reset profiling data
```

### Debugging Real-World Scripts

```bash
function backup_system(backup_path, compression="gzip") {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_file="$backup_path/system_backup_$timestamp.tar"
    local temp_list="/tmp/backup_files_$$"
    
    echo "Starting system backup to $backup_file"
    
    # Create list of files to backup
    find /etc /home -type f -size -100M > "$temp_list" 2>/dev/null
    
    if [ ! -s "$temp_list" ]; then
        echo "Error: No files found to backup"
        return 1
    fi
    
    # Perform backup based on compression type
    case "$compression" in
        "gzip")
            tar czf "$backup_file.gz" -T "$temp_list"
            ;;
        "bzip2")
            tar cjf "$backup_file.bz2" -T "$temp_list"
            ;;
        "none")
            tar cf "$backup_file" -T "$temp_list"
            ;;
        *)
            echo "Error: Unknown compression: $compression"
            return 2
            ;;
    esac
    
    local exit_code=$?
    rm -f "$temp_list"
    
    if [ $exit_code -eq 0 ]; then
        echo "Backup completed successfully"
        return_value "$backup_file"
    else
        echo "Backup failed with exit code $exit_code"
        return $exit_code
    fi
}

# Debug the backup function
debug break add backup.sh 15            # Break at compression switch
debug on 2                              # Enable detailed debugging
backup_result=$(backup_system /backups gzip)
echo "Backup result: $backup_result"
```

## 🔄 Multiline Construct Mastery

### Complex Multiline Functions

Lusush handles sophisticated multiline function definitions correctly:

```bash
function deploy_application(environment, config_path, options="") {
    local app_name="myapp"
    local deploy_dir="/opt/$app_name"
    local config_file="$config_path/$environment.conf"
    local log_file="/var/log/deploy_$(date +%Y%m%d).log"
    
    echo "Starting deployment to $environment" | tee -a "$log_file"
    
    # Validate environment configuration
    if [ ! -f "$config_file" ]; then
        echo "Error: Configuration not found: $config_file" | tee -a "$log_file"
        return 1
    fi
    
    # Create deployment directory if needed
    if [ ! -d "$deploy_dir" ]; then
        if ! mkdir -p "$deploy_dir"; then
            echo "Error: Cannot create deploy directory" | tee -a "$log_file"
            return 2
        fi
    fi
    
    # Deploy application files
    for component in api web worker; do
        echo "Deploying $component..." | tee -a "$log_file"
        
        if [ -d "dist/$component" ]; then
            if ! cp -r "dist/$component" "$deploy_dir/"; then
                echo "Error: Failed to deploy $component" | tee -a "$log_file"
                return 3
            fi
        else
            echo "Warning: Component $component not found" | tee -a "$log_file"
        fi
    done
    
    # Apply configuration
    if ! cp "$config_file" "$deploy_dir/config.conf"; then
        echo "Error: Failed to copy configuration" | tee -a "$log_file"
        return 4
    fi
    
    echo "Deployment completed successfully" | tee -a "$log_file"
    return_value "$deploy_dir"
}
```

### Advanced Case Statement Patterns

```bash
function process_request(method, path, data="") {
    local response_code=200
    local response_body=""
    
    case "$method" in
        "GET")
            case "$path" in
                "/api/users")
                    response_body='{"users": []}'
                    ;;
                "/api/status")
                    response_body='{"status": "healthy"}'
                    ;;
                *)
                    response_code=404
                    response_body='{"error": "Not found"}'
                    ;;
            esac
            ;;
        "POST")
            case "$path" in
                "/api/users")
                    if [ -n "$data" ]; then
                        response_body='{"message": "User created"}'
                        response_code=201
                    else
                        response_code=400
                        response_body='{"error": "Missing data"}'
                    fi
                    ;;
                *)
                    response_code=404
                    response_body='{"error": "Not found"}'
                    ;;
            esac
            ;;
        "PUT"|"PATCH")
            response_code=501
            response_body='{"error": "Not implemented"}'
            ;;
        *)
            response_code=405
            response_body='{"error": "Method not allowed"}'
            ;;
    esac
    
    echo "HTTP/1.1 $response_code"
    echo "Content-Type: application/json"
    echo
    echo "$response_body"
}
```

### Here Document Applications

```bash
function generate_config(app_name, environment, database_url) {
    local config_file="/tmp/${app_name}_${environment}.conf"
    
    cat <<CONFIG_END > "$config_file"
# Generated configuration for $app_name
# Environment: $environment
# Generated: $(date)

[application]
name = $app_name
environment = $environment
debug = $([ "$environment" = "development" ] && echo "true" || echo "false")

[database]
url = $database_url
pool_size = 10
timeout = 30

[logging]
level = $([ "$environment" = "production" ] && echo "INFO" || echo "DEBUG")
file = /var/log/$app_name.log
CONFIG_END

    return_value "$config_file"
}

function send_notification(recipient, subject, message) {
    local email_file="/tmp/notification_$$.txt"
    
    cat <<EMAIL_END > "$email_file"
To: $recipient
Subject: $subject
Date: $(date -R)

$message

--
This is an automated message from $(hostname)
Generated at $(date)
EMAIL_END

    # Send email (mock implementation)
    echo "Email saved to $email_file"
    return_value "$email_file"
}
```

## 🛠️ Real-World Scripting Patterns

### Configuration Management

```bash
function load_config(config_file, section="") {
    local current_section=""
    local found_value=""
    
    if [ ! -f "$config_file" ]; then
        echo "Error: Configuration file not found: $config_file"
        return 1
    fi
    
    while IFS='=' read -r key value; do
        # Handle section headers
        case "$key" in
            \[*\])
                current_section="${key#[}"
                current_section="${current_section%]}"
                ;;
            \#*|"")
                # Skip comments and empty lines
                continue
                ;;
            *)
                if [ -n "$section" ] && [ "$current_section" = "$section" ]; then
                    echo "$key=$value"
                elif [ -z "$section" ]; then
                    echo "[$current_section] $key=$value"
                fi
                ;;
        esac
    done < "$config_file"
}

function validate_config(config_file) {
    local errors=0
    local required_keys="database_url app_name environment"
    
    for key in $required_keys; do
        if ! grep -q "^$key=" "$config_file"; then
            echo "Error: Missing required configuration: $key"
            errors=$((errors + 1))
        fi
    done
    
    return $errors
}
```

### Error Handling and Retry Logic

```bash
function retry_command(max_attempts, delay, command_args) {
    local attempt=1
    local exit_code=0
    
    while [ $attempt -le "$max_attempts" ]; do
        echo "Attempt $attempt/$max_attempts: $command_args"
        
        if eval "$command_args"; then
            echo "Command succeeded on attempt $attempt"
            return 0
        fi
        
        exit_code=$?
        
        if [ $attempt -eq "$max_attempts" ]; then
            echo "Command failed after $max_attempts attempts (exit code: $exit_code)"
            return $exit_code
        fi
        
        echo "Attempt $attempt failed, retrying in $delay seconds..."
        sleep "$delay"
        attempt=$((attempt + 1))
    done
}

function safe_execute(command_args, timeout="30") {
    local temp_output="/tmp/safe_execute_$$"
    local temp_error="/tmp/safe_execute_err_$$"
    local exit_code=0
    
    # Execute with timeout
    timeout "$timeout" sh -c "$command_args" > "$temp_output" 2> "$temp_error"
    exit_code=$?
    
    case $exit_code in
        0)
            cat "$temp_output"
            ;;
        124)
            echo "Error: Command timed out after $timeout seconds"
            ;;
        *)
            echo "Error: Command failed with exit code $exit_code"
            cat "$temp_error" >&2
            ;;
    esac
    
    rm -f "$temp_output" "$temp_error"
    return $exit_code
}
```

### Data Processing Pipelines

```bash
function process_log_file(log_file, pattern, output_format="summary") {
    local temp_matches="/tmp/matches_$$"
    local temp_summary="/tmp/summary_$$"
    
    if [ ! -f "$log_file" ]; then
        echo "Error: Log file not found: $log_file"
        return 1
    fi
    
    # Extract matching lines
    grep "$pattern" "$log_file" > "$temp_matches" || {
        echo "No matches found for pattern: $pattern"
        return 0
    }
    
    local match_count=$(wc -l < "$temp_matches")
    echo "Found $match_count matches for pattern: $pattern"
    
    case "$output_format" in
        "summary")
            # Generate summary
            {
                echo "Log Analysis Summary"
                echo "===================="
                echo "File: $log_file"
                echo "Pattern: $pattern"
                echo "Matches: $match_count"
                echo "Analysis time: $(date)"
                echo ""
                echo "Recent matches:"
                tail -10 "$temp_matches"
            } > "$temp_summary"
            
            cat "$temp_summary"
            return_value "$temp_summary"
            ;;
        "full")
            cat "$temp_matches"
            return_value "$temp_matches"
            ;;
        "count")
            echo "$match_count"
            return_value "$match_count"
            ;;
        *)
            echo "Error: Unknown output format: $output_format"
            rm -f "$temp_matches" "$temp_summary"
            return 1
            ;;
    esac
    
    rm -f "$temp_matches"
}
```

## 🚀 Best Practices

### Function Organization

```bash
# Library-style function organization
function string_length(input_string) {
    return_value "${#input_string}"
}

function string_contains(haystack, needle) {
    case "$haystack" in
        *"$needle"*) return_value "true" ;;
        *) return_value "false" ;;
    esac
}

function string_trim(input_string) {
    # Remove leading whitespace
    local trimmed="${input_string#"${input_string%%[![:space:]]*}"}"
    # Remove trailing whitespace
    trimmed="${trimmed%"${trimmed##*[![:space:]]}"}"
    return_value "$trimmed"
}

# Use functions as building blocks
function validate_email(email_address) {
    local length=$(string_length "$email_address")
    
    if [ "$length" -lt 5 ]; then
        return_value "false"
        return
    fi
    
    if $(string_contains "$email_address" "@"); then
        return_value "true"
    else
        return_value "false"
    fi
}
```

### Error Handling Patterns

```bash
function robust_file_operation(source_file, dest_file, operation="copy") {
    # Input validation
    if [ -z "$source_file" ] || [ -z "$dest_file" ]; then
        echo "Error: Source and destination files required"
        return 1
    fi
    
    if [ ! -f "$source_file" ]; then
        echo "Error: Source file not found: $source_file"
        return 2
    fi
    
    # Check destination directory
    local dest_dir=$(dirname "$dest_file")
    if [ ! -d "$dest_dir" ]; then
        echo "Creating destination directory: $dest_dir"
        if ! mkdir -p "$dest_dir"; then
            echo "Error: Cannot create destination directory"
            return 3
        fi
    fi
    
    # Perform operation
    case "$operation" in
        "copy")
            if cp "$source_file" "$dest_file"; then
                echo "File copied successfully"
                return_value "$dest_file"
            else
                echo "Error: Copy operation failed"
                return 4
            fi
            ;;
        "move")
            if mv "$source_file" "$dest_file"; then
                echo "File moved successfully"
                return_value "$dest_file"
            else
                echo "Error: Move operation failed"
                return 5
            fi
            ;;
        *)
            echo "Error: Unknown operation: $operation"
            return 6
            ;;
    esac
}
```

### Testing and Validation

```bash
function run_tests() {
    local test_count=0
    local pass_count=0
    local fail_count=0
    
    echo "Running Lusush script tests..."
    echo "=============================="
    
    # Test function parameter validation
    test_count=$((test_count + 1))
    if result=$(string_length "hello"); then
        if [ "$result" = "5" ]; then
            echo "PASS: String length calculation"
            pass_count=$((pass_count + 1))
        else
            echo "FAIL: String length calculation (expected 5, got $result)"
            fail_count=$((fail_count + 1))
        fi
    else
        echo "FAIL: String length function error"
        fail_count=$((fail_count + 1))
    fi
    
    # Test return value system
    test_count=$((test_count + 1))
    if $(string_contains "hello world" "world"); then
        echo "PASS: String contains detection"
        pass_count=$((pass_count + 1))
    else
        echo "FAIL: String contains detection"
        fail_count=$((fail_count + 1))
    fi
    
    # Test error handling
    test_count=$((test_count + 1))
    if ! robust_file_operation "/nonexistent" "/tmp/test" "copy" 2>/dev/null; then
        echo "PASS: Error handling for missing file"
        pass_count=$((pass_count + 1))
    else
        echo "FAIL: Should have failed for missing file"
        fail_count=$((fail_count + 1))
    fi
    
    echo ""
    echo "Test Results:"
    echo "============="
    echo "Total: $test_count"
    echo "Passed: $pass_count"
    echo "Failed: $fail_count"
    
    if [ $fail_count -eq 0 ]; then
        echo "All tests passed!"
        return 0
    else
        echo "Some tests failed."
        return 1
    fi
}
```

## 🔧 Development Workflow Integration

### Script Development Process

1. **Write functions with parameter validation**
2. **Use return_value for complex return data**
3. **Add debug breakpoints during development**
4. **Test multiline constructs thoroughly**
5. **Use function introspection for documentation**
6. **Implement comprehensive error handling**

### Debugging Session Example

```bash
# 1. Enable debugging
debug on 2
debug trace on

# 2. Set strategic breakpoints
debug break add myscript.sh 25 'error_count > 0'
debug break add myscript.sh 50

# 3. Run your script
./myscript.sh production

# 4. When breakpoint hits:
# - Use 'debug vars' to inspect variables
# - Use 'debug stack' to see call hierarchy
# - Use 'debug step' to step through execution
# - Use 'debug continue' to resume

# 5. Analyze performance
debug profile report
```

This guide demonstrates the advanced scripting capabilities available in Lusush. The enhanced function system, debugging tools, and robust multiline support make it suitable for professional shell script development while maintaining POSIX compatibility.

---

**Note**: All examples in this guide have been designed to work with Lusush's POSIX-compliant syntax. Test your scripts thoroughly and use `debug help` to explore available debugging commands in your version of Lusush.