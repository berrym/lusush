#!/bin/bash

echo "=== CASE STATEMENT TEST ==="
echo

echo "Test 1: Simple exact match"
case "hello" in
    hello) echo "  Exact match works" ;;
    *) echo "  Default case" ;;
esac

echo
echo "Test 2: Wildcard pattern"
case "hello" in
    h*) echo "  Wildcard h* matches" ;;
    *) echo "  Default case" ;;
esac

echo
echo "Test 3: Character set pattern"
case "hello" in
    [h-z]*) echo "  Character range [h-z]* matches" ;;
    *) echo "  Default case" ;;
esac

echo
echo "Test 4: Multiple patterns"
case "world" in
    hello | hi) echo "  Greeting" ;;
    world | earth) echo "  Planet name matches" ;;
    *) echo "  Default case" ;;
esac

echo
echo "Test 5: Question mark pattern"
case "hello" in
    h?llo) echo "  Single char ? pattern matches" ;;
    *) echo "  Default case" ;;
esac

echo
echo "=== CASE STATEMENT TEST COMPLETE ==="
