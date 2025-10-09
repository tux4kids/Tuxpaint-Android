# Category Reorganization - Child Mode

**Datum:** 9.10.2025, 08:06

## Änderungen

Brushes wurden zwischen Kategorien reorganisiert, um bessere thematische Gruppierung zu erreichen.

## Verschobene Brushes

### Von Kategorie 3 → Kategorie 4 (Shapes)
- **50=sphere** - Passt besser zu Grundformen
- **52=spiral** - Passt besser zu Grundformen

### Von Kategorie 9 → Kategorie 3 (Mixed Icon Brushes)
- **48=smoke** (frames=3)
- **54=spray** (frames=6)
- **65=vine** (frames=6)
- **66=watercolor-texture** (frames=4)

### Von Kategorie 6 → Kategorie 3 (Mixed Icon Brushes)
- **67=water_still** (frames=3)

### Von Kategorie 10 → Kategorie 3 (Mixed Icon Brushes)
- **51=spines**

## Aktualisierte Kategorien

### Kategorie 3: Mixed Icon Brushes
**Vorher:** 9 Brushes (35, 34, 50, 30, 39, 33, 38, 49, 52)
**Nachher:** 13 Brushes (35, 34, 48, 30, 39, 33, 38, 49, 51, 54, 65, 66, 67)

**Brushes:**
- 35=lines-angled (directional)
- 34=inksplat (frames=5)
- 48=smoke (frames=3)
- 30=graphite (frames=7)
- 39=pencil (frames=4)
- 33=impasto (frames=10)
- 38=paint_splats (frames=4)
- 49=sparkles (frames=4)
- 51=spines
- 54=spray (frames=6)
- 65=vine (frames=6)
- 66=watercolor-texture (frames=4)
- 67=water_still (frames=3)

**Slider:** 13 Positionen (0-12)

### Kategorie 4: Shapes (Grundformen)
**Vorher:** 8 Brushes (19, 31, 32, 40, 61, 63, 64, 68)
**Nachher:** 10 Brushes (19, 31, 32, 40, 61, 63, 64, 68, 50, 52)

**Brushes:**
- 19=diamond
- 31=heart
- 32=hexagon
- 40=pentagon
- 61=star
- 63=triangle_down
- 64=triangle_up
- 68=x
- 50=sphere
- 52=spiral

**Slider:** 10 Positionen (0-9)

### Kategorie 6: Animals & Nature
**Vorher:** 8 Brushes (14, 15, 16, 27, 28, 29, 53, 67)
**Nachher:** 7 Brushes (14, 15, 16, 27, 28, 29, 53)

**Brushes:**
- 14=critter_dog (frames=2, directional)
- 15=critter_kuroneko (frames=2, directional)
- 16=critter_squirrel (frames=3, directional)
- 27=footprints-human (frames=2, directional)
- 28=footprints-human-shoes (frames=2, directional)
- 29=footprints-paws (frames=2, directional)
- 53=splat

**Slider:** 7 Positionen (0-6)

### Kategorie 9: Texture Brushes
**Vorher:** 8 Brushes (9, 25, 26, 48, 54, 62, 65, 66)
**Nachher:** 4 Brushes (9, 25, 26, 62)

**Brushes:**
- 9=acrylic (frames=4)
- 25=fluff_gradient (frames=4)
- 26=fluff (frames=4)
- 62=tiny

**Slider:** 4 Positionen (0-3)

### Kategorie 10: Effect Brushes
**Vorher:** 8 Brushes (10, 11, 12, 13, 17, 18, 41, 51)
**Nachher:** 7 Brushes (10, 11, 12, 13, 17, 18, 41)

**Brushes:**
- 10=arrow (rotate)
- 11=arrow_triangles (directional)
- 12=blob
- 13=chisle
- 17=cutout_square_diamond
- 18=cutout_star_circle
- 41=rotating_dash (rotate)

**Slider:** 7 Positionen (0-6)

## Code-Änderungen

### IMPLEMENTATION_PLAN.md
- Kategorie 3: 9 → 13 Brushes
- Kategorie 4: 8 → 10 Brushes
- Kategorie 6: 8 → 7 Brushes
- Kategorie 9: 8 → 4 Brushes
- Kategorie 10: 8 → 7 Brushes

### tuxpaint.c (init_child_brush_category)

**Zeilen 11516-11528:** Kategorie 3
```c
else if (expert_mode_brush == 35 || expert_mode_brush == 34 || expert_mode_brush == 48 ||
         expert_mode_brush == 30 || expert_mode_brush == 39 || expert_mode_brush == 33 ||
         expert_mode_brush == 38 || expert_mode_brush == 49 || expert_mode_brush == 51 ||
         expert_mode_brush == 54 || expert_mode_brush == 65 || expert_mode_brush == 66 ||
         expert_mode_brush == 67) {
  child_brush_category = 3;
  int brushes[] = {35, 34, 48, 30, 39, 33, 38, 49, 51, 54, 65, 66, 67};
  child_brush_count = 13;
  child_brush_use_icons = 1;
  child_brush_variable_size = 0;
}
```

**Zeilen 11529-11540:** Kategorie 4
```c
else if (expert_mode_brush == 19 || expert_mode_brush == 31 || expert_mode_brush == 32 ||
         expert_mode_brush == 40 || expert_mode_brush == 61 || expert_mode_brush == 63 ||
         expert_mode_brush == 64 || expert_mode_brush == 68 || expert_mode_brush == 50 ||
         expert_mode_brush == 52) {
  child_brush_category = 4;
  int brushes[] = {19, 31, 32, 40, 61, 63, 64, 68, 50, 52};
  child_brush_count = 10;
  child_brush_use_icons = 1;
  child_brush_variable_size = 0;
}
```

**Zeilen 11550-11560:** Kategorie 6
```c
else if (expert_mode_brush == 14 || expert_mode_brush == 15 || expert_mode_brush == 16 ||
         expert_mode_brush == 27 || expert_mode_brush == 28 || expert_mode_brush == 29 ||
         expert_mode_brush == 53) {
  child_brush_category = 6;
  int brushes[] = {14, 15, 16, 27, 28, 29, 53};
  child_brush_count = 7;
  child_brush_use_icons = 1;
  child_brush_variable_size = 0;
}
```

**Zeilen 11579-11588:** Kategorie 9
```c
else if (expert_mode_brush == 9 || expert_mode_brush == 25 || expert_mode_brush == 26 ||
         expert_mode_brush == 62) {
  child_brush_category = 9;
  int brushes[] = {9, 25, 26, 62};
  child_brush_count = 4;
  child_brush_use_icons = 1;
  child_brush_variable_size = 0;
}
```

**Zeilen 11589-11599:** Kategorie 10
```c
else if (expert_mode_brush == 10 || expert_mode_brush == 11 || expert_mode_brush == 12 ||
         expert_mode_brush == 13 || expert_mode_brush == 17 || expert_mode_brush == 18 ||
         expert_mode_brush == 41) {
  child_brush_category = 10;
  int brushes[] = {10, 11, 12, 13, 17, 18, 41};
  child_brush_count = 7;
  child_brush_use_icons = 1;
  child_brush_variable_size = 0;
}
```

## Begründung

### Kategorie 3 erweitert
Kategorie 3 ist jetzt die größte Icon-Kategorie mit 13 Brushes. Sie enthält hauptsächlich:
- **Textur-Brushes mit Frames:** smoke, spray, vine, watercolor-texture, water_still
- **Effekt-Brushes:** graphite, pencil, impasto, paint_splats, sparkles, spines
- **Directional:** lines-angled

### Shapes besser gruppiert
sphere und spiral passen besser zu den geometrischen Grundformen als zu Mixed Icons.

### Texture Brushes fokussiert
Kategorie 9 ist nun kleiner und fokussiert auf die Kern-Textur-Brushes ohne Überschneidung mit Kategorie 3.

### Effect Brushes bereinigt
spines wurde zu Kategorie 3 verschoben, da es besser zu den Effekt-Brushes mit Icons passt.

## Kompilierung

```bash
./gradlew assembleDebug
BUILD SUCCESSFUL in 5s
```

## Zusammenfassung

✅ **5 Kategorien aktualisiert**
- Kategorie 3: 9 → 13 Brushes (+4)
- Kategorie 4: 8 → 10 Brushes (+2)
- Kategorie 6: 8 → 7 Brushes (-1)
- Kategorie 9: 8 → 4 Brushes (-4)
- Kategorie 10: 8 → 7 Brushes (-1)

✅ **Alle 69 Brushes weiterhin abgedeckt**

✅ **Bessere thematische Gruppierung**

✅ **Code erfolgreich kompiliert**
