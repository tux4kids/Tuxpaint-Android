#!/bin/bash
# Test slider bounds and height with colorbar visible

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"

echo "================================"
echo "Slider Bounds Test"
echo "================================"

# Start app
echo "Starting app in child mode..."
adb -s $DEVICE shell am force-stop $PACKAGE
adb -s $DEVICE logcat -c
adb -s $DEVICE shell am start -n $PACKAGE/org.tuxpaint.tuxpaintActivity > /dev/null 2>&1
sleep 8

echo ""
echo "Test 1: Click slider at TOP position"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 150
sleep 1

echo "  → Getting slider dimensions..."
adb -s $DEVICE logcat -d | grep "Slider dimensions" | tail -1

clicks=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
if [ "$clicks" -ge 1 ]; then
    echo "  ✅ Slider responds at TOP"
else
    echo "  ❌ Slider doesn't respond at TOP"
fi

echo ""
echo "Test 2: Click slider at MIDDLE position (y=700)"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 700
sleep 1

clicks=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
if [ "$clicks" -ge 1 ]; then
    echo "  ✅ Slider responds at MIDDLE"
else
    echo "  ❌ Slider doesn't respond at MIDDLE"
fi

echo ""
echo "Test 3: Click slider at BOTTOM position (y=1220, near colorbar)"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 1220
sleep 1

clicks=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
click_info=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | tail -1)
echo "  → Click info: $click_info"
if [ "$clicks" -ge 1 ]; then
    echo "  ✅ Slider responds at BOTTOM"
else
    echo "  ❌ Slider doesn't respond at BOTTOM"
fi

echo ""
echo "Test 4: Click BELOW slider bounds (y=1300, in colorbar area)"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 1300
sleep 1

clicks=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
mouselog=$(adb -s $DEVICE logcat -d | grep "MOUSEBUTTONDOWN: x=3000, y=1300" | head -1)
echo "  → Mouse log: $mouselog"
if [ "$clicks" -eq 0 ]; then
    echo "  ✅ Click below slider correctly ignored"
else
    echo "  ❌ Click below slider incorrectly accepted"
fi

echo ""
echo "Test 5: Drag slider from top to bottom"
echo "  Starting drag from y=200 to y=1400 (beyond bounds)..."
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input swipe 3000 200 3000 1400 500
sleep 1

# Check clamping logs
clamping=$(adb -s $DEVICE logcat -d | grep "Clamp motion" | head -1)
echo "  → Clamping: $clamping"

# Check final position
final_pos=$(adb -s $DEVICE logcat -d | grep "percentage" | tail -1)
echo "  → Final position: $final_pos"

echo ""
echo "Test 6: Verify slider height matches colorbar position"
echo "  → Checking slider dimensions and colorbar position..."
adb -s $DEVICE logcat -d | grep "Slider dimensions" | tail -1
adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*button_top" | tail -1

echo ""
echo "================================"
echo "All Bounds Tests Complete"
echo "================================"
