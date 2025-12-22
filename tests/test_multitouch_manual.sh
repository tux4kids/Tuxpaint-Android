#!/bin/bash

# Manual Multitouch Test Script
# This script helps test multitouch functionality on real device

DEVICE="19181FDEE004X0"

echo "=================================================="
echo "MANUAL MULTITOUCH TEST"
echo "=================================================="
echo ""
echo "Device: $DEVICE"
echo ""
echo "Steps to test:"
echo "1. App is launching now..."
echo "2. Wait for app to fully load (15 seconds)"
echo "3. Use TWO FINGERS simultaneously to draw on canvas"
echo "4. Watch logcat output for multitouch detection"
echo ""
echo "Expected behavior:"
echo "  - Single finger: 'Touch fingers detected: 1'"
echo "  - Two fingers:   'Touch fingers detected: 2'"
echo "  - Two fingers:   'Finger 0 DOWN at canvas(...)'"
echo "  - Two fingers:   'Finger 1 DOWN at canvas(...)'"
echo ""
echo "=================================================="

# Launch app
adb -s $DEVICE shell am force-stop org.tuxpaint.android
sleep 2
adb -s $DEVICE logcat -c
adb -s $DEVICE shell am start -n org.tuxpaint.android/org.tuxpaint.tuxpaintActivity

echo ""
echo "Waiting 15 seconds for app to load..."
sleep 15

echo ""
echo "NOW: Draw with ONE finger first..."
echo "=================================================="
echo "Watching logs (Ctrl+C to stop):"
echo ""

# Monitor logs in real-time
adb -s $DEVICE logcat | grep --line-buffered "TuxPaint"
