# Multitouch Gap Fill Fix

## Problem
Secondary fingers in multitouch drawing produced "dotted lines" because stroke segments were not connected when there were gaps between consecutive touch events.

## Solution
Implemented a gap-filling mechanism that:
1. Stores the last end position for each finger (up to MAX_FINGERS=10)
2. When a new stroke segment starts, checks if there's a gap from the previous end position
3. If a gap exists and is within a reasonable range (2-50 pixels), draws a connecting line using `brush_draw()`
4. Saves the current end position for the next iteration

## Implementation Details

### Data Structure
```c
typedef struct {
  int valid;          // 1 if we have a previous end position
  int x, y;           // Last end position
  long pointer_id;    // To track if it's the same finger
} FingerEndPos;
static FingerEndPos finger_end_pos[MAX_FINGERS] = {{0}};
```

### Gap Detection
- Gap distance is calculated as: `gap_dist_sq = (dx*dx) + (dy*dy)`
- Gap fill is triggered when: `4 < gap_dist_sq < 2500` (i.e., 2-50 pixels)
- This range prevents over-correction for tiny movements and avoids connecting distant strokes

### Key Changes
- **File**: `/var/www/Tuxpaint-Android/app/src/main/jni/tuxpaint/src/tuxpaint.c`
- **Location**: Inside the `if (cur_tool == TOOL_BRUSH && button_down && has_multitouch)` block
- **Lines**: ~6784-6870

## Testing
The fix was validated through:
1. Manual testing with 2 fingers drawing simultaneously
2. Observing that both fingers produce continuous lines without dots
3. Verified through Android logcat that both fingers trigger draw and gap-fill operations

## Result
✅ Both Finger 0 and Finger 1 (and all additional fingers up to MAX_FINGERS) now draw continuous lines without gaps.
