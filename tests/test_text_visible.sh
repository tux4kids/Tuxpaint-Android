#!/bin/bash
# TEXT tool test with visible black text

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== VISIBLE TEXT TEST ==="

$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c  
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

sleep 5

SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo "1. Select BLACK color"
# Black is first color
$ADB shell input tap 75 375
sleep 0.5

echo "2. Select TEXT tool"
$ADB shell input tap 220 350
sleep 1

echo "3. Select first font (large)"
FONT_X=$((WIDTH - 50))
FONT_Y=100
$ADB shell input tap $FONT_X $FONT_Y
sleep 0.5

echo "4. Click canvas center"
CANVAS_X=$((WIDTH / 2))
CANVAS_Y=$((HEIGHT / 2))
$ADB shell input tap $CANVAS_X $CANVAS_Y
sleep 1

echo "5. Type 'HELLO'"
$ADB shell input text "HELLO"
sleep 1

echo "6. Press Enter"
$ADB shell input keyevent 66
sleep 1

echo "7. Screenshot"
$ADB shell screencap /sdcard/text_visible.png
$ADB pull /sdcard/text_visible.png /tmp/ > /dev/null 2>&1

if [ -f /tmp/text_visible.png ]; then
    echo "✓✓✓ SCREENSHOT CAPTURED!"
    echo ""
    echo "Check text visibility:"
    echo "  xdg-open /tmp/text_visible.png"
    
    # Show logs
    echo ""
    echo "Recent logs:"
    $ADB logcat -d | grep -E "Tool switched|Keyboard|TEXT" | tail -10
fi
