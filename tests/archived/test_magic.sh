#!/bin/bash
# Test MAGIC tool

set -e

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "=== MAGIC TOOL TEST ==="

# Install and start
echo "Installing..."
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

echo "Waiting for app..."
sleep 5

# Check logs for magic plugin loading
echo "Checking if magic plugins loaded..."
$ADB logcat -d | grep -i "magic\|plugin" | tail -20

# Count loaded plugins
MAGIC_COUNT=$($ADB logcat -d | grep "Loaded.*magic" | tail -1)
echo ""
echo "Magic plugins status: $MAGIC_COUNT"

if echo "$MAGIC_COUNT" | grep -q "Loaded [1-9]"; then
    echo "✓✓✓ MAGIC PLUGINS LOADED!"
else
    echo "✗ No magic plugins loaded"
    echo ""
    echo "Checking nativelibdir:"
    $ADB logcat -d | grep "nativelibdir\|Loading magic"
fi

# Check if MAGIC tool is available
TOOL_STATUS=$($ADB logcat -d | grep "tool_avail.*MAGIC" | tail -1)
if [ -n "$TOOL_STATUS" ]; then
    echo "MAGIC tool status: $TOOL_STATUS"
fi

exit 0
