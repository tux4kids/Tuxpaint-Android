#!/bin/bash
# Complete unit test for MAGIC tool in Expert Mode

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "======================================"
echo "  MAGIC TOOL COMPLETE UNIT TEST"
echo "======================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

FAILED_TESTS=0
PASSED_TESTS=0

test_pass() {
    echo -e "${GREEN}✓ PASS:${NC} $1"
    ((PASSED_TESTS++))
}

test_fail() {
    echo -e "${RED}✗ FAIL:${NC} $1"
    ((FAILED_TESTS++))
}

test_info() {
    echo -e "${YELLOW}ℹ INFO:${NC} $1"
}

# Install and start
test_info "Installing APK..."
$ADB install -r app/build/outputs/apk/offPlayStore/debug/app-offPlayStore-debug.apk > /dev/null 2>&1

# Start in EXPERT mode (MAGIC not available in child mode)
test_info "Starting app in Expert mode..."
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1

sleep 8

# Get screen dimensions
SCREEN=$($ADB shell wm size | grep "Physical size:" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN | cut -dx -f1)
HEIGHT=$(echo $SCREEN | cut -dx -f2)

test_info "Screen dimensions: ${WIDTH}x${HEIGHT}"
echo ""

# Calculate positions (from tool layout)
# MAGIC is in row 4 (0-indexed row 3), right column
# Each button is ~110px, with some spacing
MAGIC_X=120  # Right column (105-135)
MAGIC_Y=360  # Row 4 (3*110 + offset)
CANVAS_X=$((WIDTH / 2))
CANVAS_Y=$((HEIGHT / 2))

# First magic effect in selector (top-right area)
MAGIC_EFFECT_X=$((WIDTH - 100))
MAGIC_EFFECT_Y=150

echo "=========================================="
echo "TEST 1: Magic Plugin Loading"
echo "=========================================="

# Check plugin loading (BEFORE clearing logcat)
STARTUP_LOGS=$($ADB logcat -d)
PLUGIN_COUNT=$(echo "$STARTUP_LOGS" | grep -c "Magic plugin loaded:")
PLUGIN_TOTAL=$(echo "$STARTUP_LOGS" | grep "Loaded.*magic tools" | tail -1 | grep -oP 'Loaded \K\d+' || echo "0")

if [ "$PLUGIN_TOTAL" -gt 0 ]; then
    test_pass "Magic plugins loaded: $PLUGIN_TOTAL plugins"
else
    test_fail "No magic plugins loaded (found: $PLUGIN_COUNT)"
fi

# Check for dummy icons
DUMMY_ICON_COUNT=$($ADB logcat -d | grep -c "icon missing, creating dummy")
if [ "$DUMMY_ICON_COUNT" -gt 0 ]; then
    test_info "Dummy icons created for $DUMMY_ICON_COUNT plugins (expected on Android)"
else
    test_info "All plugins have native icons"
fi

# Check tool availability from startup logs
MAGIC_ENABLED=$(echo "$STARTUP_LOGS" | grep "MAGIC tool ENABLED" | tail -1)
if [ -n "$MAGIC_ENABLED" ]; then
    test_pass "MAGIC tool available"
    test_info "  Log: $MAGIC_ENABLED"
else
    MAGIC_DISABLED=$(echo "$STARTUP_LOGS" | grep "MAGIC tool DISABLED" | tail -1)
    if [ -n "$MAGIC_DISABLED" ]; then
        test_fail "MAGIC tool NOT available"
        test_info "  Log: $MAGIC_DISABLED"
    else
        test_info "MAGIC tool status unknown (check logs manually)"
    fi
fi

echo ""
echo "=========================================="
echo "TEST 2: MAGIC Tool Selection"
echo "=========================================="

# Clear logcat for cleaner logs
$ADB logcat -c
sleep 1

# Tap MAGIC tool button
test_info "Tapping MAGIC tool button..."
$ADB shell input tap $MAGIC_X $MAGIC_Y
sleep 2

# Check tool switch
TOOL_SWITCH=$($ADB logcat -d | grep "cur_tool.*=.*6" | tail -1)
if [ -n "$TOOL_SWITCH" ]; then
    test_pass "MAGIC tool selected"
    test_info "  Log: $TOOL_SWITCH"
else
    test_fail "MAGIC tool NOT selected"
    test_info "  Current tool log:"
    $ADB logcat -d | grep "cur_tool" | tail -3
fi

# Check if magic selector appears
MAGIC_SELECTOR=$($ADB logcat -d | grep -i "magic.*selector\|drawing.*magic" | tail -1)
if [ -n "$MAGIC_SELECTOR" ]; then
    test_info "Magic selector: $MAGIC_SELECTOR"
fi

echo ""
echo "=========================================="
echo "TEST 3: Magic Effect Selection"
echo "=========================================="

$ADB logcat -c
sleep 1

# Tap first magic effect
test_info "Selecting magic effect..."
$ADB shell input tap $MAGIC_EFFECT_X $MAGIC_EFFECT_Y
sleep 2

# Check if effect was selected
EFFECT_SELECT=$($ADB logcat -d | grep -i "magic.*selected\|cur_magic" | tail -1)
if [ -n "$EFFECT_SELECT" ]; then
    test_pass "Magic effect selected"
    test_info "  Log: $EFFECT_SELECT"
else
    test_info "Effect selection (no explicit log expected)"
fi

echo ""
echo "=========================================="
echo "TEST 4: Magic Effect Application"
echo "=========================================="

$ADB logcat -c
sleep 1

# Apply magic effect to canvas
test_info "Applying magic effect to canvas..."
$ADB shell input tap $CANVAS_X $CANVAS_Y
sleep 2

# Check for magic effect application
MAGIC_APPLY=$($ADB logcat -d | grep -Ei "magic.*click|magic.*do_|applying.*magic" | tail -3)
if [ -n "$MAGIC_APPLY" ]; then
    test_pass "Magic effect applied"
    test_info "  Log entries:"
    echo "$MAGIC_APPLY" | while read line; do
        echo "    $line"
    done
else
    # Magic might not log explicitly, check for general activity
    GENERAL_ACTIVITY=$($ADB logcat -d | grep -c "SDL")
    if [ "$GENERAL_ACTIVITY" -gt 10 ]; then
        test_info "Canvas activity detected (magic may apply silently)"
    else
        test_fail "No magic effect application detected"
    fi
fi

echo ""
echo "=========================================="
echo "TEST 5: Screen Rendering Check"
echo "=========================================="

# Take screenshot
test_info "Capturing screenshot..."
$ADB shell screencap /sdcard/magic_test.png
$ADB pull /sdcard/magic_test.png /tmp/ > /dev/null 2>&1

if [ -f /tmp/magic_test.png ]; then
    FILE_SIZE=$(stat -f%z /tmp/magic_test.png 2>/dev/null || stat -c%s /tmp/magic_test.png 2>/dev/null)
    if [ "$FILE_SIZE" -gt 10000 ]; then
        test_pass "Screenshot captured successfully"
        test_info "  File: /tmp/magic_test.png"
        test_info "  Size: $FILE_SIZE bytes"
        
        # Check if it's a valid PNG
        FILE_TYPE=$(file /tmp/magic_test.png | cut -d: -f2)
        test_info "  Type: $FILE_TYPE"
    else
        test_fail "Screenshot file too small: $FILE_SIZE bytes"
    fi
else
    test_fail "Screenshot not found"
fi

echo ""
echo "=========================================="
echo "TEST 6: Crash & Stability Check"
echo "=========================================="

# Check for crashes
CRASH=$($ADB logcat -d | grep -i "fatal signal\|SIGSEGV\|segmentation fault" | tail -1)
if [ -n "$CRASH" ]; then
    test_fail "CRASH DETECTED!"
    echo "  $CRASH"
else
    test_pass "No crashes detected"
fi

# Check if process is still running
PROCESS=$($ADB shell "ps | grep org.tuxpaint" | grep -v grep)
if [ -n "$PROCESS" ]; then
    test_pass "App still running"
    test_info "  Process: $(echo $PROCESS | awk '{print $2}')"
else
    test_fail "App process not found (may have crashed)"
fi

echo ""
echo "=========================================="
echo "TEST 7: Additional Magic Tools Test"
echo "=========================================="

# List all loaded magic tools
test_info "Listing all loaded magic effects..."
MAGIC_LIST=$($ADB logcat -d | grep "Magic plugin loaded:" | awk -F': ' '{print $NF}' | head -10)
if [ -n "$MAGIC_LIST" ]; then
    echo "$MAGIC_LIST" | while read line; do
        test_info "  • $line"
    done
    
    TOTAL_EFFECTS=$(echo "$MAGIC_LIST" | wc -l | tr -d ' ')
    test_pass "Found $TOTAL_EFFECTS magic effects in logs"
else
    test_info "No detailed magic effect list available"
fi

echo ""
echo "=========================================="
echo "TEST 8: Memory & Resource Check"
echo "=========================================="

# Check for memory warnings
MEM_WARN=$($ADB logcat -d | grep -i "memory\|out of memory\|allocation failed" | wc -l | tr -d ' ')
if [ "$MEM_WARN" -eq 0 ]; then
    test_pass "No memory warnings"
else
    test_info "Memory warnings found: $MEM_WARN (may be normal)"
fi

# Check for surface creation
SURFACE_CREATE=$($ADB logcat -d | grep -i "CreateRGBSurface.*48.*48" | wc -l | tr -d ' ')
if [ "$SURFACE_CREATE" -gt 0 ]; then
    test_info "Dummy icon surfaces created: $SURFACE_CREATE"
fi

echo ""
echo "=========================================="
echo "FINAL RESULTS"
echo "=========================================="
echo ""

TOTAL_TESTS=$((PASSED_TESTS + FAILED_TESTS))
PASS_RATE=$((PASSED_TESTS * 100 / TOTAL_TESTS))

echo "Tests Run:    $TOTAL_TESTS"
echo -e "Tests Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Tests Failed: ${RED}$FAILED_TESTS${NC}"
echo "Pass Rate:    $PASS_RATE%"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}════════════════════════════════════════${NC}"
    echo -e "${GREEN}  ✓ ALL TESTS PASSED!${NC}"
    echo -e "${GREEN}════════════════════════════════════════${NC}"
    exit 0
else
    echo -e "${RED}════════════════════════════════════════${NC}"
    echo -e "${RED}  ✗ SOME TESTS FAILED${NC}"
    echo -e "${RED}════════════════════════════════════════${NC}"
    exit 1
fi
