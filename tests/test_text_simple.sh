#!/bin/bash
# Simple TEXT tool test - works in any mode

set -e

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== SIMPLE TEXT TOOL TEST ==="

# Install and start app
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

echo "App started, waiting 5 seconds..."
sleep 5

# Get logs
$ADB logcat -d | grep -i "tuxpaint\|Tool switched\|Keyboard started\|child_mode" | tail -30 > /tmp/startup_logs.txt

echo ""
echo "=== Startup Logs ==="
cat /tmp/startup_logs.txt

# Check mode
if grep -q "child_mode=1" /tmp/startup_logs.txt; then
    echo ""
    echo "✓ Running in CHILD MODE"
    MODE="child"
else
    echo ""
    echo "✓ Running in EXPERT MODE"
    MODE="expert"
fi

# Now test TEXT tool by simulating clicks
# Get screen info
SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo "Screen: ${WIDTH}x${HEIGHT}"

# Click TEXT tool based on mode
if [ "$MODE" = "child" ]; then
    # Child mode: single column, TEXT is 4th tool
    TOOL_X=50
    TOOL_Y=$((HEIGHT / 12 * 3 + HEIGHT / 24))
else
    # Expert mode: grid layout
    # TEXT is tool #4, in position (row, col) depending on layout
    # Let's try clicking in tool area and see what happens
    TOOL_X=100
    TOOL_Y=250
fi

echo "Clicking TEXT tool at ($TOOL_X, $TOOL_Y)..."
$ADB shell input tap $TOOL_X $TOOL_Y
sleep 1

# Check logs again
$ADB logcat -d | grep -i "Tool switched" | tail -5
LAST_TOOL=$($ADB logcat -d | grep -i "Tool switched" | tail -1)

echo "Last tool switch: $LAST_TOOL"

if echo "$LAST_TOOL" | grep -q "new=4"; then
    echo "✓✓✓ SUCCESS! TEXT tool selected (tool #4)"
else
    echo "✗ TEXT tool not selected yet"
    echo "Trying different position..."
    
    # Try different positions
    for y in 150 250 350 450; do
        for x in 80 150 220; do
            $ADB shell input tap $x $y
            sleep 0.5
            TOOL_CHECK=$($ADB logcat -d | grep "Tool switched" | tail -1)
            if echo "$TOOL_CHECK" | grep -q "new=4"; then
                echo "✓✓✓ SUCCESS at ($x, $y)! TEXT tool selected"
                exit 0
            fi
        done
    done
fi

echo ""
echo "=== All TuxPaint logs ==="
$ADB logcat -d | grep -i tuxpaint | tail -40
