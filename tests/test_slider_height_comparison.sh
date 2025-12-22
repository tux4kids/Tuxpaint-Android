#!/bin/bash
# Test to compare slider height in child mode with toolbar height in expert mode

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"

echo "================================"
echo "Slider Height Comparison Test"
echo "================================"

# Start in expert mode
echo "Step 1: Starting in Expert Mode..."
adb -s $DEVICE shell am force-stop $PACKAGE
adb -s $DEVICE logcat -c
adb -s $DEVICE shell am start -n $PACKAGE/org.tuxpaint.tuxpaintActivity > /dev/null 2>&1
sleep 8

# Take screenshot of expert mode
echo "  → Taking Expert Mode screenshot..."
adb -s $DEVICE exec-out screencap -p > /tmp/expert_toolbar.png
echo "  → Saved to /tmp/expert_toolbar.png"
echo "  → In Expert Mode, the toolbar goes from y=~155 (after title) to y=~1440 (r_colors.y)"
echo ""

# Switch to child mode by creating a saved image (triggers child mode on next start)
echo "Step 2: Preparing Child Mode..."
echo "  → Please manually switch to Child Mode using the UI"
echo "  → Run this command to check slider height:"
echo "    adb -s $DEVICE shell input tap 3000 500 && adb -s $DEVICE logcat -d | grep 'Slider dimensions'"
echo ""
echo "Expected result:"
echo "  - Slider in Child Mode should have h = r_colors.y - (r_ttoolopt.h + 30)"
echo "  - This should match the toolbar height in Expert Mode"
echo "  - Approximately h = 1440 - 155 = 1285 pixels"
