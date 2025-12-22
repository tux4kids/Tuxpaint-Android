#!/bin/bash
# Test child mode slider interaction after color picker button usage

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"

echo "================================"
echo "Child Mode Slider Interaction Test"
echo "================================"

# Force enable child mode
echo "Setting up Child Mode..."
adb -s $DEVICE shell am force-stop $PACKAGE
adb -s $DEVICE logcat -c

# Start in child mode (assuming it's already set or we start fresh)
adb -s $DEVICE shell am start -n $PACKAGE/org.tuxpaint.tuxpaintActivity > /dev/null 2>&1
echo "Waiting for app to start..."
sleep 8

echo ""
echo "Test Sequence:"
echo "1. Use slider (colorbar visible)"
echo "2. Paint on canvas"
echo "3. Click canvas to hide colorbar"
echo "4. Click color picker button to show colorbar"
echo "5. Use slider again (should work!)"
echo "6. Paint again"
echo "7. Hide colorbar"
echo "8. Use slider without showing colorbar (should work!)"
echo ""

# Test 1: Use slider initially
echo "Step 1: Click slider (top position) - colorbar visible"
adb -s $DEVICE shell input tap 3000 200
sleep 1

slider_clicks=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
echo "  → Slider clicks: $slider_clicks"
if [ "$slider_clicks" -ge 1 ]; then
    echo "  ✅ Initial slider interaction works"
else
    echo "  ❌ Initial slider click failed"
fi

# Test 2: Paint
echo ""
echo "Step 2: Paint on canvas"
adb -s $DEVICE shell input tap 1500 700
sleep 1

brush_strokes=$(adb -s $DEVICE logcat -d | grep "BRUSH DOWN" | wc -l | tr -d ' ')
echo "  → Brush strokes: $brush_strokes"
if [ "$brush_strokes" -ge 1 ]; then
    echo "  ✅ Canvas painting works"
else
    echo "  ❌ Canvas painting failed"
fi

# Test 3: Hide colorbar
echo ""
echo "Step 3: Click canvas to hide colorbar"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 1500 700
sleep 1

slide_out=$(adb -s $DEVICE logcat -d | grep "slide_colorbar_out" | wc -l | tr -d ' ')
echo "  → Slide out events: $slide_out"
if [ "$slide_out" -ge 1 ]; then
    echo "  ✅ Colorbar hides"
else
    echo "  ⚠️  Colorbar didn't hide"
fi

# Check that color picker button is drawn
picker_drawn=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*Drawing button" | wc -l | tr -d ' ')
echo "  → Color picker button drawn: $picker_drawn"
if [ "$picker_drawn" -ge 1 ]; then
    echo "  ✅ Color picker button appears"
else
    echo "  ❌ Color picker button not drawn"
fi

# Test 4: Click color picker button
echo ""
echo "Step 4: Click color picker button to show colorbar"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 1350
sleep 1

slide_in=$(adb -s $DEVICE logcat -d | grep "slide_colorbar_in" | wc -l | tr -d ' ')
picker_click=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*Button clicked" | wc -l | tr -d ' ')
echo "  → Slide in events: $slide_in"
echo "  → Picker clicks: $picker_click"
if [ "$slide_in" -ge 1 ] && [ "$picker_click" -ge 1 ]; then
    echo "  ✅ Color picker button shows colorbar"
else
    echo "  ❌ Color picker button didn't work"
fi

# Test 5: CRITICAL - Use slider after color picker button click
echo ""
echo "Step 5: CRITICAL - Use slider after color picker button (middle position)"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 700
sleep 1

slider_clicks_after=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
echo "  → Slider clicks: $slider_clicks_after"
if [ "$slider_clicks_after" -ge 1 ]; then
    echo "  ✅ SLIDER WORKS after color picker button!"
else
    echo "  ❌ SLIDER STUCK after color picker button!"
fi

# Test 6: Paint again
echo ""
echo "Step 6: Paint on canvas again"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 1500 900
sleep 1

brush_strokes_2=$(adb -s $DEVICE logcat -d | grep "BRUSH DOWN" | wc -l | tr -d ' ')
echo "  → Brush strokes: $brush_strokes_2"
if [ "$brush_strokes_2" -ge 1 ]; then
    echo "  ✅ Canvas painting still works"
else
    echo "  ❌ Canvas painting failed"
fi

# Test 7: Hide colorbar again
echo ""
echo "Step 7: Hide colorbar again"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 1500 700
sleep 1

slide_out_2=$(adb -s $DEVICE logcat -d | grep "slide_colorbar_out" | wc -l | tr -d ' ')
echo "  → Slide out events: $slide_out_2"
if [ "$slide_out_2" -ge 1 ]; then
    echo "  ✅ Colorbar hides again"
else
    echo "  ⚠️  Colorbar didn't hide"
fi

# Test 8: CRITICAL - Use slider WITHOUT showing colorbar first
echo ""
echo "Step 8: CRITICAL - Use slider with colorbar hidden (bottom position)"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 1200
sleep 1

slider_clicks_hidden=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
colorbar_state=$(adb -s $DEVICE logcat -d | grep "colorbar_is_visible" | tail -1)
echo "  → Slider clicks: $slider_clicks_hidden"
echo "  → Colorbar state: $colorbar_state"
if [ "$slider_clicks_hidden" -ge 1 ]; then
    echo "  ✅ SLIDER WORKS with colorbar hidden!"
else
    echo "  ❌ SLIDER DOESN'T WORK with colorbar hidden!"
fi

# Test 9: Verify slider is still responsive
echo ""
echo "Step 9: Use slider one more time (top position again)"
adb -s $DEVICE logcat -c
adb -s $DEVICE shell input tap 3000 300
sleep 1

slider_clicks_final=$(adb -s $DEVICE logcat -d | grep "SLIDER CLICK" | wc -l | tr -d ' ')
echo "  → Slider clicks: $slider_clicks_final"
if [ "$slider_clicks_final" -ge 1 ]; then
    echo "  ✅ Slider remains responsive"
else
    echo "  ❌ Slider lost responsiveness"
fi

# Test 10: Drag slider beyond bounds (with colorbar hidden)
echo ""
echo "Step 10: CRITICAL - Drag slider beyond bottom boundary"
adb -s $DEVICE logcat -c
echo "  Dragging from y=200 to y=1400 (beyond limits)..."
adb -s $DEVICE shell input swipe 3000 200 3000 1400 500
sleep 1

clamping=$(adb -s $DEVICE logcat -d | grep "SLIDER CLAMP.*BOTTOM" | wc -l | tr -d ' ')
echo "  → Clamping events: $clamping"
if [ "$clamping" -ge 1 ]; then
    echo "  ✅ Slider correctly clamped at bottom boundary"
else
    echo "  ❌ No clamping detected - bounds may not be enforced"
fi

# Test 11: Show colorbar and test drag clamping again
echo ""
echo "Step 11: Show colorbar and test drag clamping"
adb -s $DEVICE shell input tap 3000 1350
sleep 1

adb -s $DEVICE logcat -c
echo "  Dragging from y=200 to y=1400 (with colorbar visible)..."
adb -s $DEVICE shell input swipe 3000 200 3000 1400 500
sleep 1

clamping_colorbar=$(adb -s $DEVICE logcat -d | grep "SLIDER CLAMP.*BOTTOM" | wc -l | tr -d ' ')
slider_end=$(adb -s $DEVICE logcat -d | grep "SLIDER CLAMP.*slider_end" | tail -1 | grep -o "slider_end=[0-9]*" | cut -d= -f2)
echo "  → Clamping events: $clamping_colorbar"
echo "  → Slider end position: $slider_end"
if [ "$clamping_colorbar" -ge 1 ]; then
    echo "  ✅ Slider correctly clamped with colorbar visible"
else
    echo "  ❌ No clamping detected with colorbar visible"
fi

# Test 12: Verify slider height is correct
echo ""
echo "Step 12: Verify slider dimensions"
echo "  → With colorbar visible:"
adb -s $DEVICE logcat -d | grep "SLIDER END.*colorbar_is_visible=1" | tail -1
adb -s $DEVICE logcat -d | grep "Slider dimensions" | tail -1

echo ""
echo "  → Hiding colorbar..."
adb -s $DEVICE shell input tap 1500 700
sleep 1
echo "  → With colorbar hidden:"
adb -s $DEVICE logcat -d | grep "SLIDER END.*colorbar_is_visible=0" | tail -1
adb -s $DEVICE logcat -d | grep "Slider dimensions" | tail -1

# Summary
echo ""
echo "================================"
echo "Summary - Extended Tests"
echo "================================"

# Count total events from full logcat
full_logcat=$(adb -s $DEVICE logcat -d)

total_slider_clicks=$(echo "$full_logcat" | grep "SLIDER CLICK" | wc -l | tr -d ' ')
total_brush_strokes=$(echo "$full_logcat" | grep "BRUSH DOWN" | wc -l | tr -d ' ')
total_picker_clicks=$(echo "$full_logcat" | grep "COLOR_PICKER.*Button clicked" | wc -l | tr -d ' ')
total_clamping=$(echo "$full_logcat" | grep "SLIDER CLAMP.*BOTTOM" | wc -l | tr -d ' ')

echo "Total slider clicks: $total_slider_clicks"
echo "Total brush strokes: $total_brush_strokes"
echo "Total picker clicks: $total_picker_clicks"
echo "Total boundary clamping events: $total_clamping"
echo ""

# Check key functionality
all_passed=true

if [ "$total_slider_clicks" -lt 5 ]; then
    echo "⚠️  Expected at least 5 slider clicks"
    all_passed=false
fi

if [ "$total_brush_strokes" -lt 2 ]; then
    echo "⚠️  Expected at least 2 brush strokes"
    all_passed=false
fi

if [ "$total_picker_clicks" -lt 1 ]; then
    echo "⚠️  Expected at least 1 picker click"
    all_passed=false
fi

if [ "$total_clamping" -lt 2 ]; then
    echo "⚠️  Expected at least 2 clamping events (hidden + visible colorbar)"
    all_passed=false
fi

if [ "$all_passed" = true ]; then
    echo "✅ ALL EXTENDED TESTS PASSED!"
    echo "   - Slider works with colorbar visible/hidden"
    echo "   - Canvas painting works"
    echo "   - Color picker button works"
    echo "   - Boundary clamping works correctly"
    exit 0
else
    echo "❌ SOME TESTS FAILED - see details above"
    exit 1
fi
