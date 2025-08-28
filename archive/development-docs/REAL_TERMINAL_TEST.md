# Lusush Shell - Real Terminal Testing Guide

**Purpose:** Verify display integration features in actual terminal environments  
**Target:** Human users with real terminals (not AI simulation environments)  
**Time Required:** 10-15 minutes  

## Prerequisites

1. **Build the shell:**
   ```bash
   cd lusush
   ninja -C builddir
   ```

2. **Verify binary exists:**
   ```bash
   ls -la builddir/lusush
   ```

## Critical Tests for Real Terminals

### Test 1: Basic Functionality (Must Pass)
```bash
# Start interactive shell
./builddir/lusush -i

# Try these commands:
echo "Hello World"
pwd
ls
exit
```

**Expected:** No crashes, commands execute normally, clean exit

### Test 2: Clear Screen Functionality
```bash
./builddir/lusush -i

# Fill screen with text
echo "Line 1"
echo "Line 2" 
echo "Line 3"
echo "Line 4"
echo "Line 5"

# Test built-in clear command
clear

# You should see: Empty screen with just the prompt at top-left
echo "This should be on clean screen"

# Test Ctrl+L key binding
echo "More text"
echo "Even more text"
# Press Ctrl+L here (hold Ctrl, press L)

# You should see: Empty screen again with prompt at top-left
exit
```

**CRITICAL QUESTION:** Does `clear` command actually clear your screen? Does Ctrl+L work?

### Test 3: Theme System Verification
```bash
./builddir/lusush -i

# List available themes
theme list

# Test different themes
theme set corporate
echo "Testing corporate theme - do you see different prompt formatting?"

theme set dark  
echo "Testing dark theme - do you see different colors/formatting?"

theme set minimal
echo "Testing minimal theme - do you see simplified prompt?"

theme set classic
echo "Testing classic theme - do you see traditional shell appearance?"

exit
```

**CRITICAL QUESTION:** Do you see visual differences between themes? Colors? Formatting changes?

### Test 4: Enhanced Prompts
```bash
./builddir/lusush -i

# Observe the prompt format
# Default should show something like: [username@hostname] directory $

cd /tmp
# Does prompt update to show /tmp?

cd ~
# Does prompt show home directory correctly?

exit
```

**CRITICAL QUESTION:** Does prompt show username, hostname, and current directory?

### Test 5: Syntax Highlighting
```bash
./builddir/lusush -i

# Type the following commands slowly and observe:
# (Don't press Enter yet, just type and watch)

for i in 1 2 3
# Do you see colors on the word "for"?

# Press Enter to continue
do
# Do you see colors on the word "do"?

# Press Enter and continue
echo "Hello $i"
# Do you see colors on "echo", quotes around "Hello $i", or the variable $i?

# Press Enter and finish
done
# Do you see colors on "done"?

exit
```

**CRITICAL QUESTION:** Do you see different colors for keywords (for, do, done), strings ("Hello $i"), and variables ($i)?

### Test 6: Environment Control
```bash
# Test with display disabled
LUSUSH_LAYERED_DISPLAY=0 ./builddir/lusush -i

display status
# Should show display integration as DISABLED or INACTIVE

exit

# Test with display enabled (default)
LUSUSH_LAYERED_DISPLAY=1 ./builddir/lusush -i

display status  
# Should show display integration as ACTIVE or ENABLED

exit
```

**CRITICAL QUESTION:** Does environment variable actually control display features?

### Test 7: Multiline Commands
```bash
./builddir/lusush -i

# Test complex multiline construct
if [ -d /tmp ]; then
echo "tmp directory exists"
else  
echo "tmp directory missing"
fi

# Test for loop
for file in *.md; do
echo "Found: $file"
done

exit
```

**CRITICAL QUESTION:** Do multiline commands work correctly? Any crashes or hangs?

## What to Report Back

### Visual Features (Most Important)
- **Clear Screen:** Does `clear` command actually clear? Does Ctrl+L work?
- **Colors:** Do you see any colors in prompts, themes, or syntax highlighting?
- **Theme Differences:** Are themes visually distinct from each other?
- **Syntax Highlighting:** Different colors for keywords, strings, variables?

### Functionality
- **Stability:** Any crashes, hangs, or error messages?
- **Prompt Updates:** Does prompt show current directory changes?
- **Environment Control:** Does LUSUSH_LAYERED_DISPLAY=0 change behavior?
- **Multiline Commands:** Do complex commands execute properly?

### Terminal Information
Please also note:
- **Terminal Emulator:** (e.g., gnome-terminal, xterm, iTerm2, Windows Terminal)
- **Operating System:** (e.g., Ubuntu 22.04, macOS 13, Fedora 38)
- **TERM Environment:** Run `echo $TERM` and report the value

## Expected Results

**What Should Work:**
- All commands execute without crashes
- Clear screen functionality (both `clear` and Ctrl+L)
- Theme switching changes prompt appearance
- Multiline commands work correctly
- Environment variables control features

**What Might Need Attention:**
- Syntax highlighting colors may not be visible in all terminals
- Some theme colors may not display correctly
- Clear screen might not work in certain terminal configurations

## Quick One-Line Test
If you're short on time, run this single test:
```bash
echo 'echo "Test 1"; clear; echo "Test 2 after clear"; theme set dark; echo "Test 3 with dark theme"; exit' | ./builddir/lusush -i
```

Report what you observe: crashes, clear screen behavior, theme changes, and any colors.

---

**This testing will help identify what works in real terminals vs the AI simulation environment.**