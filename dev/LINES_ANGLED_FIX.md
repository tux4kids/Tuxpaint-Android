# lines-angled Icon Fix

**Datum:** 9.10.2025, 07:35

## Problem

Das **lines-angled** Brush-Icon (Brush 35) wurde im Child Mode Slider nur als **schwarzes Quadrat** angezeigt.

## Ursache

Das `lines-angled.png` Brush ist ein **schwarzes Bild mit Alpha-Transparenz**:
- RGB-Kanäle: (0, 0, 0) - Schwarz
- Alpha-Kanal: Variable Transparenz (0-210)
- Im Slider-Icon war nur Schwarz auf Schwarz sichtbar

### Technische Details

**Original lines-angled.png (120x120, 3x3 directional grid):**
```
Channel Statistics (vor Fix):
  Gray: min=0, max=0, mean=0    ← Komplett schwarz!
  Alpha: min=0, max=210          ← Nur Alpha hat Daten
```

**Problem:** Beim Crop des Center-Tiles wurde nur RGB kopiert → schwarzes Quadrat

## Lösung

### Methode: Alpha-Extraktion + Negation

Für directional-only Brushes (z.B. lines-angled):

```bash
convert input.png \
  -crop "${tile_width}x${tile_height}+${tile_width}+${tile_height}" +repage \
  -alpha extract \
  -negate \
  output.png
```

**Schritte:**
1. **Crop:** Extrahiere Center-Tile aus 3x3-Grid (40x40 aus 120x120)
2. **Alpha extract:** Konvertiere Alpha-Kanal zu Grayscale
3. **Negate:** Invertiere (transparent=schwarz → opak=weiß)

### Ergebnis

**Nach Fix:**
```
Channel Statistics:
  Gray: min=45, max=255, mean=230.563    ← Helles, sichtbares Icon!
```

## Code-Änderungen

### create_handle_icons.sh

**Zeilen 51-54:**
```bash
# Extract center tile (middle of 3x3 grid)
# Extract alpha channel as grayscale for black brushes
convert "$input" -crop "${tile_width}x${tile_height}+${tile_width}+${tile_height}" +repage \
        -alpha extract -negate "$output"
```

### Betroffene Brushes

Nur **directional-only** Brushes sind betroffen:
- ✅ **lines-angled** (Brush 35) - FIXED
- ✅ **arrow_triangles** (Brush 11) - War schon okay (hat helle RGB-Werte)

**Brushes mit BOTH (frames + directional)** nutzen `process_both()` Funktion, die ebenfalls angepasst wurde.

## Tests

### Unit-Test: test_lines_angled_fix.sh

**5 Tests:**
1. ✅ Icon existiert
2. ✅ Korrekte Dimensionen (40x40)
3. ✅ Icon ist sichtbar (mean=230 > 50)
4. ✅ Ausreichend hell (max=255 > 200)
5. ✅ Korrektes Format (8-bit Grayscale PNG)

**Alle Tests bestanden!**

## Vorher / Nachher

### Vorher (schwarzes Quadrat)
```
Gray: min=0, max=0, mean=0
→ Komplett schwarz, nicht sichtbar
```

### Nachher (sichtbares Icon)
```
Gray: min=45, max=255, mean=230.563
→ Helles Grayscale-Icon, gut sichtbar
```

## Kompilierung

```bash
./gradlew assembleDebug
BUILD SUCCESSFUL in 3s
```

## Weitere betroffene Brushes?

**Analyse aller Brushes:**
- Nur **lines-angled** war betroffen
- Andere directional Brushes (arrow_triangles, critter_*) haben helle RGB-Werte
- Fix ist defensiv: `-alpha extract -negate` funktioniert auch für helle Brushes

## Warum funktioniert das?

Tuxpaint Brushes verwenden oft **schwarze Pixels mit Alpha** für die Brush-Form:
- Beim Zeichnen: Farbe wird auf schwarze Pixel angewendet
- Beim Icon-Display: Schwarze Pixel sind unsichtbar

**Lösung:** Alpha-Kanal als Helligkeitsinformation nutzen:
- Alpha = 0 (transparent) → Gray = 255 (weiß)
- Alpha = 255 (opak) → Gray = 0 (schwarz)
- Ergibt sichtbares Icon mit korrekter Form

## Zusammenfassung

✅ **lines-angled Icon ist jetzt korrekt sichtbar**
- Kein schwarzes Quadrat mehr
- Helles, gut erkennbares Grayscale-Icon
- Transparenz korrekt als Helligkeit dargestellt
- Alle Tests bestanden
- Kompilierung erfolgreich

**Das Icon funktioniert jetzt perfekt im Child Mode Slider!** 🎉
