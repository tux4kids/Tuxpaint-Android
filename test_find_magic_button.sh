#!/bin/bash
# Find MAGIC button by trying all tool positions

DEVICE=$(adb devices | grep -v "List" | grep "device$" | head -1 | awk '{print $1}')
ADB="adb -s $DEVICE"

echo "Finding MAGIC button position..."

# Get screen  dimensions
SCREEN=$($ADB shell wm size | grep "Physical size:" | cut -d: -f2 | tr -d ' ')
WIDTH=$(echo $SCREEN | cut -dx -f1)
HEIGHT=$(echo $SCREEN | cut -dx -f2)

echo "Screen: ${WIDTH}x${HEIGHT}"
echo ""

# Start app
$ADB shell am start -n org.tuxpaint/.tuxpaintActivity > /dev/null 2>&1
sleep 8

# Test grid of positions in tool area
# Left column: X=~50-60
# Right column: X=~110-120
# Rows: Y=50, 160, 270, 380, 490, 600, 710, 820

echo "Testing tool positions:"
echo "Row | Left Col (X=55) | Right Col (X=115)"
echo "----|-----------------|------------------"

for row in 0 1 2 3 4 5 6 7 8; do
    Y=$((50 + row * 110))
    
    # Test left column
    $ADB logcat -c
    $ADB shell input tap 55 $Y
    sleep 1
    LEFT_TOOL=$($ADB logcat -d | grep "Tool switched.*new=" | tail -1 | grep -oP 'new=\K\d+' || echo "?")
    LEFT_NAME=""
    case $LEFT_TOOL in
        0) LEFT_NAME="BRUSH" ;;
        1) LEFT_NAME="STAMP" ;;
        2) LEFT_NAME="LINES" ;;
        3) LEFT_NAME="SHAPES" ;;
        4) LEFT_NAME="TEXT" ;;
        5) LEFT_NAME="LABEL" ;;
        6) LEFT_NAME="MAGIC" ;;
        7) LEFT_NAME="UNDO" ;;
        8) LEFT_NAME="REDO" ;;
        9) LEFT_NAME="ERASER" ;;
        10) LEFT_NAME="NEW" ;;
        11) LEFT_NAME="OPEN" ;;
        12) LEFT_NAME="SAVE" ;;
        13) LEFT_NAME="PRINT" ;;
        14) LEFT_NAME="QUIT" ;;
        15) LEFT_NAME="FILL" ;;
        *) LEFT_NAME="?" ;;
    esac
    
    # Test right column
    $ADB logcat -c
    $ADB shell input tap 115 $Y
    sleep 1
    RIGHT_TOOL=$($ADB logcat -d | grep "Tool switched.*new=" | tail -1 | grep -oP 'new=\K\d+' || echo "?")
    RIGHT_NAME=""
    case $RIGHT_TOOL in
        0) RIGHT_NAME="BRUSH" ;;
        1) RIGHT_NAME="STAMP" ;;
        2) RIGHT_NAME="LINES" ;;
        3) RIGHT_NAME="SHAPES" ;;
        4) RIGHT_NAME="TEXT" ;;
        5) RIGHT_NAME="LABEL" ;;
        6) RIGHT_NAME="MAGIC" ;;
        7) RIGHT_NAME="UNDO" ;;
        8) RIGHT_NAME="REDO" ;;
        9) RIGHT_NAME="ERASER" ;;
        10) RIGHT_NAME="NEW" ;;
        11) RIGHT_NAME="OPEN" ;;
        12) RIGHT_NAME="SAVE" ;;
        13) RIGHT_NAME="PRINT" ;;
        14) RIGHT_NAME="QUIT" ;;
        15) RIGHT_NAME="FILL" ;;
        *) RIGHT_NAME="?" ;;
    esac
    
    printf " %d  | %s (Y=%d)    | %s (Y=%d)\n" $row "$LEFT_NAME" $Y "$RIGHT_NAME" $Y
    
    # Check if we found MAGIC
    if [ "$LEFT_TOOL" == "6" ]; then
        echo ""
        echo "✓ FOUND MAGIC at LEFT column: X=55, Y=$Y"
        echo "  Use: MAGIC_X=55 MAGIC_Y=$Y"
        exit 0
    fi
    if [ "$RIGHT_TOOL" == "6" ]; then
        echo ""
        echo "✓ FOUND MAGIC at RIGHT column: X=115, Y=$Y"
        echo "  Use: MAGIC_X=115 MAGIC_Y=$Y"
        exit 0
    fi
done

echo ""
echo "✗ MAGIC button not found in tested positions"
echo "  Check tool_avail[6] or child mode settings"
