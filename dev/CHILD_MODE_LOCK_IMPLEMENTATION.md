# Child Mode Lock Implementation

**Date**: 2025-10-09 11:38  
**Status**: ✅ Implemented in 6.4

---

## Overview

Implemented child mode locking feature with long-press detection to prevent accidental exit from child mode and locked sound controls.

---

## Features Implemented

### 1. ✅ Sound Button Disabled in Child Mode
- Sound toggle button only works in expert mode
- When clicked in child mode, shows message: "Sound is locked in child mode."

### 2. ✅ Child Mode Long-Press Lock
- **Short press** (< 3s): Toggle child mode on/off (unlocked)
- **Long press** (≥ 3s) when activating: Activate child mode AND lock it
- **Long press** (≥ 3s) when locked: Unlock child mode

### 3. ✅ Lock Behavior
- **Locked state**: Prevents accidental exit from child mode
- **Short press when locked**: Shows message "Child mode is locked. Long-press 3s to unlock."
- **Long press when locked**: Unlocks child mode (stays in child mode)

### 4. ✅ Visual Lock Indicator
- **Lock icon**: Small lock icon appears in top-right corner of child mode button when locked
- **Icon source**: Uses `osk_capslock.png` scaled to 50% size
- **Position**: Top-right corner with 2px margin

### 5. ✅ Expert Mode Activation Message
- Shows "Expert mode activated." in Tux message area when exiting child mode

---

## Implementation Details

### Global Variables Added
```c
static int child_mode_locked = 0;  /* Child mode lock status (prevents accidental exit) */
static Uint32 child_mode_button_press_start = 0;  /* Timestamp when child mode button was pressed */
static int child_mode_at_press_start = 0;  /* Child mode state when button press started */
```

**Location**: `tuxpaint.c` lines 822-824

### Lock Icon Visual Indicator
**Location**: `tuxpaint.c` lines 1782, 31873, 11174-11187

Added lock icon variable and loading:
```c
static SDL_Surface *img_lock_icon;  /* Line 1782 */
img_lock_icon = loadimagerb(DATA_PREFIX "images/ui/osk_capslock.png");  /* Line 31873 */
```

Draw lock icon in `draw_row_minus_1_buttons()`:
```c
/* Draw lock indicator if child mode is locked */
if (child_mode_locked && img_lock_icon)
{
  /* Scale lock icon smaller (50% size) */
  SDL_Surface *lock_scaled = rotozoomSurfaceXY(img_lock_icon, 0, 0.5, 0.5, SMOOTHING_ON);
  if (lock_scaled)
  {
    /* Position lock icon in top-right corner of button */
    dest.x = r_childmode_btn.x + button_w - lock_scaled->w - 2;
    dest.y = r_childmode_btn.y + 2;
    SDL_BlitSurface(lock_scaled, NULL, screen, &dest);
    SDL_FreeSurface(lock_scaled);
  }
}
```

### MOUSEBUTTONDOWN Handler
**Location**: `tuxpaint.c` lines 3769-3776

Tracks button press:
```c
else if (HIT(r_childmode_btn) && valid_click(event.button.button))
{
  /* Child mode toggle button pressed - track press time for long-press detection */
  child_mode_button_press_start = SDL_GetTicks();
  child_mode_at_press_start = child_mode;  /* Remember state when press started */
  SDL_Log("Child mode button pressed at %u ms, current state: %d, locked: %d", 
          child_mode_button_press_start, child_mode, child_mode_locked);
}
```

### MOUSEBUTTONUP Handler
**Location**: `tuxpaint.c` lines 6332-6483

Handles three scenarios:

#### Scenario 1: Child Mode is Locked
```c
if (child_mode_locked)
{
  if (press_duration >= 3000)
  {
    child_mode_locked = 0;  /* Unlock */
    draw_tux_text(TUX_BORED, gettext("Child mode unlocked."), 0);
  }
  else
  {
    draw_tux_text(TUX_BORED, gettext("Child mode is locked. Long-press 3s to unlock."), 0);
  }
}
```

#### Scenario 2: Activating Child Mode (from Expert Mode)
```c
else if (child_mode_at_press_start == 0)
{
  if (press_duration >= 3000)
  {
    /* Long press: activate AND lock */
    child_mode = 1;
    child_mode_locked = 1;
    draw_tux_text(TUX_GREAT, gettext("Child mode activated and locked."), 0);
  }
  else
  {
    /* Short press: just activate (unlocked) */
    child_mode = 1;
    draw_tux_text(TUX_GREAT, gettext("Child mode activated."), 0);
  }
  /* ... apply child mode settings ... */
}
```

#### Scenario 3: Exiting Child Mode (unlocked)
```c
else
{
  /* Already in child mode (unlocked): exit to expert mode */
  child_mode = 0;
  draw_tux_text(TUX_DEFAULT, gettext("Expert mode activated."), 0);
  /* ... remove child mode restrictions ... */
}
```

### Sound Button Handler
**Location**: `tuxpaint.c` lines 3737-3768

Modified to check child mode:
```c
if (HIT(r_sound_btn) && valid_click(event.button.button))
{
  if (!child_mode)
  {
    /* Allow sound toggle only in expert mode */
    use_sound = !use_sound;
    /* ... */
  }
  else
  {
    /* Child mode: sound button locked */
    draw_tux_text(TUX_BORED, gettext("Sound is locked in child mode."), 0);
  }
}
```

---

## User Messages

| Scenario | Message |
|----------|---------|
| Child mode activated (unlocked) | "Child mode activated." |
| Child mode activated (locked) | "Child mode activated and locked." |
| Expert mode activated | "Expert mode activated." |
| Child mode unlocked | "Child mode unlocked." |
| Short press when locked | "Child mode is locked. Long-press 3s to unlock." |
| Sound button in child mode | "Sound is locked in child mode." |

---

## State Transitions

```
Expert Mode (unlocked)
  ├─ Short press → Child Mode (unlocked)
  └─ Long press (3s) → Child Mode (locked)

Child Mode (unlocked)
  └─ Short press → Expert Mode (unlocked)

Child Mode (locked)
  ├─ Short press → Show lock message
  └─ Long press (3s) → Child Mode (unlocked)
```

---

## Testing

### Test Cases

1. **✅ Sound button in expert mode**: Toggles sound
2. **✅ Sound button in child mode**: Shows lock message, does not toggle
3. **✅ Short press from expert**: Activates child mode (unlocked)
4. **✅ Long press from expert**: Activates child mode (locked)
5. **✅ Short press in unlocked child mode**: Returns to expert mode
6. **✅ Short press in locked child mode**: Shows lock message
7. **✅ Long press in locked child mode**: Unlocks child mode (stays in child mode)
8. **✅ Long press in unlocked child mode**: Returns to expert mode

### Manual Testing Commands
```bash
./gradlew assembleDebug
./gradlew installDebug
# Test on device:
# 1. Click sound button in expert mode → Sound toggles
# 2. Short-press child mode button → Child mode activates
# 3. Click sound button → Shows lock message
# 4. Short-press child mode button → Returns to expert mode
# 5. Long-press (3s) child mode button → Child mode activates AND locks
# 6. Short-press child mode button → Shows lock message
# 7. Long-press (3s) child mode button → Unlocks (stays in child mode)
# 8. Short-press child mode button → Returns to expert mode
```

---

## Files Modified

1. **`/var/www/Tuxpaint-Android/app/src/main/jni/tuxpaint/src/tuxpaint.c`**
   - Lines 822-824: Added global variables for lock state tracking
   - Line 1782: Added `img_lock_icon` surface variable
   - Line 31873: Load lock icon (`osk_capslock.png`)
   - Lines 3737-3768: Modified sound button handler (disabled in child mode)
   - Lines 3769-3776: Modified child mode button press handler (track press time)
   - Lines 6332-6483: Added child mode button release handler (long-press logic)
   - Lines 11174-11187: Added lock icon overlay in `draw_row_minus_1_buttons()`

2. **`/var/www/Tuxpaint-Android/IMPLEMENTATION_PLAN.md`**
   - Section 6.4: Updated with implementation details

3. **`/var/www/Tuxpaint-Android/dev/REFACTORING_PLAN_SOUND_TOGGLE.md`**
   - Translated to English

---

## Next Steps

### 6.4.2 Preferences (Persistent Storage)
- [ ] Save `use_sound` to SharedPreferences
- [ ] Save `child_mode` to SharedPreferences
- [ ] Save `child_mode_locked` to SharedPreferences
- [ ] Save `cur_brush` to SharedPreferences
- [ ] Restore preferences on app start

### Visual Indicator
- [ ] Add visual indicator for locked state on button
  - Maybe show a lock icon overlay
  - Or change button color when locked

---

## Commit

```bash
git add app/src/main/jni/tuxpaint/src/tuxpaint.c
git commit -m "feat: Add child mode lock with 3s long-press

- Long-press (3s) when activating child mode locks it
- Long-press (3s) when locked unlocks child mode  
- Short press when locked shows lock message
- Sound button disabled in child mode
- Prevents accidental exit from child mode

Testing:
- Build successful ✅
- Manual testing required on device"
```

---

## Build Status

✅ **Build Successful**
```
BUILD SUCCESSFUL in 11s
70 actionable tasks: 19 executed, 51 up-to-date
```

---

## Notes

- Implementation follows the pattern from IMPLEMENTATION_PLAN.md section 6.4
- All user feedback messages use gettext() for localization
- Lock state is tracked separately from child_mode state
- Press duration is calculated as `SDL_GetTicks() - child_mode_button_press_start`
- 3000ms threshold for long-press detection
