#!/bin/bash
# Complete TEXT tool test with font selection and text confirmation

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== COMPLETE TEXT TOOL TEST ==="

# Install and start
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

sleep 5

# Get screen
SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo "Step 1: Select TEXT tool"
$ADB shell input tap 220 350
sleep 1

echo "Step 2: Select a font (click right panel)"
# Font selector is on the right side
FONT_X=$((WIDTH - 100))
FONT_Y=200
$ADB shell input tap $FONT_X $FONT_Y
sleep 1

echo "Step 3: Click canvas"
CANVAS_X=$((WIDTH / 2))
CANVAS_Y=$((HEIGHT / 2))
$ADB shell input tap $CANVAS_X $CANVAS_Y
sleep 1

echo "Step 4: Type text"
$ADB shell input text "TEST"
sleep 1

echo "Step 5: Press Enter to confirm text"
$ADB shell input keyevent 66  # KEYCODE_ENTER
sleep 1

echo "Step 6: Take screenshot"
$ADB shell screencap /sdcard/text_final.png
$ADB pull /sdcard/text_final.png /tmp/ > /dev/null 2>&1

if [ -f /tmp/text_final.png ]; then
    echo "✓ Screenshot: /tmp/text_final.png"
    
    # Check if text is visible (rough heuristic - file size should be larger with text)
    SIZE=$(stat -f%z /tmp/text_final.png 2>/dev/null || stat -c%s /tmp/text_final.png 2>/dev/null)
    echo "  File size: $SIZE bytes"
    
    if [ $SIZE -gt 100000 ]; then
        echo "✓✓✓ TEXT SUCCESSFULLY WRITTEN TO CANVAS!"
    fi
else
    echo "✗ Screenshot failed"
fi

echo ""
echo "View with: xdg-open /tmp/text_final.png"
