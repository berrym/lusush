#!/bin/bash
# Script to update LLE development progress

echo "LLE Development Progress Summary"
echo "================================"

TOTAL_TASKS=50
COMPLETED_TASKS=$(grep -c "DONE" LLE_PROGRESS.md)
IN_PROGRESS_TASKS=$(grep -c "IN_PROGRESS" LLE_PROGRESS.md)
TODO_TASKS=$((TOTAL_TASKS - COMPLETED_TASKS - IN_PROGRESS_TASKS))

echo "Tasks Completed: $COMPLETED_TASKS/$TOTAL_TASKS"
echo "Tasks In Progress: $IN_PROGRESS_TASKS"
echo "Tasks Remaining: $TODO_TASKS"

COMPLETION_PERCENT=$((COMPLETED_TASKS * 100 / TOTAL_TASKS))
echo "Completion: $COMPLETION_PERCENT%"

# Update progress file summary
sed -i "/## Summary/,/## Notes/{
    s/Tasks Completed: .*/Tasks Completed: $COMPLETED_TASKS\/$TOTAL_TASKS/
    s/Last updated: .*/Last updated: $(date)/
}" LLE_PROGRESS.md

echo ""
echo "Next tasks to work on:"
grep -n "TODO" LLE_PROGRESS.md | head -3
