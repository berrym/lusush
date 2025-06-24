#!/usr/bin/env ./builddir/lusush
trap 'echo "This should NOT execute"' EXIT
trap - EXIT
echo "EXIT trap removed - script ending"
