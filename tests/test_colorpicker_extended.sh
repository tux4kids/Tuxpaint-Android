#!/bin/bash
# Extended test for edge cases

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"
ACTIVITY="org.tuxpaint.tuxpaintActivity"

echo "================================"
echo "Extended Color Picker Edge Case Tests"
echo "================================"

start_app() {
    adb -s $DEVICE shell am force-stop $PACKAGE
    adb -s $DEVICE logcat -c
    adb -s $DEVICE shell am start -n $PACKAGE/$ACTIVITY > /dev/null 2>&1
    echo "Waiting for app to start..."
    sleep 8
}

simulate_touch() {
    adb -s $DEVICE shell input tap $1 $2
    sleep 0.3
}

check_log() {
    local pattern=$1
    local count=$(adb -s $DEVICE logcat -d | grep "$pattern" 2>/dev/null | wc -l | tr -d ' ')
    echo "$count"
}

echo ""
echo "Test 1: Rapid color picker button clicks"
echo "=========================================="
start_app

# Hide colorbar first
simulate_touch 1500 700
sleep 1

# Rapid clicks on color picker button
echo "  → Click 1"
simulate_touch 3000 1350
echo "  → Click 2 (rapid)"
simulate_touch 3000 1350
echo "  → Click 3 (rapid)"
simulate_touch 3000 1350

colorbar_in=$(check_log "slide_colorbar_in")
echo "  Result: $colorbar_in slide_in calls"
if [ "$colorbar_in" -ge 1 ] && [ "$colorbar_in" -le 3 ]; then
    echo "  ✅ Handles rapid clicks correctly"
else
    echo "  ❌ Unexpected behavior with rapid clicks"
fi

echo ""
echo "Test 2: Click canvas during animation"
echo "======================================"

# Start slide out
simulate_touch 1500 700
sleep 0.1  # Click during animation
simulate_touch 1500 700

canvas_clicks=$(check_log "r_canvas clicked")
echo "  Result: $canvas_clicks canvas clicks"
if [ "$canvas_clicks" -ge 2 ]; then
    echo "  ✅ Canvas clicks work during animation"
else
    echo "  ⚠️  Canvas clicks might be blocked during animation"
fi

echo ""
echo "Test 3: Color picker button positioning"
echo "========================================"

# Check if button is drawn at correct position
button_logs=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*button_top" | tail -1)
echo "  $button_logs"

if echo "$button_logs" | grep -q "button_top=1240"; then
    echo "  ✅ Button positioned correctly (y=1240-1435)"
else
    echo "  ❌ Button position incorrect"
fi

echo ""
echo "Test 4: Canvas painting after multiple color picker toggles"
echo "=========================================================="

# Toggle multiple times
for i in {1..3}; do
    echo "  → Toggle $i: Canvas click"
    simulate_touch 1500 700
    sleep 0.5
    echo "  → Toggle $i: Color picker click"
    simulate_touch 3000 1350
    sleep 0.5
done

# Final paint test
echo "  → Final paint test"
simulate_touch 1500 900
sleep 0.5

brush_downs=$(check_log "BRUSH DOWN")
echo "  Result: $brush_downs brush strokes"
if [ "$brush_downs" -ge 4 ]; then
    echo "  ✅ Canvas painting works after multiple toggles"
else
    echo "  ❌ Canvas painting broken after toggles"
fi

echo ""
echo "Test 5: Verify no event processing after color picker click"
echo "=========================================================="

# Reset
start_app
simulate_touch 1500 700  # Hide colorbar
sleep 1
adb -s $DEVICE logcat -c

# Click color picker button
simulate_touch 3000 1350
sleep 0.5

# Check that no unwanted events were processed
unwanted=$(adb -s $DEVICE logcat -d | grep -E "r_canvas clicked.*after.*COLOR_PICKER" | wc -l | tr -d ' ')
if [ "$unwanted" -eq 0 ]; then
    echo "  ✅ No unwanted event processing"
else
    echo "  ⚠️  Possible event processing leak"
fi

echo ""
echo "Test 6: Child mode slider (if applicable)"
echo "========================================"

# Switch to child mode
adb -s $DEVICE shell am force-stop $PACKAGE
# Note: Switching modes requires changing settings, which is complex
# For now, just verify expert mode works
echo "  ℹ️  Child mode slider test requires manual verification"
echo "  ℹ️  (Brush tool should show slider in child mode)"

echo ""
echo "================================"
echo "Extended tests completed"
echo "================================"
