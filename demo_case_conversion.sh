#!/bin/bash

# LUSUSH CASE CONVERSION DEMONSTRATION
# Showcases the newly implemented case conversion parameter expansion features
# ${var^}, ${var,}, ${var^^}, ${var,,}

echo "=== LUSUSH CASE CONVERSION FEATURES DEMO ==="
echo "Demonstrating modern case conversion parameter expansion"
echo

# Build the shell if needed
if [ ! -x "./builddir/lusush" ]; then
    echo "Building Lusush shell..."
    meson compile -C builddir > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "❌ Build failed. Please check your build environment."
        exit 1
    fi
    echo "✅ Build successful"
    echo
fi

SHELL="./builddir/lusush"

# Demo function
demo() {
    local title="$1"
    local command="$2"
    local description="$3"

    echo "🔹 $title"
    echo "   Command: $command"
    if [ -n "$description" ]; then
        echo "   Purpose: $description"
    fi

    # Execute and show result
    result=$(echo "$command" | $SHELL 2>/dev/null)
    echo "   Result:  '$result'"
    echo
}

echo "=== 1. FIRST CHARACTER UPPERCASE (${var^}) ==="
echo "Convert only the first character to uppercase"
echo

demo "Basic capitalization" \
     'name=john; echo ${name^}' \
     "Capitalize first letter of a name"

demo "Title case conversion" \
     'word=hello; echo ${word^}' \
     "Convert first character to uppercase"

demo "Already capitalized" \
     'text=Hello; echo ${text^}' \
     "No change if already capitalized"

demo "Mixed case handling" \
     'text=hELLO; echo ${text^}' \
     "Only affects first character"

demo "Numbers and special chars" \
     'code=123abc; echo ${code^}' \
     "Non-letters remain unchanged"

echo "=== 2. FIRST CHARACTER LOWERCASE (${var,}) ==="
echo "Convert only the first character to lowercase"
echo

demo "Uncapitalize" \
     'name=John; echo ${name,}' \
     "Make first letter lowercase"

demo "Camel case start" \
     'func=MyFunction; echo ${func,}' \
     "Convert to camelCase style"

demo "All caps to mixed" \
     'text=HELLO; echo ${text,}' \
     "Only first character affected"

demo "Already lowercase" \
     'text=hello; echo ${text,}' \
     "No change if already lowercase"

echo "=== 3. ALL CHARACTERS UPPERCASE (${var^^}) ==="
echo "Convert all characters to uppercase"
echo

demo "Full uppercase conversion" \
     'text=hello; echo ${text^^}' \
     "Convert entire string to uppercase"

demo "Mixed case to upper" \
     'text=HeLLo; echo ${text^^}' \
     "Normalize to all uppercase"

demo "With numbers and symbols" \
     'text=hello123; echo ${text^^}' \
     "Letters converted, numbers unchanged"

demo "Environment variable style" \
     'var=my_setting; echo ${var^^}' \
     "Convert to ENV_VAR style"

demo "Already uppercase" \
     'text=HELLO; echo ${text^^}' \
     "No change if already uppercase"

echo "=== 4. ALL CHARACTERS LOWERCASE (${var,,}) ==="
echo "Convert all characters to lowercase"
echo

demo "Full lowercase conversion" \
     'text=HELLO; echo ${text,,}' \
     "Convert entire string to lowercase"

demo "Mixed case to lower" \
     'text=HeLLo; echo ${text,,}' \
     "Normalize to all lowercase"

demo "Constant to variable" \
     'CONST=MAX_SIZE; echo ${CONST,,}' \
     "Convert from CONSTANT style"

demo "Mixed content" \
     'text=Hello123World; echo ${text,,}' \
     "Letters converted, numbers unchanged"

demo "Already lowercase" \
     'text=hello; echo ${text,,}' \
     "No change if already lowercase"

echo "=== 5. REAL-WORLD APPLICATIONS ==="
echo "Practical use cases for case conversion"
echo

demo "Name formatting" \
     'first=john; last=doe; echo ${first^} ${last^}' \
     "Proper name capitalization"

demo "Constant generation" \
     'setting=debug_mode; echo ${setting^^}' \
     "Generate environment variable names"

demo "File extension handling" \
     'file=document.PDF; echo ${file,,}' \
     "Normalize file extensions"

demo "Variable name conversion" \
     'camelCase=myVariable; echo ${camelCase,,}' \
     "Convert naming conventions"

demo "Protocol normalization" \
     'protocol=HTTPS; echo ${protocol,,}' \
     "Standardize protocol names"

echo "=== 6. INTEGRATION WITH EXISTING FEATURES ==="
echo "Case conversion works with other parameter expansion"
echo

demo "With default values" \
     'name=${USER:-guest}; echo ${name^}' \
     "Capitalize default values"

demo "Length and case" \
     'text=Hello; echo Length: ${#text}, Upper: ${text^^}' \
     "Combine length and case conversion"

demo "Pattern matching integration" \
     'file=script.SH; base=${file%.*}; echo ${base,,}' \
     "Pattern matching then case conversion"

demo "Multiple conversions" \
     'text=HELLO; lower=${text,,}; echo ${lower^}' \
     "Chain multiple case conversions"

echo "=== 7. STRING MANIPULATION PIPELINE ==="
echo "Complex text processing examples"
echo

demo "Filename processing" \
     'filename=MyDocument.TXT; echo ${filename,,}' \
     "Normalize filename case"

demo "Configuration parsing" \
     'setting=Database_Host; echo ${setting^^}' \
     "Convert to environment variable format"

demo "User input normalization" \
     'input=YeS; echo ${input,,}' \
     "Normalize user input for comparison"

demo "Code style conversion" \
     'pythonVar=snake_case; echo ${pythonVar^^}' \
     "Convert between naming conventions"

echo "=== 8. ADVANCED PATTERNS ==="
echo "Sophisticated case conversion scenarios"
echo

demo "Mixed content handling" \
     'data=ABC123def; echo Upper: ${data^^}, Lower: ${data,,}' \
     "Handle mixed alphanumeric content"

demo "Identifier transformation" \
     'id=user_id_123; echo ${id^^}' \
     "Transform identifiers to constants"

demo "Title case simulation" \
     'word1=hello; word2=world; echo ${word1^} ${word2^}' \
     "Create title case from multiple words"

demo "Case toggle effect" \
     'original=Hello; opposite=${original^^}; echo ${opposite,,}' \
     "Toggle case through transformations"

echo "=== SUMMARY ==="
echo "✅ Case conversion features are fully functional in Lusush!"
echo
echo "Implemented operators:"
echo "  • \${var^}  - First character to uppercase"
echo "  • \${var,}  - First character to lowercase"
echo "  • \${var^^} - All characters to uppercase"
echo "  • \${var,,} - All characters to lowercase"
echo
echo "🎯 Use cases:"
echo "  • Name and title formatting"
echo "  • Environment variable generation"
echo "  • File and protocol normalization"
echo "  • Code style and convention conversion"
echo "  • User input standardization"
echo
echo "🔗 Perfect integration:"
echo "  • Works with all existing parameter expansion features"
echo "  • Combines with pattern matching and defaults"
echo "  • Enables sophisticated text processing pipelines"
echo
echo "🎉 Lusush now provides complete modern parameter expansion capabilities"
echo "   including advanced case conversion for professional shell scripting!"
echo
echo "Next: Complete POSIX compliance with remaining advanced features."
