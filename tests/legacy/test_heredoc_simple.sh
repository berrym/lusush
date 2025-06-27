#!/bin/bash
echo "Testing here document with cat:"
cat << DELIMITER
This is line 1
This is line 2
This is line 3
DELIMITER
echo "Done"
