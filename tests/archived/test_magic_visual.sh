#!/bin/bash
# Visual test - Check if MAGIC tool is visible

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "======================================"
echo "  MAGIC TOOL VISUAL TEST"
echo "======================================"
echo ""

# Install and start
echo "1. Installing APK..."
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1

echo "2. Starting app..."
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1
sleep 8

echo "3. Checking startup logs..."
STARTUP=$($ADB logcat -d | grep -E "MAGIC tool|Loaded.*magic")
echo "$STARTUP" | while read line; do
    echo "   $line"
done

echo ""
echo "4. Taking screenshot..."
$ADB shell screencap /sdcard/magic_visual.png
$ADB pull /sdcard/magic_visual.png /tmp/ > /dev/null 2>&1

if [ -f /tmp/magic_visual.png ]; then
    echo "   ✓ Screenshot saved: /tmp/magic_visual.png"
    
    # Check file
    FILE_INFO=$(file /tmp/magic_visual.png)
    echo "   Info: $FILE_INFO"
    
    echo ""
    echo "5. Manual verification required:"
    echo "   Open /tmp/magic_visual.png and check:"
    echo "   - Is MAGIC button visible in left toolbar?"
    echo "   - Is it in row 4 (between Label and Undo)?"
    echo "   - Does it have an icon (magenta dummy or proper icon)?"
    echo ""
    echo "Screenshot viewer command:"
    echo "   xdg-open /tmp/magic_visual.png"
    echo "   # or"
    echo "   feh /tmp/magic_visual.png"
else
    echo "   ✗ Screenshot failed"
    exit 1
fi

echo ""
echo "6. Checking for crashes..."
CRASH=$($ADB logcat -d | grep -i "fatal signal\|SIGSEGV")
if [ -n "$CRASH" ]; then
    echo "   ✗ CRASH DETECTED:"
    echo "$CRASH"
    exit 1
else
    echo "   ✓ No crashes"
fi

echo ""
echo "7. Summary from logs:"
MAGIC_COUNT=$($ADB logcat -d | grep "Loaded.*magic tools" | tail -1 | grep -oP 'Loaded \K\d+' || echo "0")
MAGIC_STATUS=$($ADB logcat -d | grep "MAGIC tool" | tail -1)

if [ "$MAGIC_COUNT" -gt 0 ]; then
    echo "   ✓ $MAGIC_COUNT magic tools loaded"
    echo "   Status: $MAGIC_STATUS"
    echo ""
    echo "======================================"
    echo "  ✓ MAGIC TOOL ENABLED"  
    echo "======================================"
    exit 0
else
    echo "   ✗ No magic tools loaded"
    echo ""
    echo "======================================"
    echo "  ✗ MAGIC TOOL NOT WORKING"
    echo "======================================"
    exit 1
fi
