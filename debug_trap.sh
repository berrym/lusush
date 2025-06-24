#!/usr/bin/env ./builddir/lusush
echo "Setting EXIT trap..."
trap 'echo "EXIT trap executing"' EXIT
echo "Current traps:"
trap
echo "Removing EXIT trap..."
trap - EXIT
echo "Traps after removal:"
trap
echo "Script ending..."
