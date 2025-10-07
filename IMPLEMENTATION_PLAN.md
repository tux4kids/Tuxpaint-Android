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

### 6.4 Left Toolbar

**Layout:**
- Größere Buttons (mehr Platz pro Button)
- nur 1 Spalte statt 2
- den speaker in row -1 auch sperren in dem Zustand, wie er gerade ist

**New-Button Behavior:**
```c
if (child_mode && cur_tool == TOOL_NEW) {
  do_save(current_pic, SAVE_OVER_NO_PROMPT);
  do_new_dialog();
}
```

### 6.5 Exit Child Mode Button
- Spezieller Button unten im Toolbar
- Setzt `child_mode = 0`
- Trigger normales Layout

**Test Scenarios:**
1. Toggle Child Mode aktiviert
2. UI zeigt nur simplified tools
3. Slider steuert Brush-Size
4. New macht Auto-Save
5. Exit Child Mode → normales UI

**Commit:** `feat: Implement child mode with simplified UI`

---

## 7. 🧪 Testing & Documentation

**Unittests:**
- [ ] Default Brush ist zweiter
- [ ] Colors Label entfernt, Buttons volle Breite
- [ ] Tools Label entfernt
- [ ] Sound Button togglet Sound
- [ ] Child Mode Button togglet Mode
- [ ] Child Mode: Simplified UI funktioniert
- [ ] Child Mode: Slider funktioniert
- [ ] Child Mode: New macht Auto-Save
- [ ] Child Mode: Exit zurück zu normal

**Performance:**
- Kein Slowdown durch neue Features
- Smooth drawing weiterhin gewährleistet

**Commit:** `docs: Update documentation with new UI features`

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
