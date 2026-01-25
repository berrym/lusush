if [ -f file ]; then
    echo exists
elif [ -d dir ]; then
    echo directory
else
    echo neither
fi
