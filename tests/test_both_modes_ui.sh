#!/bin/bash
# Test both modes by switching via UI

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"

echo "================================"
echo "Both Modes Test via UI"
echo "================================"

test_current_mode() {
    local mode_name=$1
    echo ""
    echo "Testing current mode: $mode_name"
    echo "================================"
    
    # Wait for stable state
    sleep 2
    adb -s $DEVICE logcat -c
    
    # Test 1: Paint
    echo "  → Test: Paint on canvas"
    adb -s $DEVICE shell input tap 1500 700
    sleep 1
    
    brush=$(adb -s $DEVICE logcat -d | grep "BRUSH DOWN" | wc -l | tr -d ' ')
    if [ "$brush" -ge 1 ]; then
        echo "  ✅ Canvas painting works"
    else
        echo "  ❌ Canvas painting failed"
    fi
    
    # Test 2: Hide colorbar
    echo "  → Test: Hide colorbar"
    adb -s $DEVICE shell input tap 1500 700
    sleep 1
    
    slide_out=$(adb -s $DEVICE logcat -d | grep "slide_colorbar_out" | wc -l | tr -d ' ')
    if [ "$slide_out" -ge 1 ]; then
        echo "  ✅ Colorbar slides out"
    else
        echo "  ⚠️  Colorbar didn't slide out"
    fi
    
    # Test 3: Check mode in logs
    current_mode=$(adb -s $DEVICE logcat -d | grep -E "child_mode=[01]" | tail -1)
    echo "  Mode detected: $current_mode"
    
    # Test 4: Color picker button
    echo "  → Test: Color picker button"
    adb -s $DEVICE shell input tap 3000 1350
    sleep 1
    
    picker=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*Button clicked" | wc -l | tr -d ' ')
    if [ "$picker" -ge 1 ]; then
        echo "  ✅ Color picker button works"
    else
        echo "  ❌ Color picker button failed"
    fi
    
    # Test 5: Brush button
    echo "  → Test: Brush button"
    adb -s $DEVICE shell input tap 2950 200
    sleep 0.5
    
    toolopt=$(adb -s $DEVICE logcat -d | grep "HIT(r_toolopt)=1" | wc -l | tr -d ' ')
    if [ "$toolopt" -ge 1 ]; then
        echo "  ✅ Brush button works"
    else
        echo "  ⚠️  Brush button might not have been clicked"
    fi
    
    # Summary
    echo ""
    echo "  Summary for $mode_name:"
    echo "  - Brush strokes: $brush"
    echo "  - Colorbar slides: $slide_out"
    echo "  - Color picker: $picker"
    echo "  - Toolopt hits: $toolopt"
}

# Start app
echo ""
echo "Starting app..."
adb -s $DEVICE shell am force-stop $PACKAGE
adb -s $DEVICE logcat -c
adb -s $DEVICE shell am start -n $PACKAGE/org.tuxpaint.tuxpaintActivity > /dev/null 2>&1
sleep 8

# Test current mode (probably Expert)
test_current_mode "Current Mode (likely Expert)"

# Try to access settings to switch mode
echo ""
echo "================================"
echo "Manual mode switch required"
echo "================================"
echo "To fully test Child Mode with slider:"
echo "1. Open Tuxpaint app"
echo "2. Click Config button (bottom left)"
echo "3. Enable 'Simple Shapes Only' or 'Child Mode'"
echo "4. Return to app"
echo "5. Run this script again"
echo ""
echo "For now, verifying core functionality in current mode..."

# Additional verification
echo ""
echo "================================"
echo "Core Functionality Verification"
echo "================================"

adb -s $DEVICE logcat -c
echo ""
echo "Test sequence: Paint → Hide → Show → Paint"

adb -s $DEVICE shell input tap 1500 600
sleep 0.5
echo "  1. Paint stroke"

adb -s $DEVICE shell input tap 1500 700
sleep 0.8
echo "  2. Hide colorbar"

adb -s $DEVICE shell input tap 3000 1350
sleep 0.8
echo "  3. Show colorbar (via picker button)"

adb -s $DEVICE shell input tap 1500 900
sleep 0.5
echo "  4. Paint stroke again"

# Check results
sleep 1
echo ""
echo "Results:"
brushes=$(adb -s $DEVICE logcat -d | grep "BRUSH DOWN" | wc -l | tr -d ' ')
slides_out=$(adb -s $DEVICE logcat -d | grep "slide_colorbar_out" | wc -l | tr -d ' ')
slides_in=$(adb -s $DEVICE logcat -d | grep "slide_colorbar_in" | wc -l | tr -d ' ')
picker_clicks=$(adb -s $DEVICE logcat -d | grep "COLOR_PICKER.*Button clicked" | wc -l | tr -d ' ')

echo "  - Brush strokes: $brushes (expected: 2)"
echo "  - Slides out: $slides_out (expected: ≥1)"
echo "  - Slides in: $slides_in (expected: ≥1)"
echo "  - Picker clicks: $picker_clicks (expected: ≥1)"

if [ "$brushes" -ge 2 ] && [ "$slides_out" -ge 1 ] && [ "$slides_in" -ge 1 ] && [ "$picker_clicks" -ge 1 ]; then
    echo ""
    echo "✅ ALL CORE FUNCTIONALITY TESTS PASSED!"
else
    echo ""
    echo "❌ SOME TESTS FAILED"
fi

echo ""
echo "================================"
echo "Test completed"
echo "================================"
