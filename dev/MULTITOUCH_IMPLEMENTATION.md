# Multitouch Support Implementation

## Current Situation

**Problem:** Only one finger can paint at a time.

**Root Cause:**
- Line 29800 in `tuxpaint.c`: `SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1")`
  - This converts ALL touch events to mouse events
  - Multiple fingers are treated as a single mouse pointer
- Global `button_down` flag (line 1399) - only tracks one state
- Single `old_x, old_y` position tracking - only one cursor position

## Solution Overview

Enable SDL2's native multitouch events and track multiple simultaneous finger positions.

## Implementation Steps

### 1. Enable Native Touch Events

In `tuxpaint.c` around line 29800, change:
```c
// BEFORE:
SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");  // Converts touch to mouse

// AFTER:
#ifdef ENABLE_MULTITOUCH
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");  // Keep touch as finger events
  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");  // Don't convert mouse to touch
#else
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");  // Legacy mode
#endif
```

### 2. Add Finger Tracking Structure

Add near line 1399 (where `button_down` is declared):
```c
#ifdef ENABLE_MULTITOUCH
#define MAX_FINGERS 10

typedef struct {
  SDL_FingerID finger_id;
  int active;
  int x;
  int y;
  int last_x;
  int last_y;
} FingerState;

static FingerState active_fingers[MAX_FINGERS];
static int num_active_fingers = 0;
#endif
```

### 3. Add Finger Event Handlers

Add new functions to handle finger events:
```c
#ifdef ENABLE_MULTITOUCH

static void init_multitouch(void) {
  for (int i = 0; i < MAX_FINGERS; i++) {
    active_fingers[i].active = 0;
  }
  num_active_fingers = 0;
}

static int find_finger_slot(SDL_FingerID finger_id) {
  for (int i = 0; i < MAX_FINGERS; i++) {
    if (active_fingers[i].active && active_fingers[i].finger_id == finger_id) {
      return i;
    }
  }
  return -1;
}

static int allocate_finger_slot(SDL_FingerID finger_id) {
  for (int i = 0; i < MAX_FINGERS; i++) {
    if (!active_fingers[i].active) {
      active_fingers[i].active = 1;
      active_fingers[i].finger_id = finger_id;
      num_active_fingers++;
      return i;
    }
  }
  return -1;
}

static void release_finger_slot(int slot) {
  if (slot >= 0 && slot < MAX_FINGERS) {
    active_fingers[slot].active = 0;
    num_active_fingers--;
  }
}

#endif
```

### 4. Handle Finger Events in Main Loop

In the main event loop (around line 2700), add after the existing event handling:
```c
#ifdef ENABLE_MULTITOUCH
      else if (event.type == SDL_FINGERDOWN) {
        // Only handle finger down on canvas when using brush tool
        if (cur_tool == TOOL_BRUSH) {
          int slot = allocate_finger_slot(event.tfinger.fingerId);
          if (slot >= 0) {
            // Convert normalized coordinates (0.0-1.0) to canvas pixels
            active_fingers[slot].x = (int)(event.tfinger.x * WINDOW_WIDTH) - r_canvas.x;
            active_fingers[slot].y = (int)(event.tfinger.y * WINDOW_HEIGHT) - r_canvas.y;
            active_fingers[slot].last_x = active_fingers[slot].x;
            active_fingers[slot].last_y = active_fingers[slot].y;
            
            // Play sound for first finger only (avoid sound chaos)
            if (num_active_fingers == 1) {
              playsound(screen, 1, paintsound(img_cur_brush_w), 1, 
                       active_fingers[slot].x + r_canvas.x, SNDDIST_NEAR);
            }
          }
        }
      }
      else if (event.type == SDL_FINGERUP) {
        int slot = find_finger_slot(event.tfinger.fingerId);
        if (slot >= 0) {
          release_finger_slot(slot);
        }
      }
      else if (event.type == SDL_FINGERMOTION) {
        int slot = find_finger_slot(event.tfinger.fingerId);
        if (slot >= 0 && cur_tool == TOOL_BRUSH) {
          // Update position
          int new_x = (int)(event.tfinger.x * WINDOW_WIDTH) - r_canvas.x;
          int new_y = (int)(event.tfinger.y * WINDOW_HEIGHT) - r_canvas.y;
          
          // Draw brush stroke for this finger
          brush_draw(active_fingers[slot].last_x, active_fingers[slot].last_y,
                    new_x, new_y, 1);
          
          // Update last position
          active_fingers[slot].last_x = new_x;
          active_fingers[slot].last_y = new_y;
        }
      }
#endif
```

### 5. Initialize on Startup

In the initialization section (around line 2665), add:
```c
#ifdef ENABLE_MULTITOUCH
  init_multitouch();
#endif
```

### 6. Build Configuration

Add to `Application.mk`:
```makefile
# Enable multitouch support for painting
APP_CFLAGS += -DENABLE_MULTITOUCH
```

Or make it configurable in `build.gradle`:
```gradle
android {
    defaultConfig {
        externalNativeBuild {
            ndkBuild {
                // Add this for multitouch support:
                cFlags "-DENABLE_MULTITOUCH"
            }
        }
    }
}
```

## Testing

1. Build with multitouch enabled
2. Use 2-3 fingers simultaneously on canvas
3. Each finger should draw independently
4. Verify no crashes when fingers are added/removed rapidly

## Limitations

**Only works with TOOL_BRUSH:**
- Other tools (stamps, shapes, lines) use different interaction patterns
- Would need separate multitouch handling for each tool type

**Sound:**
- Only first finger triggers sound to avoid audio chaos
- Alternative: Mix sounds or limit sound rate

**UI Interactions:**
- Multitouch only active when painting on canvas
- UI buttons still use single-touch (mouse event) handling

## Future Enhancements

- Support multitouch for eraser tool
- Pinch-to-zoom on canvas
- Two-finger rotate for stamps
- Gesture recognition (swipe to undo, etc.)
