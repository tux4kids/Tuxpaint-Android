#!/bin/bash

# Test runner script to observe EGL threading errors
# This script runs the instrumented test and monitors logcat for EGL errors

echo "=========================================="
echo "TuxPaint EGL Threading Error Test"
echo "=========================================="
echo ""

# Clear logcat
echo "Clearing logcat..."
adb logcat -c

# Start logcat monitoring in background
echo "Starting logcat monitor for EGL errors..."
adb logcat -s EGL_emulation:E libEGL:E TuxpaintActivityTest:D AndroidRuntime:E | \
    grep --line-buffered -E "(EGL_BAD_ACCESS|eglMakeCurrent|TuxpaintActivityTest|FATAL)" &
LOGCAT_PID=$!

# Give logcat time to start
sleep 2

echo ""
echo "Building and running instrumented test..."
echo "=========================================="
./gradlew :app:connectedOffPlayStoreDebugAndroidTest

TEST_EXIT_CODE=$?

# Keep monitoring for a few more seconds
echo ""
echo "Test complete. Monitoring logcat for 3 more seconds..."
sleep 3

# Stop logcat
kill $LOGCAT_PID 2>/dev/null

echo ""
echo "=========================================="
echo "Test finished with exit code: $TEST_EXIT_CODE"
echo "=========================================="
echo ""
echo "To see full logcat output, run:"
echo "  adb logcat | grep -E '(org.tuxpaint|EGL)'"
