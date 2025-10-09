# Implementation Plan - UI/UX Improvements

**Datum:** 6. Oktober 2025, 10:45

## 🎯 Features zu implementieren (in dieser Reihenfolge)

---

## 1. ✅ Default Brush: Zweiter Brush (4px) statt erster

**Dateien:** `tuxpaint.c`

**Änderungen:**
- Zeile ~31483: `cur_brush = 0;` → `cur_brush = 1;`
- Suche nach allen Initialisierungen von `cur_brush`

**Test:** App starten, prüfen ob zweiter Brush automatisch ausgewählt ist

**Commit:** `feat: Set default brush to second brush (4px)`

---

## 2. 🎨 Remove "Colors" Label + Stretch Color Buttons

**Dateien:** `tuxpaint.c`

**Analyse:**
- Suche nach "Colors" String/Label Rendering
- Finde Layout-Berechnung für Color Buttons (`r_colors`)
- Entferne Label-Bereich
- Erweitere Color Buttons Breite um freigewordenen Platz

**Änderungen:**
- Layout calculation: Buttons breiter machen
- Label rendering: Auskommentieren/entfernen

**Test:** Farb-Buttons füllen ganze Breite ohne Label

**Commit:** `feat: Remove 'Colors' label and stretch color buttons to full width`

---

## 3. 🛠️ Remove "Tools" Label

**Dateien:** `tuxpaint.c`

**Analyse:**
- Suche nach "Tools" String/Label
- Finde `r_tools` Layout
- Entferne Label, erweitere Tool-Buttons nach oben

**Änderungen:**
- Label rendering entfernen
- Layout anpassen

**Test:** Tool-Buttons ohne "Tools" Label

**Commit:** `feat: Remove 'Tools' label from toolbar`

---

## 4. ✅ Add Sound Toggle Button (Bottom Left)

**Dateien:** `tuxpaint.c`, `playsound.c`

**Implementiert:** 6.10.2025 14:25

**Änderungen:**
- Button `r_sound_btn` bei Row 8 (unterhalb Print/Quit)
- Event handler VOR `HIT(r_tools)` verschoben (da Button in r_tools rect liegt)
- Toggle `mute` Variable (definiert in playsound.c)
- `Mix_HaltChannel(-1)` stoppt alle laufenden Sounds
- Alle `Mix_PlayChannel()` Calls prüfen `if (!mute)`

**Test:** ✅ Getestet auf echtem Gerät - Sound stoppt korrekt

**Commits:** 
- `Fix sound button event handling order + add debug logging`
- `Fix sound button: Move event handler before r_tools check`

---

## 5. ✅ Add Child Mode Toggle Button (Bottom Left)

**Dateien:** `tuxpaint.c`

**Implementiert:** 6.10.2025 15:20

**Änderungen:**
- Globale Variable `child_mode` hinzugefügt (Zeile 821)
- Button Handler aktiviert (Zeile 3717-3737)
- Toggle-Logik implementiert
- Button visual state (UP/DOWN) basierend auf `child_mode`
- Trigger `setup_screen_layout()` bei Toggle
- Vollständiges Screen-Redraw

**Test:** ✅ Button togglet Child Mode on/off

**Commit:** `2472ec75 - feat: Add child mode toggle button with Hide Tux + Text Area`

---

## 6. 🧒 Implement Child Mode UI

**Dateien:** `tuxpaint.c`

**Child Mode Anforderungen:**

### 6.1 ✅ Auto-select 24px Brush and switch to paint mode

**Implementiert:** 7.10.2025 11:02

**Änderungen in `tuxpaint.c` (Zeile 3745-3772):**
- Beim Aktivieren von Child Mode:
  - Automatischer Wechsel zu TOOL_BRUSH
  - cur_brush = 3 (aa_round_24.png - 24px Brush)
  - brush_scroll = 0 (Scroll an den Anfang)
  - render_brush() + draw_brushes() aufrufen

```c
if (child_mode)
{
  if (cur_tool != TOOL_BRUSH)
  {
    cur_tool = TOOL_BRUSH;
  }
  cur_brush = 3;  /* aa_round_24.png (24px brush) */
  brush_scroll = 0;
  render_brush();
}
```

**Test:** ✅ Beim Aktivieren von Child Mode aus einem anderen Tool wird automatisch zum Paint-Tool mit 24px Brush gewechselt


### 6.2 ✅ Hide Tux + Text Area + Stretch Color Buttons

**Implementiert:** 6.10.2025 18:30

**Änderungen in `setup_normal_screen_layout()` (Zeile 963-983):**
- Wenn `child_mode == 1`:
  - `r_tuxarea.h = 0` (Tux versteckt)
  - `r_tuxarea.y = WINDOW_HEIGHT` (off-screen)
  - `r_colors.h = WINDOW_HEIGHT - r_colors.y` (Farben bis unten gestreckt)
  - `color_button_h` neu berechnet für größere Höhe
- Normal mode: Original Layout wiederhergestellt

**Änderungen in `draw_colors()` (Zeile 11141-11189):**
- Normale Farb-Buttons werden dynamisch skaliert mit `thumbnail()`
- Spezial-Buttons (Pipette, Color Picker, Mixer) bleiben in Original-Größe zentriert
- Clickable Area bleibt für alle Buttons gestreckt

**Test:** ✅ Getestet - Tux verschwindet, Farben füllen Platz, Buttons skalieren korrekt

**Commits:** 
- `2472ec75` - Initial implementation
- `63dfe05e` - Child mode: stretch color buttons vertically, center special buttons

### 6.2.2 ✅ Simplify Left Toolbar
**Nur diese Tools anzeigen (alle anderen disablen)**
- Child Mode Button
- Paint (TOOL_BRUSH)
- Eraser (TOOL_ERASER)
- Fill (TOOL_FILL)
- Save (TOOL_SAVE)
- New (TOOL_NEW mit Auto-Save)
- Undo (TOOL_UNDO)
- Redo (TOOL_REDO)

### 6.3 ✅ Replace Right Toolbar → Brush Size Slider
- ✅ Rechte Button-Spalte ausblenden
- ✅ Großer vertikaler Slider anzeigen (5% komprimiert für Margin)
- ✅ Slider steuert Brush-Size (0-100% → brush sizes 0-4)
- ✅ Weißer Hintergrund
- ✅ Slider-Schiene: Hellblau oben (bis Handle), Hellgrau unten (ab Handle)
- ✅ Handle: Hellblauer Kreis mit weißem Rand
  - Inverses Größenverhältnis: Brush 0 → kleiner Kreis (5px) + großer Rand (25px), Brush 4 → großer Kreis (29px) + kleiner Rand (1px)
  - Total-Radius konstant bei 30px
- ✅ Fließende Auswahl: Handle kann überall auf Schiene positioniert werden (nicht nur diskrete Positionen)
- 1. der anfasser soll insgesamt 3 pixel grösser und einen pixel mehr weissen rand bekommen
- 2. die bewegung soll mit ease stattfinden, also z.b. 0.5s dauern, bis der slider sich sichtbar zu der position hin bewegt, wo der taouch passierrt, bei touchend dann in 0.2s zu einer der erlaubten positionen hin snappen
**Implementierung:**
```c
if (child_mode) {
  // Hide r_toolopt
  // Draw custom slider
  // Handle slider drag events
}
```

### 6.3.1 ✅ Kontextabhängige Brush-Auswahl im Child Mode

Der Slider soll verschiedene Brush-Sets anbieten, abhängig davon, welcher Brush im Expert Mode aktiv war beim Wechsel in den Child Mode.

#### Brush-Index nach Dateinamen (alphabetisch geladen)
```
Brushes werden alphabetisch aus data/brushes/ geladen:
0=aa_round_03
1=aa_round_06
2=aa_round_12
3=aa_round_24
4=aa_round_36
5=aa_round_fuzz
6=aa_round_seethru_05
7=aa_round_seethru_10
8=aa_round_seethru
9=acrylic (frames=4)
10=arrow (rotate)
11=arrow_triangles (directional)
12=blob
13=chisle
14=critter_dog (frames=2, directional) *BOTH*
15=critter_kuroneko (frames=2, directional) *BOTH*
16=critter_squirrel (frames=3, directional) *BOTH*
17=cutout_square_diamond
18=cutout_star_circle
19=diamond
20=flower_5
21=flower_5_small
22=flower_6
23=flower_6_small
24=flower_7
25=fluff_gradient (frames=4)
26=fluff (frames=4)
27=footprints-human (frames=2, directional) *BOTH*
28=footprints-human-shoes (frames=2, directional) *BOTH*
29=footprints-paws (frames=2, directional) *BOTH*
30=graphite (frames=7)
31=heart
32=hexagon
33=impasto (frames=10, rotate)
34=inksplat (frames=5)
35=lines-angled (directional)
36=lozenge
37=oval
38=paint_splats (frames=4)
39=pencil (frames=4, rotate)
40=pentagon
41=rotating_dash (rotate)
42=slash_10_lt
43=slash_10_rt
44=slash_16_lt
45=slash_16_rt
46=slash_20_lt
47=slash_20_rt
48=smoke (frames=3)
49=sparkles (frames=4)
50=sphere
51=spines
52=spiral (frames=4)
53=splat
54=spray (frames=6)
55=square_06
56=square_12
57=square_24
58=square_36
59=square_fuzz
60=square_seethru
61=star
62=tiny
63=triangle_down
64=triangle_up
65=vine (frames=6)
66=watercolor-texture (frames=4)
67=water_still (frames=3)
68=x
```

**Brush-Kategorien: ✅**

#### Kategorie 1: Standard Brushes (0-4)
- **Bedingung:** Wenn Brush 0, 1, 2, 3 oder 4 im Expert Mode gewählt war
- **Slider-Verhalten:** Regelt Brushes 0, 1, 2, 3, 4 in dieser Reihenfolge
- **Handle-Design:** Größe variiert von klein (oben) bis groß (unten)
- **Weißer Rand:** Konstant 4px

#### Kategorie 2: Spezielle Brushes (5-8, 36-37)
- **Bedingung:** Wenn Brush 5, 6, 7, 8, 36 oder 37 im Expert Mode gewählt war
- **Slider-Verhalten:** Regelt Brushes in dieser Reihenfolge: **7, 8, 5, 6, 37, 36**
- **Handle-Design:** Größe variiert von klein (oben) bis groß (unten)
- **Weißer Rand:** Konstant 4px
- **Slider-Positionen:** 6 Positionen (0-5)

#### Kategorie 3: Mixed Icon Brushes (35, 34, 48, 30, 39, 33, 38, 49, 51, 54, 65, 66, 67)
- **Bedingung:** Wenn einer dieser Brushes im Expert Mode gewählt war
- **Brushes:** 35=lines-angled, 34=inksplat, 48=smoke, 30=graphite, 39=pencil, 33=impasto, 38=paint_splats, 49=sparkles, 51=spines, 54=spray, 65=vine, 66=watercolor-texture, 67=water_still
- **Slider-Verhalten:** Regelt alle 13 Brushes in dieser Reihenfolge
- **Handle-Design:** 
  - **Konstante Größe:** Handle bleibt immer voll groß (50px Radius)
  - **Brush-Icon:** Das Icon des jeweiligen Brushes wird **auf dem Ball angezeigt**
  - **Weißer Rand:** Konstant 4px
- **Slider-Positionen:** 13 Positionen (0-12)

#### Kategorie 4: Shapes (Grundformen)
- **Bedingung:** Wenn einer dieser Brushes gewählt war: **19, 31, 32, 40, 61, 63, 64, 68, 50, 52**
- **Brushes:** 19=diamond, 31=heart, 32=hexagon, 40=pentagon, 61=star, 63=triangle_down, 64=triangle_up, 68=x, 50=sphere, 52=spiral
- **Slider-Verhalten:** 10 Positionen mit Icons
- **Handle:** Konstante Größe (50px), Icon wird angezeigt

#### Kategorie 5: Flowers (Blumen - verschiedene Größen)
- **Bedingung:** Wenn einer dieser Brushes gewählt war: **20, 21, 22, 23, 24**
- **Brushes:** 20=flower_5, 21=flower_5_small, 22=flower_6, 23=flower_6_small, 24=flower_7
- **Slider-Verhalten:** 5 Positionen, gruppiert: [flower_5, flower_5_small], [flower_6, flower_6_small], [flower_7]
- **Handle:** Variable Größe (30-50px), zeigt die Blumen-Icons

#### Kategorie 6: Animals & Nature (Tiere, Fußabdrücke & Natur)
- **Bedingung:** Wenn einer dieser Brushes gewählt war: **14, 15, 16, 27, 28, 29, 53**
- **Brushes:** 
  - 14=critter_dog, 15=critter_kuroneko (schwarze Katze), 16=critter_squirrel (Eichhörnchen)
  - 27=footprints-human, 28=footprints-human-shoes, 29=footprints-paws
  - 53=splat
- **Slider-Verhalten:** 7 Positionen mit Icons
- **Handle:** Konstante Größe (50px), Icon wird angezeigt

#### Kategorie 7: Slash Lines (Diagonale Linien - verschiedene Größen)
- **Bedingung:** Wenn einer dieser Brushes gewählt war: **42, 43, 44, 45, 46, 47**
- **Brushes:** 42=slash_10_lt, 43=slash_10_rt, 44=slash_16_lt, 45=slash_16_rt, 46=slash_20_lt, 47=slash_20_rt
- **Slider-Verhalten:** 6 Positionen, gruppiert nach Größe: [10_lt, 10_rt], [16_lt, 16_rt], [20_lt, 20_rt]
- **Handle:** Variable Größe (30-50px), zeigt die Icons

#### Kategorie 8: Squares (Quadrate - verschiedene Größen)
- **Bedingung:** Wenn einer dieser Brushes gewählt war: **55, 56, 57, 58, 59, 60**
- **Brushes:** 55=square_06, 56=square_12, 57=square_24, 58=square_36, 59=square_fuzz, 60=square_seethru
- **Slider-Verhalten:** 6 Positionen, gruppiert: [06, 12, 24, 36] Größen, [fuzz, seethru] Effekte
- **Handle:** Variable Größe (30-50px) für erste 4, konstant für letzten 2 mit Icons

#### Kategorie 9: Texture Brushes (Textur-Pinsel)
- **Bedingung:** Wenn einer dieser Brushes gewählt war: **9, 25, 26, 62**
- **Brushes:** 9=acrylic, 25=fluff_gradient, 26=fluff, 62=tiny
- **Slider-Verhalten:** 4 Positionen mit Icons
- **Handle:** Konstante Größe (50px), Icon wird angezeigt

#### Kategorie 10: Effect Brushes (Effekt-Pinsel)
- **Bedingung:** Wenn einer dieser Brushes gewählt war: **10, 11, 12, 13, 17, 18, 41**
- **Brushes:** 10=arrow, 11=arrow_triangles, 12=blob, 13=chisle, 17=cutout_square_diamond, 18=cutout_star_circle, 41=rotating_dash
- **Slider-Verhalten:** 7 Positionen mit Icons
- **Handle:** Konstante Größe (50px), Icon wird angezeigt

**Zusammenfassung:**
- **10 Kategorien** insgesamt (statt 12)
- **69 Brushes** vollständig abgedeckt
- **Kategorien 1-2:** Variable Größe ohne Icons (Standard-Rundpinsel)
- **Kategorien 3-12:** Icons werden auf dem Handle angezeigt
- **Variable vs. Konstante Größe:** Brushes mit verschiedenen Größen-Varianten nutzen variable Handle-Größe, andere konstante Größe mit Icons


**Implementierung:**

```c
/* Global variables for brush category */
static int child_brush_category = 1;  /* 1-12 for different categories */
static int child_brush_indices[13];   /* Array of brush indices for current category (max 13) */
static int child_brush_count = 5;     /* Number of brushes in current category */
static int child_brush_use_icons = 0; /* 1 if category should show icons on handle */
static int child_brush_variable_size = 1; /* 1 if handle size should vary, 0 for constant size */

/* Determine brush category when entering child mode */
void init_child_brush_category(int expert_mode_brush) {
  child_brush_use_icons = 0;
  child_brush_variable_size = 1;
  
  if (expert_mode_brush >= 0 && expert_mode_brush <= 4) {
    /* Category 1: Standard brushes (0-4) */
    child_brush_category = 1;
    int brushes[] = {0, 1, 2, 3, 4};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 5;
  }
  else if (expert_mode_brush == 5 || expert_mode_brush == 6 || 
           expert_mode_brush == 7 || expert_mode_brush == 8 ||
           expert_mode_brush == 36 || expert_mode_brush == 37) {
    /* Category 2: Special round brushes (7,8,5,6,37,36) */
    child_brush_category = 2;
    int brushes[] = {7, 8, 5, 6, 37, 36};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 6;
  }
  else if (expert_mode_brush == 55 || expert_mode_brush == 35 || expert_mode_brush == 34 ||
           expert_mode_brush == 50 || expert_mode_brush == 30 || expert_mode_brush == 39 ||
           expert_mode_brush == 33 || expert_mode_brush == 38 || expert_mode_brush == 49 ||
           expert_mode_brush == 52 || expert_mode_brush == 66 || expert_mode_brush == 67 ||
           expert_mode_brush == 68) {
    /* Category 3: Mixed icon brushes */
    child_brush_category = 3;
    int brushes[] = {55, 35, 34, 50, 30, 39, 33, 38, 49, 52, 66, 67, 68};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 13;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
  }
  else if (expert_mode_brush == 19 || expert_mode_brush == 31 || expert_mode_brush == 32 ||
           expert_mode_brush == 40 || expert_mode_brush == 61 || expert_mode_brush == 63 ||
           expert_mode_brush == 64 || expert_mode_brush == 68) {
    /* Category 4: Shapes */
    child_brush_category = 4;
    int brushes[] = {19, 31, 32, 40, 61, 63, 64, 68};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
  }
  else if (expert_mode_brush >= 20 && expert_mode_brush <= 24) {
    /* Category 5: Flowers */
    child_brush_category = 5;
    int brushes[] = {20, 21, 22, 23, 24};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 5;
    child_brush_use_icons = 1;
    child_brush_variable_size = 1;
  }
  else if (expert_mode_brush == 14 || expert_mode_brush == 15 || expert_mode_brush == 16 ||
           expert_mode_brush == 27 || expert_mode_brush == 28 || expert_mode_brush == 29 ||
           expert_mode_brush == 53 || expert_mode_brush == 67) {
    /* Category 6: Animals & Nature (Critters, Footprints, Splats & Water) */
    child_brush_category = 6;
    int brushes[] = {14, 15, 16, 27, 28, 29, 53, 67};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
  }
  else if (expert_mode_brush >= 42 && expert_mode_brush <= 47) {
    /* Category 7: Slash lines */
    child_brush_category = 7;
    int brushes[] = {42, 43, 44, 45, 46, 47};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 6;
    child_brush_use_icons = 1;
    child_brush_variable_size = 1;
  }
  else if (expert_mode_brush >= 55 && expert_mode_brush <= 60) {
    /* Category 8: Squares */
    child_brush_category = 8;
    int brushes[] = {55, 56, 57, 58, 59, 60};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 6;
    child_brush_use_icons = 1;
    child_brush_variable_size = 1;
  }
  else if (expert_mode_brush == 9 || expert_mode_brush == 25 || expert_mode_brush == 26 ||
           expert_mode_brush == 48 || expert_mode_brush == 54 || expert_mode_brush == 62 ||
           expert_mode_brush == 65 || expert_mode_brush == 66) {
    /* Category 9: Texture brushes */
    child_brush_category = 9;
    int brushes[] = {9, 25, 26, 48, 54, 62, 65, 66};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
  }
  else if (expert_mode_brush == 10 || expert_mode_brush == 11 || expert_mode_brush == 12 ||
           expert_mode_brush == 13 || expert_mode_brush == 17 || expert_mode_brush == 18 ||
           expert_mode_brush == 41 || expert_mode_brush == 51) {
    /* Category 10: Effect brushes */
    child_brush_category = 10;
    int brushes[] = {10, 11, 12, 13, 17, 18, 41, 51};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
  }
  else {
    /* Default to category 1 if no match */
    child_brush_category = 1;
    int brushes[] = {0, 1, 2, 3, 4};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 5;
  }
  
  /* Set cur_brush to first brush in category */
  cur_brush = child_brush_indices[0];
  
  SDL_Log("Child mode: category=%d, count=%d, first_brush=%d, icons=%d, var_size=%d",
          child_brush_category, child_brush_count, cur_brush, 
          child_brush_use_icons, child_brush_variable_size);
}

/* Modified slider drawing with icon and size support */
void draw_child_mode_brush_slider(void) {
  /* ... existing slider code for track ... */
  
  int total_radius, inner_radius;
  
  /* Determine handle size based on category settings */
  if (child_brush_variable_size) {
    /* Variable size: 30px (top) to 50px (bottom) */
    int slider_pos = 0;
    for (int i = 0; i < child_brush_count; i++) {
      if (child_brush_indices[i] == cur_brush) {
        slider_pos = i;
        break;
      }
    }
    float size_factor = (float)slider_pos / (float)(child_brush_count - 1);
    total_radius = 30 + (int)(size_factor * 20);  /* 30-50px */
  }
  else {
    /* Constant size for icon brushes */
    total_radius = 50;
  }
  
  inner_radius = total_radius - 4;  /* Constant 4px white border */
  
  /* Draw outer white circle */
  for (int y = -total_radius; y <= total_radius; y++) {
    int x_width = (int)sqrt((float)(total_radius * total_radius - y * y));
    SDL_Rect line;
    line.x = handle_center_x - x_width;
    line.y = handle_center_y + y;
    line.w = x_width * 2;
    line.h = 1;
    SDL_FillRect(screen, &line, white_color);
  }
  
  /* Draw inner blue circle */
  for (int y = -inner_radius; y <= inner_radius; y++) {
    int x_width = (int)sqrt((float)(inner_radius * inner_radius - y * y));
    SDL_Rect line;
    line.x = handle_center_x - x_width;
    line.y = handle_center_y + y;
    line.w = x_width * 2;
    line.h = 1;
    SDL_FillRect(screen, &line, blue_color);
  }
  
  /* Draw brush icon on handle if enabled for this category */
  if (child_brush_use_icons && cur_brush < num_brushes) {
    SDL_Surface *brush_thumb = img_brushes_thumbs[cur_brush];
    if (brush_thumb != NULL) {
      SDL_Rect icon_dest;
      /* Scale icon to fit inside handle (leave some margin) */
      int max_icon_size = inner_radius * 2 - 8;  /* 4px margin on each side */
      float scale = 1.0f;
      if (brush_thumb->w > max_icon_size || brush_thumb->h > max_icon_size) {
        float scale_w = (float)max_icon_size / (float)brush_thumb->w;
        float scale_h = (float)max_icon_size / (float)brush_thumb->h;
        scale = (scale_w < scale_h) ? scale_w : scale_h;
      }
      
      int icon_w = (int)(brush_thumb->w * scale);
      int icon_h = (int)(brush_thumb->h * scale);
      icon_dest.x = handle_center_x - icon_w / 2;
      icon_dest.y = handle_center_y - icon_h / 2;
      icon_dest.w = icon_w;
      icon_dest.h = icon_h;
      
      /* Blit scaled icon */
      if (scale != 1.0f) {
        SDL_Surface *scaled_icon = thumbnail2(brush_thumb, icon_w, icon_h, 0, 1);
        SDL_BlitSurface(scaled_icon, NULL, screen, &icon_dest);
        SDL_FreeSurface(scaled_icon);
      } else {
        SDL_BlitSurface(brush_thumb, NULL, screen, &icon_dest);
      }
    }
  }
}

/* Modified slider interaction for variable brush count */
void handle_slider_drag(int click_y) {
  int child_brush_min = 0;
  int child_brush_max = child_brush_count - 1;
  
  float click_percentage = (float)(click_y - slider_y) / (float)slider_h;
  if (click_percentage < 0.0f) click_percentage = 0.0f;
  if (click_percentage > 1.0f) click_percentage = 1.0f;
  
  int slider_position = (int)(click_percentage * child_brush_count);
  if (slider_position >= child_brush_count) slider_position = child_brush_count - 1;
  
  cur_brush = child_brush_indices[slider_position];
}
```

**Aufgaben:**
1. [ ] Globale Variablen für Brush-Kategorie-System hinzufügen (category, indices, count, use_icons, variable_size)
2. [ ] `init_child_brush_category()` Funktion für alle 12 Kategorien implementieren
3. [ ] Funktion beim Wechsel in Child Mode aufrufen (TOOL_CHILD_MODE Toggle)
4. [ ] `draw_child_mode_brush_slider()` anpassen:
   - [ ] Variable vs. konstante Handle-Größe basierend auf `child_brush_variable_size`
   - [ ] Icon-Rendering wenn `child_brush_use_icons == 1`
   - [ ] Skalierung von Icons auf Handle-Größe
5. [ ] Slider-Interaktion für variable Brush-Count anpassen (MOUSEBUTTONDOWN, MOUSEMOTION, MOUSEBUTTONUP)
6. [ ] Snap-Animation mit variablem `child_brush_count` anpassen
7. [ ] Testen aller 12 Kategorien:
   - [ ] Kategorie 1: Standard (0-4) - variable Größe, keine Icons
   - [ ] Kategorie 2: Special round (5-8,36-37) - variable Größe, keine Icons
   - [ ] Kategorie 3: Mixed icons - konstante Größe, mit Icons
   - [ ] Kategorie 4: Shapes - konstante Größe, mit Icons
   - [ ] Kategorie 5: Flowers - variable Größe, mit Icons
   - [ ] Kategorie 6: Footprints - konstante Größe, mit Icons
   - [ ] Kategorie 7: Critters - konstante Größe, mit Icons
   - [ ] Kategorie 8: Slash lines - variable Größe, mit Icons
   - [ ] Kategorie 9: Squares - variable Größe, mit Icons
   - [ ] Kategorie 10: Texture brushes - konstante Größe, mit Icons
   - [ ] Kategorie 11: Effect brushes - konstante Größe, mit Icons
   - [ ] Kategorie 12: Splats & Water - konstante Größe, mit Icons
8. [ ] Wechsel zwischen Expert Mode → Child Mode testen (Kategorie-Auswahl basierend auf aktivem Brush)
9. [ ] Performance-Test mit Icon-Rendering bei Animation

### 6.4 ✅ Lock Child Mode Features

**Requirements:**
- [x] In child mode, sound toggle button must be disabled (cannot be changed)
- [x] when changing to child mode, the buttton can be held for 3s longpress, to lock it
- [x] if childmode is locked, the Child mode button requires 3s long-press to unlock
- [x] Locked state prevents accidental exit from child mode
- [x] Visual indicator for locked state (lock icon in top-right corner)
- [ ] "Expert mode activated" message when exiting child mode

**Implementation:**
```c
/* Global variables */
static int child_mode_locked = 0;
static Uint32 child_mode_button_press_start = 0;
static int child_mode_at_press_start = 0;

/* In event handler (MOUSEBUTTONDOWN) */
if (HIT(r_childmode_btn)) {
  child_mode_button_press_start = SDL_GetTicks();
  child_mode_at_press_start = child_mode;  /* Remember state when press started */
}

/* In event handler (MOUSEBUTTONUP) */
if (HIT(r_childmode_btn)) {
  Uint32 press_duration = SDL_GetTicks() - child_mode_button_press_start;
  
  if (child_mode_locked) {
    /* Child mode is locked: require 3s long-press to unlock */
    if (press_duration >= 3000) {
      child_mode_locked = 0;
      draw_tux_text(TUX_BORED, gettext("Child mode unlocked."), 0);
    }
    else {
      /* Too short: show locked message */
      draw_tux_text(TUX_BORED, gettext("Child mode is locked. Long-press 3s to unlock."), 0);
    }
  }
  else if (child_mode_at_press_start == 0) {
    /* Transitioning FROM expert TO child mode */
    if (press_duration >= 3000) {
      /* Long press: activate child mode AND lock it */
      child_mode = 1;
      child_mode_locked = 1;
      setup_screen_layout();
      draw_tux_text(TUX_BORED, gettext("Child mode activated and locked."), 0);
    }
    else {
      /* Short press: just activate child mode (unlocked) */
      child_mode = 1;
      setup_screen_layout();
      draw_tux_text(TUX_BORED, gettext("Child mode activated."), 0);
    }
  }
  else {
    /* Already in child mode (unlocked): exit to expert mode */
    child_mode = 0;
    setup_screen_layout();
    draw_tux_text(TUX_BORED, gettext("Expert mode activated."), 0);
  }
  
  child_mode_button_press_start = 0;
}

/* Disable sound button in child mode */
if (HIT(r_sound_btn)) {
  if (!child_mode) {
    /* Allow sound toggle only in expert mode */
    use_sound = !use_sound;
    Mix_HaltChannel(-1);
    draw_row_minus_1_buttons();
    update_screen_rect(&r_sound_btn);
    // ...
  }
  else {
    /* Child mode: sound button locked */
    draw_tux_text(TUX_BORED, gettext("Sound is locked in child mode."), 0);
  }
}
```

### 6.4.2 ✅ Preferences (Persistent Storage)

**Save the following in SharedPreferences:**
- [x] Sound toggle status (`use_sound`)
- [x] Child mode status (`child_mode`)
- [x] Child mode locked status (`child_mode_locked`)
- [x] Last brush in child mode (restore when switching back from expert mode)
- [x] Last brush category in child mode

**Implementation (Android JNI):**
```c
/* Save preferences via JNI */
void save_preferences(void) {
  JNIEnv *env = (JNIEnv*)SDL_AndroidGetJNIEnv();
  jobject activity = (jobject)SDL_AndroidGetActivity();
  
  jclass clazz = (*env)->GetObjectClass(env, activity);
  jmethodID method = (*env)->GetMethodID(env, clazz, "savePreferences", "(IIIII)V");
  
  (*env)->CallVoidMethod(env, activity, method, 
    use_sound, child_mode, child_mode_locked, cur_brush, child_brush_category);
  
  (*env)->DeleteLocalRef(env, clazz);
  (*env)->DeleteLocalRef(env, activity);
}

/* Load preferences via JNI */
void load_preferences(void) {
  JNIEnv *env = (JNIEnv*)SDL_AndroidGetJNIEnv();
  jobject activity = (jobject)SDL_AndroidGetActivity();
  
  jclass clazz = (*env)->GetObjectClass(env, activity);
  jmethodID method_sound = (*env)->GetMethodID(env, clazz, "getUseSoundPref", "()I");
  jmethodID method_child = (*env)->GetMethodID(env, clazz, "getChildModePref", "()I");
  jmethodID method_locked = (*env)->GetMethodID(env, clazz, "getChildModeLockedPref", "()I");
  jmethodID method_brush = (*env)->GetMethodID(env, clazz, "getLastBrushPref", "()I");
  
  use_sound = (*env)->CallIntMethod(env, activity, method_sound);
  child_mode = (*env)->CallIntMethod(env, activity, method_child);
  child_mode_locked = (*env)->CallIntMethod(env, activity, method_locked);
  cur_brush = (*env)->CallIntMethod(env, activity, method_brush);
  
  (*env)->DeleteLocalRef(env, clazz);
  (*env)->DeleteLocalRef(env, activity);
}
```

**Java Side (TuxPaintActivity.java):**
```java
public void savePreferences(int useSound, int childMode, int childModeLocked, 
                           int lastBrush, int brushCategory) {
  SharedPreferences prefs = getPreferences(MODE_PRIVATE);
  SharedPreferences.Editor editor = prefs.edit();
  editor.putInt("use_sound", useSound);
  editor.putInt("child_mode", childMode);
  editor.putInt("child_mode_locked", childModeLocked);
  editor.putInt("last_brush", lastBrush);
  editor.putInt("brush_category", brushCategory);
  editor.apply();
}

public int getUseSoundPref() {
  return getPreferences(MODE_PRIVATE).getInt("use_sound", 1);
}

public int getChildModePref() {
  return getPreferences(MODE_PRIVATE).getInt("child_mode", 0);
}

public int getChildModeLockedPref() {
  return getPreferences(MODE_PRIVATE).getInt("child_mode_locked", 0);
}

public int getLastBrushPref() {
  return getPreferences(MODE_PRIVATE).getInt("last_brush", 3);
}
```

**Call on app start/exit:**
```c
/* In setup() */
load_preferences();

/* In cleanup() or on pause */
save_preferences();
```


### 6.5 Left Toolbar in Child-Mode
- Larger buttons (more space per button)
- Only 1 column instead of 2
  - Child Mode Button
  - Paint (TOOL_BRUSH)
  - Eraser (TOOL_ERASER)
  - Fill (TOOL_FILL)
  - Save (TOOL_SAVE)
  - New (TOOL_NEW mit Auto-Save)
    - the New button should not ask, but just do an Auto-Save in child mode
  - Undo (TOOL_UNDO)
  - Redo (TOOL_REDO)

**New-Button Behavior:**
```c
if (child_mode && cur_tool == TOOL_NEW) {
  do_save(current_pic, SAVE_OVER_NO_PROMPT);
  do_new_dialog();
}
```


---

## 7. 🧪 Testing & Documentation

**Unit Tests:**
- [ ] Child Mode: New performs auto-save
- [x] Child Mode Lock: 3s long-press locks/unlocks
- [x] Sound button disabled in child mode
- [x] Preferences save/load correctly

**Performance:**
- No slowdown from new features
- Smooth drawing still guaranteed

**Commits:**
- `feat: Add child mode lock with 3s long-press`
- `feat: Disable sound toggle in child mode`
- `feat: Add preferences storage for child mode state`
- `docs: Update documentation with new UI features`

---

## 📦 Final Commit Summary

Alle Features in separaten Commits, dann:

```bash
git log --oneline
feat: Implement child mode with simplified UI
feat: Add child mode toggle button at bottom left
feat: Add sound toggle button at bottom left
feat: Remove 'Tools' label from toolbar
feat: Remove 'Colors' label and stretch color buttons to full width
feat: Set default brush to second brush (4px)
```

---

## ⚠️ Potential Issues & Solutions

### Issue 1: Layout Conflicts
**Problem:** Child Mode Layout könnte mit bestehendem Code konfligieren
**Solution:** Wrap alle Layout-Berechnungen in `if (child_mode) {...} else {...}`

### Issue 2: Button Space
**Problem:** Nicht genug Platz für Sound + Child Mode Buttons
**Solution:** Buttons klein halten oder übereinander anordnen

### Issue 3: Slider Implementation
**Problem:** Slider ist komplett neues UI-Element
**Solution:** Simple Rect mit Drag-Detection, ähnlich wie Scrollbars

### Issue 4: Icon Assets
**Problem:** Icons für Sound/Child Mode fehlen eventuell
**Solution:** Temporär Text-Labels oder simple SDL_gfx Shapes

---

## 🎯 Success Criteria

- ✅ Alle 6 Features implementiert
- ✅ Keine Regressions in bestehendem Code
- ✅ Multitouch weiterhin funktionsfähig
- ✅ Child Mode vollständig nutzbar
- ✅ Clean Commits mit klaren Messages
- ✅ Code gut dokumentiert (comments in English)

---

## 📝 Notes

- Alle UI Strings sollten lokalisierbar sein
- Child Mode State könnte persistent gespeichert werden
- Slider könnte später auch für andere Tools verwendet werden
- Exit Child Mode könnte auch via Hardware Back-Button
