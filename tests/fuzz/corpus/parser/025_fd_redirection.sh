exec 3>&1
echo test >&3
exec 3>&-
