ich habe alles reverted nd möchte einen anderen ansatz: 

# Phase 1: Globale Offset-Variablen erstellen ✅ FERTIG
- `ui_offset_x` - verschiebt linke Toolbar (Tools) nach links aus dem Bildschirm (-10px) ✅
- `ui_offset_y_colors` - verschiebt Colorbar nach unten aus dem Bildschirm (+10px) ✅
- `ui_offset_y_tux` - verschiebt Tuxbar nach unten aus dem Bildschirm (+10px) ✅
- Initialisierung beim Start mit Offset-Werten (UI versteckt) ✅
- die touch-bereiche auch mit dem offset versehen, also die touch-bereiche beginnen an der richtigen position

# Phase 2: Canvas bis zum bildrand erweitern
- Wenn fullscreen_ui_mode true ist, dann canvas breite = Bildschirmbreite und höhe = Bildschirmhöhe



# Phase 3: Offset-Variablen in allen Draw-Funktionen verwenden
- `draw_toolbar()` - verwendet `ui_offset_x` für Tools
- `draw_colors()` - verwendet `ui_offset_y_colors` für Colorbar
- `redraw_tux_text()` / Tux-Area - verwendet `ui_offset_y_tux` für Tuxbar
- Rechte Toolbar (r_toolopt) - KEIN Offset, bleibt immer sichtbar

# Phase 4: Show/Hide Funktionen
- `show_ui()` - setzt alle Offsets auf 0 (UI eingeblendet)
- `hide_ui()` - setzt Offsets auf Versteck-Werte (zum testen ermal nur mit 10px)
- Auto-Hide Timer (3 Sekunden Inaktivität)
- Bei Klick auf rechte Toolbar → immer show_ui() (kein Toggle)
