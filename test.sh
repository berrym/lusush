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
echo 4^6 / 2 = $((4**6/2))
echo "\nThe expanded glob \`*' of \`$(echo `pwd`)' looks like this..."
echo *
echo "\nThe detailed directory listing of ~ looks like...\n$(ls -alF --color=force ~)\n"
echo "Goodbye!\n"
