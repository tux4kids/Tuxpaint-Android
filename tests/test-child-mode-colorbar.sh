#!/bin/bash
# Test Child Mode Colorbar visibility

echo "=========================================="
echo "CHILD MODE COLORBAR TEST"
echo "=========================================="

echo "Building APK..."
./gradlew assembleDebug -q

echo "Installing and starting app..."
adb install -r -t app/build/outputs/apk/playStore/debug/app-playStore-debug.apk
adb shell am force-stop org.tuxpaint.android
adb logcat -c
adb shell am start org.tuxpaint.android/org.tuxpaint.tuxpaintActivity

echo "Waiting for app initialization (12 seconds)..."
sleep 12

echo ""
echo "=========================================="
echo "TEST 1: Switch to Child Mode"
echo "=========================================="
# Tap child mode button (second button in top-left toolbar)
adb shell input tap 150 50

sleep 3

echo "Checking Child Mode setup logs..."
adb logcat -d | grep -E "TuxPaint.*setup_normal_screen_layout.*child_mode" | tail -3

echo ""
echo "=========================================="
echo "MANUAL VERIFICATION REQUIRED"
echo "=========================================="
echo "Please check on device:"
echo "1. Are ALL color buttons visible (not just the rightmost 3)?"
echo "2. Is the colorbar stretched to fill the space?"
echo "3. Are the special buttons (color picker, etc.) visible?"
echo ""
echo "If colors are visible: ✓ Test PASSED"
echo "If colors are cut off: ✗ Test FAILED"
echo "=========================================="
