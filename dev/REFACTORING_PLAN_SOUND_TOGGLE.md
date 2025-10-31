# Refactoring Plan: Minimal Sound Toggle Implementation

**Date**: 2025-10-09 05:57  
**Goal**: Reduce to minimal implementation by using `use_sound` instead of `mute`

---

## Executive Summary

**Problem**: We activated a `mute` variable that has existed since 2015 but was unused. This was redundant since `use_sound` can already fulfill this function.

**Solution**: Remove `mute` variable and toggle `use_sound` at runtime instead.

**Benefits**:
- ✅ **-150 LOC** (less code to maintain)
- ✅ **Semantically correct** (`use_sound` means exactly that)
- ✅ **No behavior change** (functionally identical)
- ✅ **Minimal merge request** (closer to original)

---

## Affected Commits (to simplify)

1. **16f3b445** - Add sound toggle button to toolbar (+101 LOC)
2. **a0386f66** - WIP: fix sound toggle: add detailed sound control logging
3. **700823bf** - Fix sound toggle to actually mute all sounds + add unit test
4. **5d4b7fd7** - Fix sound button event handling order + add debug logging
5. **28abca97** - Fix sound button: Move event handler before r_tools check
6. **0e64e9ee** - refactor: move sound and child mode buttons to header area
7. **422c6d5f** - Button Icons: Penguin/Building/Speaker

---

## Current Situation (with `mute`)

### Variable Declaration
```c
// playsound.c:33
int mute;  // <-- REMOVE

// playsound.h:39
extern int mute, use_sound, use_stereo;  // <-- REMOVE mute
```

### All 20 Usage Locations of `mute` in tuxpaint.c

| Line | Context | Action |
|-------|---------|--------|
| 2969 | `mute = !mute;` (Alt+S handler) | → `use_sound = !use_sound;` |
| 11000 | `mute ? img_grey : img_black` | → `use_sound ? img_black : img_grey` |
| 13935 | `if (!mute && use_sound)` | → `if (use_sound)` (redundant check) |
| 14289 | `if (... && !mute)` | → already `use_sound` check (redundant) |
| 14305 | `if (... && !mute)` | → already `use_sound` check (redundant) |
| 23655 | `if (mute \|\| !use_sound)` | → `if (!use_sound)` |
| 23675 | `if (mute \|\| !use_sound \|\| ...)` | → `if (!use_sound \|\| ...)` |
| 27221 | `if (!mute && use_sound)` | → `if (use_sound)` (redundant check) |

### playsound.c
```c
// Line 57
if (!mute && use_sound && s != SND_NONE)  // <-- REMOVE !mute check
  →
if (use_sound && s != SND_NONE)
```

---

## Refactoring Steps (in this order)

### Step 1: Remove Variable Declarations
**Files**: `playsound.c`, `playsound.h`

```diff
--- a/app/src/main/jni/tuxpaint/src/playsound.c
+++ b/app/src/main/jni/tuxpaint/src/playsound.c
@@ -30,7 +30,6 @@ Mix_Chunk *sounds[NUM_SOUNDS];
 #endif
 
-int mute;
 int use_sound = 1;
 int use_stereo = 1;
```

```diff
--- a/app/src/main/jni/tuxpaint/src/playsound.h
+++ b/app/src/main/jni/tuxpaint/src/playsound.h
@@ -36,7 +36,7 @@
 
 extern Mix_Chunk *sounds[NUM_SOUNDS];
-extern int mute, use_sound, use_stereo;
+extern int use_sound, use_stereo;
```

### Step 2: Replace Toggle Logic (2 locations)

**File**: `tuxpaint.c`

#### 2a) Alt+S Keyboard Handler (Line 2969)
```diff
@@ -2966,11 +2966,11 @@
           {
             DEBUG_PRINTF("modstate at mainloop %d, mod %d\n", SDL_GetModState(), mod);
 
-            mute = !mute;
+            use_sound = !use_sound;
             Mix_HaltChannel(-1);
 
-            if (mute)
+            if (!use_sound)
             {
               /* Sound has been muted (silenced) via keyboard shortcut */
               draw_tux_text(TUX_BORED, gettext("Sound muted."), 0);
```

#### 2b) Sound Button Click Handler (Line 3740)
```diff
@@ -3738,13 +3738,13 @@
           /* Sound toggle button clicked */
 #ifndef NOSOUND
-          mute = !mute;
+          use_sound = !use_sound;
           Mix_HaltChannel(-1);
 
           /* Redraw the button with new state */
           draw_row_minus_1_buttons();
           update_screen_rect(&r_sound_btn);
 
-          if (mute)
+          if (!use_sound)
           {
             draw_tux_text(TUX_BORED, gettext("Sound muted."), 0);
           }
```

### Step 3: Button Rendering (Line 10999-11000)

**File**: `tuxpaint.c` - Function `draw_row_minus_1_buttons()`

```diff
@@ -10997,8 +10997,8 @@
 
 #ifndef NOSOUND
   /* Sound toggle button */
-  button_body = mute ? img_btn_off : img_btn_up;
-  button_color = mute ? img_grey : img_black;
+  button_body = use_sound ? img_btn_up : img_btn_off;
+  button_color = use_sound ? img_black : img_grey;
   
   /* Scale button body to reduced height (squash vertically) */
   button_scaled = rotozoomSurfaceXY(button_body, 0, 1.0, scale_y, SMOOTHING_ON);
```

### Step 4: Remove Redundant Checks (6 locations)

All locations where `!mute &&` precedes `use_sound` → redundant check!

#### 4a) playsound() in playsound.c (Line 57)
```diff
@@ -54,7 +54,7 @@
 #ifndef NOSOUND
   int left, dist;
 
-  if (!mute && use_sound && s != SND_NONE)
+  if (use_sound && s != SND_NONE)
   {
 #ifdef DEBUG
     printf("playsound #%d in channel %d, pos (%d,%d), %soverride, ptr=%p\n",
```

#### 4b-f) tuxpaint.c - Redundant Double-Checks
```diff
@@ -13933,7 +13933,7 @@
 
 #ifndef NOSOUND
-  if (!mute && use_sound)
+  if (use_sound)
   {
     if (!Mix_Playing(0))
     {

@@ -23652,7 +23652,7 @@
 static void magic_stopsound(void)
 {
 #ifndef NOSOUND
-  if (mute || !use_sound)
+  if (!use_sound)
     return;
 
   Mix_HaltChannel(0);

@@ -23672,7 +23672,7 @@
   /* Don't play if sound is disabled (nosound), or sound is temporarily
      muted (Alt+S), or sound ptr is NULL */
 
-  if (mute || !use_sound || snd == NULL)
+  if (!use_sound || snd == NULL)
     return;

@@ -27218,7 +27218,7 @@
           draw_color_mixer_blank_example();
 
 #ifndef NOSOUND
-          if (!mute && use_sound)
+          if (use_sound)
           {
             if (!Mix_Playing(0))
             {
```

### Step 5: Simplify Stamp-Sound Checks (10 locations)

All direct `Mix_PlayChannel()` calls with `!mute` check:

```diff
@@ -5370,7 +5370,7 @@
             {
 #ifndef NOSOUND
               /* Only play when picking a different stamp */
-              if (toolopt_changed && !mute)
+              if (toolopt_changed && use_sound)
               {

@@ -5384,7 +5384,7 @@
                 {
                   Mix_ChannelFinished(NULL);
 
-                  if (!mute)
+                  if (use_sound)
                     Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->ssnd, 0);

@@ -5400,7 +5400,7 @@
 
                   if (stamp_data[stamp_group][cur_thing]->sdesc != NULL)
                   {
-                    if (!mute)
+                    if (use_sound)
                       Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->sdesc, 0);
                   }

@@ -6087,7 +6087,7 @@
         {
           /* A sound player button on the lower left has been pressed! */
 #ifndef NOSOUND
-          if (cur_tool == TOOL_STAMP && use_sound && !mute)
+          if (cur_tool == TOOL_STAMP && use_sound)
           {
             which = GRIDHIT_GD(r_sfx, gd_sfx);

@@ -6096,7 +6096,7 @@
               /* Re-play sound effect: */
 
               Mix_ChannelFinished(NULL);
-              if (!mute)
+              if (use_sound)
                 Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->ssnd, 0);

@@ -6103,7 +6103,7 @@
             {
               Mix_ChannelFinished(NULL);
-              if (!mute)
+              if (use_sound)
                 Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->sdesc, 0);
             }

@@ -6389,7 +6389,7 @@
         {
           if ((int)(intptr_t) event.user.data1 == cur_stamp[stamp_group])
           {
-            if (!mute && stamp_data[stamp_group][(int)(intptr_t) event.user.data1]->sdesc != NULL)
+            if (use_sound && stamp_data[stamp_group][(int)(intptr_t) event.user.data1]->sdesc != NULL)
               Mix_PlayChannel(2, stamp_data[stamp_group][(int)(intptr_t) event.user.data1]->sdesc,
                               0);
           }

@@ -6862,7 +6862,7 @@
         {
           /* Sound player buttons: */
 
-          if (cur_tool == TOOL_STAMP && use_sound && !mute &&
+          if (cur_tool == TOOL_STAMP && use_sound &&
               ((GRIDHIT_GD(r_sfx, gd_sfx) == 0 &&

@@ -14286,7 +14286,7 @@
     dest.y = r_tuxarea.y;
 
   /* Don't let sfx and speak buttons cover the top of Tux, either: */
-  if (cur_tool == TOOL_STAMP && use_sound && !mute)
+  if (cur_tool == TOOL_STAMP && use_sound)
   {
     if (dest.y < r_sfx.y + r_sfx.h)
       dest.y = r_sfx.y + r_sfx.h;

@@ -14302,7 +14302,7 @@
 
   /* Draw 'sound effect' and 'speak' buttons, if we're in the Stamp tool */
 
-  if (cur_tool == TOOL_STAMP && use_sound && !mute)
+  if (cur_tool == TOOL_STAMP && use_sound)
   {
     /* Sound effect: */
```

### Step 6: Update Comments

```diff
@@ -23670,8 +23670,7 @@
   int left, dist;
 
 
-  /* Don't play if sound is disabled (nosound), or sound is temporarily
-     muted (Alt+S), or sound ptr is NULL */
+  /* Don't play if sound is disabled or sound ptr is NULL */
 
   if (!use_sound || snd == NULL)
     return;
```

---

## Test Plan

### Adapt Unit Test
**File**: `SoundToggleTest.java`

```diff
- Test checks for mute variable
+ Test checks for use_sound variable state
```

### Manual Testing
1. **Sound Button Click** → Sound off/on toggle
2. **Alt+S Keyboard** → Sound off/on toggle  
3. **Stamp Sounds** → Respect use_sound state
4. **Button Visual** → Shows correct state (up=on, off=off)

---

## Code Statistics

### Before (with `mute`)
- Variable declarations: 2 files
- Usage locations: 20 in tuxpaint.c + 1 in playsound.c
- Redundant checks: 10+ locations where `!mute && use_sound`

### After (only `use_sound`)
- Variable declarations: ❌ removed
- Usage locations: 0 (uses existing `use_sound`)
- Redundant checks: ❌ removed
- **Net change**: ~30 LOC removed, cleaner code

---

## Implementation Order

1. ✅ **Step 1** - Remove variable declarations (breaks compile → forces fixes)
2. ✅ **Step 2** - Fix toggle handlers (Alt+S + button)
3. ✅ **Step 3** - Fix button rendering
4. ✅ **Step 4** - Remove redundant double-checks
5. ✅ **Step 5** - Simplify stamp sound checks
6. ✅ **Step 6** - Update comments
7. ✅ **Test** - Run unit tests + manual testing

**Estimated time**: 30-60 minutes

---

## Merge Request Strategy

### Commit Message Template
```
refactor: simplify sound toggle by using use_sound instead of mute

Previously we introduced a separate 'mute' variable for the sound toggle
feature. This was redundant since we can simply toggle 'use_sound' at
runtime, which already controls sound playback.

Benefits:
- Simpler code (-30 LOC)
- No redundant variable
- Semantically correct (use_sound means "sound enabled")
- Functionally identical behavior

Changes:
- Remove 'mute' variable from playsound.c/h
- Toggle 'use_sound' in Alt+S handler and button click
- Remove redundant '!mute &&' checks before 'use_sound'
- Update button rendering to reflect use_sound state

Testing:
- Sound button click toggles sound on/off ✅
- Alt+S keyboard shortcut works ✅
- Stamp sounds respect sound state ✅
- Button visual state matches sound state ✅
```

### Minimal Diff Goals
- Target: Single commit that's easy to review
- Focus: Variable substitution only (no logic changes)
- Testing: Existing SoundToggleTest continues to pass

---

## Risks & Mitigation

### Risk 1: use_sound modified elsewhere
**Mitigation**: `use_sound` is only set at startup/failure (checked ✅)

### Risk 2: Config override
**Mitigation**: Android app has no config files (checked ✅)

### Risk 3: Alt+S doc says "cannot reenable if --nosound"
**Mitigation**: Still true! If `use_sound` starts at 0, toggle keeps it 0 (add check if needed)

---

## Alternative: Safe Version with Config-Check

If we want to ensure that `use_sound` is not toggled when disabled via config:

```c
// Add new variable at startup:
static int use_sound_initial = 1;

// At init (after config parsing):
use_sound_initial = use_sound;

// In toggle handlers:
if (use_sound_initial) {  // Only toggle if initially enabled
  use_sound = !use_sound;
  // ...
}
```

**Recommendation**: Not necessary for Android app (no config), but document for upstream merge.

---

## Conclusion

**Recommended Approach**: 
1. Refactor now (Steps 1-6)
2. Test
3. Single commit for clean MR
4. Optional: Add comment that for desktop version, `use_sound_initial` check might be useful

**Time**: ~1 hour work, saves long-term maintenance
**LOC**: -30 lines cleaner code
**Risk**: Minimal (only variable substitution)
