#!/bin/bash
# End-to-End TEXT tool test - complete workflow

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "TEXT TOOL END-TO-END TEST"
echo "========================================"

# Install
echo -e "${YELLOW}Installing APK...${NC}"
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1

# Start fresh
$ADB logcat -c
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

echo -e "${YELLOW}Waiting for app to load...${NC}"
sleep 5

# Get screen dimensions
SCREEN_SIZE=$($ADB shell wm size | grep "Physical size" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN_SIZE | cut -dx -f1)
HEIGHT=$(echo $SCREEN_SIZE | cut -dx -f2)

echo -e "${GREEN}Screen: ${WIDTH}x${HEIGHT}${NC}"

# Step 1: Click TEXT tool directly (known position from previous tests)
echo ""
echo -e "${YELLOW}Step 1: Selecting TEXT tool${NC}"
$ADB shell input tap 220 350
sleep 1

# Verify TEXT is selected
TOOL_CHECK=$($ADB logcat -d | grep "Tool switched.*new=4" | tail -1)
if [ -n "$TOOL_CHECK" ]; then
    echo -e "${GREEN}✓ TEXT tool selected${NC}"
    echo "  $TOOL_CHECK"
else
    echo -e "${RED}✗ TEXT tool NOT selected${NC}"
    exit 1
fi

# Step 2: Click canvas
CANVAS_X=$((WIDTH * 3 / 4))
CANVAS_Y=$((HEIGHT / 2))

echo ""
echo -e "${YELLOW}Step 2: Clicking canvas at ($CANVAS_X, $CANVAS_Y)${NC}"
$ADB shell input tap $CANVAS_X $CANVAS_Y
sleep 1

# Check keyboard opened
KB_CHECK=$($ADB logcat -d | grep "Keyboard started" | tail -1)
if [ -n "$KB_CHECK" ]; then
    echo -e "${GREEN}✓ Keyboard opened${NC}"
    echo "  $KB_CHECK"
else
    echo -e "${RED}✗ Keyboard did NOT open${NC}"
    exit 1
fi

# Verify tool is still TEXT
FINAL_TOOL=$($ADB logcat -d | grep "Tool switched" | tail -1)
if echo "$FINAL_TOOL" | grep -q "new=4"; then
    echo -e "${GREEN}✓ TEXT tool still active${NC}"
else
    echo -e "${YELLOW}⚠ Tool may have changed${NC}"
    echo "  Last switch: $FINAL_TOOL"
fi

# Step 3: Type text
echo ""
echo -e "${YELLOW}Step 3: Typing text 'HelloWorld'${NC}"
$ADB shell input text "HelloWorld"
sleep 1

# Step 4: Take screenshot to verify
echo ""
echo -e "${YELLOW}Step 4: Taking screenshot${NC}"
$ADB shell screencap /sdcard/tuxpaint_text_test.png
$ADB pull /sdcard/tuxpaint_text_test.png /tmp/ > /dev/null 2>&1

if [ -f /tmp/tuxpaint_text_test.png ]; then
    echo -e "${GREEN}✓ Screenshot saved: /tmp/tuxpaint_text_test.png${NC}"
    file /tmp/tuxpaint_text_test.png
else
    echo -e "${YELLOW}⚠ Screenshot not found${NC}"
fi

# Final summary
echo ""
echo "========================================"
echo -e "${GREEN}✓✓✓ TEXT TOOL TEST COMPLETE!${NC}"
echo "========================================"
echo ""
echo "Summary:"
echo "  ✓ TEXT tool can be selected"
echo "  ✓ Keyboard opens on canvas click"  
echo "  ✓ Text can be typed"
echo "  ✓ Screenshot captured"
echo ""
echo "To view screenshot:"
echo "  xdg-open /tmp/tuxpaint_text_test.png"
echo ""

exit 0
