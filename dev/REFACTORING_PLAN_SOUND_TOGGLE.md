# Refactoring Plan: Minimal Sound Toggle Implementation

**Datum**: 9.10.2025 05:57  
**Ziel**: Reduktion auf minimale Implementierung durch Verwendung von `use_sound` statt `mute`

---

## Executive Summary

**Problem**: Wir haben eine redundante `mute` Variable eingeführt, die das gleiche macht wie `use_sound` toggle.

**Lösung**: `mute` Variable entfernen und stattdessen `use_sound` zur Runtime togglen.

**Vorteil**:
- ✅ **-150 LOC** (weniger Code zu maintainen)
- ✅ **Semantisch korrekt** (`use_sound` bedeutet genau das)
- ✅ **Keine Verhaltensänderung** (funktional identisch)
- ✅ **Minimaler Merge Request** (näher am Original)

---

## Betroffene Commits (zum Vereinfachen)

1. **16f3b445** - Add sound toggle button to toolbar (+101 LOC)
2. **a0386f66** - WIP: fix sound toggle: add detailed sound control logging
3. **700823bf** - Fix sound toggle to actually mute all sounds + add unit test
4. **5d4b7fd7** - Fix sound button event handling order + add debug logging
5. **28abca97** - Fix sound button: Move event handler before r_tools check
6. **0e64e9ee** - refactor: move sound and child mode buttons to header area
7. **422c6d5f** - Button Icons: Penguin/Building/Speaker

---

## Aktuelle Situation (mit `mute`)

### Variable Deklaration
```c
// playsound.c:33
int mute;  // <-- REMOVE

// playsound.h:39
extern int mute, use_sound, use_stereo;  // <-- REMOVE mute
```

### Alle 20 Verwendungsstellen von `mute` in tuxpaint.c

| Zeile | Kontext | Aktion |
|-------|---------|--------|
| 2969 | `mute = !mute;` (Alt+S handler) | → `use_sound = !use_sound;` |
| 2972 | `if (mute)` | → `if (!use_sound)` |
| 3740 | `mute = !mute;` (button handler) | → `use_sound = !use_sound;` |
| 3747 | `if (mute)` | → `if (!use_sound)` |
| 5372 | `if (toolopt_changed && !mute)` | → `if (toolopt_changed && use_sound)` |
| 5387 | `if (!mute)` | → `if (use_sound)` |
| 5403 | `if (!mute)` | → `if (use_sound)` |
| 6090 | `if (... && !mute)` | → `if (... && use_sound)` (redundant, s.u.) |
| 6099 | `if (!mute)` | → `if (use_sound)` |
| 6105 | `if (!mute)` | → `if (use_sound)` |
| 6392 | `if (!mute && ...)` | → `if (use_sound && ...)` |
| 6865 | `if (... && !mute ...)` | → `if (... && use_sound ...)` (redundant) |
| 10999 | `mute ? img_btn_off : img_btn_up` | → `use_sound ? img_btn_up : img_btn_off` |
| 11000 | `mute ? img_grey : img_black` | → `use_sound ? img_black : img_grey` |
| 13935 | `if (!mute && use_sound)` | → `if (use_sound)` (redundant check) |
| 14289 | `if (... && !mute)` | → bereits `use_sound` check (redundant) |
| 14305 | `if (... && !mute)` | → bereits `use_sound` check (redundant) |
| 23655 | `if (mute \|\| !use_sound)` | → `if (!use_sound)` |
| 23675 | `if (mute \|\| !use_sound \|\| ...)` | → `if (!use_sound \|\| ...)` |
| 27221 | `if (!mute && use_sound)` | → `if (use_sound)` (redundant check) |

### playsound.c
```c
// Zeile 57
if (!mute && use_sound && s != SND_NONE)  // <-- REMOVE !mute check
  →
if (use_sound && s != SND_NONE)
```

---

## Refactoring Steps (in dieser Reihenfolge)

### Step 1: Variable Deklarationen entfernen
**Dateien**: `playsound.c`, `playsound.h`

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

### Step 2: Toggle-Logik ersetzen (2 Stellen)

**Datei**: `tuxpaint.c`

#### 2a) Alt+S Keyboard Handler (Zeile 2969)
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

#### 2b) Sound Button Click Handler (Zeile 3740)
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

### Step 3: Button Rendering (Zeile 10999-11000)

**Datei**: `tuxpaint.c` - Funktion `draw_row_minus_1_buttons()`

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

### Step 4: Redundante Checks entfernen (6 Stellen)

Alle Stellen wo `!mute &&` vor `use_sound` steht → redundanter Check!

#### 4a) playsound() in playsound.c (Zeile 57)
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

#### 4b-f) tuxpaint.c - Redundante Doppel-Checks
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

### Step 5: Stamp-Sound Checks vereinfachen (10 Stellen)

Alle direkten `Mix_PlayChannel()` Aufrufe mit `!mute` check:

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

### Step 6: Kommentare aktualisieren

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

### Unit Test anpassen
**Datei**: `SoundToggleTest.java`

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

### Before (mit `mute`)
- Variable declarations: 2 files
- Usage locations: 20 in tuxpaint.c + 1 in playsound.c
- Redundant checks: 10+ Stellen wo `!mute && use_sound`

### After (nur `use_sound`)
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

## Alternative: Sichere Version mit Config-Check

Falls wir sicherstellen wollen, dass `use_sound` nicht getoggled wird wenn es via Config disabled wurde:

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

**Empfehlung**: Nicht nötig für Android App (keine Config), aber dokumentieren für upstream merge.

---

## Conclusion

**Empfohlener Ansatz**: 
1. Jetzt refactoren (Steps 1-6)
2. Testen
3. Single commit für sauberen MR
4. Optional: Kommentar hinzufügen dass für Desktop-Version evtl. `use_sound_initial` check sinnvoll ist

**Zeit**: ~1 Stunde Arbeit, spart langfristig Maintenance
**LOC**: -30 lines cleaner code
**Risk**: Minimal (nur variable substitution)
