#!/bin/bash
# Test LABEL tool crash fix

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== LABEL TOOL CRASH TEST ==="

$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c

# Start in expert mode (LABEL not available in child mode)
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

sleep 5

# Get screen dimensions
SCREEN=$($ADB shell wm size | grep "Physical size:" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN | cut -dx -f1)
HEIGHT=$(echo $SCREEN | cut -dx -f2)

echo "Screen: ${WIDTH}x${HEIGHT}"
echo ""

# Calculate positions
LABEL_X=50
LABEL_Y=550
CANVAS_X=$((WIDTH / 2))
CANVAS_Y=$((HEIGHT / 2))
BRUSH_X=50
BRUSH_Y=150

echo "Step 1: Select LABEL tool"
$ADB shell input tap $LABEL_X $LABEL_Y
sleep 1

TOOL_LOG=$($ADB logcat -d | grep "Tool switched" | tail -1)
echo "  $TOOL_LOG"

echo ""
echo "Step 2: Click canvas to add label"
$ADB shell input tap $CANVAS_X $CANVAS_Y
sleep 1

echo ""
echo "Step 3: Type text"
$ADB shell input text "TestLabel"
sleep 1

echo ""
echo "Step 4: Press Enter to confirm"
$ADB shell input keyevent 66
sleep 1

echo ""
echo "Step 5: Switch to BRUSH tool (trigger derender_node)"
$ADB shell input tap $BRUSH_X $BRUSH_Y
sleep 2

# Check for crash
CRASH=$($ADB logcat -d | grep -i "fatal signal\|SIGSEGV" | tail -1)
DERENDER_LOG=$($ADB logcat -d | grep "derender_node" | tail -1)

echo ""
if [ -n "$CRASH" ]; then
    echo "✗ CRASH DETECTED:"
    echo "  $CRASH"
    exit 1
else
    echo "✓ NO CRASH - LABEL tool works correctly"
    if [ -n "$DERENDER_LOG" ]; then
        echo "  $DERENDER_LOG"
    fi
fi

echo ""
echo "Screenshot captured:"
$ADB shell screencap /sdcard/label_test.png
$ADB pull /sdcard/label_test.png /tmp/ > /dev/null 2>&1
if [ -f /tmp/label_test.png ]; then
    echo "  ✓ /tmp/label_test.png"
    file /tmp/label_test.png | cut -d: -f2
fi
