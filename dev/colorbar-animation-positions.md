# Colorbar Animation - Simplified Full Redraw Approach

**Date**: 14.10.2025, 18:55  
**Purpose**: Simplified animation clearing using full area redraw in each frame  
**Functions**: `slide_colorbar_in()`, `slide_colorbar_out()`

---

## Simplified Approach

Instead of calculating complex dynamic rectangles, each animation frame:
1. **Draw large clearing rectangle** (covers colorbar + tux bar area)
2. **Redraw colorbar** → `draw_colors(COLORSEL_FORCE_REDRAW)`
3. **Redraw tux bar** → `redraw_tux_text()`
4. **Flip screen** → `SDL_Flip(screen)`

---

## Rectangle Area

### Slide IN
Clears from colorbar position to bottom:

```c
SDL_Rect clear_rect;
clear_rect.x = 0;
clear_rect.y = r_colors.y + ui_offset_y_colors;  // Start at animated colorbar position
clear_rect.w = WINDOW_WIDTH - r_ttoolopt.w;     // Full width minus right panel
clear_rect.h = WINDOW_HEIGHT - clear_rect.y;    // From colorbar to bottom
```

### Slide OUT
**No clearing rectangle needed!** The canvas redraw (`update_canvas`) overwrites old content.

**Color**: N/A - no clearing needed

---

## Drawing Order

### Slide IN (every frame)
```c
// 1. Clear the area
SDL_FillRect(screen, &clear_rect, WHITE);

// 2. Redraw UI elements at new position
draw_colors(COLORSEL_FORCE_REDRAW);
redraw_tux_text();

// 3. Display
SDL_Flip(screen);
```

### Slide OUT (every frame)
```c
// 1. Redraw canvas (overwrites old content - no clearing needed!)
update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w, WINDOW_HEIGHT);

// 2. Redraw UI elements at new position
draw_colors(COLORSEL_FORCE_REDRAW);
redraw_tux_text();

// 3. Display
SDL_Flip(screen);
```

### After Slide OUT Complete
```c
// Final canvas redraw to ensure clean state
update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w, WINDOW_HEIGHT);
draw_colors(COLORSEL_FORCE_REDRAW);
SDL_Flip(screen);
```

---

## Benefits

- **Much simpler**: No complex dynamic calculations
- **Reliable**: Always clears the right areas
- **Clean**: Proper redraw order prevents artifacts
- **Maintainable**: Easy to understand and modify
