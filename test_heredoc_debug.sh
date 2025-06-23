#!/bin/bash

echo "=== HERE DOCUMENT DEBUG TEST ==="
echo "Testing here document functionality in Lusush shell"
echo ""

echo "Test 1: Simple here document"
echo "Command: cat <<EOF"
echo "hello"
echo "world"
echo "EOF"
echo ""
echo "Expected output: hello\\nworld"
echo "Actual output:"

./builddir/lusush -c 'cat <<EOF
hello
world
EOF'

echo ""
echo "---"
echo ""

echo "Test 2: Here document with delimiter detection"
echo "Command: cat <<END"
echo "line1"
echo "line2"
echo "END"
echo ""
echo "Expected output: line1\\nline2"
echo "Actual output:"

./builddir/lusush -c 'cat <<END
line1
line2
END'

echo ""
echo "---"
echo ""

echo "Test 3: Here document with tab stripping"
echo "Command: cat <<-TAB"
echo "\\tindented line"
echo "TAB"
echo ""
echo "Expected output: indented line"
echo "Actual output:"

./builddir/lusush -c 'cat <<-TAB
	indented line
TAB'

echo ""
echo "---"
echo ""

echo "Test 4: Here string (simpler alternative)"
echo "Command: cat <<<\"hello world\""
echo ""
echo "Expected output: hello world"
echo "Actual output:"

./builddir/lusush -c 'cat <<<"hello world"'

echo ""
echo "---"
echo ""

echo "Test 5: Interactive here document test"
echo "This will test multiline input processing"
echo "Run manually: ./builddir/lusush"
echo "Then type:"
echo "cat <<END"
echo "test line"
echo "END"
echo ""
echo "=== END DEBUG TEST ==="
