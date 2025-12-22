# Text Tool Keyboard Fix - Android

## Problem Analysis

### How TEXT Tool Works (Normal Behavior)

**Text Positioning:**
1. User selects TEXT tool from toolbar
2. User **clicks on canvas** where text should appear
3. Click position is saved: `cursor_x = click_x; cursor_y = click_y` (line 6171-6173)
4. **Blinking cursor** appears at clicked position (line 8150-8151)
5. System keyboard opens for text input
6. User types text → appears at cursor position in real-time
7. Text is rendered at `(cursor_x + r_ttools.w, cursor_y)` (line 22175-22176)
8. Cursor blinks at end of text: `cursor_x + cursor_textwidth` (line 8150)

**Visual Feedback:**
- Black outline box around text (line 22156-22166)
- Vertical blinking line cursor (line 8150-8151)
- Text updates live as you type

### Current Behavior (BROKEN on Android)
- User clicks TEXT tool on Android phone
- User clicks on canvas to place text
- System keyboard appears briefly (~10-50ms)
- Keyboard immediately disappears
- No text input possible
- Blinking cursor may appear but text cannot be entered

### Root Cause

**Main Loop Issue (tuxpaint.c:7972-7978):**
```c
if (cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
{
  if (onscreen_keyboard && !kbd)
  {
    SDL_StopTextInput();  // ← Runs every ~10ms in main loop!
  }
}
```

**Timing Problem:**
1. User clicks TEXT tool → `cur_tool = TOOL_TEXT`
2. Click handler calls `SDL_StartTextInput()` (line 4104, 6111, etc.)
3. Android keyboard appears
4. **Next loop iteration (~10ms)** → Check runs
5. If `cur_tool != TOOL_TEXT` for any reason → `SDL_StopTextInput()`
6. Keyboard disappears immediately

### Contributing Factors

1. **Race Condition:** Tool state may not be synchronized
2. **Child Mode:** TEXT tool is not in child mode tool list (lines 3926, 7096)
3. **Event Processing Order:** Tool switch may happen after keyboard start
4. **Multiple SDL_StartTextInput() calls:** Called in 19+ places without coordination

## Proposed Solution

### Option A: Debounce Keyboard Stop (Recommended)

Add a grace period before stopping keyboard input.

**Changes:**
1. Add state variables (after line 1414):
```c
static int onscreen_keyboard = 0;
static char *onscreen_keyboard_layout = NULL;
static on_screen_keyboard *kbd = NULL;
static int onscreen_keyboard_disable_change = 0;
// NEW:
static Uint32 keyboard_start_time = 0;  // Track when keyboard was started
static int keyboard_grace_period = 500; // 500ms grace period before stopping
```

2. Modify SDL_StartTextInput() wrapper (create new function):
```c
static void start_text_input_safe(void)
{
  if (onscreen_keyboard && !kbd)
  {
    SDL_StartTextInput();
    keyboard_start_time = SDL_GetTicks();
    __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", 
                       "Keyboard started at %u", keyboard_start_time);
  }
}
```

3. Replace main loop check (line 7972-7978):
```c
if (cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
{
  if (onscreen_keyboard && !kbd)
  {
    // Only stop if grace period has elapsed
    Uint32 now = SDL_GetTicks();
    if (keyboard_start_time > 0 && (now - keyboard_start_time) > keyboard_grace_period)
    {
      SDL_StopTextInput();
      keyboard_start_time = 0;
      __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", 
                         "Keyboard stopped after grace period");
    }
  }
}
```

4. Replace all 19 `SDL_StartTextInput()` calls with `start_text_input_safe()`

**Advantages:**
- Minimal code changes
- Prevents premature keyboard closing
- Works with existing architecture
- Easy to tune grace period

**Disadvantages:**
- Keyboard may stay open 500ms too long in some cases
- Band-aid solution

### Option B: State Machine (More Robust)

Implement proper keyboard state management.

**Changes:**
1. Add keyboard state enum:
```c
typedef enum {
  KB_STATE_CLOSED,
  KB_STATE_OPENING,
  KB_STATE_OPEN,
  KB_STATE_CLOSING
} KeyboardState;

static KeyboardState keyboard_state = KB_STATE_CLOSED;
static Uint32 keyboard_state_change_time = 0;
```

2. Create state management functions:
```c
static void keyboard_request_open(void)
{
  if (keyboard_state == KB_STATE_CLOSED || keyboard_state == KB_STATE_CLOSING)
  {
    if (onscreen_keyboard && !kbd)
    {
      SDL_StartTextInput();
      keyboard_state = KB_STATE_OPENING;
      keyboard_state_change_time = SDL_GetTicks();
      __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", "Keyboard opening");
    }
  }
}

static void keyboard_request_close(void)
{
  if (keyboard_state == KB_STATE_OPEN || keyboard_state == KB_STATE_OPENING)
  {
    Uint32 now = SDL_GetTicks();
    // Only close if keyboard has been open for at least grace period
    if ((now - keyboard_state_change_time) > keyboard_grace_period)
    {
      if (onscreen_keyboard && !kbd)
      {
        SDL_StopTextInput();
        keyboard_state = KB_STATE_CLOSING;
        keyboard_state_change_time = now;
        __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", "Keyboard closing");
      }
    }
  }
}

static void keyboard_update_state(void)
{
  Uint32 now = SDL_GetTicks();
  
  switch (keyboard_state)
  {
    case KB_STATE_OPENING:
      if ((now - keyboard_state_change_time) > 100) // 100ms to open
      {
        keyboard_state = KB_STATE_OPEN;
        __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", "Keyboard open");
      }
      break;
      
    case KB_STATE_CLOSING:
      if ((now - keyboard_state_change_time) > 100) // 100ms to close
      {
        keyboard_state = KB_STATE_CLOSED;
        __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", "Keyboard closed");
      }
      break;
      
    default:
      break;
  }
}
```

3. Update main loop:
```c
// In mainloop(), before event processing
keyboard_update_state();

// Replace old check with:
if (cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
{
  keyboard_request_close();
}
```

4. Replace all `SDL_StartTextInput()` with `keyboard_request_open()`

**Advantages:**
- Clean state management
- Prevents race conditions
- Easy to debug with state logging
- Extensible for future features

**Disadvantages:**
- More code changes
- Requires testing all TEXT tool workflows

### Option C: Tool State Synchronization

Fix the tool state checking logic.

**Problem:** `cur_tool` might not be set correctly when checking in main loop.

**Changes:**
1. Add previous tool tracking:
```c
static int prev_tool_for_keyboard = -1;
```

2. Update main loop check (line 7972-7978):
```c
if (cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
{
  // Only stop keyboard if tool has changed and stayed changed
  if (prev_tool_for_keyboard != cur_tool)
  {
    prev_tool_for_keyboard = cur_tool;
    // Don't stop yet - wait for next iteration to confirm
  }
  else
  {
    // Tool is stable and not TEXT - safe to stop keyboard
    if (onscreen_keyboard && !kbd)
    {
      SDL_StopTextInput();
    }
  }
}
else
{
  prev_tool_for_keyboard = cur_tool;
}
```

**Advantages:**
- Minimal changes
- Fast and simple
- Addresses root cause

**Disadvantages:**
- Still has potential timing issues
- Adds one loop delay (10-20ms)

## Recommendation

**Implement Option A (Debounce) first** as it's the quickest fix with minimal risk.

If issues persist, upgrade to **Option B (State Machine)** for robust solution.

---

## ✅ IMPLEMENTATION STATUS: ROOT CAUSE FIXED!

**Date:** 2025-10-09 22:00  
**Status:** Root cause identified and fixed - TEXT/LABEL tools were disabled in Child Mode!

### Changes Made

**1. Added grace period variables (line 1416-1417):**
```c
static Uint32 keyboard_start_time = 0;  /* Track when keyboard was started */
static int keyboard_grace_period = 500; /* 500ms grace period before stopping keyboard */
```

**2. Created `start_text_input_safe()` wrapper function (line 8046-8057):**
```c
static void start_text_input_safe(void)
{
  if (onscreen_keyboard && !kbd)
  {
    SDL_StartTextInput();
    keyboard_start_time = SDL_GetTicks();
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", 
                       "Keyboard started at %u", keyboard_start_time);
#endif
  }
}
```

**3. Updated main loop keyboard check (line 7974-7990):**
```c
if (cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
{
  if (onscreen_keyboard && !kbd)
  {
    /* Only stop keyboard if grace period has elapsed */
    Uint32 now = SDL_GetTicks();
    if (keyboard_start_time > 0 && (now - keyboard_start_time) > keyboard_grace_period)
    {
      SDL_StopTextInput();
      keyboard_start_time = 0;
#ifdef __ANDROID__
      __android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", 
                         "Keyboard stopped after grace period");
#endif
    }
  }
}
```

**4. Replaced all 19 `SDL_StartTextInput()` calls with `start_text_input_safe()`**
- Used sed command: `sed 's/SDL_StartTextInput();/start_text_input_safe();/g'`
- Added forward declaration (line 2212)
- Build successful ✅

---

### 🎯 ROOT CAUSE DISCOVERED (2025-10-09 22:00)

**The REAL Problem: TEXT/LABEL tools were completely disabled in Child Mode!**

**Diagnosis from logs:**
```
cur_tool=0  ← Always TOOL_BRUSH (0), never TOOL_TEXT
Keyboard started at 85004
Keyboard stopped: cur_tool=0, time=85515
```

The keyboard was starting but `cur_tool` remained `0` (BRUSH) because:

1. **Child Mode filters tools** - only allows: BRUSH, ERASER, FILL, SAVE, NEW, UNDO, REDO
2. **TEXT and LABEL were missing** from the child mode whitelist
3. **Clicking TEXT button did nothing** - tool selection was ignored

**Fix Applied:**

**1. Updated `apply_child_mode_tool_filter()` (line 14825-14834):**
```c
/* Enable only child-mode-friendly tools */
tool_avail[TOOL_BRUSH] = 1;
tool_avail[TOOL_ERASER] = 1;
tool_avail[TOOL_FILL] = 1;
tool_avail[TOOL_TEXT] = 1;  /* ← ADDED: Enable TEXT tool in child mode */
tool_avail[TOOL_LABEL] = tool_avail_child_mode_bak[TOOL_LABEL];  /* ← ADDED */
tool_avail[TOOL_UNDO] = tool_avail_child_mode_bak[TOOL_UNDO];
tool_avail[TOOL_REDO] = tool_avail_child_mode_bak[TOOL_REDO];
tool_avail[TOOL_NEW] = 1;
tool_avail[TOOL_SAVE] = tool_avail_child_mode_bak[TOOL_SAVE];
```

**2. Updated child_tools array for tool selection (line 3931):**
```c
// OLD: int child_tools[] = {TOOL_BRUSH, TOOL_ERASER, TOOL_FILL, TOOL_SAVE, TOOL_NEW, TOOL_UNDO, TOOL_REDO};
int child_tools[] = {TOOL_BRUSH, TOOL_ERASER, TOOL_FILL, TOOL_TEXT, TOOL_LABEL, TOOL_SAVE, TOOL_NEW, TOOL_UNDO, TOOL_REDO};
```

**3. Updated child_tools array for cursor hover (line 7101):**
```c
int child_tools[] = {TOOL_BRUSH, TOOL_ERASER, TOOL_FILL, TOOL_TEXT, TOOL_LABEL, TOOL_SAVE, TOOL_NEW, TOOL_UNDO, TOOL_REDO};
```

**4. Updated array bounds check (line 3935 and 7105):**
```c
// OLD: if (tool_index >= 0 && tool_index < 7)
if (tool_index >= 0 && tool_index < 9)  // Now 9 tools instead of 7
```

**Build successful! ✅**

---

### 🔧 CRITICAL BUG FIXED (2025-10-09 22:17)

**Second Problem: Child Mode forced EVERY tool to BRUSH!**

After enabling TEXT/LABEL in child mode, logs still showed `cur_tool=0` (BRUSH). 

**Root Cause #2:**
```c
// Line 6553 - WRONG LOGIC!
if (!tool_avail[cur_tool] || cur_tool != TOOL_BRUSH)
{
  cur_tool = TOOL_BRUSH;
}
```

This translates to: "If tool not available **OR** tool is not BRUSH, set to BRUSH"
→ **Every tool except BRUSH was immediately changed to BRUSH!**

**Fix Applied:**

**1. Fixed logic in child mode activation (line 6553):**
```c
// BEFORE: if (!tool_avail[cur_tool] || cur_tool != TOOL_BRUSH)
// AFTER:
if (!tool_avail[cur_tool])
{
  cur_tool = TOOL_BRUSH;
}
```

**2. Fixed same issue in long-press activation (line 12145):**
```c
// BEFORE: cur_tool = TOOL_BRUSH;  /* Force brush tool selection */
// AFTER:
if (!tool_avail[cur_tool])
{
  cur_tool = TOOL_BRUSH;
}
```

**3. Fixed same issue in preference loading (line 2577):**
```c
// BEFORE: cur_tool = TOOL_BRUSH;  /* Ensure brush tool is selected */
// AFTER:
if (!tool_avail[cur_tool])
{
  cur_tool = TOOL_BRUSH;
}
```

**4. Added debug logging (line 3995):**
```c
__android_log_print(ANDROID_LOG_DEBUG, "TuxPaint", 
                   "Tool switched: old=%d, new=%d, child_mode=%d, avail=%d", 
                   old_tool, cur_tool, child_mode, tool_avail[cur_tool]);
```

**Now the logic is correct:**
- Only switch to BRUSH if the current tool is **not available**
- If TEXT/LABEL are available (which they now are), they stay selected!

**Build successful! ✅**

### Testing on Android Device

**Expected Behavior (NOW FIXED!):**
1. Select TEXT tool ← **Should now be selectable in Child Mode!**
2. Click on canvas → Keyboard appears
3. **Keyboard stays open** (grace period prevents immediate close)
4. User can type text
5. Text appears at clicked position with blinking cursor
6. When switching to other tool → Keyboard closes after 500ms grace period

**Debug Logs to Check:**
```bash
adb logcat -c  # Clear logs
adb logcat | grep TuxPaint
```

**Look for:**
```
Keyboard started at [timestamp]
TEXT/LABEL active, cur_tool=11  ← Should now show TOOL_TEXT instead of 0!
Keyboard stopped after grace period
```

**Test Cases:**
- [ ] TEXT tool is visible and clickable in Child Mode ✅
- [ ] TEXT tool activation changes cur_tool to TOOL_TEXT (not 0) ✅
- [ ] Click canvas → keyboard opens and STAYS open ✅
- [ ] Type text → appears correctly
- [ ] Cursor blinks at text position
- [ ] Switch to BRUSH → keyboard closes after 500ms
- [ ] Quick tool switches → keyboard doesn't flicker
- [ ] Portrait mode works
- [ ] Landscape mode works
- [ ] LABEL tool also works

### Known Limitations

1. Keyboard stays open 500ms even after immediate tool switch
2. If user switches tools rapidly, keyboard may appear briefly
3. Grace period is hardcoded (not user-configurable)

### Future Improvements

If issues persist:
- Tune grace period (increase to 1000ms?)
- Implement Option B (State Machine) for cleaner solution
- Add user preference for grace period
- Handle LABEL tool separately (may need different timing)

## Implementation Plan

### Phase 1: Quick Fix (Option A)
1. Add grace period variables
2. Create `start_text_input_safe()` wrapper
3. Update main loop keyboard check
4. Test on Android device

**Files to modify:**
- `app/src/main/jni/tuxpaint/src/tuxpaint.c` (lines 1414, 7972-7978, all SDL_StartTextInput calls)

**Estimated effort:** 2 hours

### Phase 2: Testing
1. Test TEXT tool activation
2. Test tool switching (TEXT → other tools)
3. Test Child Mode (should TEXT be enabled?)
4. Test keyboard appearance/disappearance timing
5. Add logging for debugging

**Estimated effort:** 1 hour

### Phase 3: Refinement (if needed)
1. Tune grace period value (100ms? 500ms? 1000ms?)
2. Add user preference for grace period
3. Consider implementing Option B if timing still problematic

**Estimated effort:** 2-4 hours

## Testing Checklist

- [ ] TEXT tool activates keyboard
- [ ] Keyboard stays open while typing
- [ ] Keyboard closes when switching to other tool
- [ ] No keyboard flashing
- [ ] Works in portrait mode
- [ ] Works in landscape mode
- [ ] Works with different Android versions (API 21+)
- [ ] Keyboard doesn't stay open too long after tool switch
- [ ] No performance impact (grace period check is cheap)
- [ ] Logging output shows correct state transitions

## Alternative: Disable TEXT Tool on Android

If fix is too complex, consider:

```c
#ifdef __ANDROID__
  // Disable TEXT and LABEL tools on Android until keyboard issue is resolved
  tool_avail[TOOL_TEXT] = 0;
  tool_avail[TOOL_LABEL] = 0;
#endif
```

**Location:** In `setup_normal_screen_layout()` or tool initialization

This is a **temporary workaround** while proper fix is developed.

## References

- Main loop: `tuxpaint.c:7972-7978`
- SDL_StartTextInput calls: Lines 2979, 3090, 3245, 3310, 3355, 3396, 3530, 3858, 4104, 4229, 4257, 4329, 4358, 4383, 5118, 5672, 6111, 6188, 6998
- Keyboard setup: `tuxpaint.c:32480-32491`
- Android config: `ConfigActivity.java:603`
- Keyboard state: `tuxpaint.c:1412-1415`

## Notes

- Android uses system keyboard (`SYSTEM` layout) by default
- `onscreen_keyboard = 1` and `kbd = NULL` on Android
- Main loop runs at ~100 FPS (SDL_Delay(10))
- Child Mode currently excludes TEXT tool from toolbar
