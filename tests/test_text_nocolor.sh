#!/bin/bash
# TEXT test WITHOUT color selection

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

sleep 5

SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo "1. TEXT tool ONLY (no color, no font click)"
$ADB shell input tap 220 350
sleep 1.5

echo "Tool after selection:"
$ADB logcat -d | grep "Tool switched" | tail -2

echo ""
echo "2. Canvas click"
$ADB shell input tap $((WIDTH / 2)) $((HEIGHT / 2))
sleep 1

echo "Tool after canvas:"
$ADB logcat -d | grep "Tool switched" | tail -2

echo ""
echo "3. Type"
$ADB shell input text "XXX"
sleep 1

echo "4. Enter"
$ADB shell input keyevent 66
sleep 1

echo ""
echo "=== ALL TOOL SWITCHES ==="
$ADB logcat -d | grep "Tool switched" | nl

echo ""
echo "Final tool status:"
$ADB logcat -d | grep "cur_tool=" | tail -5
