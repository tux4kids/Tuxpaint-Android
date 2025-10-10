#!/bin/bash
# Detailed TEXT tool test with full logging

set -e

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== DETAILED TEXT TOOL DEBUG TEST ==="

# Install and start
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

echo "Waiting 5s for app..."
sleep 5

# Get screen
SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo "Screen: ${WIDTH}x${HEIGHT}"

# Find and click TEXT tool
echo "Step 1: Finding TEXT tool..."
for y in 150 250 350 450; do
    for x in 80 150 220; do
        $ADB shell input tap $x $y
        sleep 0.3
        TOOL=$($ADB logcat -d | grep "Tool switched" | tail -1)
        if echo "$TOOL" | grep -q "new=4"; then
            echo "✓ TEXT tool selected at ($x,$y)"
            echo "  $TOOL"
            break 2
        fi
    done
done

sleep 1
echo ""
echo "Step 2: Current state before canvas click..."
$ADB logcat -d | grep -E "cur_tool=|Tool switched" | tail -3

# Calculate canvas position (right side, away from tools)
CANVAS_X=$((WIDTH * 3 / 4))
CANVAS_Y=$((HEIGHT / 2))

echo ""
echo "Step 3: Clicking canvas at ($CANVAS_X, $CANVAS_Y)..."
$ADB shell input tap $CANVAS_X $CANVAS_Y

sleep 1

echo ""
echo "Step 4: Logs AFTER canvas click..."
$ADB logcat -d | grep -E "Tool switched|cur_tool=|Keyboard|MOUSE|BUTTON" | tail -20

echo ""
echo "=== ANALYSIS ==="
FINAL_TOOL=$($ADB logcat -d | grep "Tool switched" | tail -1)
echo "Final tool: $FINAL_TOOL"

if echo "$FINAL_TOOL" | grep -q "new=4"; then
    echo "✓✓✓ TEXT TOOL STILL ACTIVE!"
    
    # Try typing
    echo "Typing test text..."
    $ADB shell input text "SUCCESS"
    sleep 1
    
    echo "✓✓✓ TEXT SUCCESSFULLY WRITTEN TO CANVAS!"
else
    echo "✗✗✗ TOOL CHANGED - TEXT LOST"
    echo ""
    echo "Canvas click event chain:"
    $ADB logcat -d | grep -E "SDL.*event|motion|button.*down|Tool switched" | tail -30
fi
