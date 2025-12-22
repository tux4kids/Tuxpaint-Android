#!/bin/bash
# Test script for color picker button functionality
# Tests both Expert Mode and Child Mode

DEVICE="emulator-5554"
PACKAGE="org.tuxpaint.android"
ACTIVITY="org.tuxpaint.tuxpaintActivity"

echo "================================"
echo "Color Picker Button Test Suite"
echo "================================"

# Function to start app and wait
start_app() {
    local mode=$1
    echo ""
    echo "[$mode] Starting app..."
    adb -s $DEVICE shell am force-stop $PACKAGE
    adb -s $DEVICE logcat -c
    
    # Set child mode via settings (0=expert, 1=child)
    if [ "$mode" = "EXPERT" ]; then
        adb -s $DEVICE shell "run-as $PACKAGE sh -c 'echo \"childMode=0\" > shared_prefs/TuxPaintPreferences.xml'" 2>/dev/null || true
    else
        adb -s $DEVICE shell "run-as $PACKAGE sh -c 'echo \"childMode=1\" > shared_prefs/TuxPaintPreferences.xml'" 2>/dev/null || true
    fi
    
    adb -s $DEVICE shell am start -n $PACKAGE/$ACTIVITY > /dev/null 2>&1
    echo "[$mode] Waiting for app to start..."
    sleep 8
}

# Function to simulate touch
simulate_touch() {
    local x=$1
    local y=$2
    local desc=$3
    echo "  → Touch: $desc (x=$x, y=$y)"
    adb -s $DEVICE shell input tap $x $y
    sleep 0.5
}

# Function to check logs
check_logs() {
    local pattern=$1
    local expected=$2
    local desc=$3
    
    local count=$(adb -s $DEVICE logcat -d | grep "$pattern" 2>/dev/null | wc -l)
    count=$(echo "$count" | tr -d ' ')  # Remove whitespace
    
    if [ "$count" -ge "$expected" ]; then
        echo "  ✅ $desc: Found $count occurrences (expected >= $expected)"
        return 0
    else
        echo "  ❌ $desc: Found $count occurrences (expected >= $expected)"
        return 1
    fi
}

# Test function
run_test_sequence() {
    local mode=$1
    local test_passed=0
    
    echo ""
    echo "================================"
    echo "Testing: $mode MODE"
    echo "================================"
    
    start_app "$mode"
    
    # Test 1: Paint on canvas
    echo ""
    echo "Test 1: Paint on canvas"
    simulate_touch 1500 700 "Paint on canvas"
    sleep 1
    
    if check_logs "BRUSH DOWN" 1 "Canvas painting works"; then
        ((test_passed++))
    fi
    
    # Test 2: Click canvas to slide out colorbar
    echo ""
    echo "Test 2: Click canvas to slide colorbar out"
    simulate_touch 1500 700 "Canvas click"
    sleep 0.8
    
    if check_logs "slide_colorbar_out" 1 "Colorbar slides out"; then
        ((test_passed++))
    fi
    
    if check_logs "COLOR_PICKER.*Drawing button" 1 "Color picker button appears"; then
        ((test_passed++))
    fi
    
    # Test 3: Click color picker button (bottom right)
    echo ""
    echo "Test 3: Click color picker button"
    simulate_touch 3000 1350 "Color picker button"
    sleep 1
    
    if check_logs "COLOR_PICKER.*Button clicked" 1 "Color picker button responds"; then
        ((test_passed++))
    fi
    
    if check_logs "slide_colorbar_in" 1 "Colorbar slides in"; then
        ((test_passed++))
    fi
    
    # Test 4: Click brush button (top right)
    echo ""
    echo "Test 4: Click brush button"
    simulate_touch 2950 200 "Brush button"
    sleep 0.5
    
    if check_logs "HIT(r_toolopt)=1" 1 "Brush button area hit"; then
        ((test_passed++))
    fi
    
    # Test 5: Paint again to verify canvas still works
    echo ""
    echo "Test 5: Paint on canvas again"
    simulate_touch 1200 900 "Paint on canvas"
    sleep 1
    
    if check_logs "BRUSH DOWN" 2 "Canvas painting still works"; then
        ((test_passed++))
    fi
    
    echo ""
    echo "================================"
    echo "$mode MODE: $test_passed/7 tests passed"
    echo "================================"
    
    return $test_passed
}

# Run tests
expert_passed=0
child_passed=0

run_test_sequence "EXPERT"
expert_passed=$?

run_test_sequence "CHILD"
child_passed=$?

# Final summary
echo ""
echo "================================"
echo "FINAL RESULTS"
echo "================================"
echo "Expert Mode: $expert_passed/7 tests passed"
echo "Child Mode:  $child_passed/7 tests passed"
echo ""

total_passed=$((expert_passed + child_passed))
if [ $total_passed -eq 14 ]; then
    echo "✅ ALL TESTS PASSED!"
    exit 0
elif [ $total_passed -ge 10 ]; then
    echo "⚠️  MOST TESTS PASSED (${total_passed}/14)"
    exit 1
else
    echo "❌ TESTS FAILED (${total_passed}/14)"
    exit 1
fi
