#!/bin/bash
# Helper script to switch to child mode and test slider

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"

echo "================================"
echo "Switch to Child Mode & Test Slider"
echo "================================"
echo ""
echo "Manual steps:"
echo "1. Click Paint tool (if not already selected)"
echo "2. Draw something on canvas"
echo "3. Click Save button (3rd from bottom on left)"
echo "4. Click 'Yes' to save"
echo "5. Restart app - it should now be in child mode"
echo ""
echo "Or use automated version below:"
echo ""

read -p "Do you want to automate this? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
    echo "Drawing on canvas..."
    adb -s $DEVICE shell input tap 500 500
    sleep 1
    
    echo "Clicking Save button (bottom left)..."
    adb -s $DEVICE shell input tap 40 240
    sleep 2
    
    echo "Confirming save..."
    adb -s $DEVICE shell input tap 720 720
    sleep 2
    
    echo "Restarting app..."
    adb -s $DEVICE shell am force-stop $PACKAGE
    adb -s $DEVICE logcat -c
    adb -s $DEVICE shell am start -n $PACKAGE/org.tuxpaint.tuxpaintActivity
    sleep 10
    
    echo "Checking mode..."
    mode=$(adb -s $DEVICE logcat -d | grep "child_mode=" | tail -1)
    echo "$mode"
    
    echo ""
    echo "Testing slider..."
    adb -s $DEVICE logcat -c
    adb -s $DEVICE shell input tap 3000 500
    sleep 1
    
    adb -s $DEVICE logcat -d | grep "Slider dimensions"
    
    echo ""
    echo "Taking screenshot..."
    adb -s $DEVICE exec-out screencap -p > /tmp/child_mode_slider.png
    echo "Saved to /tmp/child_mode_slider.png"
fi
