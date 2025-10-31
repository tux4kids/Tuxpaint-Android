# Tool Layout Documentation

## Tool IDs (from tools.h)
```c
enum {
  TOOL_BRUSH,   // 0
  TOOL_STAMP,   // 1
  TOOL_LINES,   // 2
  TOOL_SHAPES,  // 3
  TOOL_TEXT,    // 4
  TOOL_LABEL,   // 5
  TOOL_FILL,    // 6
  TOOL_MAGIC,   // 7
  TOOL_UNDO,    // 8
  TOOL_REDO,    // 9
  TOOL_ERASER,  // 10
  TOOL_NEW,     // 11
  TOOL_OPEN,    // 12
  TOOL_SAVE,    // 13
  TOOL_PRINT,   // 14
  TOOL_QUIT,    // 15
  NUM_TOOLS     // 16
};
```

## Current Layout (2 columns, gd_tools.cols = 2)

### NEW Structure (with SND/KID in r_ttools):
- **Row -1 (r_ttools area)**: SND, KID buttons (not in tool array)
- **Row 0** (Position 0-1): TOOL_BRUSH (0), TOOL_STAMP (1)
- **Row 1** (Position 2-3): TOOL_LINES (2), TOOL_SHAPES (3)
- **Row 2** (Position 4-5): TOOL_TEXT (4), TOOL_LABEL (5)
- **Row 3** (Position 6-7): TOOL_FILL (6), TOOL_MAGIC (7)
- **Row 4** (Position 8-9): TOOL_UNDO (8), TOOL_REDO (9)
- **Row 5** (Position 10-11): TOOL_ERASER (10), TOOL_NEW (11)
- **Row 6** (Position 12-13): TOOL_OPEN (12), TOOL_SAVE (13)
- **Row 7** (Position 14-15): TOOL_PRINT (14), TOOL_QUIT (15)

## Important Code Areas

### r_ttools (Tools Label Area)
```c
r_ttools.h = 38 * button_scale;  // Height of label area (reduced to prevent overlap)
```
This area is now used for SND/KID buttons!

### r_tools (Tool Buttons Area)
```c
r_tools.y = r_ttools.h + r_ttools.y;  // Starts after r_ttools
r_tools.h = gd_tools.rows * button_h; // 8 rows for the tools
```

### SND Button Position
```c
r_sound_btn.x = r_tools.x;           // Column 0 (left)
r_sound_btn.y = r_ttools.y;          // In r_ttools row!
```

### KID Button Position
```c
r_childmode_btn.x = r_tools.x + button_w;  // Column 1 (right)
r_childmode_btn.y = r_ttools.y;            // In r_ttools row!
```
