#!/bin/bash
# Script to help complete LLE development tasks

if [ $# -ne 1 ]; then
    echo "Usage: $0 <task-number>"
    echo "Example: $0 001"
    exit 1
fi

TASK_NUM=$1
TASK_BRANCH="task/lle-${TASK_NUM}"

echo "Completing LLE-${TASK_NUM}..."

# Run tests
if [ -f "scripts/lle_build.sh" ]; then
    if scripts/lle_build.sh test; then
        echo "✓ Tests passed"
    else
        echo "✗ Tests failed - fix before completing task"
        exit 1
    fi
else
    echo "⚠ Build script not found - manually verify tests pass"
fi

# Update progress file
sed -i "s/LLE-${TASK_NUM}:.*TODO/LLE-${TASK_NUM}: $(date '+%Y-%m-%d') - DONE/" LLE_PROGRESS.md

# Commit changes
git add .
git commit -m "LLE-${TASK_NUM}: Complete task implementation

$(git diff --cached --stat)"

echo "✓ Task LLE-${TASK_NUM} completed and committed"
