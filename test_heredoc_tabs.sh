#!/bin/bash
echo "Testing here document with tab stripping (<<-):"
cat <<- DELIMITER
	This line has a leading tab
		This line has two leading tabs
	Another line with one tab
Normal line without tabs
DELIMITER
echo "Done with tab stripping test"

echo ""
echo "Testing regular here document (<<) with tabs:"
cat << DELIMITER2
	This line has a leading tab
		This line has two leading tabs
	Another line with one tab
Normal line without tabs
DELIMITER2
echo "Done with regular heredoc test"
