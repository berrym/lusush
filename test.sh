echo "\nHello, \
world!\n\
\n\
\tThis\tis\tthe\t\`lusush'\tshell!\n\
\n\
It's \
a \
fantastic \
`date +'%A'`!\n"
echo "\tIn\t$(date +'%B')\t$(date +'%Y')\n"
${n1=4} ${n2=6} ${n3=2}
echo $n1^$n2 / $n3 = $(($n1**$n2/$n3))
echo "\nThe expanded glob \`*' of \`$(echo `pwd`)' looks like this..."
echo *
echo "\nThe detailed directory listing of ~ looks like...\n$(ls -alF --color=force ~)\n"
echo "Goodbye!\n"
