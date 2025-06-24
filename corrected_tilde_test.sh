#!/usr/bin/env ./builddir/lusush

echo Testing basic tilde expansion:
echo HOME directory: ~
echo HOME with path: ~/Documents  
echo Current HOME: $HOME
echo "Should NOT expand in quotes: ~"
echo 'Should NOT expand in single quotes: ~'
