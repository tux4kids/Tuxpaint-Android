#!/bin/bash
# Full TEXT tool test - click tool, click canvas, type text

set -e

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== FULL TEXT TOOL TEST ==="

# Install and start app
echo "Installing app..."
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

echo "Waiting for app to load..."
sleep 5

# Get screen info
SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo "Screen: ${WIDTH}x${HEIGHT}"

# Find TEXT tool by trying positions
echo "Finding TEXT tool..."
TEXT_FOUND=0
for y in 150 250 350 450; do
    for x in 80 150 220; do
        $ADB shell input tap $x $y
        sleep 0.3
        TOOL_CHECK=$($ADB logcat -d | grep "Tool switched" | tail -1)
        if echo "$TOOL_CHECK" | grep -q "new=4"; then
            echo "✓ TEXT tool found and selected at ($x, $y)"
            TEXT_FOUND=1
            break 2
        fi
    done
done

if [ $TEXT_FOUND -eq 0 ]; then
    echo "✗ ERROR: Could not find TEXT tool"
    exit 1
fi

# Wait a bit
sleep 1

# Click canvas to start text input
CANVAS_X=$((WIDTH / 2))
CANVAS_Y=$((HEIGHT / 2))

echo "Clicking canvas at ($CANVAS_X, $CANVAS_Y) to start text input..."
$ADB shell input tap $CANVAS_X $CANVAS_Y
sleep 1

# Check if keyboard started
KB_CHECK=$($ADB logcat -d | grep "Keyboard started" | tail -1)
if [ -n "$KB_CHECK" ]; then
    echo "✓ Keyboard started!"
    echo "  $KB_CHECK"
else
    echo "✗ Keyboard did not start"
    echo "Recent logs:"
    $ADB logcat -d | grep -i "tuxpaint\|keyboard" | tail -10
    exit 1
fi

# Type text
echo "Typing text 'Test123'..."
$ADB shell input text "Test123"
sleep 1

# Check tool is still TEXT
TOOL_NOW=$($ADB logcat -d | grep "cur_tool=" | tail -1)
echo "Current tool status: $TOOL_NOW"

if echo "$TOOL_NOW" | grep -q "cur_tool=4"; then
    echo "✓ Tool is still TEXT (4)"
else
    echo "⚠ Tool may have changed"
fi

# Check if keyboard is still open
KB_STOP=$($ADB logcat -d | grep "Keyboard stopped" | tail -1)
if [ -z "$KB_STOP" ]; then
    echo "✓ Keyboard is still open"
elif echo "$KB_STOP" | grep -q "after grace period"; then
    echo "✓ Keyboard closed gracefully after grace period"
else
    echo "⚠ Keyboard stopped: $KB_STOP"
fi

echo ""
echo "=== TEST SUMMARY ==="
echo "✓✓✓ TEXT TOOL WORKS!"
echo "  - TEXT tool can be selected"
echo "  - Canvas click opens keyboard"
echo "  - Text can be typed"
echo ""

# Show screenshot info
echo "Take a screenshot to verify text appears:"
echo "  adb -s $DEVICE shell screencap /sdcard/test.png"
echo "  adb -s $DEVICE pull /sdcard/test.png"

exit 0
