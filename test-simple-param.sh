#!/bin/bash
# Simple parameter expansion test

echo "Test 1: Assignment if unset"
echo "${var1=test1}"
echo "var1 is now: $var1"

echo "Test 2: Default without assignment"  
echo "${var2:-test2}"
echo "var2 is: $var2"

echo "Test 3: Assignment if empty"
echo "${var3:=test3}"
echo "var3 is now: $var3"

echo "Done"
