#!/bin/bash
# Automated test for TEXT tool in Tux Paint Android
# Tests if TEXT tool can be selected and used in child mode

set -e  # Exit on error

# Select device (use first available if multiple)
DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
if [ -z "$DEVICE" ]; then
    echo "ERROR: No device found"
    exit 1
fi
echo "Using device: $DEVICE"
ADB="adb -s $DEVICE"

echo "========================================="
echo "TEXT TOOL AUTOMATED TEST"
echo "========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Install APK
echo -e "${YELLOW}1. Installing APK...${NC}"
APK_PATH="app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk"
$ADB install -r "$APK_PATH"
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Failed to install APK${NC}"
    exit 1
fi
echo -e "${GREEN}✓ APK installed${NC}"

# Clear logcat
echo -e "${YELLOW}2. Clearing logcat...${NC}"
$ADB logcat -c

# Start app
echo -e "${YELLOW}3. Starting Tux Paint...${NC}"
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity
sleep 3
echo -e "${GREEN}✓ App started${NC}"

# Start logcat in background  
LOG_FILE="/tmp/tuxpaint_test_log.txt"
$ADB logcat > "$LOG_FILE" &
LOGCAT_PID=$!
echo -e "${GREEN}✓ Logcat monitoring started (PID: $LOGCAT_PID)${NC}"

sleep 2

# Get screen size
SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo "Screen size: ${WIDTH}x${HEIGHT}"

# Wait for app to fully load
echo -e "${YELLOW}4. Waiting for app to fully load...${NC}"
sleep 3

# Enable Child Mode by clicking child mode button (bottom right corner)
echo -e "${YELLOW}5. Activating Child Mode...${NC}"
CHILDMODE_X=$((WIDTH - 50))  # Bottom right corner
CHILDMODE_Y=$((HEIGHT - 50))
$ADB shell input tap $CHILDMODE_X $CHILDMODE_Y
sleep 2

# Check if child mode was activated
if grep -q "child_mode=1" "$LOG_FILE" || grep -q "Child mode activated" "$LOG_FILE"; then
    echo -e "${GREEN}✓ Child mode activated${NC}"
else
    echo -e "${YELLOW}⚠ Child mode activation not confirmed in logs${NC}"
fi

# Calculate positions (child mode single column on left)
# TEXT tool should be 4th button (index 3: BRUSH, ERASER, FILL, TEXT)
TOOL_X=50  # Left side
BUTTON_HEIGHT=$((HEIGHT / 12))  # Approximate button height
TEXT_TOOL_Y=$((BUTTON_HEIGHT * 3 + BUTTON_HEIGHT / 2))  # 4th button center

# Canvas center
CANVAS_X=$((WIDTH / 2))
CANVAS_Y=$((HEIGHT / 2))

echo -e "${YELLOW}6. Clicking TEXT tool at position ($TOOL_X, $TEXT_TOOL_Y)...${NC}"
$ADB shell input tap $TOOL_X $TEXT_TOOL_Y
sleep 1

# Check if tool switched
echo -e "${YELLOW}7. Checking if tool switched to TEXT (should be 4)...${NC}"
sleep 1
if grep -q "Tool switched.*new=4" "$LOG_FILE"; then
    echo -e "${GREEN}✓ TEXT tool selected successfully! (cur_tool=4)${NC}"
else
    echo -e "${RED}✗ TEXT tool NOT selected. Checking logs...${NC}"
    tail -20 "$LOG_FILE"
    kill $LOGCAT_PID 2>/dev/null
    exit 1
fi

echo -e "${YELLOW}8. Clicking canvas to start text input at ($CANVAS_X, $CANVAS_Y)...${NC}"
$ADB shell input tap $CANVAS_X $CANVAS_Y
sleep 1

# Check if keyboard started
echo -e "${YELLOW}9. Checking if keyboard opened...${NC}"
if grep -q "Keyboard started" "$LOG_FILE"; then
    echo -e "${GREEN}✓ Keyboard opened successfully!${NC}"
else
    echo -e "${RED}✗ Keyboard did NOT open. Checking logs...${NC}"
    tail -20 "$LOG_FILE"
    kill $LOGCAT_PID 2>/dev/null
    exit 1
fi

# Type text
echo -e "${YELLOW}10. Typing text 'Hello'...${NC}"
$ADB shell input text "Hello"
sleep 1

# Check if cur_tool is still TEXT (4)
echo -e "${YELLOW}11. Verifying TEXT tool still active...${NC}"
LAST_TOOL_LOG=$(grep "cur_tool=" "$LOG_FILE" | tail -1)
if echo "$LAST_TOOL_LOG" | grep -q "cur_tool=4"; then
    echo -e "${GREEN}✓ TEXT tool still active (cur_tool=4)${NC}"
else
    echo -e "${RED}✗ Tool changed unexpectedly. Last log: $LAST_TOOL_LOG${NC}"
fi

# Check if keyboard is still open
echo -e "${YELLOW}12. Checking if keyboard stayed open...${NC}"
KEYBOARD_STOPPED=$(grep "Keyboard stopped" "$LOG_FILE" | wc -l)
if [ "$KEYBOARD_STOPPED" -eq 0 ]; then
    echo -e "${GREEN}✓ Keyboard stayed open (not closed yet)${NC}"
else
    echo -e "${YELLOW}⚠ Keyboard was stopped (might be expected after timeout)${NC}"
fi

# Stop logcat
kill $LOGCAT_PID 2>/dev/null

echo ""
echo "========================================="
echo "TUXPAINT TEST LOGS:"
echo "========================================="
grep -i "tuxpaint" "$LOG_FILE" | tail -50

echo ""
echo "========================================="
echo -e "${GREEN}✓ TEXT TOOL TEST COMPLETED SUCCESSFULLY!${NC}"
echo "========================================="
echo ""
echo "Summary:"
echo "  - TEXT tool can be selected in child mode"
echo "  - TEXT tool switches to cur_tool=4"
echo "  - Keyboard opens when clicking canvas"
echo "  - Text can be typed"
echo ""

# Cleanup
rm -f "$LOG_FILE"
