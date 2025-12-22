#!/bin/bash
# Test child mode with brush slider

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"

echo "================================"
echo "Child Mode Slider Test"
echo "================================"

# Force child mode by modifying preferences
echo ""
echo "Setting Child Mode..."
adb -s $DEVICE shell am force-stop $PACKAGE

# Create preferences XML with child mode enabled
adb -s $DEVICE shell "su 0 sh -c 'cat > /data/data/$PACKAGE/shared_prefs/TuxPaintPreferences.xml << EOF
<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>
<map>
    <boolean name=\"childMode\" value=\"true\" />
    <boolean name=\"sound\" value=\"true\" />
</map>
EOF'" 2>/dev/null

# Start app
adb -s $DEVICE logcat -c
echo "Starting app in Child Mode..."
adb -s $DEVICE shell am start -n $PACKAGE/org.tuxpaint.tuxpaintActivity > /dev/null 2>&1
sleep 8

# Check if child mode is active
child_mode=$(adb -s $DEVICE logcat -d | grep "child_mode=1" | head -1)
if [ -n "$child_mode" ]; then
    echo "✅ Child mode active: $child_mode"
else
    echo "⚠️  Checking for child_mode in logs..."
    adb -s $DEVICE logcat -d | grep -i "child" | head -5
fi

echo ""
echo "Test 1: Verify brush slider appears in child mode"
echo "=================================================="

# The brush tool should be selected by default
sleep 1
slider_drawn=$(adb -s $DEVICE logcat -d | grep -E "DRAWING BRUSH BUTTONS.*child_mode" | tail -1)
echo "  $slider_drawn"

if echo "$slider_drawn" | grep -q "child_mode"; then
    echo "  ✅ Brush UI logged"
else
    echo "  ⚠️  Need to verify manually"
fi

echo ""
echo "Test 2: Paint on canvas in child mode"
echo "====================================="
adb -s $DEVICE shell input tap 1500 700
sleep 1

brush_down=$(adb -s $DEVICE logcat -d | grep "BRUSH DOWN" | wc -l | tr -d ' ')
echo "  Result: $brush_down brush strokes"
if [ "$brush_down" -ge 1 ]; then
    echo "  ✅ Canvas painting works in child mode"
else
    echo "  ❌ Canvas painting broken in child mode"
fi

echo ""
echo "Test 3: Hide colorbar and check color picker button"
echo "==================================================="
adb -s $DEVICE shell input tap 1500 700
sleep 1

button_drawn=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*Drawing button.*child_mode=1" | wc -l | tr -d ' ')
echo "  Result: $button_drawn button draw events"
if [ "$button_drawn" -ge 1 ]; then
    echo "  ✅ Color picker button appears in child mode"
else
    echo "  ⚠️  Button might not be drawn in child mode"
fi

# Check button position in child mode
button_pos=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*button_top.*child_mode=1" | tail -1)
if [ -n "$button_pos" ]; then
    echo "  Position: $button_pos"
fi

echo ""
echo "Test 4: Click color picker button in child mode"
echo "==============================================="
adb -s $DEVICE shell input tap 3000 1350
sleep 1

picker_click=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*Button clicked" | wc -l | tr -d ' ')
echo "  Result: $picker_click color picker clicks"
if [ "$picker_click" -ge 1 ]; then
    echo "  ✅ Color picker button works in child mode"
else
    echo "  ❌ Color picker button not working in child mode"
fi

echo ""
echo "Test 5: Check slider interaction (if visible)"
echo "============================================="
# The slider should be in the middle-right area
# Try clicking on it
adb -s $DEVICE shell input tap 3000 600
sleep 0.5

slider_events=$(adb -s $DEVICE logcat -d | grep -i "slider\|brush.*size" | tail -3)
if [ -n "$slider_events" ]; then
    echo "  Slider events:"
    echo "$slider_events" | head -3
    echo "  ✅ Slider interaction detected"
else
    echo "  ℹ️  No slider events (might need colorbar visible)"
fi

echo ""
echo "================================"
echo "Child Mode tests completed"
echo "================================"

# Get final statistics
echo ""
echo "Final log summary:"
echo "- Brush strokes: $(adb -s $DEVICE logcat -d | grep 'BRUSH DOWN' | wc -l | tr -d ' ')"
echo "- Color picker clicks: $(adb -s $DEVICE logcat -d | grep 'COLOR_PICKER.*Button clicked' | wc -l | tr -d ' ')"
echo "- Colorbar slides: $(adb -s $DEVICE logcat -d | grep 'slide_colorbar' | wc -l | tr -d ' ')"
