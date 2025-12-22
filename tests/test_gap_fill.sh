#!/bin/bash

# Wait for device
echo "⏳ Waiting for emulator..."
adb wait-for-device
echo "✅ Device connected!"

# Install APK
echo "📦 Installing APK..."
adb install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk

# Clear logs and run test
echo "🧪 Running unittest..."
adb logcat -c
adb shell am instrument -w -e class 'org.tuxpaint.tests.MultitouchTest#testTwoFingerSimultaneousPainting' \
  org.tuxpaint.test/androidx.test.runner.AndroidJUnitRunner &

# Wait for test to complete
sleep 15

echo ""
echo "=== 📊 GAP FILL RESULTS ==="
echo ""

# Count gap fills
GAP_FILLS=$(adb logcat -d | grep "TuxPaint" | grep "GAP_FILL" | wc -l)
echo "Gap fills: $GAP_FILLS"

# Count normal draws
DRAWS=$(adb logcat -d | grep "TuxPaint" | grep "DRAW finger" | wc -l)
echo "Normal draws: $DRAWS"

# Count Finger 0 vs Finger 1
F0_DRAWS=$(adb logcat -d | grep "TuxPaint" | grep "DRAW finger=0" | wc -l)
F1_DRAWS=$(adb logcat -d | grep "TuxPaint" | grep "DRAW finger=1" | wc -l)
echo "Finger 0 draws: $F0_DRAWS | Finger 1 draws: $F1_DRAWS"

# Count gap fills per finger
F0_GAPS=$(adb logcat -d | grep "TuxPaint" | grep "GAP_FILL finger=0" | wc -l)
F1_GAPS=$(adb logcat -d | grep "TuxPaint" | grep "GAP_FILL finger=1" | wc -l)
echo "Finger 0 gaps: $F0_GAPS | Finger 1 gaps: $F1_GAPS"

echo ""
echo "=== 📝 SAMPLE LOGS ==="
adb logcat -d | grep "TuxPaint" | grep -E "(GAP_FILL|DRAW finger)" | head -30

echo ""
echo "=== ✅ TEST COMPLETED ==="
echo "Expected: Gap fills should connect dotted lines"
echo "Total events: $((GAP_FILLS + DRAWS)) (gap_fills + draws)"
