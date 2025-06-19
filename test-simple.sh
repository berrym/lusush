#!/bin/bash
# Simplified test that doesn't use multiline strings
echo "Hello, world!"
echo "This is the \`lusush' shell!"
echo "It's a fantastic `date +'%A'`!"
echo "In $(date +'%B') $(date +'%Y')"
${n1=4} ${n2=6} ${n3=2}
echo $n1^$n2 / $n3 = $(($n1**$n2/$n3))
echo "The expanded glob \`*' of \`$(echo `pwd`)' looks like this..."
echo *
echo "Goodbye!"
