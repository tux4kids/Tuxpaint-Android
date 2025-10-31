#!/bin/bash
# Timeline analysis of TEXT tool

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== TEXT TOOL TIMELINE ANALYSIS ==="

$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

sleep 5

SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)

# Mark timestamp T0
echo "[T0] Clicking TEXT tool..."
$ADB shell input tap 220 350
sleep 0.2

# Mark timestamp T1
echo "[T1] 200ms after TEXT click - checking tool..."
$ADB logcat -d | grep "Tool switched" | tail -3 | while read line; do
    echo "  $line"
done

sleep 0.3

# Mark timestamp T2
echo "[T2] 500ms after TEXT click - checking tool..."
CURRENT=$($ADB logcat -d | grep "Tool switched" | tail -1)
echo "  $CURRENT"

sleep 0.5

# Mark timestamp T3
echo "[T3] 1000ms after TEXT click - final state before canvas..."
CURRENT=$($ADB logcat -d | grep "Tool switched" | tail -1)
echo "  $CURRENT"

# Mark timestamp T4
CANVAS_X=$((WIDTH * 3 / 4))
echo "[T4] Clicking canvas at x=$CANVAS_X..."
$ADB shell input tap $CANVAS_X 1496

sleep 0.2

# Mark timestamp T5
echo "[T5] 200ms after canvas click..."
$ADB logcat -d | grep "Tool switched" | tail -3 | while read line; do
    echo "  $line"
done

echo ""
echo "=== FULL TOOL SWITCH HISTORY ==="
$ADB logcat -d | grep "Tool switched" | nl

echo ""
echo "===

 DIAGNOSIS ==="
SWITCHES=$($ADB logcat -d | grep "Tool switched" | wc -l)
LAST_TO_TEXT=$($ADB logcat -d | grep "Tool switched.*new=4" | tail -1)
LAST_SWITCH=$($ADB logcat -d | grep "Tool switched" | tail -1)

echo "Total tool switches: $SWITCHES"
echo "Last switch TO text (4): $LAST_TO_TEXT"
echo "Last switch overall: $LAST_SWITCH"

if [ "$LAST_TO_TEXT" = "$LAST_SWITCH" ]; then
    echo "✓ TEXT tool is final state"
else
    echo "✗ TEXT tool was overridden"
fi
