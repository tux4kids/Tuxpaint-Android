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

## 4. 🔇 Add Sound Toggle Button (Bottom Left)

**Dateien:** `tuxpaint.c`, möglicherweise Icons

**Implementierung:**
1. **Neue globale Variable:** `int sound_enabled = 1;`
2. **Button Position:** Links unten, neben zukünftigem Child-Mode Button
3. **Icon:** Lautsprecher (on) / durchgestrichener Lautsprecher (off)
4. **Click Handler:** Toggle `sound_enabled`, update `use_sound` oder `mute`
5. **Rendering:** In `draw_toolbar()` oder separater Bereich

**Änderungen:**
- Neue Button-Koordinaten definieren
- Click-Detection in `MOUSEBUTTONDOWN` Handler
- Toggle-Logik für Sound
- Icon rendering

**Test:** Klick togglet Sound on/off

**Commit:** `feat: Add sound toggle button at bottom left`

---

## 5. 👶 Add Child Mode Toggle Button (Bottom Left)

**Dateien:** `tuxpaint.c`, möglicherweise Icons

**Implementierung:**
1. **Neue globale Variable:** `int child_mode = 0;`
2. **Button Position:** Links unten, neben Sound Button
3. **Icon:** Kind-Symbol
4. **Click Handler:** Toggle `child_mode`, trigger UI re-layout
5. **State Persistence:** in Preferences speichern

**Änderungen:**
- Button-Koordinaten
- Click-Detection
- Toggle child_mode
- Trigger `setup_screen_layout()` oder ähnliches

**Test:** Klick aktiviert/deaktiviert Child Mode (vorerst nur Toggle)

**Commit:** `feat: Add child mode toggle button at bottom left`

---

## 6. 🧒 Implement Child Mode UI

**Dateien:** `tuxpaint.c`

**Child Mode Anforderungen:**

### 6.1 Auto-select 4px Brush
```c
if (child_mode && cur_tool == TOOL_BRUSH) {
  cur_brush = 1; // 4px brush
}
```

### 6.2 Hide Tux + Text Area
- Text-Bereich unter Color-Buttons ausblenden
- Tux-Pinguin ausblenden
- Color-Buttons in Höhe erweitern (füllen bis Screen-Bottom)

**Layout Änderungen:**
```c
if (child_mode) {
  r_tuxarea.h = 0;  // Hide Tux
  r_colors.h = SCREEN_HEIGHT - r_colors.y;  // Extend to bottom
}
```

### 6.3 Replace Right Toolbar → Brush Size Slider
- Rechte Button-Spalte ausblenden
- Großer vertikaler Slider anzeigen
- Slider steuert Brush-Size (0-100% → brush sizes)

**Implementierung:**
```c
if (child_mode) {
  // Hide r_toolopt
  // Draw custom slider
  // Handle slider drag events
}
```

### 6.4 Simplified Left Toolbar
**Nur diese Tools anzeigen:**
- Paint (TOOL_BRUSH)
- Eraser (TOOL_ERASER)  
- Fill (TOOL_FILL)
- Save (TOOL_SAVE)
- New (TOOL_NEW mit Auto-Save)
- Undo (TOOL_UNDO)
- Redo (TOOL_REDO)
- Exit Child Mode (Custom Button)

**Layout:**
- Größere Buttons (mehr Platz pro Button)
- nur 1 Spalte statt 2

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
