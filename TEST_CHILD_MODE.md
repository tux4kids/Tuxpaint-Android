# Child Mode Button - Manual Test Guide

## Test Date
6. Oktober 2025

## Prerequisites
- APK built from commit `2472ec75` or later
- Android device with Tuxpaint installed
- ADB connected: `adb devices`

## Test Procedure

### Step 1: Install and Start App
```bash
# Install the APK
adb install -r app/build/outputs/apk/playStore/debug/app-playStore-debug.apk

# Clear old logs
adb logcat -c

# Start Tuxpaint
adb shell am start -n org.tuxpaint.android/org.tuxpaint.tuxpaintActivity

# Start logging (in separate terminal)
adb logcat | grep "TuxPaint"
```

### Step 2: Locate Child Mode Button
- **Position:** Bottom left, Row 8, Column 1 (right of Sound button)
- **Label:** "KID"
- **Initial state:** Button UP (not pressed)

### Step 3: Test Child Mode Activation

#### 3.1 Click Child Mode Button
- Tap the "KID" button
- **Expected behavior:**
  - Button changes to DOWN state (pressed appearance)
  - Tux area disappears
  - Color palette extends to fill entire bottom area
  - Tux message: "Child mode activated!"
  - Click sound plays

#### 3.2 Verify Layout Changes
- **Tux area:** Should be completely hidden
- **Color buttons:** Should extend vertically to screen bottom
- **Color palette:** More visible color buttons (taller rows)

#### 3.3 Deactivate Child Mode
- Click "KID" button again
- **Expected behavior:**
  - Button returns to UP state
  - Tux area reappears at bottom
  - Color palette returns to normal size
  - Tux message: "Child mode deactivated."

### Step 4: Test Persistence Across Tools
- Activate child mode
- Switch between different tools (Paint, Eraser, Stamp, etc.)
- **Expected:** Child mode stays active (Tux stays hidden)

### Step 5: Verify Logcat Output (Optional)
No specific log messages for child mode yet, but you can verify:
- No crashes
- Smooth layout transitions

## Success Criteria

✅ Child mode button toggles correctly
✅ Button visual state changes (UP ↔ DOWN)
✅ Tux area hides when child mode active
✅ Color buttons extend to fill freed space
✅ Layout returns to normal when deactivated
✅ No crashes or visual glitches
✅ Feedback messages appear

## Known Limitations (Current Implementation)

- ⚠️ Child mode state is NOT persistent (resets on app restart)
- ⚠️ Only Tux hiding implemented so far
- ⚠️ Right toolbar (tool options) still visible
- ⚠️ Left toolbar not yet simplified
- ⚠️ No brush size slider yet

## Next Steps (Planned Features)

See IMPLEMENTATION_PLAN.md sections 6.3-6.5:
- Hide right toolbar, add brush size slider
- Simplify left toolbar (only 8 essential tools)
- Add "Exit Child Mode" button to toolbar
- Auto-select 4px brush in child mode

## Troubleshooting

### Button doesn't respond
- Check if click is registered: `adb logcat | grep "MOUSEBUTTONDOWN"`
- Verify button coordinates in logs

### Tux doesn't hide
- Check `child_mode` variable in C code
- Verify `setup_screen_layout()` is called on toggle

### Layout glitches
- Try toggling child mode on/off multiple times
- Switch tools to force redraw
- Report specific visual artifacts

## Commit Reference
- Initial implementation: `2472ec75`
- Feature: Child mode toggle + Hide Tux + Text Area
